CC=g++
FLAGS=-std=c++11 
SSL_DIR=/usr/local/ssl
INCS= -I$(SSL_DIR)/include
LIBS= -L$(SSL_DIR)/lib -lssl -lcrypto -pthread -ldl

SOURCES=main.cpp \
		MQTTClient.cpp \
		MQTTConnectOptions.cpp \
		MQTTMessage.cpp \
		Network.cpp \
		NetworkSecurityOptions.cpp \
		Socket.cpp \
		SSLSocket.cpp \
		TCPSocket.cpp \
		Utils.cpp
BIN=mqtt_client

all: clean $(SOURCES) $(BIN)

$(BIN): $(SOURCES)
	$(CC) $(SOURCES) -o $@ $(INCS) $(LIBS) $(FLAGS)

run:
	./$(BIN)

clean:
	rm -f *.o $(BIN) *.h~ *.cpp~ *.out
