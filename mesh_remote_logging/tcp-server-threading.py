#!/usr/bin/env python3
#-*- encoding: utf-8 -*-
import socket
import argparse
import threading

def on_new_client(clientsocket,addr):
	print("on_new_client")
	while True:
		msg = clientsocket.recv(1024)
		if (type(msg) is bytes):
			msg=msg.decode('utf-8')
		print("msg={}".format(msg))
	clientsocket.close()

if __name__=='__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('--port', type=int, help='tcp port', default=8070)
	args = parser.parse_args()
	print("port={}".format(args.port))

	listen_num = 5
	#server_ip = "0.0.0.0"
	tcp_server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	tcp_server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	#tcp_server.bind((server_ip, args.port))
	tcp_server.bind(('', args.port))
	tcp_server.listen(listen_num)

	while True:
		client,address = tcp_server.accept()
		print("Connected from {}".format(address))
		thread = threading.Thread(target=on_new_client, args=(client,address,),daemon = True)
		thread.start()
	client.close()
