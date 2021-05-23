import subprocess
import platform
import re

cmd = "gvpr \'N[outdegree == 1]{print($.label)}\' dot.dot"
if platform.system() == "Windows":
    cmd = "powershell " + cmd
result = subprocess.run(cmd, stdout=subprocess.PIPE)
res = result.stdout.decode('utf-8').split('\n')

for line in res:
    match = re.search(r'history.+/\\\\', line)
    if match:
        print(match.group().replace("history = ", "").replace(
            "\\n", "").replace("/\\\\", "").replace(" ", "")[2:-2])
print(len(res))
