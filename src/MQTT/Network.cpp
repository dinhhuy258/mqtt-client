#include "Network.h"
#include <iostream>
#include "Utils.h"

Network::Network() : connectedCallback(nullptr), disconnectedCallback(nullptr), receivedCallback(nullptr), sentCallback(nullptr), socket(nullptr)
{
}

void Network::Connect(std::string host, uint32_t port, bool security)
{
	if (security)
	{
		socket = make_unique<SSLSocket>();
	}
	else
	{
		socket = make_unique<TCPSocket>();
	}
	socket->Initialize();
	socket->Connect(host, port, std::bind(&Network::ConnectHandler, this, std::placeholders::_1));
}

void Network::Disconnect()
{
	socket->Close();
	if (disconnectedCallback)
	{
		disconnectedCallback();
	}
}

void Network::WriteData(uint8_t *data, std::size_t dataLength)
{
	socket->WriteData(data, dataLength, std::bind(&Network::WriteHandler, this, std::placeholders::_1, std::placeholders::_2));
}

void Network::RegisterConnectedCallback(std::function<void()> connectedCallback)
{
	this->connectedCallback = connectedCallback;
}

void Network::RegisterDisconnectedCallback(std::function<void()> disconnectedCallback)
{
	this->disconnectedCallback = disconnectedCallback;
}

void Network::RegisterReceivedCallback(std::function<void(uint8_t*, std::size_t)> receivedCallback)
{
	this->receivedCallback = receivedCallback;
}

void Network::RegisterSentCallback(std::function<void(std::size_t)> sentCallback)
{
	this->sentCallback = sentCallback;
}

void Network::ConnectHandler(bool error)
{
	if (!error)
	{
		if (connectedCallback)
		{
			connectedCallback();
		}
		bufferIndex = 0;
		readDone = false;
		socket->ReadData(readBuffer, 1, std::bind(&Network::ReadHandler, this, std::placeholders::_1, std::placeholders::_2));
	}
	else
	{
		LOGI("Connect error");
		Disconnect();
	}
}

void Network::WriteHandler(bool error, std::size_t bytesTransferred)
{
	if (!error)
	{
		if (sentCallback)
		{
			sentCallback(bytesTransferred);
		}
	}
	else
	{
		LOGI("Write data error");
		Disconnect();
	}
}

void Network::ReadHandler(bool error, std::size_t bytesTransferred)
{
	if (!error)
	{
		if (readDone)
		{
			// Read variable header and payload done. Send it to receivedCallback
			for (std::size_t i = 0; i < bytesTransferred; ++i)
			{
				buffer[bufferIndex++] = readBuffer[i];
			}
			if (receivedCallback)
			{
				receivedCallback(buffer, bufferIndex);
			}
			bufferIndex = 0;
			readDone = false;
			socket->ReadData(readBuffer, 1, std::bind(&Network::ReadHandler, this, std::placeholders::_1, std::placeholders::_2));
		}
		else if (!bufferIndex)
		{
			buffer[bufferIndex++] = readBuffer[0]; 
			socket->ReadData(readBuffer, 1, std::bind(&Network::ReadHandler, this, std::placeholders::_1, std::placeholders::_2));
		}
		else
		{
			buffer[bufferIndex++] = readBuffer[0];
			if (readBuffer[0] == 0)
			{
				//Some MQTT message has no variable header and payload
				if (receivedCallback)
				{
					receivedCallback(buffer, bufferIndex);
				}
				bufferIndex = 0;
				socket->ReadData(readBuffer, 1, std::bind(&Network::ReadHandler, this, std::placeholders::_1, std::placeholders::_2));
			}
			else if ((readBuffer[0] & 0x80) != 0x80)
			{
				//Reading reamaining length bytes done
				readDone = true;
				uint32_t multiplier = 1;
				uint32_t remainingLength = 0;
				for (uint8_t i = 1; i < bufferIndex; ++i)
				{
					remainingLength += (buffer[i] & 127) * multiplier;
					multiplier *= 128;
				}
				//Try to read variable header and payload
				socket->ReadData(readBuffer, remainingLength, std::bind(&Network::ReadHandler, this, std::placeholders::_1, std::placeholders::_2));
			}
			else
			{
				//Continue read remaining length bytes
				socket->ReadData(readBuffer, 1, std::bind(&Network::ReadHandler, this, std::placeholders::_1, std::placeholders::_2));
			}
		}
	}
	else
	{		
		LOGI("Read data error %d", bytesTransferred);
		Disconnect();
	}
}
