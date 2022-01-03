import numpy as np
import matplotlib.pyplot as plt
"""
data_type = 5
data_num = 10
#one data has 135 point
datafile = []
datafile.append(open("sample_data/up.txt","r"))
datafile.append(open("sample_data/down.txt","r"))
datafile.append(open("sample_data/right.txt","r"))
datafile.append(open("sample_data/left.txt","r"))
datafile.append(open("sample_data/forward.txt","r"))
fig, (p0,p1,p2,p3,p4) = plt.subplots(5,1)
p = [p0,p1,p2,p3,p4]

for t in range(data_type):
    for i in range(data_num):
        x = []
        for j in range(9):
            x = x + datafile[t].readline().split()
        x = np.array(x).astype('double')
        p[t].plot(x)
fig.savefig('test.png')
fig.show()
"""
datafile = open("sample_data/down.txt","r")
for i in range(5):
    temp = datafile.readline().split()
temp = np.array(temp).astype('double')
fig, (p0,p1,p2) = plt.subplots(3,1)
p = [p0,p1,p2]
for axis in range(3):
    sequence = []
    for i in range(int(len(temp)/3)):
        sequence.append(temp[3*i+axis])
    p[axis].plot(sequence)
fig.savefig('test.png')
fig.show()