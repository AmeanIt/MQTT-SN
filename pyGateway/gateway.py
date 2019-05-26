import paho.mqtt.client as mqtt
import time
from threading import Thread
import os
from subprocess import Popen, PIPE, STDOUT

class MQTTGateway(object):

	def __init__(self, broker,port, topics):
		self.broker = broker
		#command for the device
		self.cmd = cmd = Popen(['make','login','TARGET=z1','MOTES='+port], stdout = PIPE, stdin = PIPE)
		self.port = port #device port
		self.clientCounter = 0 
		self.sending = False #Use to say to the gateway in c if he can send the data
		self.sendingMode = 0 #0 for periodical / 1 for modification
		self.packetToSend = [] #queue of packet to send
		self.nodes = []
		self.topics = topics
		self.initClient()
		thread = Thread(target = self.readThread)
		thread.start()
		self.interface()

	def initClient(self):
		"""
		The gateway is seen by the broker as a client.
		The main difference between it and other client is that it communicates
		with the gateway in C (which communicates with sensors)
		"""
		self.client = mqtt.Client("Gateway", True, None, mqtt.MQTTv31)
		self.client.on_connect = self.on_connect
		self.client.on_message = self.on_message
		self.client.connect(self.broker, 1883, 60)
		self.client.subscribe("subscribe") #MQTT msg send by client
		self.client.subscribe("unsubscribe")
		self.client.loop_start()

	def on_connect(self, client, userdata, flags, rc):
		if rc==0:
			print("Client connected ")
		else:
			print("Client failed to connect")
	
	def on_message(self, client, userdata, message):
		#msg received from client have the type of message in the topic (sub/unsub)
		#and the topic and node in the payload (topic+node)
		node = message.payload.decode()[-1]
		topic = message.payload.decode()[:-1]
		if topic in self.topics:
			if message.topic == "subscribe":
				if self.clientCounter == 0: #packet to the gateway in c that data
					self.packetToSend.append(b'S\n')#can be sent (at least 1 client) Opti 1
					self.sending = True			
				self.clientCounter += 1
				print("A client is interested in the topic " + str(topic) + " of node " + str(node) + "\n")
			else:
				#if there are no more client, gateway in C will stop sending data
				self.clientCounter -= 1
				if self.clientCounter == 0:
					self.packetToSend.append(b'S\n')
					self.sending = False
				print("A client lost interest in " + str(topic) + " of node " + str(node) + "\n")
		else:
			print("Invalid message !\n")

	def send_data(self):
		"""
		Thread used for sending our packet to the sensors (C gateway)
		"""
		while  1:				
			if (len(self.packetToSend) > 0):
				time.sleep(5)
				packet = self.packetToSend.pop(0)
				self.cmd.stdin.write(packet)
				self.cmd.stdin.flush()

	def readThread(self):
		"""
		Thread to read data from the C Gateway
		"""
		thread = Thread(target = self.send_data)
		thread.start()
		
		while True:
			line = self.cmd.stdout.readline() #read the output from C
			time.sleep(1)
			line = line.decode()
			line = line.strip("\n").split(":")
			if (line[0] == 'Sensor'):
				temp = "temperature"+ line[1]
				humi = "humidity" + line[1]
				if self.clientCounter > 0 and self.sending:
					self.client.publish(temp, line[2])
					self.client.publish(humi, line[3])

	def interface(self):
		"""
		Basic interface to allow the change of mode
		"""
		while 1:		
			print("\nChange the mode ?\n" +
					"1 - Periodical mode\n" + 
					"2 - Send when values change\n")
			choice = input("Choice: ")
			if choice == '1':
				if self.sendingMode == 1:
					self.sendingMode = 0
					self.packetToSend.append(b"M\n")
			elif choice == '2':
				if self.sendingMode == 0:
					self.sendingMode = 1
					self.packetToSend.append(b"M\n")

if __name__ == '__main__':
	print("Welcome !\n")
	print("Start of the MQTT Gateway\n")
	topics = ["temperature", "humidity"]
	port = '/dev/pts/9'
	#port = '/dev/ttyUSB0' #if real hardware
	os.chdir('/home/user/Desktop/MQTT-SN/cGateway')
	
	gateway = MQTTGateway("127.0.0.1", port, topics)
