# Project LLN 2019 - Groupe H

## Structure

The project is structured in 4 folders;

1. cGateway: contains the code for the Z1 motes (root) used in Cooja for the simulation (communicates with pyGateway)
2. client: contains the Python code for the mqtt clients
3. pyGateway: contains the Python code for the mqtt gateway (communicate with the cGateway)
4. sensor: contains the code for the Z1 motes used in the Cooja simulation


### 1. cGateway
* gateway.c : contains the code used for the root mote, also handles the communication with the python side (pyGateway)
* gateway.h : header file for the root mote

### 2. client
* client.py : contains the Python code for a client, used for testing of the functionalities

### 3. pyGateway
* gateway.py : contains the Python code for the gateway between the C part (root mote) and the MQTT Broker

### 4. sensor
* sensor.h : header file for the sensor(s) mote(s)
* sensor.c : contains the code for the Z1 motes (handle data to send, communication between sensors, building of the tree)

## Testing the simulation

To test the simulation, we will have to use Cooja in Contiki (3.0).
Firstly, create several motes in Cooja, then launch the Python files, gateway (in pyGateway) and client(s).

We suppose that an MQTT Broker is running on localhost (Mosquitto was used for the test).

1/ Inside Cooja:
* Start a new simulation
* Create a first Z1 motes using gateway.c (this will be the root mote)
* Create several others Z1 motes using sensor.c (sensor that will send their data)
* Start the simulation and wait until the tree is created
* Save the serial port used by the root mote (mote with ID 1)

2/ Start the gateway:
* Go to the pyGateway directory
* If necessary, change the serial port inside gateway.py
* "Launch" `python3 gateway.py`

3/ Start a Python client:
* Go to the client directory
* "Launch" `python3 client.py`
* A client interface will display, use the command line to subscribe/unsubscribe to topic
