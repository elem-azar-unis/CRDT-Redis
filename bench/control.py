# coding=utf-8
import random
import time
import paramiko
import redis


# async def aredis_exec(conn, *cm, prt=0):
#     try:
#         r = await conn.execute_command(*cm)
#         if prt != 0:
#             print(r)
#     except redis.exceptions.ResponseError as e:
#         print(repr(e))


def redis_exec(conn, prt, *cm):
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


def _set_delay(ssh, lo_delay, delay, ip1, ip2):
    cmd = (
        "sudo tc qdisc del dev ens33 root",
        "sudo tc qdisc add dev ens33 root handle 1: prio",
        "sudo tc qdisc add dev ens33 parent 1:1 handle 10: netem delay {delay}".format(delay=delay),
        "sudo tc qdisc add dev ens33 parent 1:2 handle 20: pfifo_fast",
        "sudo tc filter del dev ens33",
        "sudo tc filter add dev ens33 protocol ip parent 1: prio 1 u32 match ip dst {ip1} flowid 1:1".format(ip1=ip1),
        "sudo tc filter add dev ens33 protocol ip parent 1: prio 1 u32 match ip dst {ip2} flowid 1:1".format(ip2=ip2),
        "sudo tc qdisc del dev lo root",
        "sudo tc qdisc add dev lo root netem delay {lo_delay}".format(lo_delay=lo_delay)
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
    ips = ("192.168.188.128",
           "192.168.188.129",
           "192.168.188.130")
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

    def start(self):
        ssh_exec(self.sshs, "./server.sh " + " ".join(str(p) for p in self.ports))
        for ip in self.ips:
            for port in self.ports:
                conn = redis.Redis(host=ip, port=port, decode_responses=True)
                self.conns.append(conn)
        print("start & connect.")

    def shutdown(self):
        ssh_exec(self.sshs, "./shutdown.sh " + " ".join(str(p) for p in self.ports))
        print("shutdown.")

    def clean(self):
        ssh_exec(self.sshs, "./clean.sh " + " ".join(str(p) for p in self.ports))
        print("clean.")

    def construct_repl(self):
        n = len(self.ports)
        repl_cmd = ["REPLICATE", str(len(self.ips) * n), "0"]
        i = 0
        # loop = asyncio.get_event_loop()
        for ip in self.ips:
            for port in self.ports:
                redis_exec(self.conns[i], 0, *repl_cmd)
                # print(" ".join(repl_cmd))
                # loop.run_until_complete(aredis_exec(self.conns[i], *repl_cmd))
                i = i + 1
                repl_cmd[2] = str(i)
                repl_cmd.append(self.localhost)
                repl_cmd.append(str(port))
            for k in range(n):
                repl_cmd[-2 * (k + 1)] = ip
        time.sleep(1)
        print("Replication construct complete.")

    def set_delay(self, lo_delay, delay):
        _set_delay(self.sshs[0], lo_delay, delay, self.ips[1], self.ips[2])
        _set_delay(self.sshs[1], lo_delay, delay, self.ips[2], self.ips[0])
        _set_delay(self.sshs[2], lo_delay, delay, self.ips[0], self.ips[1])
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

    def __del__(self):
        # ssh_exec(self.sshs, "./shutdown.sh " + " ".join(str(p) for p in self.ports))
        # ssh_exec(self.sshs, "./clean.sh " + " ".join(str(p) for p in self.ports))
        for ssh in self.sshs:
            ssh.close()


def clean():
    c = Connection(5)
    c.shutdown()
    c.clean()
    c.remove_delay()
    time.sleep(1)
    print("Clean done.")


# def th_test(conn):
#     for i in range(1000):
#         redis_exec(conn,0, "VCINC", "s")


def test():
    c = Connection(3)
    c.start()
    c.construct_repl()
    c.set_delay("10ms 2ms", "100ms 10ms")
    try:
        redis_exec(c.conns[0], 0, "VCNEW", "s")
        time.sleep(1)

        ti = time.perf_counter()
        for k in range(10000):
            redis_exec(c.conns[random.randint(0, 8)], 0, "VCINC", "s")
        print(10000 / (time.perf_counter() - ti))

        redis_exec(c.conns[8], 1, "VCGET", "s")
        time.sleep(1)
        redis_exec(c.conns[2], 1, "VCGET", "s")

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
        # redis_exec(c.conns[0],0, "VCNEW", "s")
        # for i in c.conns:
        #     t = threading.Thread(target=th_test(i))
        #     threads.append(t)
        #     t.start()
        # for t in threads:
        #     t.join()
        # redis_exec(c.conns[2],0, "VCGET", "s")
        # print((1000 * len(c.conns) + 2) / (time.perf_counter() - ti))

    finally:
        c.remove_delay()
        c.shutdown()
        c.clean()


# clean()
test()