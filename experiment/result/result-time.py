import matplotlib.pyplot as plt
import matplotlib
import statistics
from math import ceil, floor

data_skipped = 0
err_sp = 1e10

matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['savefig.bbox'] = 'tight'
matplotlib.rcParams['savefig.pad_inches'] = 0.01
matplotlib.rcParams['legend.loc'] = 'best'
matplotlib.rcParams['xtick.labelsize'] = 8
matplotlib.rcParams['ytick.labelsize'] = 8


def freq(li):
    return len([x for x in li if x != 0]) / len(li)


def dt_read(root_dir, pattern, dt_type, speed, diff_file, lambda_diff):
    d = f"{root_dir}/{pattern}/0/{dt_type}_9,{speed},(50,10)"
    global data_skipped
    ovhd = []
    diff = []
    with open(d + "/ovhd.csv", "r") as file:
        for line in file.readlines():
            tmp = [float(x) for x in line.split(',')]
            if tmp[0] > err_sp or tmp[1] > err_sp:
                data_skipped += 1
                continue
            ovhd.append(tmp[1] / tmp[0])
    with open(d + "/"+diff_file, "r") as file:
        for line in file.readlines():
            tmp = [float(x) for x in line.split(',')]
            for data in tmp:
                if data > err_sp:
                    data_skipped += 1
            else:
                diff.append(lambda_diff(tmp))
    return diff, ovhd


def rpq_read(root_dir, pattern, ztype):
    return dt_read(root_dir, pattern, ztype, 10000, "max.csv", lambda x: x[1] - x[3])


def list_read(root_dir, pattern, ltype):
    return dt_read(root_dir, pattern, ltype, 1000, "distance.csv", lambda x: x[1])


def smooth(x, lenth, step):
    x = x[:floor(lenth/step)*step]
    ret = []
    for i in range(floor(lenth/step)):
        sum = 0.0
        for j in range(i*step, (i+1)*step):
            sum += x[j]
        ret.append(sum/step)
    return ret


def cmp_o_r(name, read_lable, read_func, root_dir, updd_name, ard_name, step=10, ylim=None):
    xlable = 'time: second'

    updd_rread, updd_rovhd = read_func(root_dir, updd_name, "o")
    updd_rwfread, updd_rwfovhd = read_func(root_dir, updd_name, "r")

    ard_rread, ard_rovhd = read_func(root_dir, ard_name, "o")
    ard_rwfread, ard_rwfovhd = read_func(root_dir, ard_name, "r")

    lread = min(len(updd_rread), len(updd_rwfread),
                len(ard_rread), len(ard_rwfread))
    lovhd = min(len(updd_rovhd), len(updd_rwfovhd),
                len(ard_rovhd), len(ard_rwfovhd))

    x1 = [ceil(i*step+step/2) for i in range(floor(lread/step))]
    updd_rread = smooth(updd_rread, lread, step)
    updd_rwfread = smooth(updd_rwfread, lread, step)
    ard_rread = smooth(ard_rread, lread, step)
    ard_rwfread = smooth(ard_rwfread, lread, step)

    x2 = [ceil(i*step+step/2) for i in range(floor(lovhd/step))]
    updd_rovhd = smooth(updd_rovhd, lovhd, step)
    updd_rwfovhd = smooth(updd_rwfovhd, lovhd, step)
    ard_rovhd = smooth(ard_rovhd, lovhd, step)
    ard_rwfovhd = smooth(ard_rwfovhd, lovhd, step)

    fig = plt.figure(figsize=(11, 4))

    plt.subplot(1, 2, 1)
    if (ylim is not None):
        plt.ylim(ylim)
    plt.plot(x1, updd_rread, linestyle="-", label="Add-Win: inc dom")
    plt.plot(x1, ard_rread, linestyle="-", label="Add-Win: a-r dom")
    plt.plot(x1, updd_rwfread, linestyle="-", label="Rmv-Win: inc dom")
    plt.plot(x1, ard_rwfread, linestyle="-", label="Rmv-Win: a-r dom")
    plt.xlabel(xlable)
    plt.ylabel(read_lable)
    plt.legend()

    plt.subplot(1, 2, 2)
    plt.plot(x2, updd_rovhd, linestyle="-", label="Add-Win: inc dom")
    plt.plot(x2, ard_rovhd, linestyle="-", label="Add-Win: a-r dom")
    plt.plot(x2, updd_rwfovhd, linestyle="-", label="Rmv-Win: inc dom")
    plt.plot(x2, ard_rwfovhd, linestyle="-", label="Rmv-Win: a-r dom")
    plt.xlabel(xlable)
    plt.ylabel("overhead: byte")
    plt.legend()

    plt.tight_layout()
    plt.savefig(f"{name}.pdf")
    plt.close(fig)

    print(name)
    print("inc-dom: Add-Win", statistics.mean(updd_rovhd),
          statistics.mean([abs(x) for x in updd_rread]), freq(updd_rread))
    print("inc-dom: Rmv-Win", statistics.mean(updd_rwfovhd),
          statistics.mean([abs(x) for x in updd_rwfread]), freq(updd_rwfread))
    print("a/r-dom: Add-Win", statistics.mean(ard_rovhd),
          statistics.mean([abs(x) for x in ard_rread]), freq(ard_rread))
    print("a/r-dom: Rmv-Win", statistics.mean(ard_rwfovhd),
          statistics.mean([abs(x) for x in ard_rwfread]), freq(ard_rwfread))


def cmp_r_rwf(name, read_lable, read_func, root_dir, updd_name, ard_name, step=10, ylim=None):
    xlable = 'time: second'

    updd_rread, updd_rovhd = read_func(root_dir, updd_name, "r")
    updd_rwfread, updd_rwfovhd = read_func(root_dir, updd_name, "rwf")

    ard_rread, ard_rovhd = read_func(root_dir, ard_name, "r")
    ard_rwfread, ard_rwfovhd = read_func(root_dir, ard_name, "rwf")

    lread = min(len(updd_rread), len(updd_rwfread),
                len(ard_rread), len(ard_rwfread))
    lovhd = min(len(updd_rovhd), len(updd_rwfovhd),
                len(ard_rovhd), len(ard_rwfovhd))

    x1 = [ceil(i*step+step/2) for i in range(floor(lread/step))]
    updd_rread = smooth(updd_rread, lread, step)
    updd_rwfread = smooth(updd_rwfread, lread, step)
    ard_rread = smooth(ard_rread, lread, step)
    ard_rwfread = smooth(ard_rwfread, lread, step)

    x2 = [ceil(i*step+step/2) for i in range(floor(lovhd/step))]
    updd_rovhd = smooth(updd_rovhd, lovhd, step)
    updd_rwfovhd = smooth(updd_rwfovhd, lovhd, step)
    ard_rovhd = smooth(ard_rovhd, lovhd, step)
    ard_rwfovhd = smooth(ard_rwfovhd, lovhd, step)

    fig = plt.figure(figsize=(11, 4))

    plt.subplot(1, 2, 1)
    if (ylim is not None):
        plt.ylim(ylim)
    plt.plot(x1, updd_rread, linestyle="-", label="upd_d: Remove-Win")
    plt.plot(x1, updd_rwfread, linestyle="-", label="upd_d: RWF")
    plt.plot(x1, ard_rread, linestyle="-", label="ar_d: Remove-Win")
    plt.plot(x1, ard_rwfread, linestyle="-", label="ar_d: RWF")
    plt.xlabel(xlable)
    plt.ylabel(read_lable)
    plt.legend()

    plt.subplot(1, 2, 2)
    plt.plot(x2, updd_rovhd, linestyle="-", label="upd_d: Remove-Win")
    plt.plot(x2, updd_rwfovhd, linestyle="-", label="upd_d: RWF")
    plt.plot(x2, ard_rovhd, linestyle="-", label="ar_d: Remove-Win")
    plt.plot(x2, ard_rwfovhd, linestyle="-", label="ar_d: RWF")
    plt.xlabel(xlable)
    plt.ylabel("overhead: byte")
    plt.legend()

    plt.tight_layout()
    plt.savefig(f"{name}.pdf")
    plt.close(fig)

    print(name)
    print("upd_d: Remove-Win", statistics.mean(updd_rovhd),
          statistics.mean([abs(x) for x in updd_rread]), freq(updd_rread))
    print("upd_d: RWF", statistics.mean(updd_rwfovhd),
          statistics.mean([abs(x) for x in updd_rwfread]), freq(updd_rwfread))
    print("ar_d: Remove-Win", statistics.mean(ard_rovhd),
          statistics.mean([abs(x) for x in ard_rread]), freq(ard_rread))
    print("ar_d: RWF", statistics.mean(ard_rwfovhd),
          statistics.mean([abs(x) for x in ard_rwfread]), freq(ard_rwfread))


cmp_o_r("rpq_o_r", "get_max diff", rpq_read,
        "rpq", "default", "ardominant", 1, (-150, 150))

# cmp_r_rwf("rpq_r_rwf", "read max diff", rpq_read,
#           "rpq", "default", "ardominant", 1, (-300, 300))
# cmp_r_rwf("rpq_cmp_r_rwf", "read max diff", rpq_read,
#           "rpq,cmp", "default", "ardominant", 1, (-300, 300))
# cmp_r_rwf("list_r_rwf",  "list editing distance",
#           list_read, "list", "upddominant", "default", 1)
# cmp_r_rwf("list_cmp_r_rwf",  "list editing distance",
#           list_read, "list,cmp", "upddominant", "default", 1)


print("Data skipped: ", data_skipped)
