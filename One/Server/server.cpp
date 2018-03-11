#undef UNICODE
#define WIN32_LEAN_AND_MEAN

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <process.h>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iomanip>

#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

std::vector<SOCKET> client_socket_vector = { INVALID_SOCKET };
std::vector < std::string > user_name_vector = { "" };

unsigned __stdcall clientSendData(std::string user_name, std::string data)
{
	std::stringstream send_buffer;
	int last_error;
	const int buffer_length = DEFAULT_BUFLEN;

	// Empty buffer
	send_buffer.str("");
	send_buffer << user_name.c_str() << ": " << data;
	for (int i = 0; i < client_socket_vector.size(); i++)
	{
		last_error = send(client_socket_vector[i], send_buffer.str().c_str(), buffer_length, 0);
	}

	return 0;
}

unsigned __stdcall clientReceiveData(void *data)
{
	SOCKET client_socket = (SOCKET)data;
	std::string user_name;
	char receive_buffer[DEFAULT_BUFLEN] = { 0 };
	int last_error;
	const int buffer_length = DEFAULT_BUFLEN;

	last_error = 1;
	last_error = recv(client_socket, receive_buffer, buffer_length, 0);
	if (last_error > 0)
	{
		std::cout << "User " << receive_buffer << " connected" << std::endl;
		user_name = receive_buffer;
	}

	while (true)
	{
		// Empty buffer
		memset(receive_buffer, 0, sizeof receive_buffer);
		last_error = recv(client_socket, receive_buffer, buffer_length, 0);
		if (last_error > 0)
		{
			clientSendData(user_name, receive_buffer);
		}
	}

	return 0;
}

int __cdecl main(void)
{
	WSADATA wsaData;

	int last_error;

	SOCKET listen_socket = INVALID_SOCKET;

	SOCKET client_socket = INVALID_SOCKET;
	std::string user_name;

	struct addrinfo *server_address_pointer = NULL;
	struct addrinfo server_info;
	
	char receive_buffer[DEFAULT_BUFLEN] = { 0 };
	std::stringstream send_buffer;
	
	const int buffer_length = DEFAULT_BUFLEN;

	// Initialize Winsock
	last_error = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (last_error != 0) 
	{
		std::cout << "WSAStartup failed with error: " << last_error << std::endl;
		return 1;
	}

	ZeroMemory(&server_info, sizeof(server_info));
	server_info.ai_family = AF_INET;
	server_info.ai_socktype = SOCK_STREAM;
	server_info.ai_protocol = IPPROTO_TCP;
	server_info.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	last_error = getaddrinfo(NULL, DEFAULT_PORT, &server_info, &server_address_pointer);
	if (last_error != 0) 
	{
		std::cout << "getaddrinfo failed with error: " << last_error << std::endl;
		WSACleanup();
		return 1;
	}

	// Create a SOCKET for connecting to server
	listen_socket = socket(server_address_pointer->ai_family, server_address_pointer->ai_socktype, server_address_pointer->ai_protocol);
	if (listen_socket == INVALID_SOCKET) 
	{
		std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
		freeaddrinfo(server_address_pointer);
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket
	last_error = bind(listen_socket, server_address_pointer->ai_addr, (int)server_address_pointer->ai_addrlen);
	if (last_error == SOCKET_ERROR) 
	{
		freeaddrinfo(server_address_pointer);
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(server_address_pointer);

	last_error = listen(listen_socket, SOMAXCONN);
	if (last_error == SOCKET_ERROR) 
	{
		std::cout << "listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	while ((client_socket = accept(listen_socket, NULL, NULL))) 
	{
		if (client_socket == INVALID_SOCKET)
		{
			std::cout << "accept failed with error: " << WSAGetLastError() << std::endl;
			closesocket(listen_socket);
			WSACleanup();
			return 1;
		}
		client_socket_vector.push_back(client_socket);
		// Create a new thread for the accepted client (also pass the accepted client socket).
		unsigned threadID;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &clientReceiveData, (void*)client_socket, 0, &threadID);
	}

	system("PAUSE");

	// Cleanup
	closesocket(client_socket);
	WSACleanup();
	
	return 0;
}