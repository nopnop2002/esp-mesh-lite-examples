#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Simple REST Server

import tornado.httpserver
import tornado.ioloop
import tornado.options
import tornado.web
import tornado.httputil
import os
import json

from tornado.options import define, options
define("port", default=8080, help="run on the given port", type=int)

nodeList = []

class RootHandler(tornado.web.RequestHandler):
	def get(self):
		self.render("index.html", title="esp32 mesh node list", lists=sorted(nodeList))

	def post(self):
		print("post request.body={}".format(self.request.body))
		payload = self.request.body
		if (type(payload) is bytes):
			payload = payload.decode('utf-8')
		print("post: payload={}".format(payload))
		json_object = json.loads(payload)
		mac = json_object['mac']
		print("post: mac=[{}]".format(mac))
		print("post: nodeList={}".format(nodeList))
		nodeIndex = None
		for i in range(len(nodeList)):
			print("post: nodeList={}".format(nodeList[i]))
			if (mac == nodeList[i][1]):
				nodeIndex = i
	
		print("post: nodeIndex={}".format(nodeIndex))
		node = []
		level = json_object['level']
		now = json_object['now']
		cores = json_object['cores']
		target = json_object['target']
		node.append(level)
		node.append(mac)
		node.append(now)
		node.append(cores)
		node.append(target)
		if (nodeIndex is None):
			nodeList.append(node)
		else:
			nodeList[nodeIndex] = node
		#print(sorted(nodeList))
		data = json.dumps(['result', 'ok'])

if __name__ == "__main__":
	tornado.options.parse_command_line()
	app = tornado.web.Application(
		handlers=[
			(r"/", RootHandler),
		],
		template_path=os.path.join(os.path.dirname(__file__), "templates"),
		debug=True
	)
	http_server = tornado.httpserver.HTTPServer(app)
	http_server.listen(options.port)
	tornado.ioloop.IOLoop.current().start()
