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

#pragma comment (lib, "Ws2_32.lib")
// #pragma comment (lib, "Mswsock.lib")

unsigned __stdcall ClientSession(void *data)
{
	

	return 0;
}

std::vector<SOCKET> client_socket_vector = { INVALID_SOCKET };

int __cdecl main(void)
{
	WSADATA wsaData;

	int last_error;

	SOCKET listen_socket = INVALID_SOCKET;
	SOCKET client_socket = INVALID_SOCKET;

	struct addrinfo *server_address_pointer = NULL;
	struct addrinfo server_info;
	
	char receive_buffer[DEFAULT_BUFLEN] = { 0 };
	int receive_buffer_length = DEFAULT_BUFLEN;

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

	// Accept a client socket
	client_socket = accept(listen_socket, NULL, NULL);
	if (client_socket == INVALID_SOCKET) 
	{
		std::cout << "accept failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}
	
	last_error = 1;
	last_error = recv(client_socket, receive_buffer, receive_buffer_length, 0);
	if (last_error > 0)
	{
		std::cout << "Text received: " << receive_buffer << std::endl;

		// Echo the buffer back to the sender
		last_error = send(client_socket, receive_buffer, last_error, 0);
		if (last_error == SOCKET_ERROR)
		{
			std::cout << "getaddrinfo failed with error: " << last_error << std::endl;
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(client_socket);
			WSACleanup();
			return 1;
		}
	}

	last_error = 1;

	// Receive until the peer shuts down the connection
	while (true)
	{
		last_error = recv(client_socket, receive_buffer, receive_buffer_length, 0);
		if (last_error > 0) 
		{
			std::cout << "Text received: " << receive_buffer << std::endl;

			// Echo the buffer back to the sender
			last_error = send(client_socket, receive_buffer, last_error, 0);
			if (last_error == SOCKET_ERROR) 
			{
				std::cout << "getaddrinfo failed with error: " << last_error << std::endl;
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(client_socket);
				WSACleanup();
				return 1;
			}
		}
		else if (last_error == 0)
		{
			std::cout << "getaddrinfo failed with error: " << last_error << std::endl;
			printf("Connection closing...\n");
		}
		else 
		{
			std::cout << "getaddrinfo failed with error: " << last_error << std::endl;
			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(client_socket);
			WSACleanup();
			return 1;
		}
	}

	// Shutdown the connection since we're done
	last_error = shutdown(client_socket, SD_SEND);
	if (last_error == SOCKET_ERROR) {
		std::cout << "getaddrinfo failed with error: " << last_error << std::endl;
		printf("Shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(client_socket);
		WSACleanup();
		return 1;
	}

	system("PAUSE");

	// Cleanup
	closesocket(client_socket);
	WSACleanup();

	
	return 0;
}