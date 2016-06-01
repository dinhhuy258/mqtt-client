#include "MQTTMessage.h"
#include "Utils.h"

uint16_t MQTTMessage::packetIdentifier = 0;

MQTTMessage::MQTTMessage() : message(nullptr), messageLength(0)
{
}

MQTTMessage::~MQTTMessage()
{
}	

std::unique_ptr<MQTTMessage> MQTTMessage::MQTTMessageConnect(std::string clientID, MQTTConnectOptions mqttConnectOptions)
{
	std::unique_ptr<MQTTMessage> mqttMessage(new MQTTMessage());
	MessageHeader header;
	ConnectFlags flags;

	header.byte = 0;
	header.bits.type = MQTT_MSG_CONNECT;

	flags.byte = 0;
	flags.bits.username = (mqttConnectOptions.username.empty()) ? 0 : 1;
	flags.bits.password = ((mqttConnectOptions.username.empty()) || (mqttConnectOptions.password.empty())) ? 0 : 1;
	if ((mqttConnectOptions.lastWillTopic.empty()) || (mqttConnectOptions.lastWillMessage.empty()))
	{
		flags.bits.lastWillFlag = 0;
		flags.bits.lastWillQos = 0;
		flags.bits.lastWillRetain = 0;
	}
	else
	{
		flags.bits.lastWillFlag = 1;
		flags.bits.lastWillQos = mqttConnectOptions.lastWillQos;
		flags.bits.lastWillRetain = mqttConnectOptions.lastWillRetain ? 1 : 0;
	}
	flags.bits.cleanSession = mqttConnectOptions.cleanSession ? 1 : 0;
	uint32_t remainingLength = 6 /*protocol name*/ + 1 /*protocol level*/ + 1 /*connect flags*/ + 2 /*keep alive*/;
	remainingLength += clientID.size() + 2;
	if (flags.bits.lastWillFlag == 1)
	{
		remainingLength += mqttConnectOptions.lastWillTopic.size() + 2 + mqttConnectOptions.lastWillMessage.size() + 2;
	}
	if (flags.bits.username == 1)
	{
		remainingLength += mqttConnectOptions.username.size() + 2;
	}
	if (flags.bits.password == 1)
	{
		remainingLength += mqttConnectOptions.password.size() + 2;
	}
	uint8_t remainingLenghtBytes[4];
	uint8_t length = CalculateRemainingLengthBytes(remainingLenghtBytes, remainingLength);
	uint32_t totalMessageLength = remainingLength + length + 1 /*header*/;
	mqttMessage->message = new uint8_t[totalMessageLength];
	mqttMessage->messageLength = totalMessageLength;
	uint8_t *ptr = mqttMessage->message;
	WriteChar(&ptr, header.byte);
	for (uint8_t i = 0; i < length; ++i)
	{
		WriteChar(&ptr, remainingLenghtBytes[i]);
	}
	WriteUTF(&ptr, PROTOCOL_NAME);
	WriteChar(&ptr, PROTOCOL_LEVEL);
	WriteChar(&ptr, flags.byte);
	WriteShort(&ptr, mqttConnectOptions.keepAlive);
	WriteUTF(&ptr, clientID);
	if (flags.bits.lastWillFlag)
	{
		WriteUTF(&ptr, mqttConnectOptions.lastWillTopic);
		WriteUTF(&ptr, mqttConnectOptions.lastWillMessage);
	}
	if (flags.bits.username)
	{
		WriteUTF(&ptr, mqttConnectOptions.username);
	}
	if (flags.bits.password)
	{
		WriteUTF(&ptr, mqttConnectOptions.password);
	}
	return mqttMessage;
}

std::unique_ptr<MQTTMessage> MQTTMessage::MQTTMessagePublish(std::string topicName, std::string payload, bool dup, uint8_t qos, bool retain)
{
	std::unique_ptr<MQTTMessage> mqttMessage(new MQTTMessage());
	MessageHeader header;

	header.byte = 0;
	header.bits.type = MQTT_MSG_PUBLISH;
	header.bits.dup = dup ? 1 : 0;
	header.bits.qos = qos;
	header.bits.retain = retain ? 1 : 0;
	uint32_t remainingLength = topicName.size() + 2 /*topic name*/ + 2 /*package identifier*/ + payload.size() + 2 /*payload*/;
	uint8_t remainingLenghtBytes[4];
	uint8_t length = CalculateRemainingLengthBytes(remainingLenghtBytes, remainingLength);
	uint32_t totalMessageLength = remainingLength + length + 1 /*header*/;
	mqttMessage->message = new uint8_t[totalMessageLength];
	mqttMessage->messageLength = totalMessageLength;
	uint8_t *ptr = mqttMessage->message;
	WriteChar(&ptr, header.byte);
	for (uint8_t i = 0; i < length; ++i)
	{
		WriteChar(&ptr, remainingLenghtBytes[i]);
	}
	WriteUTF(&ptr, topicName);
	if (qos == 0)
	{
		WriteShort(&ptr, 0);
	}
	else
	{
		++packetIdentifier;
		if (packetIdentifier == 0)
		{
			packetIdentifier = 1;
		}
		WriteShort(&ptr, packetIdentifier);
	}
	WriteUTF(&ptr, payload);
	return mqttMessage;
}

std::unique_ptr<MQTTMessage> MQTTMessage::MQTTMessagePubAck(uint16_t packetIdentifier)
{
	std::unique_ptr<MQTTMessage> mqttMessage(new MQTTMessage());
	MessageHeader header;
	header.byte = 0;
	header.bits.type = MQTT_MSG_PUBACK;
	uint32_t totalMessageLength = 1 /*header*/ + 1 /*remaining length*/ + 2 /*packet identifier*/;
	mqttMessage->message = new uint8_t[totalMessageLength];
	mqttMessage->messageLength = totalMessageLength;
	uint8_t *ptr = mqttMessage->message;
	WriteChar(&ptr, header.byte);
	WriteChar(&ptr, 2);
	WriteShort(&ptr, packetIdentifier);
	return mqttMessage;
}

std::unique_ptr<MQTTMessage> MQTTMessage::MQTTMessagePubRec(uint16_t packetIdentifier)
{
	std::unique_ptr<MQTTMessage> mqttMessage(new MQTTMessage());
	MessageHeader header;
	header.byte = 0;
	header.bits.type = MQTT_MSG_PUBREC;
	uint32_t totalMessageLength = 1 /*header*/ + 1 /*remaining length*/ + 2 /*packet identifier*/;
	mqttMessage->message = new uint8_t[totalMessageLength];
	mqttMessage->messageLength = totalMessageLength;
	uint8_t *ptr = mqttMessage->message;
	WriteChar(&ptr, header.byte);
	WriteChar(&ptr, 2);
	WriteShort(&ptr, packetIdentifier);
	return mqttMessage;
}

std::unique_ptr<MQTTMessage> MQTTMessage::MQTTMessagePubRel(uint16_t packetIdentifier)
{
	std::unique_ptr<MQTTMessage> mqttMessage(new MQTTMessage());
	MessageHeader header;
	header.byte = 0;
	header.bits.type = MQTT_MSG_PUBREL;
	uint32_t totalMessageLength = 1 /*header*/ + 1 /*remaining length*/ + 2 /*packet identifier*/;
	mqttMessage->message = new uint8_t[totalMessageLength];
	mqttMessage->messageLength = totalMessageLength;
	uint8_t *ptr = mqttMessage->message;
	WriteChar(&ptr, header.byte);
	WriteChar(&ptr, 2);
	WriteShort(&ptr, packetIdentifier);
	return mqttMessage;
}

std::unique_ptr<MQTTMessage>MQTTMessage::MQTTMessagePubComp(uint16_t packetIdentifier)
{
	std::unique_ptr<MQTTMessage> mqttMessage(new MQTTMessage());
	MessageHeader header;
	header.byte = 0;
	header.bits.type = MQTT_MSG_PUBCOMP;
	uint32_t totalMessageLength = 1 /*header*/ + 1 /*remaining length*/ + 2 /*packet identifier*/;
	mqttMessage->message = new uint8_t[totalMessageLength];
	mqttMessage->messageLength = totalMessageLength;
	uint8_t *ptr = mqttMessage->message;
	WriteChar(&ptr, header.byte);
	WriteChar(&ptr, 2);
	WriteShort(&ptr, packetIdentifier);
	return mqttMessage;
}

std::unique_ptr<MQTTMessage> MQTTMessage::MQTTMessageSubscribe(std::string topicName, uint8_t qos)
{
	std::unique_ptr<MQTTMessage> mqttMessage(new MQTTMessage());
	MessageHeader header;

	header.byte = 0;
	header.bits.type = MQTT_MSG_SUBSCRIBE;
	uint32_t remainingLength =  2 /*package identifier*/ + topicName.size() + 2 /*topic name*/ + 1 /*qos*/;
	uint8_t remainingLenghtBytes[4];
	uint8_t length = CalculateRemainingLengthBytes(remainingLenghtBytes, remainingLength);
	uint32_t totalMessageLength = remainingLength + length + 1 /*header*/;
	mqttMessage->message = new uint8_t[totalMessageLength];
	mqttMessage->messageLength = totalMessageLength;
	uint8_t *ptr = mqttMessage->message;
	WriteChar(&ptr, header.byte);
	for (uint8_t i = 0; i < length; ++i)
	{
		WriteChar(&ptr, remainingLenghtBytes[i]);
	}
	++packetIdentifier;
	if (packetIdentifier == 0)
	{
		packetIdentifier = 1;
	}
	WriteShort(&ptr, packetIdentifier);
	WriteUTF(&ptr, topicName);
	WriteChar(&ptr, qos);
	return mqttMessage;
}

std::unique_ptr<MQTTMessage> MQTTMessage::MQTTMessageUnsubscribe(std::string topicName)
{
	std::unique_ptr<MQTTMessage> mqttMessage(new MQTTMessage());
	MessageHeader header;

	header.byte = 0;
	header.bits.type = MQTT_MSG_UNSUBSCRIBE;
	uint32_t remainingLength = 2 /*package identifier*/ + topicName.size() + 2 /*topic name*/;
	uint8_t remainingLenghtBytes[4];
	uint8_t length = CalculateRemainingLengthBytes(remainingLenghtBytes, remainingLength);
	uint32_t totalMessageLength = remainingLength + length + 1 /*header*/;
	mqttMessage->message = new uint8_t[totalMessageLength];
	mqttMessage->messageLength = totalMessageLength;
	uint8_t *ptr = mqttMessage->message;
	WriteChar(&ptr, header.byte);
	for (uint8_t i = 0; i < length; ++i)
	{
		WriteChar(&ptr, remainingLenghtBytes[i]);
	}
	++packetIdentifier;
	if (packetIdentifier == 0)
	{
		packetIdentifier = 1;
	}
	WriteShort(&ptr, packetIdentifier);
	WriteUTF(&ptr, topicName);
	return mqttMessage;
}

std::unique_ptr<MQTTMessage> MQTTMessage::MQTTMessagePingReq()
{
	std::unique_ptr<MQTTMessage> mqttMessage(new MQTTMessage());
	MessageHeader header;
	header.byte = 0;
	header.bits.type = MQTT_MSG_PINGREQ;
	uint32_t totalMessageLength = 1 /*header*/ + 1 /*remaining length*/;
	mqttMessage->message = new uint8_t[totalMessageLength];
	mqttMessage->messageLength = totalMessageLength;
	uint8_t *ptr = mqttMessage->message;
	WriteChar(&ptr, header.byte);
	WriteChar(&ptr, 0);
	return mqttMessage;
}

std::unique_ptr<MQTTMessage> MQTTMessage::MQTTMessagePingResp()
{
	std::unique_ptr<MQTTMessage> mqttMessage(new MQTTMessage());
	MessageHeader header;
	header.byte = 0;
	header.bits.type = MQTT_MSG_PINGRESP;
	uint32_t totalMessageLength = 1 /*header*/ + 1 /*remaining length*/;
	mqttMessage->message = new uint8_t[totalMessageLength];
	mqttMessage->messageLength = totalMessageLength;
	uint8_t *ptr = mqttMessage->message;
	WriteChar(&ptr, header.byte);
	WriteChar(&ptr, 0);
	return mqttMessage;
}

uint8_t MQTTMessage::CalculateRemainingLengthBytes(uint8_t* buffer, uint32_t length)
{
	uint8_t count = 0;
	do
	{
		uint8_t temp = length % 128;
		length /= 128;
		// if there are more digits to encode, set the top bit of this digit 
		if (length > 0)
		{
			temp |= 0x80;
		}
		buffer[count++] = temp;
	} while (length > 0);
	return count;
}