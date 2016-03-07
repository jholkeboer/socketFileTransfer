IMPORTANT:
Tested on flip.  If you're using flip, you must transfer between two flip sessions
and use the corresponding hostnames.


How to run client:
python ftclient.py [server host] [server port] [command] [data port] [filename]

How to run server:
// compile first
g++ -o ftserver ftserver.cpp
./ftserver [port number]


Example list:

Server (on flip1):
./ftserver 20410

Client (on flip3):
python ftclient.py flip1.engr.oregonstate.edu 20401 -l 29992


Example get:

Server (on flip1):
./ftserver 20401

Client (on flip3):
python ftclient.py flip1.engr.oregonstate.edu 20401 -g filename.txt