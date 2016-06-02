# mqtt-client C++11
I wrote this client just for studying:
+ New features of C++11
+ Nonblocking socket
+ OpenSSL
+ Understand clearly about MQTT protocol

##Feature
+ Support subscribing, publishing, authentication, will messages, keep alive pings and all 3 QoS levels
+ Support security connection

##Building
### On Linux:
- cd mqtt-client
- cd src
- cd MQTT
- make
- make run

### On windows:
- Open visual studio
- Link project to OpenSSL include and library folder
- Build

##Usage
- Edit some code on main.cpp as you want

##Limited
- Don't support retransmit for QoS1 and QoS2

Feel free to contribute to the project in any way you like!
If you find out some bad code (code not clean). Please anounce to me because the purpose that I create this project just for studying.
