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

    # send command and data port to server
    if command in ['-l','-g']:
        if command == '-l':
            sock.send("%s\t%s\n" % (command, data_port))
        elif command == '-g':
            message_to_server = "%s\t%s\t%s\n" % (command, data_port, filename)
            print message_to_server
            sock.send(message_to_server)
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
        print "Listening for data connection on port %d..." % data_port
        client_init_sock.listen(5)
        client_data_sock = client_init_sock.accept()[0]
        
        if command == "-l":
            file_list = ""
            data = client_data_sock.recv(1024)
            while data:
                file_list = file_list + data + "\n"
                data = client_data_sock.recv(1024)
            print "Listing the contents of the server directory..."
            print file_list
            try:
                client_data_sock.shutdown(socket.SHUT_RDWR)
                client_data_sock.close()
            except:
                pass
        elif command == "-g":
            print "Working on get."
            data = client_data_sock.recv(1024)

            if data.startswith("ERROR\0"):
                print "File %s does not exist on server." % filename
            else:
                # check if filename is already in use
                alt_file_number = 1
                if os.path.isfile(filename):
                    original_filename = filename
                    print "Filename %s already in use locally." % filename
                    filename = original_filename + "_" + str(alt_file_number)
                    while os.path.isfile(filename):
                        alt_file_number += 1
                        filename = original_filename + "_" + str(alt_file_number)
                    print "Using alternate filename %s" % filename
                
                # receive data and write it to file
                print "Beginning file transfer from %s" % host_ip
                with open(filename, 'w') as f:
                    while data:
                        f.write(data)
                        data = client_data_sock.recv(1024)
                print "Transfer complete."
            try:
                client_data_sock.shutdown(socket.SHUT_RDWR)
                client_data_sock.close()
            except:
                pass
    else:
        print "Response: %s" % response
    
    # close initial socket
    sock.shutdown(socket.SHUT_RDWR)
    sock.close()

except socket.gaierror:
    print 'Could not find host. Exiting.'
    sys.exit()
