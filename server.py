#!/usr/bin/env python3

import matplotlib.pyplot as plt

import socket
import numpy as np
import json
import time
import random
HOST = '192.168.43.219' # Standard loopback interface address
PORT = 65431 # Port to listen on (use ports > 1023)
#sample frequency = 100 Hz
N = 45
switch = '1'
datafile = open("test_data/hybrid.txt","a")

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    sample_num = 0;
    while switch == '1':
        count = 0
        data = []
        s.listen()
        print("listening...")
        conn, addr = s.accept()
        with conn:
            print('Connected by', addr)
            while count < N:
                temp = conn.recv(1024).decode('utf-8')
                print('Received from socket server : ', temp)
                temp = temp.split()
                for i in range(int(len(temp)/3)):
                    data.append( [ float(temp[i*3]), float(temp[i*3+1]), float(temp[i*3+2]) ] )
                    datafile.write(temp[i*3])
                    datafile.write(' ')
                    datafile.write(temp[i*3+1])
                    datafile.write(' ')
                    datafile.write(temp[i*3+2])
                    if count % 5 == 4:
                        datafile.write('\n')
                    else:
                        datafile.write(' ')
                    if count + i == N-1:
                        print("avoid extra sample")
                        break
                count += int(len(temp)/3)
            arr2d = np.array(data)
            """
            plt.plot(arr2d[:,3],arr2d[:,0:3])
            plt.title("3 axis a-t graph")
            plt.xlabel("time (s)")
            plt.ylabel("acceleration")
            plt.legend(["x","y","z"])
            plt.show()
            """
            sample_num += 1
        print(f'sample_num :{sample_num}\ntype 1 to continue, otherwise close')
        switch = input()
    datafile.flush()
    datafile.close()
        
        