#!/usr/bin/env python
# python3 -m pip install websockets
# https://github.com/python-websockets/websockets

import argparse
import asyncio
import websockets

async def receive(websocket):
	#print(dir(websocket))
	async for message in websocket:
		print("message from {}={}".format(websocket.remote_address, message))
		#await websocket.send(message)

async def main(port):
	async with websockets.serve(receive, "0.0.0.0", port):
		await asyncio.Future()

if __name__=='__main__':
		parser = argparse.ArgumentParser()
		parser.add_argument('--port', type=int, help='port', default=8080)
		args = parser.parse_args()
		print("args.port={}".format(args.port))
		asyncio.run(main(args.port))


