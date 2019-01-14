************************************************************************************************************************************************************************
						README
************************************************************************************************************************************************************************
TITLE :  NS 2 Familirisation

INTRODUCTION : 

This code is a part of the Network simulation Assignment for ECEN 602 at Texas A&M University. 
It has been sucessfully compiled, executed and tested on Network Simulator - 2 environment.
Ns is a discrete event simulator targeted at networking research. Ns provides substantial support
for simulation of TCP, routing, and multicast protocols over wired and wireless (local and satellite) 
networks. NS2 consists of two key languages: CCC and Object-oriented Tool Command Language (OTcl). 
While the CCC defines the internal mechanism (i.e., a backend) of the simulation, the OTcl sets up 
simulation by assembling and configuring the objects as well as scheduling discrete events 
(i.e., a frontend). The CCC and the OTcl are linked together using TclCL. After simulation, NS2
outputs either text-based simulation results. To interpret these results graphically and interactively,
tools such as NAM (Network AniMator) and XGraph are used. To analyze a particular behavior of the network,
users can extract a relevant subset of text-based data and transform it to a more conceivable presentation.
This step implements the design in the first step. It consists of two phases:

• Network configuration phase: In this phase, network components (e.g., node,
TCP and UDP) are created and configured according to the simulation design.
Also, the events such as data transfer are scheduled to start at a certain
time.

• Simulation Phase: This phase starts the simulation which was configured in the
Network Configuration Phase. It maintains the simulation clock and executes
events chronologically. This phase usually runs until the simulation clock reaches
a threshold value specified in the Network Configuration Phase.

Creating MAKEFILE:
-The makefile rules are already set.
-Please use following command in the folder where makefile is present:

make makefile

Native TFTF Client in Linux is used for testing the implemented server.
Then, EXECUTE using the following commands:

./team17_server 127.0.0.1 50001 12

Package content:
1. server.c
2. Makefile
3.2048_bin file
4.2047_bin file
5.random(34mb)

Usage:
1. 'make clean' to remove all previously created object files.
2. 'make server' to compile the Server source code.
3. ./server Server_IP Server_Port
4.Use tftp command and follow instructions for generating RRQ and WRQ requests.


Tests:
1. All RRQ tests run successfully. Block number wrap around feature tested OK.
2. Idle client identification and subsequent termination of file trasfer tested OK.
3. Feature with multiple clients connecting and downloading the same file from the server tested OK.
4. All WRQ tests run successfully.


