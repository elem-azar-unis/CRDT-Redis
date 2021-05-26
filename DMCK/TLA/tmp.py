import subprocess
import platform


def extract(text):
    tmp = text.replace(r'\n', '').replace('\r', '').replace(' ', '')
    return {line.split('=')[0]: line.split('=')[1] for line in tmp.split(r'/\\')[1:]}


cmd = "gvpr \'N[outdegree == 1]{print($.label)}\' dot.dot"
if platform.system() == "Windows":
    cmd = "powershell " + cmd
result = subprocess.run(cmd, stdout=subprocess.PIPE, shell=True)
result = [extract(r) for r in result.stdout.decode('utf-8').split('\n')[:-1]]

for key, val in result[-1].items():
    print(key, ":", val)
