import matplotlib.pyplot as plt

def read(ztype, server, op, delay, low_delay):
    d = "{t}:{s},{o},({d},{ld})".format(t=ztype, s=server, o=op, d=delay, ld=low_delay)
    of=open(d+"/s.ovhd")
    mf = open(d + "/s.max")
    ovhd=[]
    rmax=[]
    for line in of:
        ovhd.append([int(x) for x in line.split(' ')])
    for line in mf:
        tmp=[float(x) for x in line.split(' ')]
        tmp[0]=int(tmp[0])
        tmp[2]=int(tmp[2])
        rmax.append(tmp)
    return rmax,ovhd


def test():
    ormax,oovhd=read("o",9,5000,50,10)
    rrmax,rovhd=read("r",9,5000,50,10)
    lmax=min(len(ormax),len(rrmax))
    lovhd=min(len(oovhd),len(rovhd))
    x1=[i for i in range(lovhd)]
    y1o=[b/a for a,b in oovhd][:lovhd]
    y1r=[b/a for a,b in rovhd][:lovhd]
    x2 = [i for i in range(lmax)]
    y2o=[abs(a-b) for _,a,_,b in ormax][:lmax]
    y2r = [abs(a - b) for _, a, _, b in rrmax][:lmax]

    plt.plot(x1, y1o, color="blue", linestyle="-", label="Add-Win")
    plt.plot(x1, y1r, color="red", linestyle="-", label="Rmv-Win")
    plt.xlabel('time: second')
    plt.ylabel('overhead: byte')
    plt.legend(loc='upper left')
    plt.show()

    plt.plot(x2, y2o, color="blue", linestyle="-", label="Add-Win")
    plt.plot(x2, y2r, color="red", linestyle="-", label="Rmv-Win")
    plt.xlabel('time: second')
    plt.ylabel('read max diff')
    plt.legend(loc='upper left')
    plt.show()

def test_2():
    name=[5000,7500,10000,16000]
    ms=[]
    os=[]
    for n in name:
        m, o = read("o", 9, n, 50, 10)
        ms.append(m)
        os.append(o)

    plot(ms,os,name,'op/second')

def test_1():
    name=["25ms,5ms","50ms,10ms","100ms,20ms"]
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
    plot(ms, os, name,'latency: between DC, within DC')

def plot(ms,os,name,xlable):

    for i in range(len(ms)):
        ms[i] = [abs(a-b) for _,a,_,b in ms[i]]

    m_avg=[]
    m_count = []
    for m in ms:
        m_avg.append(sum(m)/len(m))
        m_count.append((len(m)-m.count(0))/len(m))

    o_max=[]
    for o in os:
        o_max.append(o[-1][1]/o[-1][0])

    print(m_count)
    plt.bar(range(len(name)),m_avg,tick_label=name)
    plt.xlabel(xlable)
    plt.ylabel('average read_max diff')
    plt.show()

    plt.bar(range(len(name)), m_count, tick_label=name)
    plt.xlabel(xlable)
    plt.ylabel('frequency of read_max being wrong')
    plt.show()

    plt.bar(range(len(name)), o_max, tick_label=name, color='r')
    plt.xlabel(xlable)
    plt.ylabel('max average overhead per element: byte')
    plt.show()


test_1()