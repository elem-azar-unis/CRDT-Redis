import random
import sys
import time
import paramiko
import scp
import redis


# async def aredis_exec(conn, *cm, prt=0):
#     try:
#         r = await conn.execute_command(*cm)
#         if prt != 0:
#             print(r)
#     except redis.exceptions.ResponseError as e:
#         print(repr(e))


def redis_exec(conn, *cm, prt=0):
    try:
        r = conn.execute_command(*cm)
        if prt != 0:
            print(r)
    except redis.exceptions.ResponseError as e:
        print(repr(e))


def ssh_exec(sshs, cmd):
    cmd = "cd ~/Redis-RPQ/redis_test/;" + cmd + " 1>/dev/null 2>&1"
    for ssh in sshs:
        stdin, stdout, stderr = ssh.exec_command(cmd)
        # data = stdout.read()
        # if len(data) > 0:
        #     print("d", bytes.decode(data.strip()))  # 打印正确结果
        # err = stderr.read()
        # if len(err) > 0:
        #     print(bytes.decode(err.strip()))  # 输出错误结果


def _reset_redis(sshs):
    print("scp writing")
    for ssh in sshs:
        stdin, stdout, stderr = ssh.exec_command("rm -rf ~/Redis-RPQ")
        time.sleep(1)
        s = scp.SCPClient(ssh.get_transport())
        s.put("../../Redis-RPQ", remote_path="~/", recursive=True)
        s.close()
        stdin, stdout, stderr = ssh.exec_command("cd ~/Redis-RPQ/redis-4.0.8/;sudo make install", get_pty=True)
        stdin.write("user\n")
        stdin.flush()
        data = stdout.read()
        if len(data) > 0:
            print("d", bytes.decode(data.strip()))  # 打印正确结果
        err = stderr.read()
        if len(err) > 0:
            print(bytes.decode(err.strip()))  # 输出错误结果
    time.sleep(1)
    print("reset done.")


def _set_delay(ssh, lo_delay, delay, ip1, ip2, limit=100000):
    cmd = (
        "sudo tc qdisc del dev ens33 root",
        "sudo tc qdisc add dev ens33 root handle 1: prio",
        "sudo tc qdisc add dev ens33 parent 1:1 handle 10: netem delay {delay} distribution normal limit {limit}".format(
            delay=delay, limit=limit),
        "sudo tc qdisc add dev ens33 parent 1:2 handle 20: pfifo_fast",
        "sudo tc filter del dev ens33",
        "sudo tc filter add dev ens33 protocol ip parent 1: prio 1 u32 match ip dst {ip1} flowid 1:1".format(ip1=ip1),
        "sudo tc filter add dev ens33 protocol ip parent 1: prio 1 u32 match ip dst {ip2} flowid 1:1".format(ip2=ip2),
        "sudo tc qdisc del dev lo root",
        "sudo tc qdisc add dev lo root netem delay {lo_delay} distribution normal limit {limit}".format(
            lo_delay=lo_delay, limit=limit)
    )
    for c in cmd:
        stdin, stdout, stderr = ssh.exec_command(c, get_pty=True)
        stdin.write("user\n")
        stdin.flush()
        # data = stdout.read()
        # if len(data) > 0:
        #     print("d", bytes.decode(data.strip()))  # 打印正确结果
        # err = stderr.read()
        # if len(err) > 0:
        #     print(bytes.decode(err.strip()))  # 输出错误结果


class Connection:
    localhost = "127.0.0.1"
    ips = ("192.168.188.135",
           "192.168.188.136",
           "192.168.188.137")
    ports = (6379, 6380, 6381, 6382, 6383)
    sshs = []
    conns = []

    def __init__(self, n):
        self.ports = self.ports[:n]
        for ip in self.ips:
            ssh = paramiko.SSHClient()
            ssh.set_missing_host_key_policy(paramiko.AutoAddPolicy())
            ssh.connect(ip, 22, "user", "user")
            self.sshs.append(ssh)

    def reset(self):
        _reset_redis(self.sshs)

    def start(self):
        ssh_exec(self.sshs, "./server.sh " + " ".join(str(p) for p in self.ports))
        time.sleep(3)
        for ip in self.ips:
            for port in self.ports:
                conn = redis.Redis(host=ip, port=port, decode_responses=True)
                self.conns.append(conn)
        print("start & connect.")

    def shutdown(self):
        ssh_exec(self.sshs, "./shutdown.sh")
        time.sleep(8)
        print("shutdown.")

    def clean(self):
        ssh_exec(self.sshs, "./clean.sh")
        time.sleep(2)
        print("clean.")

    def construct_repl(self):
        n = len(self.ports)
        repl_cmd = ["REPLICATE", str(len(self.ips) * n), "0"]
        i = 0
        # loop = asyncio.get_event_loop()
        for ip in self.ips:
            for port in self.ports:
                print(" ".join(repl_cmd))
                redis_exec(self.conns[i], *repl_cmd)
                # loop.run_until_complete(aredis_exec(self.conns[i], *repl_cmd))
                i = i + 1
                repl_cmd[2] = str(i)
                repl_cmd.append(self.localhost)
                repl_cmd.append(str(port))
            for k in range(n):
                repl_cmd[-2 * (k + 1)] = ip
        time.sleep(2)
        print("Replication construct complete.")

    def set_delay(self, lo_delay, delay):
        _set_delay(self.sshs[0], lo_delay, delay, self.ips[1], self.ips[2])
        _set_delay(self.sshs[1], lo_delay, delay, self.ips[2], self.ips[0])
        _set_delay(self.sshs[2], lo_delay, delay, self.ips[0], self.ips[1])
        time.sleep(2)
        print("delay set.")

    def remove_delay(self):
        cmd = ("sudo tc filter del dev ens33",
               "sudo tc qdisc del dev ens33 root",
               "sudo tc qdisc del dev lo root")
        for ssh in self.sshs:
            for c in cmd:
                stdin, stdout, stderr = ssh.exec_command(c, get_pty=True)
                stdin.write("user\n")
                stdin.flush()
                # data = stdout.read()
                # if len(data) > 0:
                #     print("d", bytes.decode(data.strip()))  # 打印正确结果
                # err = stderr.read()
                # if len(err) > 0:
                #     print(bytes.decode(err.strip()))  # 输出错误结果
        time.sleep(2)
        print("delay removed.")

    def __del__(self):
        # ssh_exec(self.sshs, "./shutdown.sh " + " ".join(str(p) for p in self.ports))
        # ssh_exec(self.sshs, "./clean.sh " + " ".join(str(p) for p in self.ports))
        for ssh in self.sshs:
            ssh.close()


def clean():
    c = Connection(5)
    c.remove_delay()
    c.shutdown()
    c.clean()


def th_test(conn):
    for i in range(1000):
        redis_exec(conn, "VCINC", "s")


def test():
    c = Connection(3)
    c.start()
    c.construct_repl()
    c.set_delay("10ms 2ms", "100ms 10ms")
    try:
        redis_exec(c.conns[0], "VCNEW", "s")
        time.sleep(1)

        ti = time.perf_counter()
        for k in range(10000):
            redis_exec(c.conns[random.randint(0, 8)], "VCINC", "s")
        print(10000 / (time.perf_counter() - ti))

        redis_exec(c.conns[8], "VCGET", "s", prt=1)
        time.sleep(1)
        redis_exec(c.conns[2], "VCGET", "s", prt=1)

        # loop = asyncio.get_event_loop()
        # loop.run_until_complete(aredis_exec(c.conns[0],"VCNEW", "s"))
        # ti = time.perf_counter()
        # for _i in range(1000):
        #     coros=[]
        #     for k in range(3):
        #         coros.append(aredis_exec(c.conns[k], "VCINC", "s"))
        #     loop.run_until_complete(asyncio.gather(*coros))
        # print(1000/(time.perf_counter() - ti))
        # loop.run_until_complete(aredis_exec(c.conns[1], "VCGET", "s",prt=1))

        # loop = asyncio.get_event_loop()
        # ti = time.perf_counter()
        # loop.run_until_complete(aredis_exec(c.conns[0], "VCNEW", "s"))
        # for k in range(10000):
        #     loop.run_until_complete(aredis_exec(c.conns[random.randint(0, 8)], "VCINC", "s"))
        # loop.run_until_complete(aredis_exec(c.conns[8], "VCGET", "s", prt=1))
        # print(10002 / (time.perf_counter() - ti))

        # threads = []
        # ti = time.perf_counter()
        # redis_exec(c.conns[0], "VCNEW", "s")
        # for i in c.conns:
        #     t = threading.Thread(target=th_test(i))
        #     threads.append(t)
        #     t.start()
        # for t in threads:
        #     t.join()
        # redis_exec(c.conns[2], "VCGET", "s", prt=1)
        # print((1000 * len(c.conns) + 2) / (time.perf_counter() - ti))

    finally:
        c.remove_delay()
        c.shutdown()
        c.clean()


def main(argv):
    n = 3
    delay = "25ms 5ms"
    lo_delay = "5ms 1ms"

    if len(argv) == 1:
        n = int(argv[0])
    elif len(argv) == 4:
        delay = "{}ms {}ms".format(argv[0], argv[1])
        lo_delay = "{}ms {}ms".format(argv[2], argv[3])

    print(n, delay, lo_delay)

    c = Connection(n)

    c.remove_delay()
    c.shutdown()
    c.clean()

    # c.reset()

    c.start()
    c.construct_repl()
    c.set_delay(lo_delay, delay)

    time.sleep(2)


if __name__ == "__main__":
    main(sys.argv[1:])
