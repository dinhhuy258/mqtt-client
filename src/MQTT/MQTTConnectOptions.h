#ifndef _MQTT_CONNECT_OPTIONS_H_
#define _MQTT_CONNECT_OPTIONS_H_
#include <stdint.h>
#include <string>

class MQTTConnectOptions
{
	friend class MQTTMessage;
	public:
		MQTTConnectOptions();
		void SetCleanSession(bool cleanSession);
		void SetKeepAlive(uint16_t keepAlive);
		void SetUsername(std::string username);
		void SetPassword(std::string password);
		void SetLWT(std::string lastWillTopic, std::string lastWillMessage, uint8_t lastWillQos, bool lastWillRetain);

		uint16_t GetKeepAlive();
	private:
		std::string username;
		std::string password;
		bool cleanSession;
		uint16_t keepAlive;
		std::string lastWillTopic;
		std::string lastWillMessage;
		bool lastWillRetain;
		uint8_t lastWillQos;
};

#endif //_MQTT_CONNECT_OPTIONS_H_