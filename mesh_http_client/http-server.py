#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# https://qiita.com/tkj/items/210a66213667bc038110

import argparse
from http.server import HTTPServer
from http.server import BaseHTTPRequestHandler
from urllib.parse import urlparse
from urllib.parse import parse_qs

class CustomHTTPRequestHandler(BaseHTTPRequestHandler):

	def do_POST(self):
		#print(dir(self))
		#parsed = urlparse(self.path)
		#print("parsed={}".format(parsed))
		#params = parse_qs(parsed.query)
		#print("params={}".format(params))
		content_len  = int(self.headers.get("content-length"))
		#print("content_len={}".format(content_len))
		req_body = self.rfile.read(content_len).decode("utf-8")
		#print("req_body={}".format(req_body))
		print("message from {}={}".format(self.client_address, req_body))

		body = "OK"
		self.send_response(200)
		self.send_header('Content-type', 'text/html; charset=utf-8')
		self.send_header('Content-length', len(body.encode()))
		self.end_headers()
		self.wfile.write(body.encode())

	def log_message(self, format, *args):
		return

if __name__=='__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('--port', type=int, help='port', default=8080)
	args = parser.parse_args()
	print("args.port={}".format(args.port))

	server = HTTPServer(('0.0.0.0', args.port), CustomHTTPRequestHandler)
	server.serve_forever()
