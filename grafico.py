import subprocess
#import matplotlib.pyplot as plt

marcos = []
randfp = []
r = []
w = []
numeros = []
i = 2
while i < 101:
    tmp=subprocess.Popen(["./virtmem",str(100),str(i),"lru","scan"], stdout=subprocess.PIPE)
    print("printing result " + str(i) + ":")
    output = tmp.stdout.read()
    numeros = [int(s) for s in output.split() if s.isdigit()]
    randfp.append(numeros[0])
    r.append(numeros[1])
    w.append(numeros[2])
    marcos.append(i)
    i+=1

#plt.plot(marcos,randfp)
#plt.plot(marcos,r)
#plt.plot(marcos,w)
print(randfp)
print(r)
print(w)
print(marcos)