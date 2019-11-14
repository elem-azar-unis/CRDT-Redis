import os
import redis


def read_ops(path):
    ops = []
    for line in open(path):
        ops.append(line.strip('\n'))
    return ops


def test(ops, t, order):
    os.system("./shutdown.sh 6379 1>/dev/null")
    os.system("./clean.sh 6379 1>/dev/null")
    os.system("./server.sh 6379 1>/dev/null")
    conn = redis.Redis(host='localhost', port=6379, decode_responses=True)
    conn.execute_command("repltest", "3", "0")
    for i in order:
        if i == -1:
            print("---------------\n",
                  "score:", conn.execute_command(t + "zscore", "s", "a"), "\n",
                  "\n".join(conn.execute_command(t + "zestatus", "s", "a")),
                  "\n---------------")
        else:
            print(len(ops[i].split(' ')), ops[i])
            conn.execute_command(*ops[i].split(' '))
            print(conn.execute_command(t + "zoverhead", "s"))


def main():
    oops = [
        "ozadd s a 0 0,0",
        "ozadd s a 1 0,1",
        "ozadd s a 2 0,2",
        "ozincrby s a 1 0,0 0,1",
        "ozincrby s a -1 0,0 0,1 0,2",
        "ozincrby s a 1 0,1 0,2",
        "ozrem s a 0,0 0,1"
    ]
    ozorders = [
        [0, 1, -1, 3, 4, 5, 6, 2, -1],
        [1, 2, 0, 4, 3, -1, 6, 5, -1],
        [2, 1, 5, 4, 3, -1, 6, 0, -1]
    ]

    rops1 = [
        "rzadd s a 0 0,0,0|0",
        "rzadd s a 1 0,0,0|1",
        "rzincrby s a 1 0,0,0|0",
        "rzincrby s a -2 0,0,0|1",
        "rzrem s a 1,0,0|0",
        "rzrem s a 0,0,1|2",
        "rzadd s a 2 1,0,1|2"
    ]
    rzorders1 = [
        [0, 2, -1, 3, -1, 1, 4, 5, 6, -1],
        [0, 2, -1, 3, -1, 1, 4, 6, -1, 5, -1],
        [1, 0, -1, 2, 3, 5, 6, -1, 4, -1],
        [1, 2, 5, -1, 0, -1, 3, -1, 4, 6, -1]
    ]

    rops2 = [
        "rzincrby s a 2 0,1,1|2",
        "rzincrby s a 4 1,1,0|1",
        "rzadd s a 3 1,1,0|0",
        "rzadd s a 8 1,1,1|2",
        "rzrem s a 1,0,0|0",
        "rzrem s a 1,1,0|2",
        "rzrem s a 0,0,1|1",
        "rzrem s a 1,1,1|1"
    ]
    rzorder2 = [0,-1, 1,-1, 2,-1, 3, -1, 4, -1, 5, -1, 6, -1,7,-1]

    test(oops, "o", ozorders[2])


if __name__ == "__main__":
    main()
