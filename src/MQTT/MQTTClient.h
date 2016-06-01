#ifndef _MQTT_CLIENT_H_
#define _MQTT_CLIENT_H_
#include "Network.h"
#include "MQTTConnectOptions.h"
#include "Timer.h"

enum class ClientState: uint8_t
{
	CONNECT = 0x01,
	DISCONNECT
};

using MQTTCallback = void(*)();
using MQTTDataCallback = void(*)(std::string topic, std::string payload);

class MQTTClient
{
	public:
		MQTTClient(std::string host, uint32_t port, std::string clientID);
		~MQTTClient();
		void Connect(MQTTConnectOptions mqttConnectOptions, bool security);
		void Publish(std::string topicName, std::string payload, uint8_t qos, bool retain);
		void Subscribe(std::string topicName, uint8_t qos);
		void Unsubscribe(std::string topicName);

		void MQTTOnConnected(MQTTCallback mqttConnectedCallback);
		void MQTTOnDisconnected(MQTTCallback mqttDisconnectedCallback);
		void MQTTOnPublished(MQTTCallback mqttPublishedCallback);
		void MQTTOnReceivedPayload(MQTTDataCallback mqttDataCallback);
	private:
		void TCPConnectedCallback();
		void TCPDisconnectedCallback();
		void TCPReceivedCallback(uint8_t* data, std::size_t dataLength);
		void TCPSentCallback(std::size_t bytesTransferred);
		void TimerCallback();
	private:
		std::unique_ptr<Network> network;
		std::string host;
		uint32_t port;
		std::string clientID;
		uint16_t keepAliveTick;
		MQTTConnectOptions mqttConnectOptions;
		Timer timer;
		ClientState clientState;
		MQTTCallback mqttConnectedCallback;
		MQTTCallback mqttDisconnectedCallback;
		MQTTCallback mqttPublishedCallback;
		MQTTDataCallback mqttDataCallback;
};	
#endif //_MQTT_CLIENT_H_