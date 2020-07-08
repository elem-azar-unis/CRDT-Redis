import matplotlib.pyplot as plt
import matplotlib
import statistics

data_skiped = 0
err_sp = 1e10

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['savefig.bbox'] = 'tight'
matplotlib.rcParams['savefig.pad_inches'] = 0.01
matplotlib.rcParams['legend.loc'] = 'best'
matplotlib.rcParams['xtick.labelsize'] = 8
matplotlib.rcParams['ytick.labelsize'] = 8


def freq(li):
    return len([x for x in li if x != 0]) / len(li)


def read(ztype, server, op, delay, low_delay, root_dir='.'):
    d = "{dir}/{t}_{s},{o},({d},{ld})".format(dir=root_dir,
                                              t=ztype, s=server, o=op, d=delay, ld=low_delay)
    global data_skiped
    ovhd = []
    rmax = []
    for line in open(d + "/ovhd.csv"):
        tmp = [float(x) for x in line.split(',')]
        if tmp[0] > err_sp or tmp[1] > err_sp:
            data_skiped += 1
            continue
        ovhd.append(tmp[1] / tmp[0])
    for line in open(d + "/max.csv"):
        tmp = [float(x) for x in line.split(',')]
        if tmp[1] > err_sp or tmp[3] > err_sp:
            data_skiped += 1
            continue
        rmax.append(abs(tmp[1] - tmp[3]))
    return rmax, ovhd


def cmp_or(name, directory, server=9, op=10000, delay=50, low_delay=10):
    xlable = 'time: second'
    ormax, oovhd = read("o", server, op, delay, low_delay, root_dir=directory)
    rrmax, rovhd = read("r", server, op, delay, low_delay, root_dir=directory)
    lmax = min(len(ormax), len(rrmax))
    lovhd = min(len(oovhd), len(rovhd))
    x1 = [i for i in range(lovhd)]
    y1o = oovhd[:lovhd]
    y1r = rovhd[:lovhd]
    x2 = [i for i in range(lmax)]
    y2o = ormax[:lmax]
    y2r = rrmax[:lmax]

    fig = plt.figure(figsize=(11, 4))

    plt.subplot(1, 2, 1)
    plot_line(y2o, y2r, x2, xlable, 'read max diff')

    plt.subplot(1, 2, 2)
    plot_line(y1o, y1r, x1, xlable, 'overhead: byte')

    plt.tight_layout()
    plt.savefig("{}.pdf".format(name))
    plt.close(fig)

    print(name)
    print("add-win", statistics.mean(y2o), freq(y2o))
    print("rmv-win", statistics.mean(y2r), freq(y2r))


def preliminary_dispose(oms, oos, rms, ros):
    om_avg = []
    om_count = []
    for m in oms:
        om_avg.append(statistics.mean(m))
        om_count.append(freq(m))
    rm_avg = []
    rm_count = []
    for m in rms:
        rm_avg.append(statistics.mean(m))
        rm_count.append(freq(m))

    oo_max = []
    for o in oos:
        oo_max.append(o[-1])
    ro_max = []
    for o in ros:
        ro_max.append(o[-1])

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
    plt.legend(handles=[l1, l2, ], labels=['Add-Win', 'Rmv-Win'])
    plt.xticks(index)


def plot_line(o_data, r_data, name, x_lable, y_lable):
    plt.plot(name, o_data, linestyle="-", label="Add-Win")
    plt.plot(name, r_data, linestyle="-", label="Rmv-Win")
    plt.xlabel(x_lable)
    plt.ylabel(y_lable)
    plt.legend()


def _cmp_generic(exp_settings, directory, x_paras=None, plot_func=None):
    oms = []
    oos = []
    for setting in exp_settings:
        setting[-1] = directory
        m, o = read("o", *setting)
        oms.append(m)
        oos.append(o)

    rms = []
    ros = []
    for setting in exp_settings:
        setting[-1] = directory
        m, o = read("r", *setting)
        rms.append(m)
        ros.append(o)

    om_avg, rm_avg, om_count, rm_count, oo_max, ro_max = preliminary_dispose(
        oms, oos, rms, ros)

    if plot_func is not None and x_paras is not None:
        plot_func(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, x_paras)
    return om_avg, rm_avg, om_count, rm_count, oo_max, ro_max


def cmp_generic(x_paras, dirs, exp_settings, plot_func, low, high):
    om_avg_collect = [[] for i in range(len(x_paras))]
    rm_avg_collect = [[] for i in range(len(x_paras))]
    om_count_collect = [[] for i in range(len(x_paras))]
    rm_count_collect = [[] for i in range(len(x_paras))]
    oo_max_collect = [[] for i in range(len(x_paras))]
    ro_max_collect = [[] for i in range(len(x_paras))]

    for setting in exp_settings:
        setting.append("")
    for root_dir in dirs:
        a, b, c, d, e, f = _cmp_generic(exp_settings, root_dir)
        for i in range(len(x_paras)):
            om_avg_collect[i].append(a[i])
            rm_avg_collect[i].append(b[i])
            om_count_collect[i].append(c[i])
            rm_count_collect[i].append(d[i])
            oo_max_collect[i].append(e[i])
            ro_max_collect[i].append(f[i])

    om_avg = []
    rm_avg = []
    om_count = []
    rm_count = []
    oo_max = []
    ro_max = []
    for i in range(len(x_paras)):
        om_avg.append(statistics.mean(
            om_avg_collect[i][low:(len(dirs) - high)]))
        rm_avg.append(statistics.mean(
            rm_avg_collect[i][low:(len(dirs) - high)]))
        om_count.append(statistics.mean(
            om_count_collect[i][low:(len(dirs) - high)]))
        rm_count.append(statistics.mean(
            rm_count_collect[i][low:(len(dirs) - high)]))
        oo_max.append(statistics.mean(
            oo_max_collect[i][low:(len(dirs) - high)]))
        ro_max.append(statistics.mean(
            ro_max_collect[i][low:(len(dirs) - high)]))
    return plot_func(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, x_paras)


def cmp_delay(rounds, low=0, high=0):
    delays = ["{hd}ms,\n{ld}ms".format(
        hd=20 + x * 40, ld=4 + x * 8) for x in range(10)]
    dirs = ["delay/{}".format(x) for x in range(rounds)]
    exp_settings = [[9, 10000, 20 + x * 40, 4 + x * 8] for x in range(10)]
    return cmp_generic(delays, dirs, exp_settings, delay_plot, low, high)


def delay_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, name):
    xlable = 'latency: between DC, within DC'
    pname = 'delay'

    # fig = plt.figure(figsize=(18, 4))
    fig = plt.figure(figsize=(11, 4))

    # plt.subplot(1, 3, 1)
    plt.subplot(1, 2, 1)
    plot_bar(om_avg, rm_avg, name, xlable, 'average read_max diff')

    # plt.subplot(1, 3, 2)
    plt.subplot(1, 2, 2)
    plot_bar(om_count, rm_count, name, xlable,
             'frequency of read_max being wrong')

    # plt.subplot(1, 3, 3)
    # plot_bar(oo_max, ro_max, name, xlable, 'average max overhead per element: bytes')

    plt.tight_layout()
    plt.savefig("{}.pdf".format(pname))
    plt.close(fig)

    rtn = (oo_max, ro_max, name, xlable,
           'average max overhead per element: bytes')
    return rtn


def cmp_replica(rounds, low=0, high=0):
    replicas = [3, 6, 9, 12, 15]
    dirs = ["replica/{}".format(x) for x in range(rounds)]
    exp_settings = [[r, 10000, 50, 10] for r in replicas]
    return cmp_generic(replicas, dirs, exp_settings, replica_plot, low, high)


def replica_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, name):
    xlable = 'num of replicas'
    pname = 'replica'

    # fig = plt.figure(figsize=(16, 4))
    fig = plt.figure(figsize=(11, 4))

    # plt.subplot(1, 3, 1)
    plt.subplot(1, 2, 1)
    plot_bar(om_avg, rm_avg, name, xlable, 'average read_max diff')

    # plt.subplot(1, 3, 2)
    plt.subplot(1, 2, 2)
    plot_bar(om_count, rm_count, name, xlable,
             'frequency of read_max being wrong')

    # plt.subplot(1, 3, 3)
    # plot_bar(oo_max, ro_max, name, xlable, 'average max overhead per element: bytes')

    plt.tight_layout()
    plt.savefig("{}.pdf".format(pname))
    plt.close(fig)

    rtn = (oo_max, ro_max, name, xlable,
           'average max overhead per element: bytes')
    return rtn


def cmp_speed(rounds, low=0, high=0):
    speeds = [500 + x * 100 for x in range(96)]
    dirs = ["speed/{}".format(x) for x in range(rounds)]
    exp_settings = [[9, n, 50, 10] for n in speeds]
    return cmp_generic(speeds, dirs, exp_settings, speed_plot, low, high)


def speed_plot(om_avg, rm_avg, om_count, rm_count, oo_max, ro_max, name):
    xlable = 'op/second'
    pname = 'op_speed'
    s_name = [x for x in name]
    for i in range(len(s_name)):
        if (i - 500) % 10 != 0:
            s_name[i] = ''

    # fig = plt.figure(figsize=(16, 4))
    fig = plt.figure(figsize=(11, 4))

    # plt.subplot(1, 3, 1)
    plt.subplot(1, 2, 1)
    # plot_bar(om_avg, rm_avg, name, xlable, 'average read_max diff', s_name=s_name)
    plot_line(om_avg, rm_avg, name, xlable, 'average read_max diff')

    # plt.subplot(1, 3, 2)
    plt.subplot(1, 2, 2)
    # plot_bar(om_count, rm_count, name, xlable, 'frequency of read_max being wrong')
    plot_line(om_count, rm_count, name, xlable,
              'frequency of read_max being wrong')

    # plt.subplot(1, 3, 3)
    # plot_bar(oo_max, ro_max, name, xlable, 'average max overhead per element: bytes', s_name=s_name)

    plt.tight_layout()
    plt.savefig("{}.pdf".format(pname))
    plt.close(fig)

    rtn = (oo_max, ro_max, name, xlable,
           'average max overhead per element: bytes')
    return rtn, s_name


cmp_or("inc_d", "replica/0")
cmp_or("ar_d", "ardominant")
sp, sn = cmp_speed(30)
rp = cmp_replica(30)
dl = cmp_delay(30)

fig = plt.figure(figsize=(11, 4))

plt.subplot(1, 2, 1)
plot_bar(*sp, s_name=sn)

plt.subplot(1, 2, 2)
plot_bar(*dl)

plt.tight_layout()
plt.savefig("ovhd_sd.pdf")
plt.close(fig)

fig = plt.figure(figsize=(6, 4))
plot_bar(*rp)
plt.savefig("ovhd_r.pdf")
plt.close(fig)

print("Data skipped: ", data_skiped)
