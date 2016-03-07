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
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)

try:
    # get host and connect socket
    host_ip = socket.gethostbyname(host)
    sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    sock.connect((host_ip, control_port))
    # recv_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # print "Binding data conn on host %s port %s" % (host_ip, data_port)
    # recv_socket.bind((host_ip, data_port))
    # recv_socket.listen(5)

    # (ftserver, address) = recv_socket.accept()
    # print "Listening on host %s port %d" % (host_ip, data_port)

    # send command and data port to server
    if command in ['-l','-g']:
        if command == '-l':
            sock.send("%s%s\n" % (command, data_port))
        elif command == '-g':
            sock.send("%s\t%s\t%s\n" % (command, data_port, filename))
    else:
        sock.send("%s\t\n" % command)    
    
    # get initial response over control connection
    response = sock.recv(1024)
    if response.startswith("ERROR\0"):
        print "Received error message from server. Exiting."
        sys.exit()
    elif response.startswith("OK\0"):
        client_init_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM, 0)
        client_init_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        client_init_sock.bind(("", data_port))
        
        # listen for server to make data conneciton
        print "Listening for data connection..."
        client_init_sock.listen(5)
        client_data_sock = client_init_sock.accept()[0]
        
        if command == "-l":
            file_list = ""
            data = client_data_sock.recv(1024)
            while data:
                print data
        elif command == "-g":
            print "Working on get."
        
    else:
        print "Response: %s" % response
    
    
    
    
    # ftserver, address = recv_socket.accept()
    # print "Accepted connection on %s" % address
    
    
    # response = sock.recv(1024)
    # print response
    # if response != "ERROR\n":
    #     if command == '-l':
    #         file_list = ''
    #         data = recv_socket.recv(1024)
    #         while data:
    #             file_list = file_list + data
    #             data = ftserver.recv(1024)
    #         print file_list
    #     elif command == '-g':
    #         data = recv_socket.recv(1024)
    #         with open(filename, 'w') as f:
    #             while data:
    #                 if data == "File not found.\n":
    #                     print data
    #                     break
    #                 else:
    #                     f.write(data)
    #                     data = recv_socket.recv(1024)
        
    #     ftserver.close()
    # else:
    #     print "Invalid command.  Exiting..."
    #     sock.shutdown
    #     sock.close()
    #     sys.exit()
            

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
