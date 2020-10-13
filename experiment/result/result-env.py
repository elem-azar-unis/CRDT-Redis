import matplotlib.pyplot as plt
import matplotlib
import statistics

data_skipped = 0
err_sp = 1e10
total_rounds = 16

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['savefig.bbox'] = 'tight'
matplotlib.rcParams['savefig.pad_inches'] = 0.01
matplotlib.rcParams['legend.loc'] = 'best'
matplotlib.rcParams['xtick.labelsize'] = 8
matplotlib.rcParams['ytick.labelsize'] = 8


def dt_read(root_dir, dt_type, server, speed, delay, low_delay, diff_file, lambda_diff):
    def read_one_round(data_dir):
        global data_skipped
        ovhd = -1
        ovhd_c = 0
        diff = 0
        diff_c = 0
        with open(data_dir + "/ovhd.csv", "r") as file:
            for line in file.readlines():
                tmp = [float(x) for x in line.split(',')]
                if tmp[0] > err_sp or tmp[1] > err_sp:
                    data_skipped += 1
                    continue
                ovhd += tmp[1] / tmp[0]
                ovhd_c += 1
        with open(data_dir + "/"+diff_file, "r") as file:
            for line in file.readlines():
                tmp = [float(x) for x in line.split(',')]
                for data in tmp:
                    if data > err_sp:
                        data_skipped += 1
                else:
                    diff += lambda_diff(tmp)
                    diff_c += 1
        return diff/diff_c, ovhd/ovhd_c
    diff_return = 0
    ovhd_return = 0
    for rounds in range(total_rounds):
        d = "{dir}/{r}/{t}_{s},{sp},({d},{ld})".format(dir=root_dir, r=rounds, t=dt_type,
                                                       s=server, sp=speed, d=delay, ld=low_delay)
        diff_one, ovhd_one = read_one_round(d)
        diff_return += diff_one
        ovhd_return += ovhd_one
    return diff_return/total_rounds, ovhd_return/total_rounds


def rpq_read(root_dir, dt_type,  server, speed, delay, low_delay):
    return dt_read(root_dir, dt_type, server, speed, delay, low_delay, "max.csv", lambda x: abs(x[1] - x[3]))


def list_read(root_dir, dt_type,  server, speed, delay, low_delay):
    return dt_read(root_dir, dt_type, server, speed, delay, low_delay, "distance.csv", lambda x: x[1])


def plot_bar(r_data, rwf_data, name, x_lable, y_lable, s_name=None):
    if s_name is None:
        s_name = name
    bar_width = 0.35
    index1 = [x for x in range(len(name))]
    index2 = [x + bar_width for x in range(len(name))]
    index = [x + bar_width / 2 for x in range(len(name))]

    l1 = plt.bar(index1, r_data, bar_width)
    l2 = plt.bar(index2, rwf_data, bar_width)
    plt.xlabel(x_lable)
    plt.ylabel(y_lable)
    plt.legend(handles=[l1, l2, ], labels=['Rmv-Win', 'RWF'])
    plt.xticks(index, s_name)


def plot_line(r_data, rwf_data, name, x_lable, y_lable):
    plt.plot(name, r_data, linestyle="-", label="Rmv-Win")
    plt.plot(name, rwf_data, linestyle="-", label="RWF")
    plt.xlabel(x_lable)
    plt.ylabel(y_lable)
    plt.legend()


def gather_plot_data(root_dir, exp_settings, read_func):
    r_read_diff = []
    r_ovhd = []
    rwf_read_diff = []
    rwf_ovhd = []
    for setting in exp_settings:
        a, b = read_func(root_dir, "r", *setting)
        r_read_diff.append(a)
        r_ovhd.append(b)
        a, b = read_func(root_dir, "rwf", *setting)
        rwf_read_diff.append(a)
        rwf_ovhd.append(b)
    return r_read_diff, r_ovhd, rwf_read_diff, rwf_ovhd


def plot_generic(r_read_diff, r_ovhd, rwf_read_diff, rwf_ovhd, name, ylable_read, xlable, pname):
    fig = plt.figure(figsize=(11, 4))

    plt.subplot(1, 2, 1)
    plot_bar(r_read_diff, rwf_read_diff, name, xlable, ylable_read)

    plt.subplot(1, 2, 2)
    plot_bar(r_ovhd, rwf_ovhd, name, xlable,
             'average overhead per element: bytes')

    plt.tight_layout()
    plt.savefig("{}.pdf".format(pname))
    plt.close(fig)


def cmp_delay(root_dir, read_func, ylable_read, pnane_prefix, dft_speed):
    delays = ["{hd}ms,\n{ld}ms".format(
        hd=20 + x * 40, ld=4 + x * 8) for x in range(10)]
    exp_settings = [[9, dft_speed, 20 + x * 40, 4 + x * 8] for x in range(10)]
    a, b, c, d = gather_plot_data(root_dir+"/delay", exp_settings, read_func)
    plot_generic(a, b, c, d,  delays, ylable_read,
                 "latency: between DC, within DC", pnane_prefix+"_delay")


def cmp_replica(root_dir, read_func, ylable_read, pnane_prefix, base_speed):
    replicas = [3, 6, 9, 12, 15]
    exp_settings = [[r, r*base_speed, 50, 10] for r in replicas]
    a, b, c, d = gather_plot_data(root_dir+"/replica", exp_settings, read_func)
    plot_generic(a, b, c, d, replicas, ylable_read,
                 "num of replicas", pnane_prefix+"_replica")


def cmp_speed(root_dir, read_func, ylable_read, pnane_prefix, low, high, step):
    speeds = [x for x in range(low, high+step, step)]
    exp_settings = [[9, n, 50, 10] for n in speeds]
    a, b, c, d = gather_plot_data(root_dir+"/speed", exp_settings, read_func)
    speed_plot(a, b, c, d, speeds, ylable_read, pnane_prefix)


def speed_plot(r_read_diff, r_ovhd, rwf_read_diff, rwf_ovhd, name, ylable_read, pnane_prefix):
    xlable = 'op/second'
    pname = pnane_prefix+'_op_speed'
    s_name = [x for x in name]
    for i in range(len(s_name)):
        if i % int(len(s_name)/9) != 0:
            s_name[i] = ''

    fig = plt.figure(figsize=(11, 4))

    plt.subplot(1, 2, 1)
    # plot_bar(om_avg, rm_avg, name, xlable, ylable_read, s_name=s_name)
    plot_line(r_read_diff, rwf_read_diff, name, xlable, ylable_read)

    plt.subplot(1, 2, 2)
    plot_bar(r_ovhd, rwf_ovhd, name, xlable,
             'average overhead per element: bytes', s_name=s_name)

    plt.tight_layout()
    plt.savefig("{}.pdf".format(pname))
    plt.close(fig)


def cmp_all(root_dir, read_func, ylable_read, pnane_prefix, low, high, step, dft_speed, base_speed):
    cmp_delay(root_dir, read_func, ylable_read, pnane_prefix, dft_speed)
    cmp_replica(root_dir, read_func, ylable_read,
                pnane_prefix, base_speed)
    cmp_speed(root_dir, read_func, ylable_read, pnane_prefix, low, high, step)


cmp_all("rpq", rpq_read, "average read_max diff",
        "rpq", 500, 10000, 100, 10000, 1000)
cmp_all("list,cmp", list_read, "average list edit distance",
        "list_cmp", 50, 1000, 50, 1000, 100)

print("Data skipped: ", data_skipped)
