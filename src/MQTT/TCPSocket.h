#ifndef _TCP_SOCKET_H_
#define _TCP_SOCKET_H_
#include "Socket.h"

class TCPSocket : public Socket
{
	public:
		TCPSocket() = default;
		~TCPSocket() = default;
		bool Initialize() override { return true; };
		void Connect(std::string host, uint32_t port, std::function<void(bool)> connectedCallback) override;
	    void WriteData(uint8_t *data, std::size_t dataLength, std::function<void(bool, std::size_t)> sentCallback) override;
		void ReadData(uint8_t *buffer, std::size_t bytes, std::function<void(bool, std::size_t)> receivedCallback) override;
};

#endif //_TCP_SOCKET_H_