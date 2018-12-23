import matplotlib.pyplot as plt
import re


def read(ztype, server, op, delay, low_delay, directory='.'):
    d = "{dir}/{t}:{s},{o},({d},{ld})".format(dir=directory, t=ztype, s=server, o=op, d=delay, ld=low_delay)
    of = open(d + "/s.ovhd")
    mf = open(d + "/s.max")
    ovhd = []
    rmax = []
    for line in of:
        ovhd.append([int(x) for x in line.split(' ')])
    for line in mf:
        tmp = [float(x) for x in line.split(' ')]
        tmp[0] = int(tmp[0])
        tmp[2] = int(tmp[2])
        rmax.append(tmp)
    return rmax, ovhd


def cmp_or(server, op, delay, low_delay):
    xlable = 'time: second'
    ormax, oovhd = read("o", server, op, delay, low_delay)
    rrmax, rovhd = read("r", server, op, delay, low_delay)
    lmax = min(len(ormax), len(rrmax))
    lovhd = min(len(oovhd), len(rovhd))
    x1 = [i for i in range(lovhd)]
    y1o = [b / a for a, b in oovhd][:lovhd]
    y1r = [b / a for a, b in rovhd][:lovhd]
    x2 = [i for i in range(lmax)]
    y2o = [abs(a - b) for _, a, _, b in ormax][:lmax]
    y2r = [abs(a - b) for _, a, _, b in rrmax][:lmax]

    plot_line(y1o, y1r, x1, xlable, 'overhead: byte')
    plt.show()

    plot_line(y2o, y2r, x2, xlable, 'read max diff')
    plt.show()


def preliminary_dispose(oms, oos, rms, ros):
    for i in range(len(oms)):
        oms[i] = [abs(a - b) for _, a, _, b in oms[i]]
    for i in range(len(rms)):
        rms[i] = [abs(a - b) for _, a, _, b in rms[i]]

    om_avg = []
    om_count = []
    for m in oms:
        om_avg.append(sum(m) / len(m))
        om_count.append((len(m) - m.count(0)) / len(m))
    rm_avg = []
    rm_count = []
    for m in rms:
        rm_avg.append(sum(m) / len(m))
        rm_count.append((len(m) - m.count(0)) / len(m))

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
    plt.legend(loc='best')


def cmp_delay():
    delays = ["{hd}ms,{ld}ms".format(hd=10+x*20,ld=2+x*4) for x in range(10)]
    dirs = ["delay/{}".format(x) for x in range(10)]
    om_avg = [0] * len(delays)
    rm_avg = [0] * len(delays)
    om_count = [0] * len(delays)
    rm_count = [0] * len(delays)
    oo_max = [0] * len(delays)
    ro_max = [0] * len(delays)

    for d in dirs:
        a, b, c, d, e, f = _cmp_delay(delays, d)
        for i in range(len(delays)):
            om_avg[i] += a[i]
            rm_avg[i] += b[i]
            om_count[i] += c[i]
            rm_count[i] += d[i]
            oo_max[i] += e[i]
            ro_max[i] += f[i]
    for i in range(len(delays)):
        om_avg[i] /= len(dirs)
        rm_avg[i] /= len(dirs)
        om_count[i] /= len(dirs)
        rm_count[i] /= len(dirs)
        oo_max[i] /= len(dirs)
        ro_max[i] /= len(dirs)
    delay_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, replicas)


def _cmp_delay(delays, directory, plot=False):
    oms = []
    oos = []
    for d in delays:
        match=re.compile(r"\d+ms,\d+ms").match(d)
        hd=match.groups()[0]
        ld=match.groups()[1]
        m, o = read("o", 9, 10000, hd, ld, directory=directory)
        oms.append(m)
        oos.append(o)

    rms = []
    ros = []
    for d in delays:
        match = re.compile(r"\d+ms,\d+ms").match(d)
        hd = match.groups()[0]
        ld = match.groups()[1]
        m, o = read("r", r, 10000, 50, 10, directory=directory)
        rms.append(m)
        ros.append(o)

    om_avg, rm_avg, om_count, rm_count, oo_max, ro_max = preliminary_dispose(oms, oos, rms, ros)

    if plot:
        delay_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, replicas)
    return om_avg, rm_avg, om_count, rm_count, oo_max, ro_max


def delay_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, name):
    xlable = 'latency: between DC, within DC'
    pname = 'delay'

    plt.figure(figsize=(16, 4))

    plt.subplot(1, 3, 1)
    plot_bar(om_avg, rm_avg, name, xlable, 'average read_max diff')

    plt.subplot(1, 3, 2)
    plot_bar(om_count, rm_count, name, xlable, 'frequency of read_max being wrong')

    plt.subplot(1, 3, 3)
    plot_bar(oo_max, ro_max, name, xlable, 'average max overhead per element: bytes')

    plt.tight_layout()
    plt.savefig("{}.pdf".format(pname), format='pdf')
    plt.show()


def cmp_replica():
    replicas = [3, 6, 9, 12, 15]
    dirs = ["replica/{}".format(x) for x in range(10)]
    om_avg = [0] * len(replicas)
    rm_avg = [0] * len(replicas)
    om_count = [0] * len(replicas)
    rm_count = [0] * len(replicas)
    oo_max = [0] * len(replicas)
    ro_max = [0] * len(replicas)

    for d in dirs:
        a, b, c, d, e, f = _cmp_replica(replicas, d)
        for i in range(len(replicas)):
            om_avg[i] += a[i]
            rm_avg[i] += b[i]
            om_count[i] += c[i]
            rm_count[i] += d[i]
            oo_max[i] += e[i]
            ro_max[i] += f[i]
    for i in range(len(replicas)):
        om_avg[i] /= len(dirs)
        rm_avg[i] /= len(dirs)
        om_count[i] /= len(dirs)
        rm_count[i] /= len(dirs)
        oo_max[i] /= len(dirs)
        ro_max[i] /= len(dirs)
    replica_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, replicas)


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

    plt.figure(figsize=(16, 4))

    plt.subplot(1, 3, 1)
    plot_bar(om_avg, rm_avg, name, xlable, 'average read_max diff')

    plt.subplot(1, 3, 2)
    plot_bar(om_count, rm_count, name, xlable, 'frequency of read_max being wrong')

    plt.subplot(1, 3, 3)
    plot_bar(oo_max, ro_max, name, xlable, 'average max overhead per element: bytes')

    plt.tight_layout()
    plt.savefig("{}.pdf".format(pname), format='pdf')
    plt.show()


def cmp_speed():
    speeds = [500 + x * 100 for x in range(96)]
    dirs = ["speed/{}".format(x) for x in range(5)]

    om_avg = [0] * len(speeds)
    rm_avg = [0] * len(speeds)
    om_count = [0] * len(speeds)
    rm_count = [0] * len(speeds)
    oo_max = [0] * len(speeds)
    ro_max = [0] * len(speeds)

    for d in dirs:
        a, b, c, d, e, f = _cmp_speed(speeds, d)
        for i in range(len(speeds)):
            om_avg[i] += a[i]
            rm_avg[i] += b[i]
            om_count[i] += c[i]
            rm_count[i] += d[i]
            oo_max[i] += e[i]
            ro_max[i] += f[i]
    for i in range(len(speeds)):
        om_avg[i] /= len(dirs)
        rm_avg[i] /= len(dirs)
        om_count[i] /= len(dirs)
        rm_count[i] /= len(dirs)
        oo_max[i] /= len(dirs)
        ro_max[i] /= len(dirs)
    speed_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, speeds)


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


def test_1():
    name = ["25ms,5ms", "50ms,10ms", "100ms,20ms"]
    ms = []
    os = []
    m, o = read("o", 9, 10000, 25, 5)
    ms.append(m)
    os.append(o)
    m, o = read("o", 9, 10000, 50, 10)
    ms.append(m)
    os.append(o)
    m, o = read("o", 9, 10000, 100, 20)
    ms.append(m)
    os.append(o)
    plot(ms, os, name, 'latency: between DC, within DC')


def speed_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, name):
    xlable = 'op/second'
    pname = 'op_speed'
    s_name = [x for x in name]
    for i in range(len(s_name)):
        if (i - 500) % 10 != 0:
            s_name[i] = ''

    plt.figure(figsize=(16, 4))

    plt.subplot(1, 3, 1)
    plot_bar(om_avg, rm_avg, name, xlable, 'average read_max diff')

    plt.subplot(1, 3, 2)
    # plot_bar(om_count, rm_count, name, xlable, 'frequency of read_max being wrong')
    plot_line(om_count, rm_count, name, xlable, 'frequency of read_max being wrong')

    plt.subplot(1, 3, 3)
    plot_bar(oo_max, ro_max, name, xlable, 'average max overhead per element: bytes')

    plt.tight_layout()
    plt.savefig("{}.pdf".format(pname), format='pdf')
    plt.show()


def speed_check(sp):
    dirs = ["speed/{}".format(x) for x in range(5)]
    for d in dirs:
        a, b, c, d, e, f = _cmp_speed([sp], d)
        print(a[0], b[0], c[0], d[0])


def replica_check(r):
    dirs = ["replica/{}".format(x) for x in range(10)]
    for d in dirs:
        a, b, c, d, e, f = _cmp_replicas([r], d)
        print(a[0], b[0], c[0], d[0])


def delay_check(d):
    dirs = ["delay/{}".format(x) for x in range(10)]
    for d in dirs:
        a, b, c, d, e, f = _cmp_delay([r], d)
        print(a[0], b[0], c[0], d[0])


# speed_check(800)
# cmp_speed()
# replica_check(15)
cmp_replica()
