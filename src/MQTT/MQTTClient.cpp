#include "MQTTClient.h"
#include "MQTTMessage.h"
#include "Utils.h"

MQTTClient::MQTTClient(std::string host, uint32_t port, std::string clientID)
{	
	clientState = ClientState::DISCONNECT;
	keepAliveTick = 0;
	this->host = host;
	this->port = port;
	this->clientID = clientID;
	mqttConnectedCallback = nullptr;
	mqttDisconnectedCallback = nullptr;
	mqttPublishedCallback = nullptr;
	mqttDataCallback = nullptr;
}

MQTTClient::~MQTTClient()
{
}

void MQTTClient::Connect(MQTTConnectOptions mqttConnectOptions, bool security)
{
	this->mqttConnectOptions = mqttConnectOptions;
	network = make_unique<Network>();
	network->RegisterConnectedCallback(std::bind(&MQTTClient::TCPConnectedCallback, this));
	network->RegisterDisconnectedCallback(std::bind(&MQTTClient::TCPDisconnectedCallback, this));
	network->RegisterReceivedCallback(std::bind(&MQTTClient::TCPReceivedCallback, this, std::placeholders::_1, std::placeholders::_2));
	network->RegisterSentCallback(std::bind(&MQTTClient::TCPSentCallback, this, std::placeholders::_1));
	network->Connect(host, port, security);
	timer.Wait(1000, true, true, std::bind(&MQTTClient::TimerCallback, this));
}

void MQTTClient::Publish(std::string topicName, std::string payload, uint8_t qos, bool retain)
{
	if (clientState != ClientState::CONNECT)
	{
		return;
	}
	bool dup = false;
	std::unique_ptr<MQTTMessage> mqttMessage = MQTTMessage::MQTTMessagePublish(topicName, payload, dup, qos, retain);
	network->WriteData(mqttMessage->GetMessageData(), mqttMessage->GetMessageLength());
}

void MQTTClient::Subscribe(std::string topicName, uint8_t qos)
{
	if (clientState != ClientState::CONNECT)
	{
		return;
	}
	std::unique_ptr<MQTTMessage> mqttMessage = MQTTMessage::MQTTMessageSubscribe(topicName, qos);
	network->WriteData(mqttMessage->GetMessageData(), mqttMessage->GetMessageLength());
}

void MQTTClient::Unsubscribe(std::string topicName)
{
	if (clientState != ClientState::CONNECT)
	{
		return;
	}
	std::unique_ptr<MQTTMessage> mqttMessage = MQTTMessage::MQTTMessageUnsubscribe(topicName);
	network->WriteData(mqttMessage->GetMessageData(), mqttMessage->GetMessageLength());
} 

void MQTTClient::TCPConnectedCallback()
{
	LOGI("Connecting to broker...");
	std::unique_ptr<MQTTMessage> mqttMessage = MQTTMessage::MQTTMessageConnect(clientID, mqttConnectOptions);
	network->WriteData(mqttMessage->GetMessageData(), mqttMessage->GetMessageLength());
}

void MQTTClient::TCPDisconnectedCallback()
{
	LOGI("Disconnected");
	if (mqttDisconnectedCallback)
	{
		mqttDisconnectedCallback();
	}
}

void MQTTClient::TCPReceivedCallback(uint8_t* data, std::size_t dataLength)
{
	MQTTMessageType messageType = MQTTMessage::GetMessageType(data);
	switch (messageType)
	{
		case MQTTMessageType::MQTT_MSG_CONNACK:
		{
			MQTTConnectReturnCode connectReturnCode = MQTTMessage::GetConnectReturnCode(data);
			if (connectReturnCode == MQTT_CONNECTION_ACCEPTED)
			{
				clientState = ClientState::CONNECT;
				LOGI("Client connected to broker %s:%d", host.c_str(), port);
				if (mqttConnectedCallback)
				{
					mqttConnectedCallback();
				}
			}
			else
			{
				switch (connectReturnCode)
				{
				case MQTT_CONNECTION_UNACCEPTABLE_PROTOCOL_VERSION:
					LOGI("The Server does not support the level of the MQTT protocol requested by the Client");
					break;
				case MQTT_CONNECTION_IDENTIFIER_REJECTED:
					LOGI("The Client identifier is correct UTF-8 but not allowed by the Server");
					break;
				case MQTT_CONNECTION_SERVER_UNAVAILABLE:
					LOGI("The Network Connection has been made but the MQTT service is unavailable");
					break;
				case MQTT_CONNECTION_BAD_USERNAME_OR_PASSWORD:
					LOGI("The data in the user name or password is malformed");
					break;
				case MQTT_CONNECTION_NOT_AUTHORIZED:
					LOGI("The Client is not authorized to connect");
					break;
				}
				network->Disconnect();
			}
			break;
		}
		case MQTTMessageType::MQTT_MSG_PUBLISH:
		{
			if (mqttDataCallback)
			{
				std::string topicName = MQTTMessage::GetPublishTopicName(data);
				std::string payload = MQTTMessage::GetPublishPayload(data);

				mqttDataCallback(topicName, payload);
			}
			uint8_t qos = MQTTMessage::GetPublishQos(data);
			if (qos == 1)
			{
				std::unique_ptr<MQTTMessage> mqttMessage = MQTTMessage::MQTTMessagePubAck(MQTTMessage::GetPacketIdentifier(data));
				network->WriteData(mqttMessage->GetMessageData(), mqttMessage->GetMessageLength());
			}
			else if (qos == 2)
			{
				std::unique_ptr<MQTTMessage> mqttMessage = MQTTMessage::MQTTMessagePubRec(MQTTMessage::GetPacketIdentifier(data));
				network->WriteData(mqttMessage->GetMessageData(), mqttMessage->GetMessageLength());
			}
			break;
		}
		case MQTTMessageType::MQTT_MSG_PUBACK:
		{
			LOGI("Published QoS1 packet identifier: %d", MQTTMessage::GetPacketIdentifier(data));
			break;
		}
		case MQTTMessageType::MQTT_MSG_PUBREC:
		{
			std::unique_ptr<MQTTMessage> mqttMessage = MQTTMessage::MQTTMessagePubRel(MQTTMessage::GetPacketIdentifier(data));
			network->WriteData(mqttMessage->GetMessageData(), mqttMessage->GetMessageLength());
			break;
		}
		case MQTTMessageType::MQTT_MSG_PUBREL:
		{
			std::unique_ptr<MQTTMessage> mqttMessage = MQTTMessage::MQTTMessagePubComp(MQTTMessage::GetPacketIdentifier(data));
			network->WriteData(mqttMessage->GetMessageData(), mqttMessage->GetMessageLength());
			break;
		}
		case MQTTMessageType::MQTT_MSG_PUBCOMP:
		{
			LOGI("Published QoS2 packet identifier: %d", MQTTMessage::GetPacketIdentifier(data));
			break;
		}
		case MQTTMessageType::MQTT_MSG_SUBACK:
		{
			MQTTSubscribeReturnCode subscribeReturnCode = MQTTMessage::GetSubscribeReturnCode(data);
			switch (subscribeReturnCode)
			{
			case MQTT_SUBSCRIBE_QOS0:
				LOGI("Subscribed QoS0 packet identifier: %d", MQTTMessage::GetPacketIdentifier(data));
				break;
			case MQTT_SUBSCRIBE_QOS1:
				LOGI("Subscribed QoS1 packet identifier: %d", MQTTMessage::GetPacketIdentifier(data));
				break;
			case MQTT_SUBSCRIBE_QOS2:
				LOGI("Subscribed QoS2 packet identifier: %d", MQTTMessage::GetPacketIdentifier(data));
				break;
			case MQTT_SUBSCRIBE_FAILURE:
				LOGI("Failt to subscribe topic");
				break;
			}
			break;
		}
		case MQTTMessageType::MQTT_MSG_UNSUBACK:
		{
			LOGI("Unsubscribe packet identifier: %d", MQTTMessage::GetPacketIdentifier(data));
			break;
		}
		case MQTTMessageType::MQTT_MSG_PINGREQ:
		{
			std::unique_ptr<MQTTMessage> mqttMessage = MQTTMessage::MQTTMessagePingResp();
			network->WriteData(mqttMessage->GetMessageData(), mqttMessage->GetMessageLength());
			break;
		}
		case MQTTMessageType::MQTT_MSG_PINGRESP:
		{
			LOGI("Server respond ping request");
			break;
		}
	}
}

void MQTTClient::TCPSentCallback(std::size_t bytesTransferred)
{
	LOGI("Sent %d bytes", static_cast<int>(bytesTransferred));
}

void MQTTClient::TimerCallback()
{
	if (clientState == ClientState::CONNECT)
	{
		++keepAliveTick;
		if (keepAliveTick >= mqttConnectOptions.GetKeepAlive())
		{
			LOGI("Send keep alive message");
			keepAliveTick = 0;
			std::unique_ptr<MQTTMessage> mqttMessage = MQTTMessage::MQTTMessagePingReq();
			network->WriteData(mqttMessage->GetMessageData(), mqttMessage->GetMessageLength());
		}
	}
}

void MQTTClient::MQTTOnConnected(MQTTCallback mqttConnectedCallback)
{
	this->mqttConnectedCallback = mqttConnectedCallback;
}

void MQTTClient::MQTTOnDisconnected(MQTTCallback mqttDisconnectedCallback)
{
	this->mqttDisconnectedCallback = mqttDisconnectedCallback;
}

void MQTTClient::MQTTOnPublished(MQTTCallback mqttPublishedCallback)
{
	this->mqttPublishedCallback = mqttPublishedCallback;
}

void MQTTClient::MQTTOnReceivedPayload(MQTTDataCallback mqttDataCallback)
{
	this->mqttDataCallback = mqttDataCallback;
}
