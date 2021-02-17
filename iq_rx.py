"""
Simple test receiver for UDP streamed IQ data
Assumes an 64-bit sequence number followed by 1024 bytes of data
Will print out number of packets received every 1000 packets
Will report missed sequences

Requires Python 3
"""

import socket

ip = '0.0.0.0'
port = 50005

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind((ip, port))
expSeqData = 0
packets = 0

print(f'Listening on port {port}')

while True:
	data, addr = sock.recvfrom(1500)
	packets += 1
	seqData = int.from_bytes(data[0:8], byteorder="little", signed = False)

	if (seqData != expSeqData):
		print(f'Missed {seqData - expSeqData}')
	expSeqData = seqData + 1

	if (len(data) != 1032):
		print(f'Bad length {len(data)}')

	if (packets % 100 == 0):
		if (packets / 1000) % 2 == 0:
			print(f'{packets}\r', end = '')
			#print('\r-', end = '')
		else:
			#print('\r|', end = '')
			pass
