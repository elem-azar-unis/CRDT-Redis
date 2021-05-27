import subprocess
import platform


def extract(text):
    rtn = text.replace(r'\n', '').replace('\r', '').replace(' ', '')
    rtn = {a.split('=')[0]: a.split('=')[1] for a in rtn.split(r'/\\')[1:]}

    rtn["history"] = rtn["history"][4:-4]
    rtn["history"] = rtn["history"].replace(',\"e\"', '').replace('\"', '')
    rtn["history"] = rtn["history"].split('>>,<<')

    rtn["e_set"] = rtn["e_set"][3:-3].split('},{')
    for i in range(len(rtn["e_set"])):
        tmp = rtn["e_set"][i].replace('\"', '')[1:-1]
        if len(tmp) > 0:
            rtn["e_set"][i] = {a.split('|->')[0]: a.split('|->')[1]
                               for a in tmp.split(',')}

    rtn["t_set"] = rtn["t_set"][3:-3].split('},{')
    for i in range(len(rtn["t_set"])):
        tmp = rtn["t_set"][i].replace('\"', '')
        tmp = tmp.replace('<<', '').replace('>>', '')[1:-1]
        if len(tmp) > 0:
            tmp = tmp.split(',')
            # for 't|->0,1' is splited with ','
            tmp[1] = tmp[1] + ',' + tmp[2]
            tmp.pop()
            rtn["t_set"][i] = {a.split('|->')[0]: a.split('|->')[1]
                               for a in tmp}

    rtnList = []
    rtnList.append("")
    for l in rtn["history"]:
        rtnList[0] += l.replace(',', ' ') + ';'
    rtnList.append("")
    for i in range(len(rtn["e_set"])):
        tmpstr = ""
        if rtn["e_set"][i] != '':
            tmpstr += f"{rtn['e_set'][i]['p_ini']} {rtn['e_set'][i]['v_inn']} {rtn['e_set'][i]['v_acq']} "
        else:
            tmpstr += "n n n "
        if rtn["t_set"][i] != '':
            tmpstr += f"{rtn['t_set'][i]['t']} ;"
        else:
            tmpstr += "n ;"
        rtnList[1] += tmpstr
    return rtnList


cmd = "gvpr \'N[outdegree == 1]{print($.label)}\' dot.dot"
if platform.system() == "Windows":
    cmd = "powershell " + cmd
result = subprocess.run(cmd, stdout=subprocess.PIPE, shell=True)
result = [extract(r) for r in result.stdout.decode('utf-8').split('\n')[:-1]]

with open("rwf_rpq.script", "w") as file:
    for rcd in result:
        for line in rcd:
            file.write(line + '\n')
