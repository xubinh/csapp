#!/usr/bin/env python
# -*- coding: utf-8 -*-

# nop-server.py - This is a server that we use to create head-of-line
#                 blocking for the concurrency test. It accepts a
#                 connection, and then spins forever.
#
# usage: nop-server.py <port>
#
import socket
import sys

# create an INET, STREAMing socket
serversocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
serversocket.bind(("", int(sys.argv[1])))
serversocket.listen(5)

while 1:
    # 接受连接:
    channel, details = serversocket.accept()

    # 但是接受第一个连接之后就阻塞:
    while 1:
        continue
