#!/usr/bin/env python3

import pyvisa

rm = pyvisa.ResourceManager('@py')


def sync(t):
	ans = b''
	res = b'*'
	while res != b'':
		res = t.read_eager()
		print("read: '"+str(res)+"'\n")
		ans = ans + res
	return ans

def connect():
	tn = Telnet('wavegen', 5024)
	tn.write(b'\r\n')
	tn.write(b'\r\n')
	return tn

#Telnet.set_debuglevel(debuglevel=1)

inst = Telnet('wavegen', port=5024)
inst.set_debuglevel(5)
print("Connected:\n")
done = False 
while not done:
  a = inst.read_eager()
  print("xx: '"+str(a)+"'\n")
  if a == b'':
    done = True

inst.write(b"*IDN?\r\n")
inst.write(b"*IDN?\r\n")
inst.write(b"*IDN?\r\n")

done = False
while not done:
  a = inst.read_eager()
  print("xx: '"+str(a)+"'\n")
  if a == b'':
    done = True
