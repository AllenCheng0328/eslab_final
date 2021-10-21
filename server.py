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

sample_num = 0
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


for i in range(len(data)):
    temp = data[i].split(' ')
    for j in range(3):
        temp[j] = float(temp[j])
    temp = temp[0:4]
    temp[0] +=5
    temp[1] +=15
    temp[2] -= 1030
    temp[3] = float(i)*0.01
    data[i] = temp
arr2d = np.array(data)

plt.plot(arr2d[:,3],arr2d[:,0:3])
plt.title("3 axis a-t graph")
plt.xlabel("time (s)")
plt.ylabel("acceleration")
plt.legend(["x","y","z"])
plt.show()