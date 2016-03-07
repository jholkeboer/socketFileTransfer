# Jack Holkeboer
# Oregon State University 
# CS372
# Project 2
# holkeboj@onid.oregonstate.edu
# ftclient.py
# Based on Python Docs: https://docs.python.org/2/howto/sockets.html

import sys
import socket
import os

# check for argument
if len(sys.argv) not in [5,6]:
    print "Usage: python ftclient.py [server host] [server port] [command] [data port] [filename]"
    sys.exit()

host = sys.argv[1]
control_port = int(sys.argv[2])
command = sys.argv[3]
data_port = int(sys.argv[4])
if command == "-g":
    if len(sys.argv) == 6:
        filename = sys.argv[5]
    else:
        print "Error: Please provide a filename."
        sys.exit()

print 'Welcome to the socket file transfer client.'


# create socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

try:
    # get host and connect socket
    host_ip = socket.gethostbyname(host)
    sock.connect((host_ip, control_port))
    recv_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    print "Binding on host %s port %s" % (host_ip, data_port)
    recv_socket.bind((host_ip, data_port))
    recv_socket.listen(5)
    (ftserver, address) = recv_socket.accept()

    
    if command in ['-l','-g']:
        if command == '-l':
            sock.send("%s%s\n" % (command, data_port))
        elif command == '-g':
            sock.send("%s\t%s\t%s\n" % (command, data_port, filename))
    else:
        sock.send("%s\t\n" % command)

    
    
    response = sock.recv(1024)
    print response
    if response != "ERROR\n":
        if command == '-l':
            file_list = ''
            data = recv_socket.recv(1024)
            while data:
                file_list = file_list + data
                data = ftserver.recv(1024)
            print file_list
        elif command == '-g':
            data = recv_socket.recv(1024)
            with open(filename, 'w') as f:
                while data:
                    if data == "File not found.\n":
                        print data
                        break
                    else:
                        f.write(data)
                        data = recv_socket.recv(1024)
        
        ftserver.close()
    else:
        print "Invalid command.  Exiting..."
        sock.shutdown
        sock.close()
        sys.exit()
            

    # while (msg != '\quit'):
    #     # send message
    #     while len(msg) > 500:
    #         print 'Your message was too long. Maximum 500 characters.'
    #         msg = raw_input("%s> " % handle)
    #     sock.send("%s> %s" % (handle,msg))
            
    #     # wait to receive message
    #     print '[waiting for response...]'
    #     response = sock.recv(512)
    #     if "\quit" in response:
    #         print "Received quit signal. Closing connection."
    #         sys.exit()
    #     print response

    #     # input new message to send
    #     msg = raw_input("%s> " % handle)
    
    # react to \quit message
    print "Closing Connection."
    
    # send \quit to server
    sock.send("\quit")
except socket.gaierror:
    print 'Could not find host. Exiting.'
    sys.exit()
except:
    print 'An error occurred. Exiting.'
    sys.exit()
