import matplotlib.pyplot as plt
import re
import os
import numba as nb


@nb.jit(nopython=True)
def avg(l):
    sum = 0.0
    for a in l:
        sum += a
    sum /= len(l)
    return sum


@nb.jit(nopython=True)
def freq(l):
    sum = 0.0
    for a in l:
        if a != 0:
            sum += 1
    sum /= len(l)
    return sum


def read(ztype, server, op, delay, low_delay, directory='.'):
    d = "{dir}/{t}:{s},{o},({d},{ld})".format(dir=directory, t=ztype, s=server, o=op, d=delay, ld=low_delay)
    ovhd = []
    rmax = []
    for line in open(d + "/s.ovhd"):
        ovhd.append([int(x) for x in line.split(' ')])
    for line in open(d + "/s.max"):
        tmp = line.split(' ')
        rmax.append(abs(float(tmp[1]) - float(tmp[3])))
    return rmax, ovhd


def cmp_or(name, directory, server=9, op=10000, delay=50, low_delay=10):
    xlable = 'time: second'
    ormax, oovhd = read("o", server, op, delay, low_delay, directory=directory)
    rrmax, rovhd = read("r", server, op, delay, low_delay, directory=directory)
    lmax = min(len(ormax), len(rrmax))
    lovhd = min(len(oovhd), len(rovhd))
    x1 = [i for i in range(lovhd)]
    y1o = [b / a for a, b in oovhd][:lovhd]
    y1r = [b / a for a, b in rovhd][:lovhd]
    x2 = [i for i in range(lmax)]
    y2o = ormax[:lmax]
    y2r = rrmax[:lmax]

    plt.figure(figsize=(11, 4))

    plt.subplot(1, 2, 1)
    plot_line(y2o, y2r, x2, xlable, 'read max diff')

    plt.subplot(1, 2, 2)
    plot_line(y1o, y1r, x1, xlable, 'overhead: byte')

    plt.tight_layout()
    plt.savefig("{}.pdf".format(name), format='pdf')
    plt.show()

    print(name)
    print("add-win",avg(y2o), freq(y2o))
    print("rmv-win",avg(y2r), freq(y2r))


def preliminary_dispose(oms, oos, rms, ros):
    om_avg = []
    om_count = []
    for m in oms:
        om_avg.append(avg(m))
        om_count.append(freq(m))
    rm_avg = []
    rm_count = []
    for m in rms:
        rm_avg.append(avg(m))
        rm_count.append(freq(m))

    oo_max = []
    for o in oos:
        oo_max.append(o[-1][1] / o[-1][0])
    ro_max = []
    for o in ros:
        ro_max.append(o[-1][1] / o[-1][0])

    return om_avg, rm_avg, om_count, rm_count, oo_max, ro_max


def plot_bar(o_data, r_data, name, x_lable, y_lable, s_name=None):
    if s_name is None:
        s_name = name
    bar_width = 0.35
    index1 = [x for x in range(len(name))]
    index2 = [x + bar_width for x in range(len(name))]
    index = [x + bar_width / 2 for x in range(len(name))]

    l1 = plt.bar(index1, o_data, bar_width, tick_label=s_name)
    l2 = plt.bar(index2, r_data, bar_width, tick_label=s_name)
    plt.xlabel(x_lable)
    plt.ylabel(y_lable)
    plt.legend(handles=[l1, l2, ], labels=['Add-Win', 'Rmv-Win'], loc='best')
    plt.xticks(index)


def plot_line(o_data, r_data, name, x_lable, y_lable):
    plt.plot(name, o_data, linestyle="-", label="Add-Win")
    plt.plot(name, r_data, linestyle="-", label="Rmv-Win")
    plt.xlabel(x_lable)
    plt.ylabel(y_lable)
    plt.legend(loc='upper left')


def cmp_delay(rounds, low=0, high=0):
    delays = ["{hd}ms,\n{ld}ms".format(hd=20 + x * 40, ld=4 + x * 8) for x in range(10)]
    dirs = ["delay/{}".format(x) for x in range(rounds)]

    om_avg = [[] for i in range(len(delays))]
    rm_avg = [[] for i in range(len(delays))]
    om_count = [[] for i in range(len(delays))]
    rm_count = [[] for i in range(len(delays))]
    oo_max = [[] for i in range(len(delays))]
    ro_max = [[] for i in range(len(delays))]

    for d in dirs:
        a, b, c, d, e, f = _cmp_delay(delays, d)
        for i in range(len(delays)):
            om_avg[i].append(a[i])
            rm_avg[i].append(b[i])
            om_count[i].append(c[i])
            rm_count[i].append(d[i])
            oo_max[i].append(e[i])
            ro_max[i].append(f[i])
    for i in range(len(delays)):
        om_avg[i].sort()
        rm_avg[i].sort()
        om_count[i].sort()
        rm_count[i].sort()
        oo_max[i].sort()
        ro_max[i].sort()
        om_avg[i] = avg(om_avg[i][low:(len(dirs) - high)])
        rm_avg[i] = avg(rm_avg[i][low:(len(dirs) - high)])
        om_count[i] = avg(om_count[i][low:(len(dirs) - high)])
        rm_count[i] = avg(rm_count[i][low:(len(dirs) - high)])
        oo_max[i] = avg(oo_max[i][low:(len(dirs) - high)])
        ro_max[i] = avg(ro_max[i][low:(len(dirs) - high)])
    return delay_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, delays)


def _cmp_delay(delays, directory, plot=False):
    oms = []
    oos = []
    for d in delays:
        match = re.match(r'(\d+)ms,\n(\d+)ms', d)
        hd = match.group(1)
        ld = match.group(2)
        m, o = read("o", 9, 10000, hd, ld, directory=directory)
        oms.append(m)
        oos.append(o)

    rms = []
    ros = []
    for d in delays:
        match = re.match(r'(\d+)ms,\n(\d+)ms', d)
        hd = match.group(1)
        ld = match.group(2)
        m, o = read("r", 9, 10000, hd, ld, directory=directory)
        rms.append(m)
        ros.append(o)

    om_avg, rm_avg, om_count, rm_count, oo_max, ro_max = preliminary_dispose(oms, oos, rms, ros)

    if plot:
        delay_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, delays)
    return om_avg, rm_avg, om_count, rm_count, oo_max, ro_max


def delay_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, name):
    xlable = 'latency: between DC, within DC'
    pname = 'delay'

    # plt.figure(figsize=(18, 4))
    plt.figure(figsize=(11, 4))

    # plt.subplot(1, 3, 1)
    plt.subplot(1, 2, 1)
    plot_bar(om_avg, rm_avg, name, xlable, 'average read_max diff')

    # plt.subplot(1, 3, 2)
    plt.subplot(1, 2, 2)
    plot_bar(om_count, rm_count, name, xlable, 'frequency of read_max being wrong')

    # plt.subplot(1, 3, 3)
    # plot_bar(oo_max, ro_max, name, xlable, 'average max overhead per element: bytes')

    plt.tight_layout()
    plt.savefig("{}.pdf".format(pname), format='pdf')
    plt.show()

    rtn = (oo_max, ro_max, name, xlable, 'average max overhead per element: bytes')
    return rtn


def cmp_replica(rounds, low=0, high=0):
    replicas = [3, 6, 9, 12, 15]
    dirs = ["replica/{}".format(x) for x in range(rounds)]
    om_avg = [[] for i in range(len(replicas))]
    rm_avg = [[] for i in range(len(replicas))]
    om_count = [[] for i in range(len(replicas))]
    rm_count = [[] for i in range(len(replicas))]
    oo_max = [[] for i in range(len(replicas))]
    ro_max = [[] for i in range(len(replicas))]

    for d in dirs:
        a, b, c, d, e, f = _cmp_replica(replicas, d)
        for i in range(len(replicas)):
            om_avg[i].append(a[i])
            rm_avg[i].append(b[i])
            om_count[i].append(c[i])
            rm_count[i].append(d[i])
            oo_max[i].append(e[i])
            ro_max[i].append(f[i])
    for i in range(len(replicas)):
        om_avg[i].sort()
        rm_avg[i].sort()
        om_count[i].sort()
        rm_count[i].sort()
        oo_max[i].sort()
        ro_max[i].sort()
        om_avg[i] = avg(om_avg[i][low:(len(dirs) - high)])
        rm_avg[i] = avg(rm_avg[i][low:(len(dirs) - high)])
        om_count[i] = avg(om_count[i][low:(len(dirs) - high)])
        rm_count[i] = avg(rm_count[i][low:(len(dirs) - high)])
        oo_max[i] = avg(oo_max[i][low:(len(dirs) - high)])
        ro_max[i] = avg(ro_max[i][low:(len(dirs) - high)])
    return replica_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, replicas)


def _cmp_replica(replicas, directory, plot=False):
    oms = []
    oos = []
    for r in replicas:
        m, o = read("o", r, 10000, 50, 10, directory=directory)
        oms.append(m)
        oos.append(o)

    rms = []
    ros = []
    for r in replicas:
        m, o = read("r", r, 10000, 50, 10, directory=directory)
        rms.append(m)
        ros.append(o)

    om_avg, rm_avg, om_count, rm_count, oo_max, ro_max = preliminary_dispose(oms, oos, rms, ros)

    if plot:
        replica_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, replicas)
    return om_avg, rm_avg, om_count, rm_count, oo_max, ro_max


def replica_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, name):
    xlable = 'num of replicas'
    pname = 'replica'

    # plt.figure(figsize=(16, 4))
    plt.figure(figsize=(11, 4))

    # plt.subplot(1, 3, 1)
    plt.subplot(1, 2, 1)
    plot_bar(om_avg, rm_avg, name, xlable, 'average read_max diff')

    # plt.subplot(1, 3, 2)
    plt.subplot(1, 2, 2)
    plot_bar(om_count, rm_count, name, xlable, 'frequency of read_max being wrong')

    # plt.subplot(1, 3, 3)
    # plot_bar(oo_max, ro_max, name, xlable, 'average max overhead per element: bytes')

    plt.tight_layout()
    plt.savefig("{}.pdf".format(pname), format='pdf')
    plt.show()

    rtn = (oo_max, ro_max, name, xlable, 'average max overhead per element: bytes')
    return rtn


def cmp_speed(rounds, low=0, high=0):
    speeds = [500 + x * 100 for x in range(96)]
    dirs = ["speed/{}".format(x) for x in range(rounds)]

    om_avg = [[] for i in range(len(speeds))]
    rm_avg = [[] for i in range(len(speeds))]
    om_count = [[] for i in range(len(speeds))]
    rm_count = [[] for i in range(len(speeds))]
    oo_max = [[] for i in range(len(speeds))]
    ro_max = [[] for i in range(len(speeds))]

    for d in dirs:
        a, b, c, d, e, f = _cmp_speed(speeds, d)
        for i in range(len(speeds)):
            om_avg[i].append(a[i])
            rm_avg[i].append(b[i])
            om_count[i].append(c[i])
            rm_count[i].append(d[i])
            oo_max[i].append(e[i])
            ro_max[i].append(f[i])
    for i in range(len(speeds)):
        om_avg[i].sort()
        rm_avg[i].sort()
        om_count[i].sort()
        rm_count[i].sort()
        oo_max[i].sort()
        ro_max[i].sort()
        om_avg[i] = avg(om_avg[i][low:(len(dirs) - high)])
        rm_avg[i] = avg(rm_avg[i][low:(len(dirs) - high)])
        om_count[i] = avg(om_count[i][low:(len(dirs) - high)])
        rm_count[i] = avg(rm_count[i][low:(len(dirs) - high)])
        oo_max[i] = avg(oo_max[i][low:(len(dirs) - high)])
        ro_max[i] = avg(ro_max[i][low:(len(dirs) - high)])
    return speed_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, speeds)


def _cmp_speed(speeds, directory, plot=False):
    oms = []
    oos = []
    for n in speeds:
        m, o = read("o", 9, n, 50, 10, directory=directory)
        oms.append(m)
        oos.append(o)

    rms = []
    ros = []
    for n in speeds:
        m, o = read("r", 9, n, 50, 10, directory=directory)
        rms.append(m)
        ros.append(o)

    om_avg, rm_avg, om_count, rm_count, oo_max, ro_max = preliminary_dispose(oms, oos, rms, ros)

    if plot:
        speed_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, speeds)
    return om_avg, rm_avg, om_count, rm_count, oo_max, ro_max


def speed_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, name):
    xlable = 'op/second'
    pname = 'op_speed'
    s_name = [x for x in name]
    for i in range(len(s_name)):
        if (i - 500) % 10 != 0:
            s_name[i] = ''

    # plt.figure(figsize=(16, 4))
    plt.figure(figsize=(11, 4))

    # plt.subplot(1, 3, 1)
    plt.subplot(1, 2, 1)
    # plot_bar(om_avg, rm_avg, name, xlable, 'average read_max diff', s_name=s_name)
    plot_line(om_avg, rm_avg, name, xlable, 'average read_max diff')

    # plt.subplot(1, 3, 2)
    plt.subplot(1, 2, 2)
    # plot_bar(om_count, rm_count, name, xlable, 'frequency of read_max being wrong')
    plot_line(om_count, rm_count, name, xlable, 'frequency of read_max being wrong')

    # plt.subplot(1, 3, 3)
    # plot_bar(oo_max, ro_max, name, xlable, 'average max overhead per element: bytes', s_name=s_name)

    plt.tight_layout()
    plt.savefig("{}.pdf".format(pname), format='pdf')
    plt.show()

    rtn = (oo_max, ro_max, name, xlable, 'average max overhead per element: bytes')
    return rtn, s_name


def speed_check(sp, rounds):
    dirs = ["speed/{}".format(x) for x in range(rounds)]
    for i, d in enumerate(dirs):
        a, b, c, d, e, f = _cmp_speed([sp], d)
        print(i, ": ", a[0], b[0], c[0], d[0])


def replica_check(r, rounds):
    dirs = ["replica/{}".format(x) for x in range(rounds)]
    for i, d in enumerate(dirs):
        a, b, c, d, e, f = _cmp_replica([r], d)
        if d[0]>0.1:
            print(i, ": ", a[0], b[0], c[0], d[0])


def delay_check(delay, rounds):
    _delay = "{hd}ms,\n{ld}ms".format(hd=delay, ld=int(delay / 5))
    dirs = ["delay/{}".format(x) for x in range(rounds)]
    for i, d in enumerate(dirs):
        a, b, c, d, e, f = _cmp_delay([_delay], d)
        print(i, ": ", a[0], b[0], c[0], d[0])


# dirs = ["delay/{}".format(x) for x in range(10)]
# for d in dirs:
#     cd = os.listdir(d)
#     for f in cd:
#         match = re.match(r'(.+)\((\d+),(\d+)\)', f)
#         os.rename(os.path.join(d, f),
#                   os.path.join(d, match.group(1) + "({},{})".format(int(match.group(2)) * 2, int(match.group(3)) * 2)))

cmp_or("inc_d","replica/0")
cmp_or("ar_d","ardominant")
# speed_check(6600, 30)
sp,sn=cmp_speed(30)
# replica_check(9, 30)
rp=cmp_replica(30)
# delay_check(380, 30)
dl=cmp_delay(30)

plt.figure(figsize=(11, 4))

plt.subplot(1, 2, 1)
plot_bar(*sp,s_name=sn)

plt.subplot(1, 2, 2)
plot_bar(*dl)

plt.tight_layout()
plt.savefig("ovhd_sd.pdf", format='pdf')
plt.show()

plt.figure(figsize=(6, 4))
plot_bar(*rp)
plt.savefig("ovhd_r.pdf", format='pdf')
plt.show()
