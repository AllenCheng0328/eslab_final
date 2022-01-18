import os
from typing import List
from numpy.lib.function_base import msort
from pygame import mixer
from random import shuffle, triangular
from tkinter import Listbox, Tk, Button, Label
import socket
import time
import torch
import numpy as np
import torch.nn as nn
from torch.utils.data import Dataset, DataLoader
from threading import Thread

class Classifier(nn.Module):
    def __init__(self):
        super(Classifier, self).__init__()
        self.layer1 = nn.Linear(600, 1024)
        self.layer2 = nn.Linear(1024, 256)
        self.layer3 = nn.Linear(256, 64)
        self.out = nn.Linear(64, 17) 

        self.act_fn = nn.ReLU()

    def forward(self, x):
        x = self.layer1(x)
        x = self.act_fn(x)

        x = self.layer2(x)
        x = self.act_fn(x)

        x = self.layer3(x)
        x = self.act_fn(x)

        x = self.out(x)

        return x

def get_device():
    return 'cuda' if torch.cuda.is_available() else 'cpu'
model_path = (r"C:\Users\jscmb\es_data\model.ckpt") #change this path to where you store the model.ckpt
device = get_device()
print("device:", device)

model = Classifier().to(device)
model.load_state_dict(torch.load(model_path))


# Initialize the window and the mixer
window = Tk()
window.title("Easy music player with gesture detection")
mixer.init()

song_box = Listbox(window, bg = 'black', fg = 'green', width = 60)
# song_box.pack(pady = 60)

# get list of files
os.chdir(r"C:\Users\jscmb\songs") #change this to your song list's direction
playlist = os.listdir()
active_playlist = playlist
shuffled_playlist = []
nonshuffled_playlist = playlist

# Get number of files
file_range = len(playlist) - 1

# Settings variables used to control play logic
music_option = True
shuffle_music = False
indexed_track = 0
display_track = indexed_track + 1
is_stopped = False
is_paused = False
is_started = False
repeat_track = False

stop_reacting = False


def volumedown():
    volume = mixer.music.get_volume()
    if volume >=  0.1:
        volume = volume - 0.1
    else:
        volume = 0
    # volume = volume - 1.0
    mixer.music.set_volume(volume)
    print("Set volume to ", mixer.music.get_volume())
def volumeup():
    volume = mixer.music.get_volume()
    volume = volume + 0.1
    # volume = volume - 1.0
    mixer.music.set_volume(volume)
    print("Set volume to ", mixer.music.get_volume())

# Shuffle music logic
def shuffle_playlist():
    global shuffle_music, playlist, is_started, shuffled_playlist, indexed_track, is_stopped, active_playlist
    indexed_track = 0
    if shuffle_music:
        shuffle_music = False
        active_playlist = playlist
    else:
        shuffle_music = True
        shuffle(playlist)
        shuffled_playlist = playlist
        os.chdir(r"C:\Users\jscmb\songs") #change this to your song list's direction
        playlist = os.listdir()
    update_display()
    mixer.music.pause()
    start_music()


# Repeat one or all logic
def repeat_loop():
    global repeat_track
    if not repeat_track :
        repeat_track = True
        repeat_button.config(text="🔂")
    else :
        repeat_track = False
        repeat_button.config(text="🔁")


# Updates the display values of the current track number and name
def update_display():
    global display_track
    display_track = playlist.index(active_playlist.__getitem__(indexed_track)) + 1
    track_num_display.config(text=f"Track: {display_track}")
    track_name_display.config(text=f"Now Playing:\n{active_playlist.__getitem__(indexed_track)}")
    music_playing = music_status()
    if music_playing:
        start_button.config(text="⏸️")

    # Else it was paused so unpause and mark paused flag as False
    else:
        start_button.config(text="▶️")


# Logic to run music
def start_music():
    global indexed_track, is_started, is_stopped, active_playlist

    # Determine if random or normal play
    if shuffle_music:
        active_playlist = shuffled_playlist
    else:
        active_playlist = playlist

    # If music is not already playing, it has not been stopped or paused then grabs next song in queue
    while not mixer.music.get_busy() and not is_stopped and not is_paused and not is_started:
        mixer.music.load(f"{active_playlist.__getitem__(indexed_track)}")
        mixer.music.play()
        is_started = True
    # While music is playing check every 100 milliseconds if music track has finished playing and re-trigger music.
    if mixer.music.get_busy():
        window.after(100, start_music)
    else:
        # Repeat logic
        if not is_paused:
            # Repeat one logic reduce index call by 1 prior to adding 1 so always stay on same track
            if repeat_track:
                indexed_track -= 1

            # Else it is repeating all
            indexed_track += 1
            is_started = False

            # Checks to make sure track is not skipped past last song or into negative.
            if indexed_track < 0 or indexed_track > file_range:
                indexed_track = 0

            # Update the display with current track info
            update_display()

            # Repeat music
            start_music()


# Gets the current status of the music player
def music_status():
    music_yes = mixer.music.get_busy()
    return music_yes


# Stop the music from playing. If already stopped then it resets repeat status and the playlist
def stop_music():
    global is_stopped, is_started, indexed_track, repeat_track
    if is_stopped:
        indexed_track = 0
        repeat_track = False
        update_display()
    mixer.music.stop()
    is_stopped = True
    is_started = False
    start_music()

# Skip to next track
def next_track():
    global indexed_track, is_started, is_paused
    # If music is playing stop it and move it to the next track and start again
    is_paused = False
    mixer.music.stop()
    if indexed_track == file_range:
        indexed_track = 0
        is_started = False
    start_music()

    update_display()


# Skip to last track
def prev_track():
    global indexed_track, is_started, is_paused
    # If music is playing stop it move to prior track or if on first track move to last track.
    is_paused = False
    if display_track == 1:
        indexed_track = file_range
        is_started = False
    else:
        indexed_track -= 2
    mixer.music.stop()
    start_music()
    update_display()


# Play or pause logic
def play_track():
    global is_paused, is_stopped
    music_playing = music_status()
    # If music is playing then pause it and mark the paused flag as True
    if music_playing:
        is_paused = True
        mixer.music.pause()
        start_button.config(text="▶️")

    # Else it was paused so unpause and mark paused flag as False
    else:
        is_paused = False
        mixer.music.unpause()
        start_button.config(text="⏸️")

    # Mark stopped as False and start music
    is_stopped = False
    start_music()

# When user not using UI, repaeatedly update
def update_status():
    update_display()
    window.after(1000, update_status)

window.after(1000, update_status)

# Control buttons
previous_button = Button(text="⏮", font=("", 12, ""), command=prev_track)
previous_button.grid(row=2, column=0)
start_button = Button(text="⏯", font=("", 12, ""), command=play_track)
start_button.grid(row=2, column=1)
next_button = Button(text="⏭", font=("", 12, ""), command=next_track)
next_button.grid(row=2, column=2)
repeat_button = Button(text="🔄", font=("", 12, ""), command=repeat_loop)
repeat_button.grid(row=3, column=3)
shuffle_button = Button(text="🔀", command=shuffle_playlist)
shuffle_button.grid(row=3, column=2)
volume_up_button = Button(text = "🔊", font=("", 12, ""), command=volumeup)
volume_up_button.grid(row=3, column=1)
volume_down_button = Button(text = "🔈", font=("", 12, ""), command=volumedown)
volume_down_button.grid(row=3, column=0)
# Displayed information
track_num_display = Label(text=f"Track: {display_track}")
track_name_display = Label(text=f"Now Playing:\n{active_playlist.__getitem__(indexed_track)}")
track_num_display.grid(row=0, column=1, columnspan=2)
track_name_display.grid(row=1, column=0, columnspan=4)

HOST = '172.20.10.13' # Standard loopback interface address, change this to your ip address of wifi
PORT = 65431 # Port to listen on (use ports > 1023)

class music_player(Thread):
    def __init__(self):
        Thread.__init__(self)
        self.daemon = True
        self.start()
    def run(self):
        while True:
            window.update_idletasks()
            window.update()

def server_socket():
    global stop_reacting
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((HOST, PORT))
        sample_num = 0
        s.listen(1)
        print("listening...")
        conn, addr = s.accept()
        print('Connected by', addr)
        with conn:
            music_playing = music_status()
            if not music_playing:
                mixer.music.load(f"{active_playlist.__getitem__(indexed_track)}")
                mixer.music.play()
            while True:
                print("Connected! Waiting for data...")        
                count = 0
                data = []
                try:           
                    N = int(conn.recv(1024).decode('utf-8'))
                except:
                    print('wrong connection, restart.')
                    continue
                while count < N:
                    temp = conn.recv(1024).decode('utf-8')
                    #print('Received from socket server : ', temp)
                    temp = temp.split()
                    for i in range(len(temp)):
                        data.append(int(temp[i]))
                    count += int(len(temp)/3)
                sample_num += 1
                for j in range(len(data), 600):
                    data.append(data[-3])

                if len(data) > 600:
                    print("Your Gasture is too long to detected! Please try again.")
                else:
                    # with open(r"C:\Users\jscmb\es_data\data_LTb.txt", "a") as dt:
                    #     for i in data:
                    #         dt.write(str(i))
                    #         dt.write(" ")
                    data = np.array(data)
                    data = torch.from_numpy(data).float()
                    output = model(data)
                    _, data_pred  = torch.max(output, 0)
                    data_pred = int(data_pred)
                    print(data_pred)

                    if stop_reacting == 0:
                        if data_pred == 0:
                            prev_track()
                            print("Gasture detected: last song")
                        elif data_pred == 1:
                            next_track()
                            print("Gasture detected: next song")
                        elif data_pred == 2:
                            volumeup()
                            print("Gasture detected: volume up")
                        elif data_pred == 3:
                            volumedown()
                            print("Gasture detected: volume down")
                        elif data_pred == 4:
                            play_track()
                            music_playing = music_status()
                            if music_playing:
                                print("Gasture detected: play")
                            else:
                                print("Gasture detected: pause")
                        elif data_pred == 5:
                            repeat_loop()
                            if repeat_track:
                                print("Gasture detected: repeat song")
                            else:
                                print("Gasture detected: not repeat song")
                        elif data_pred == 6:
                            shuffle_playlist()
                            if shuffle_music:
                                print("Gasture detected: random playlist")
                            else:
                                print("Gasture detected: ordered playlist")
                        elif data_pred == 7:
                            stop_reacting = True
                            print("Gasture detected: you've stop to detect the gesture, waving the stop gesture again to enable the detection")
                        elif data_pred == 8:
                            print("Gasture detected: equalizer setting 1")
                        elif data_pred == 9:
                            print("Gasture detected: equalizer setting 2")
                        elif data_pred == 10:
                            print("Gasture detected: equalizer setting 3")
                    
                    else:
                        if data_pred == 7:
                            stop_reacting = False
                            print("Gasture detected: you've enable the gesture detection")
                        else:
                            print("You've stop to detect the gesture, waving the stop gesture again to enable the detection")


                                                
if __name__ == "__main__":
    t2 = Thread(target = server_socket)
    t2.setDaemon(True)
    t2.start()
    window.mainloop()
    while True:
        print("somthing wrong...")
        pass