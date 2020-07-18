# PACKET-LOSS
When we access the Internet for sending emails, downloading any data or image file, or looking for any information, the tiny entities of data are sent and received over the Internet these are known as packets. The flow of data packets takes place between source and destination nodes in any network and reaches its destination by passing through various transit nodes.
Now, whenever these data packets fail to reach the desired final destination then the condition is called a packet loss. It impacts the overall network throughput and QoS as due to the unsuccessful delivery of packets to the destination node the network speed slows down and real-time applications such as streaming videos, gaming also get affected.
TCP protocol has the model for re-transmission of lost packets and when TCP protocol is used for delivery of data packets, it identifies the lost packets and re-transmits the packets that are not acknowledged by the receiver

# WHY DO WE PERFORM THE TEST FOR PACKET LOSS?
The packet loss is responsible for many of the network issues especially in the WAN connectivity and Wi-Fi networks. The packet loss test results conclude the reasons behind it like the issue is due to the network connectivity or the quality of the network degrades due to TCP or UDP packet loss.

# PACKET-LOSS CAUSES
1) Network Congestion
2) Software Bugs
3) The issue with Network Hardware
4) Security threats
5) Overloaded device and inadequate infrastructure for handling network

# HOW TO FIX PACKET LOSS?
Check the physical connections
Restart the system
Update the software
Using reliable cable connection instead of Wi-Fi
Replace out-of-date hardware
Detecting error types and fixing accordingly
Link balance

###### UDP packet loss detection (see the TCP one down this file)

Basically, you start a listener on the destination machine.
You then start the talker: it will send packets a given interval (100 ms, limit seems to be around 5ms) containing increasing sequence number.
The listener tracks these sequence numbers and reports "holes" or out of order packets. (simple assumption: sequence is continuously increasing. possible bug: rollover of the counter, a long integer)
 
With the verbose option, you have a human readable output, by default it's CSV.
 
The talker has a debug flag to test the setup (it doesnt send packets with 5<seq<10)
 
<hostname>:~/packet_loss # ./talker_udp -h
usage: talker_udp [-v] [-h] [-s source_port] [-d destination_port]  [-i start_seq] [-c count] [-t interval] hostname
     -v  verbose
     -h  this page
     -s  source UDP port (default 1234)
     -d  destination UDP port (default 4950)
     -i  first sequence number to send (default 0)
     -c  send 'count' packets (default 0 = unlimited)
     -t  send a packet every t usec (default 100000 = 100ms)
     -D  debug mode: suppress packets with 5<seq<10
     
     
<hostname>:~/packet_loss # ./talker_udp -D <hostname>
^C
<hostname>:~/packet_loss # ./talker_udp <hostname> -c 20 -d 10000 -D
 
<hostname>:~/packet_loss # ./listener_udp -h
usage: listener_udp [-v] [-h] [-l listening_port] [-i expected_seq]
     -v  verbose (up to 3 times)
     -h  this page
     -l  UDP port to listen to (default 4950)
     -i  first sequence number to be expected (default 0)

<hostname>:~/packet_loss # ./listener_udp
lost:5;1179850912.190145;1179850912.790017;0.599872
^c
<hostname>:~/packet_loss # ./listener_udp -v -l 10000
lost 5 packets from 1179850965.663368 to 1179850966.263256 (0.599888 sec)
^c
<hostname>:~/packet_loss #
<hostname>:~/packet_loss # ./talker_udp <hostname> -c 20 -d 10000 -D
<hostname>:~/packet_loss # ./talker_udp <hostname> -c 20 -d 10000 -D -v
sent 0: 1 bytes to 193.5.227.102
sent 1: 1 bytes to 193.5.227.102
sent 2: 1 bytes to 193.5.227.102
sent 3: 1 bytes to 193.5.227.102
sent 4: 1 bytes to 193.5.227.102
sent 10: 2 bytes to 193.5.227.102
sent 11: 2 bytes to 193.5.227.102
sent 12: 2 bytes to 193.5.227.102
sent 13: 2 bytes to 193.5.227.102
sent 14: 2 bytes to 193.5.227.102
sent 15: 2 bytes to 193.5.227.102
sent 16: 2 bytes to 193.5.227.102
sent 17: 2 bytes to 193.5.227.102
sent 18: 2 bytes to 193.5.227.102
sent 19: 2 bytes to 193.5.227.102
<hostname>:~/packet_loss #
<hostname>:~/packet_loss # ./listener_udp -l 10000
lost:5;1179851142.126529;1179851142.726393;0.599864
OoO:1179851144.970921;0;20
OoO:1179851145.070865;1;20
OoO:1179851145.170845;2;20
OoO:1179851145.270826;3;20
OoO:1179851145.371799;4;20
OoO:1179851145.972666;10;20
OoO:1179851146.072651;11;20
OoO:1179851146.172624;12;20
OoO:1179851146.272598;13;20
OoO:1179851146.372578;14;20
OoO:1179851146.472558;15;20
OoO:1179851146.572537;16;20
OoO:1179851146.672514;17;20
OoO:1179851146.772488;18;20
OoO:1179851146.872469;19;20


###### TCP packet loss detection

<hostname>:~/packet_loss # ./talker_tcp -h
usage: talker_tcp [-v] [-h] [-s source_port] [-d destination_port] [-c count] [-t interval] hostname
     -v  verbose
     -h  this page
     -s  source TCP port (default any)
     -d  destination TCP port (default 4950)
     -c  send 'count' packets (default 0 = unlimited)
     -t  send a packet every t usec (default 100000 = 100ms)


## 1. just a small test of the talker. you need to start the listener first (see next point)     

<hostname>:~/packet_loss # ./talker_tcp -t 100000 <hostname>
^z
[1]+  Stopped                 ./talker_tcp -t 100000 <hostname>
<hostname>:~/packet_loss # fg
./talker_tcp -t 100000 <hostname>
^c
<hostname>:~/packet_loss #

## 2. the test of the listener

<hostname>:~/packet_loss # ./listener_tcp -h
usage: listener_tcp [-v] [-h] [-l listening_port] [-s sensitivity] [-r recover]
     -v  verbose (up to 3 times)
     -h  this page
     -l  TCP port to listen to (default 4950)
     -s  sensitivity: how many us should we wait before declaring a delay (default 10ms)
     -r  recover > total of variance of trip time to consider stream back in sync (default 5ms)

<hostname>:~/packet_loss # ./listener_tcp
state: synched (on stderr)
delay_detected:1179875901.294298
recover:1179875903.707797:2413499
state: synched (on stderr)
full_recover:1179875903.807761:2513463
connection has been closed
<hostname>:~/packet_loss #


## 3. How it works

So, the sender just sends the localtime stamp over a tcp connection every x usec. Because it is not too much together, it should go in independant packets.

The receiver has a state machine:
 state: 0 - syncing, 1 - synced, 2 - delay detected, 3 - stream is back

0 syncing:
The receiver gather stats (with 3 bins, each collecting info of 10 packets):
	- interval time between two packets
	- trip time: time differnece between timestamp and localtime (do not need to be synchronised, but roughly go to the same pace)

1 - synced
Once stats are stable, go to this state.
Wait for a timeout to occur. Timeout is based on the interval stat, i.e. if we don't receive a packet as usual, we declare a delay. (probably due to a packet loss) move to state 2

2- delay detected
We basically wait for the stream to come back. If it is the case, display recover time (usec since delay detected)

3- stream is back
We wait until the trip stats are stabilised, which should indicate that the TCP stack has resent all its packets, and we are in a stable phase again. (actually, after thinking again, it may be that my algorithm decide too rapidly that we are back.. Whatever, not that important...)
We print then a full_recover message, and go back in state 1. 


## 4. Issues

Potential bug: I don't know how TCP reacts to dropped packets, especially if it gather all what has been sent from the application layer and build a big TCP packet to send everything at once. In that case, my programm my suffer. Receive buffer is 100 bytes, we should be enough to eat up to 5 packets. Only the first one will be considered for the stat, but should be ok to calculate recovery time (there's no state synchronised between the two programs as in the udp version). We may have an issue if the next segment doesn't start nicely on a message boundary. In that case, the trip time calculation will be wrong, potentially corrupting a bin. It should take more than 3 secs to reset the stat by valid packets, potentially delaying full recovery by that time. (not critical)

Note: state has to go back in 1 to be able to calculate delay time correctly.

Note2: if connections get dropped, you just recieve on the sender or receiver a connection closed or broken pipe. No timestamp though, so watch your console while doing the tests.

