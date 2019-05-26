import paho.mqtt.client as mqtt
import time
    		
class MQTTClient():

	def __init__(self,broker, topics):
		self.broker = broker #broker address
		self.topics = topics
		self.subs = {} #topic in which the client is subscribed
		self.nodes = [] #nodes containing at least one topic subs
		self.values = {} #values of the topic
		self.client = mqtt.Client(client_id="", clean_session=True, userdata=None, protocol= mqtt.MQTTv31)
		self.client.on_connect = self.on_connect
		self.client.on_message = self.on_message
		print("Connecting to broker ", broker)
		self.client.connect(broker,1883,60) #host, port, keepalive period	
		time.sleep(5)
		self.client.loop_start()
		
		print("\nWelcome !")	
		self.interface()
		#self.client.loop_forever()


	def on_connect(self, client, userdata, flags, rc):
		if rc==0:
			print("Connection granted")
		else:
			print("Bad connection")

	def on_message(self, client, userdata, message):
		"""
		reception of message from the gateway in the form "topicNode"
		ex: temperature5
		"""		
		node = message.topic[-1]#extract node
		topic = message.topic[:-1]
		if node not in self.values: #if new nodes
			self.values[node] = [0,0] #add empty value in dict
		if topic == "temperature":
			#replace value
			self.values[node][0] = message.payload.decode()
		else:
			self.values[node][1] = message.payload.decode()
		

	def joinTopic(self, topic, node, stype='1'):	
		"""
		When user wishes to subscribe (stype=1) /unsubscribe (stype=2) 
		to a topic
		"""
		if stype == "1":
			if node not in self.nodes: #if new node
				self.nodes.append(node)
				self.subs[node] = [0,0]

			if topic == self.topics[0]:
				self.subs[node][0] = 1
			elif topic == self.topics[1]:
				self.subs[node][1] = 1
			topic += node #message send to broker is topic+node
			self.client.publish('subscribe', topic)
			self.client.subscribe(topic)

		else: #unsubscribe
			if node in self.nodes: #if client was sub to the node
				#change value depending of the topic
				if topic == self.topics[0]:
					if self.subs[node][0] == 1:
						self.subs[node][0] = 0
				else:
					if self.subs[node][1] == 1:
						self.subs[node][1] = 0
				#if both value are 0, then the node can be delete from the list
				if self.subs[node][0] == 0 and self.subs[node][1] == 0:
					index = ""
					for i in range(len(self.nodes)):
						if node == self.nodes[i]:
							index = i
					del self.nodes[index]
					self.client.unsubscribe(topic) #allow the client to not receive 
													#anymore msg about this node
				topic += node
				self.client.publish('unsubscribe', topic)
	
# ----------------------------------------------------------
#							INTERFACE			
# ----------------------------------------------------------
	def chooseTopicToSubscribe(self):
		"""
		User choose the topic and the node in which he wants to subscribe
		"""
		print("\nTo which topic do you want to subscribe ?")

		for topic in self.topics:
			print(topic)

		choice = input("Your choice: ")
		if choice in self.topics:
			chosenNode = input("\nWhich node do you choose: ")
			self.joinTopic(choice, chosenNode)
		else:
			print("The wanted topic does not exist.\n Try again")
			self.chooseTopicToSubscribe()

	def chooseTopicToUnsubscribe(self):
		"""
		User choose the topic and the node in which he wants to unsubscribe
		"""
		print("\nTo which topic do you want to unsubscribe ?")

		for topic in self.topics:
			print(topic)

		choice = input("Your choice: ")
		if choice in self.topics:
			chosenNode = input("\nWhich node do you choose: ")
			self.joinTopic(choice, chosenNode,'2')
		else:
			print("The wanted topic does not exist.\n Try again")
			self.chooseTopicToUnsubscribe()

	def interface(self):
		endProgram = False
		while not endProgram:
			print("0 - Leave the program\n" + \
				"1 - Subscribe to a topic\n" + \
				"2 - Unsubscribe to a topic\n"
				"3 - Show information")

			choice = input("Choice: ")

			if choice == '0':
				endProgram = True
			elif choice == '1':
				self.chooseTopicToSubscribe()
			elif choice == '2':
				self.chooseTopicToUnsubscribe()
			elif choice == '3':
				self.showInformation()
			else:
				print("Incorrect input\n")

	def showInformation(self):
		"""
		Display data received from the gateway (sensors)
		"""
		for node in self.values:
			#if client is sub to both topic of a same node
			if self.subs[node][0] == 1 and self.subs[node][1] == 1:
				print("Node {}:  Temperature: {}  /  Humidity: {}\n".format(node,self.values[node][0],self.values[node][1]))
			elif self.subs[node][0] == 1: #sub to only one
				print("Node {}:  Temperature: {}  \n".format(node,self.values[node][0]))
			elif self.subs[node][1] == 1:
				print("Node {}:  Humidity: {}  \n".format(node,self.values[node][1]))
	

if __name__ == '__main__':
	topics = ["temperature", "humidity"]
	MQTTClient("127.0.0.1", topics)


