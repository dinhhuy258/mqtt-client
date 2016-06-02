#ifndef _MQTT_MESSAGE_H_
#define _MQTT_MESSAGE_H_
#include <stdint.h>
#include <memory>
#include <string.h>
#include "MQTTConfig.h"
#include "MQTTConnectOptions.h"

#if defined(MQTT_VERSION_311)
	#define PROTOCOL_NAME "MQTT"
	#define PROTOCOL_LEVEL 0x04
#elif defined(MQTT_VERSION_31)
	#define PROTOCOL_NAME "MQIsdp"
	#define PROTOCOL_LEVEL 0x03
#endif

enum MQTTMessageType
{
	MQTT_MSG_CONNECT = 0x01,
	MQTT_MSG_CONNACK,
	MQTT_MSG_PUBLISH,
	MQTT_MSG_PUBACK,
	MQTT_MSG_PUBREC,
	MQTT_MSG_PUBREL,
	MQTT_MSG_PUBCOMP,
	MQTT_MSG_SUBSCRIBE,
	MQTT_MSG_SUBACK,
	MQTT_MSG_UNSUBSCRIBE,
	MQTT_MSG_UNSUBACK,
	MQTT_MSG_PINGREQ,
	MQTT_MSG_PINGRESP,
	MQTT_MSG_DISCONNECT
};

enum MQTTConnectReturnCode
{
	MQTT_CONNECTION_ACCEPTED = 0x00,
	MQTT_CONNECTION_UNACCEPTABLE_PROTOCOL_VERSION,
	MQTT_CONNECTION_IDENTIFIER_REJECTED,
	MQTT_CONNECTION_SERVER_UNAVAILABLE,
	MQTT_CONNECTION_BAD_USERNAME_OR_PASSWORD,
	MQTT_CONNECTION_NOT_AUTHORIZED
};

enum MQTTSubscribeReturnCode
{
	MQTT_SUBSCRIBE_QOS0		= 0x00,
	MQTT_SUBSCRIBE_QOS1		= 0x01,
	MQTT_SUBSCRIBE_QOS2		= 0x02,
	MQTT_SUBSCRIBE_FAILURE	= 0x80,
};

typedef union 
{
	uint8_t byte;
	struct
	{
#if __BYTE_ORDER == __LITTLE_ENDIAN
		uint8_t retain  : 1;
		uint8_t qos		: 2;
		uint8_t dup		: 1;
		uint8_t type	: 4;
#elif __BYTE_ORDER == __BIG_ENDIAN
		uint8_t type	: 4;
		uint8_t dup		: 1;
		uint8_t qos		: 2;
		uint8_t retain	: 1;
#endif  
	}bits;
}MessageHeader;

typedef union
{
	uint8_t byte;
	struct
	{
#if __BYTE_ORDER == __LITTLE_ENDIAN
		uint8_t reserved : 1;
		uint8_t cleanSession : 1;
		uint8_t lastWillFlag : 1;
		uint8_t lastWillQos : 2;
		uint8_t lastWillRetain : 1;
		uint8_t password : 1;
		uint8_t username : 1;
#elif __BYTE_ORDER == __BIG_ENDIAN
		uint8_t username : 1;
		uint8_t password : 1;
		uint8_t lastWillRetain : 1;
		uint8_t lastWillQos : 2;
		uint8_t lastWillFlag : 1;
		uint8_t cleanSession : 1;
		uint8_t reserved : 1;
#endif  
	}bits;
}ConnectFlags;

class MQTTMessage
{
	public:
		inline static MQTTMessageType GetMessageType(uint8_t* data) { return static_cast<MQTTMessageType>(data[0] >> 4); }
		inline static MQTTConnectReturnCode GetConnectReturnCode(uint8_t* data) { return static_cast<MQTTConnectReturnCode>(data[3]); }
		inline static MQTTSubscribeReturnCode GetSubscribeReturnCode(uint8_t* data) { return static_cast<MQTTSubscribeReturnCode>(data[4]); }
		inline static uint8_t GetPublishQos(uint8_t* data) { return (data[0] >> 1) & 0x03; }
		inline static uint16_t GetPacketIdentifier(uint8_t* data)
		{
			uint8_t index = 2;
			if (MQTTMessage::GetMessageType(data) == MQTT_MSG_PUBLISH)
			{
				index = 1;
				while ((data[index++] & 0x80) == 0x80);
				uint16_t topicLength = data[index++];
				topicLength <<= 8;
				topicLength |= data[index++];
				index += topicLength;
			}
			uint16_t packetIdentifier = data[index];
			packetIdentifier <<= 8;
			packetIdentifier |= data[index + 1];
			return packetIdentifier;
		}
		inline static std::string GetPublishTopicName(uint8_t* data)
		{
			uint32_t index = 1; //Remainning length byte start at byte 1
			while ((data[index++] & 0x80) == 0x80);
			uint16_t topicLength = 0;
			topicLength = data[index++];
			topicLength <<= 8;
			topicLength |= data[index++];
			char topicNameArray[MQTT_MAX_MESSAGE_LENGTH];
#if defined(WIN32) || defined(WIN64)
			memcpy_s(topicNameArray, MQTT_MAX_MESSAGE_LENGTH, &data[index], topicLength);
#else
			memcpy(topicNameArray, &data[index], topicLength);
#endif
			std::string topicName(topicNameArray, topicLength);
			return topicName;
		}
		inline static std::string GetPublishPayload(uint8_t* data)
		{
			uint32_t index = 1; //Remainning length byte start at byte 1
			while ((data[index++] & 0x80) == 0x80);
			uint16_t topicLength = 0;
			topicLength = data[index++];
			topicLength <<= 8;
			topicLength |= data[index++];
			index += topicLength;
			if (GetPublishQos(data) != 0)
			{
				index += 2; /*Package Identifier*/
			}
			uint16_t payloadLength = 0;
			payloadLength = data[index++];
			payloadLength <<= 8;
			payloadLength |= data[index++];
			char payloadArray[MQTT_MAX_MESSAGE_LENGTH];
#if defined(WIN32) || defined(WIN64)
			memcpy_s(payloadArray, MQTT_MAX_MESSAGE_LENGTH, &data[index], payloadLength);
#else
			memcpy(payloadArray, &data[index], payloadLength);
#endif
			std::string payload(payloadArray, payloadLength);	
			return payload;
		}
		static std::unique_ptr<MQTTMessage> MQTTMessageConnect(std::string clientID, MQTTConnectOptions mqttConnectOptions);
		static std::unique_ptr<MQTTMessage> MQTTMessagePublish(std::string topicName, std::string payload, bool dup, uint8_t qos, bool retain);
		static std::unique_ptr<MQTTMessage> MQTTMessagePubAck(uint16_t packetIdentifier);
		static std::unique_ptr<MQTTMessage> MQTTMessagePubRec(uint16_t packetIdentifier);
		static std::unique_ptr<MQTTMessage> MQTTMessagePubRel(uint16_t packetIdentifier);
		static std::unique_ptr<MQTTMessage> MQTTMessagePubComp(uint16_t packetIdentifier);
		static std::unique_ptr<MQTTMessage> MQTTMessageSubscribe(std::string topicName, uint8_t qos);
		static std::unique_ptr<MQTTMessage> MQTTMessageUnsubscribe(std::string topicName);
		static std::unique_ptr<MQTTMessage> MQTTMessagePingReq();
		static std::unique_ptr<MQTTMessage> MQTTMessagePingResp();
		~MQTTMessage();
		inline uint8_t *GetMessageData() { return message; }
		inline std::size_t GetMessageLength() { return messageLength; }
	private:
		MQTTMessage();
		static uint8_t CalculateRemainingLengthBytes(uint8_t* buffer, uint32_t length);
	private:
		uint8_t *message;
		std::size_t messageLength;
		static uint16_t packetIdentifier;
};
#endif //_MQTT_MESSAGE_H_
