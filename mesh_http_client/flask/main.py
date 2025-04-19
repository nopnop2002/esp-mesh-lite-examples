#!/usr/bin/env python
# -*- coding: utf-8 -*-
# Simple REST Server

from flask import Flask, render_template, request
#from flask import Flask, Response, request
app = Flask(__name__)
import json

nodeList = []

@app.route("/")
def get():
	return render_template("index.html", title="esp32 mesh node list", lists=sorted(nodeList))

@app.route("/", methods=["POST"])
def post():
	#print("post: request={}".format(request))
	#print("post: request.data={}".format(request.data))
	payload = request.data
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
	return data

if __name__ == "__main__":
	#app.run()
	app.run(host='0.0.0.0', port=8080, debug=True)

