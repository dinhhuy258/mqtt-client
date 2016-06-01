#include "MQTTConnectOptions.h"
#include "MQTTConfig.h"

MQTTConnectOptions::MQTTConnectOptions()
{
	this->username = std::string();
	this->password = std::string();
	this->cleanSession = false;
	this->keepAlive = MQTT_KEEP_ALIVE;
	this->lastWillTopic = std::string();
	this->lastWillMessage = std::string();
	this->lastWillQos = 0;
	this->lastWillRetain = false;
}

void MQTTConnectOptions::SetCleanSession(bool cleanSession)
{
	this->cleanSession = cleanSession;
}

void MQTTConnectOptions::SetKeepAlive(uint16_t keepAlive)
{
	this->keepAlive = keepAlive;
}

void MQTTConnectOptions::SetUsername(std::string username)
{
	this->username = username;
}

void MQTTConnectOptions::SetPassword(std::string password)
{
	this->password = password;
}

void MQTTConnectOptions::SetLWT(std::string lastWillTopic, std::string lastWillMessage, uint8_t lastWillQos, bool lastWillRetain)
{
	this->lastWillTopic = lastWillTopic;
	this->lastWillMessage = lastWillMessage;
	this->lastWillQos = lastWillQos;
	this->lastWillRetain = lastWillRetain;
}

uint16_t MQTTConnectOptions::GetKeepAlive()
{
	return keepAlive;
}