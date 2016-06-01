#ifndef _NETWORK_H_
#define _NETWORK_H_
#include <stdint.h>
#include "TCPSocket.h"
#include "SSLSocket.h"
#include "MQTTConfig.h"
#include "Utils.h"

class Network
{
	public:
		Network();
		~Network() = default;
		Network(Network&) = delete;
		Network& operator=(Network&) = delete;
		void Connect(std::string host, uint32_t port, bool security);
		void Disconnect();
		void WriteData(uint8_t *data, std::size_t dataLength);
		void RegisterConnectedCallback(std::function<void()> connectedCallback);
		void RegisterDisconnectedCallback(std::function<void()> disconnectedCallback);
		void RegisterReceivedCallback(std::function<void(uint8_t*, std::size_t)> receivedCallback);
		void RegisterSentCallback(std::function<void(std::size_t)> sentCallback);
	private:
		void ConnectHandler(bool error);
		void WriteHandler(bool error, std::size_t bytesTransferred);
		void ReadHandler(bool error, std::size_t bytesTransferred);	
	private:	
		std::unique_ptr<Socket> socket;
		std::function<void()> connectedCallback;
		std::function<void()> disconnectedCallback;
		std::function<void(uint8_t*, std::size_t)> receivedCallback;
		std::function<void(std::size_t)> sentCallback;
		uint8_t readBuffer[MQTT_MAX_MESSAGE_LENGTH];
		uint8_t buffer[MQTT_MAX_MESSAGE_LENGTH];
		uint32_t bufferIndex;
		bool readDone;
};
#endif //_NETWORK_H_