#ifndef _UTILS_H_
#define _UTILS_H_
#include <stdint.h>
#include <string>
#include <memory>
#define MQTT_DEBUG

#if defined(WIN32) || defined(WIN64)
#define SOCKET_START do { WORD winsockVersion = 0x0202; /*version 2.2*/ WSADATA wsd; WSAStartup(winsockVersion, &wsd); }while(0);
#define SOCKET_END do { WSACleanup(); }while(0);
#else
#define START
#define END
#endif

#if defined(MQTT_DEBUG)
#define LOGI(...) do { printf("MQTT: "); printf(__VA_ARGS__); printf("\n");} while(0);
#else
#define LOGI(...) 
#endif

//std::make_unique isn't in C++11 It joined	the Standar Library as C++14
//You may wonder why we can do that. std::unique_ptr does not allow copy constructor, operator assignment but how can we return it.
//Because return local_obj; automatically treats it as an rvalue
template<typename T, typename... Ts>
std::unique_ptr<T> make_unique(Ts&&... params)
{
	return std::unique_ptr<T>(new T(std::forward<Ts>(params)...));
}

void WriteShort(uint8_t **buffer, uint16_t data);
void WriteChar(uint8_t **buffer, uint8_t data);
void WriteUTF(uint8_t** pptr, std::string string);

#endif //_UTILS_H_