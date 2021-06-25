from typing import Iterable, List


def read(fp: str, n: int) -> Iterable[List[str]]:
    i = 0
    lines = []  # a buffer to cache lines
    with open(fp, "r") as f:
        for line in f:
            i += 1
            lines.append(line.strip())  # append a line
            if i >= n:
                yield lines
                # reset buffer
                i = 0
                lines.clear()
    # remaining lines
    if i > 0:
        yield lines


def parse_history(h):
    h = h[4:-4].replace(' ', '').replace(',\"e\"', '').replace('\"', '')
    h = h.replace('>>,<<', ' ; ').replace(',', ' ')
    return h + ' ; '


def parse_oracle(e, t):
    rtn = ""
    if e == r'{}':
        rtn += "n n n "
    else:
        e = e[2:-2].replace(' ', '').replace('\"', '')
        e = {a.split('|->')[0]: a.split('|->')[1] for a in e.split(',')}
        rtn += f"{e['p_ini']} {e['v_inn']} {e['v_acq']} "

    if t == r'{}':
        rtn += "n"
    else:
        t = t[2:-2].replace(' ', '').replace('\"', '')
        t = t.replace('<<', '').replace('>>', '')
        t = t.split(',')
        # for 't|->0,1,0,1' is splited with ','
        while len(t) > 2:
            t[-2] = t[-2] + ',' + t[-1]
            t.pop()
        t = {a.split('|->')[0]: a.split('|->')[1] for a in t}
        rtn += t['t']

    return rtn


N = 3
ops = 3

with open(f"rwf_rpq_{N}_{ops}.script", "w") as file:
    file.write(str(N) + '\n')
    for lines in read("result.txt", 3):
        file.write(parse_history(lines[0]) + '\n')
        file.write(parse_oracle(lines[1], lines[2]) + '\n')
