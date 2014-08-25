#!/usr/bin/env python

import os
import sys
import telnetlib
import socket
import select

debug = 1

def dump(level, prefix, data):
	if debug > level:
		print "[%s]: %r" % (prefix, data)

def telnet_nt():
	tn = telnetlib.Telnet(nt_ip, nt_port, 20)
	tn.set_debuglevel(255)
	tn.read_until("login: ")
	tn.write(user + "\n")
	tn.read_until("password: ")
	tn.write(password + "\n")
	return tn

def telnet_nego(client):
	iac_str = '\xff\xfd\x18\xff\xfd\x1f\xff\xfd"\xff\xfd-\xff\xfb\x01\r\n'
	client.sendall(iac_str)
	data = client.recv(4096)
	dump(0, "Iac from client", data)

def client_broadcast(sock_list, data):
	for cl_sock in sock_list:
		cl_sock.sendall(data)

def client_closeall(sock_list):
	for cl_sock in sock_list:
		cl_sock.close()

def share_nt_telnet(nt_ip, local_port):
	s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	print "Socket created"

	s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	s.bind(('', local_port))
	s.listen(10)
	print "Server start"

	nt_tn = None
	nt_sock = None
	inputs = [s]
	clients = {} 

	while True:
		dump(1, "inputs", inputs)
		rd, wd, ed = select.select(inputs, [], [])

		for sock in rd:
			#data from nt
			if sock == nt_sock:
				try:
					data = nt_sock.recv(4096)		
					dump(0, "Nt", data)
					if data:
						client_broadcast(clients.keys(), data)
					else:
						raise IOError
				except Exception as e:
					print e
					info = "Nt disconnected\r\n" 
					print info 
					nt_tn = None
					inputs = [s]
					client_broadcast(clients.keys(), info)
					client_closeall(clients.keys())
					clients = {} 

			else:
				#new connect from client
				if sock == s:
					sockfd, addr = s.accept()
					telnet_nego(sockfd)
					inputs.append(sockfd)
					clients[sockfd] = addr
					print "Client (%s, %s) connected" % addr
				#client
				else:
					try:
						data = sock.recv(4096)
						dump(0, "Client", data)
						if data:
							if nt_sock:
								nt_sock.sendall(data)
						else:
							raise IOError
					except Exception as e:
						print e
						print "Client (%s, %s) is offline" % clients[sock]
						inputs.remove(sock)
						del clients[sock]
						sock.close()

				#nt not connect?
				if not nt_tn:
					print "Connecting to NT"
					try:
						nt_tn = telnet_nt() 
						nt_sock = nt_tn.get_socket()
						print "Nt connected"
						inputs.append(nt_sock)
					except Exception as e:
						print e
						print "Connect NT failed"	
						info = "NT is resetting, please wait\r\n"
						client_broadcast(clients.keys(), info)
						nt_sock = None
						break

	s.close()				

def getip(itf):
	cmd = "ifconfig %s | grep 'inet addr' | awk '{print $2}' | cut -d: -f2" % itf
	return os.popen(cmd, "r").readline().strip()

if __name__ == '__main__':
	#nt side
	#nt_ip = "135.251.198.53"
	try:
		nt_ip = sys.argv[1]
		local_port = int(sys.argv[2])
	except IndexError:
		sys.exit("usage: %s nt_ip local_port" % sys.argv[0])
	nt_port = 23
	user = "isadmin"
	password = "      "
	
	print "NT ip: %s" % nt_ip
	print "Using local ip %s, port %d" % (getip('eth0'), local_port)

	share_nt_telnet(nt_ip, local_port)

