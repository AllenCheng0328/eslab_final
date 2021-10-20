#!/usr/bin/env python3

import matplotlib.pyplot as plt

import socket
import numpy as np
import json
import time
import random
HOST = '192.168.43.219' # Standard loopback interface address
PORT = 65431 # Port to listen on (use ports > 1023)
N = 1000
'''
sample_num = 0         #0~N-1
print(type(sample_num))
data = []
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen()
    conn, addr = s.accept()
    with conn:
        print('Connected by', addr)
        while sample_num<N:
            data.append(  conn.recv(1024).decode('utf-8')  )
            print('Received from socket server : ', data[sample_num])
            sample_num +=1
'''
data = ["5 100 34 0","6 50 10 1","7 25 77 2","5 100 34 3","6 50 10 4","7 25 77 5","5 100 34 6",
"6 50 10 7","7 25 77 8","5 100 34 9","6 50 10 10","7 25 77 11","5 100 34 12","6 50 10 13","7 25 77 14",
"5 100 34 15","6 50 10 16","7 25 77 17","5 100 34 18","6 50 10 19","7 25 77 20","5 100 34 21","6 50 10 22",
"7 25 77 23","5 100 34 24","6 50 10 25"]


#change 3 to N
for i in range(len(data)):
    temp = data[i].split(' ')
    for j in range(4):
        temp[j] = float(temp[j])
    data[i] = temp
arr2d = np.array(data)
plt.plot(arr2d[:,3],arr2d[:,0:3])


plt.title("3 axis a-t graph")
plt.xlabel("time (s)")
plt.ylabel("acceleration")
plt.legend(["x","y","z"])
plt.show()