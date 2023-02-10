# ARP-Third-Assignment

## Description

A logfile was created for each process to control what happens.

### MASTER
This process handles the spawning of the two processes and also their closure. Two pipes are used to obtain the pids of the processes, which are useful for closing them. In addition, the SIGINT signal is also handled in order to close the master and child processes in the best possible way.


### PROCESS A
I have added a menu that allows you to choose between three modes and exit the programme. The first is the normal mode of the second assignment. The second is client mode, here I create a socket with the socket function and after assigning all the parameters I use a connect() to connect to the server. If the connection is accepted the client_mode() function is executed which sends all the commands to the server after converting them into strings. Finally by pressing the 'and' key you have the option of returning to the menu to choose a new mode. 
The third is server mode, here I create a socket and, after assigning the parameters, I use the bind() and listen() functions so that the server is initialised and made active. Finally via the accept() function, the client is allowed to connect and the server_mode() function is called, which receives commands from the client and converts them into integers. Here too there is the possibility of pressing the 'e' key to return to the menu (after the client has disconnected from this server) and choose a new mode. The way in which shared memory is managed has not changed and will be described below.

In this process, the shared memory is created and the semaphore required to protect it is created and initialised. After opening and closing a pipe for sending the pid to the master, three functions are used. 
The first used is the delete function for cleaning the bitmap; 
the second used is "bmp_circle" for drawing a circle in the centre of the bitmap;
the third is 'static_conversion', through which the shared memory is accessed and the bitmap data is passed to it. Obviously, this function is protected by a semaphore so as to prevent the second process from attempting to access this memory.
Then we enter an endless while loop in which, if the "P" button is pressed, a screenshot of the bitmap will be taken, if one of the four arrows in the keyboard is used, then via "move_circle" and "draw_circle" the drawing on the first interface will be moved and drawn. Finally, the three functions described above will be used in the same order, the only difference being that they will be passed to the "bmp_circle" function the co-ordinates of the design on the interface.
The closure is handled with a sigaction, via the management of the SIGTERM and SIGINT signals.
This process handles the closing of all resources.

N.B. Thecoordinates are multiplied by 20 to allow their use in the bitmap.

### PROCESS B
This process is very similar to process A in some respects, but here 'ftruncate' is not used and the semaphore is simply opened and not initialised, because it is all done by process A.
A pipe is also opened and closed here to send the pid to the master.
Before the while loop, the 'delete' function, already described above, is used. 
Within the while loop, I implemented the possibility of taking a screenshot of the bitmap of this process using the 's' key on the keyboard.
Following this is the 'conversion' function protected by a semaphore.
In this function, access is made to the shared memory in order to take the data and insert it into a bitmap.
Following this function is the 'get_center' function, which obtains the coordinates of the centre of the circle in the bitmap. It is possible to obtain the co-ordinates by knowing the radius of the circle. Finally, through the use of "mvdacch", zeros will be shown on the screen that will track the movement of the drawing in the first interface.
Closure is handled in the same way as process A.
I added the possibility to delete zeros in order to using again the application.
N.B. The coordinates obtained are divided by 20 to allow them to be used in the interface.  

## Installing
### Install Libbitmap

download from https://github.com/draekko/libbitmap

navigate to the root directory of the downloaded repo and type `./configure`

type `make`

run `make install`

### Install ncurses

`sudo apt-get install libncurses-dev`

`chmod +x install.src`

`./install.src`

## Run

`chmod +x run.src`

`./run.src`

## User guide

There are two interfaces. 
In the first (process A), there is a menu that allows you to choose normal mode, client mode and server mode. The first mode directly allows you to use the arrows on the keyboard to move the cross; the second mode allows you to connect to a server and move the cross, thus sending commands to another application (server); the third mode turns the application into a server that will receive commands from a client, which can then move the cross. Thus, although the modes are different, all three have a green cross. In addition, clicking on the box where there is a 'P' will provide a screenshot of the bitmap.

In the second interface, (Process B) zeros will appear which will trace the movements of the drawing in the first interface, and by clicking the "s" key on the keyboard it will be possible to obtain a screenshot of the bitmap of process B.
There the possibility to delete zeros.
N.B. The process A works with the window 90x30, while the process B works with the window 80x30
