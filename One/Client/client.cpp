#define WIN32_LEAN_AND_MEAN

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstdlib>
#include <cstdio>
#include <iostream>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

// First parameter - server name or IP (localhost or 192.168.1.40)
// Second parameter - user name (User or Anonymous)



int __cdecl main(int argc, char **argv)
{
	WSADATA wsaData;
	SOCKET connect_socket = INVALID_SOCKET;
	struct addrinfo *client_address_pointer = NULL,
		*ptr = NULL,
		client_info;
	const char *send_buffer = "this is a test";
	const char *user_name = argv[2];
	
	int last_error;

	char receive_buffer[DEFAULT_BUFLEN] = { 0 };
	int receive_buffer_length = DEFAULT_BUFLEN;

	// Validate the parameters
	if (argc != 3) {
		std::cout << "Usage: " << argv[0] << "server-name user-name" << std::endl;
		return 1;
	}

	// Initialize Winsock
	last_error = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (last_error != 0) 
	{
		std::cout << "WSAStartup failed with error: " << last_error << std::endl;
		return 1;
	}

	ZeroMemory(&client_info, sizeof(client_info));
	client_info.ai_family = AF_UNSPEC;
	client_info.ai_socktype = SOCK_STREAM;
	client_info.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	last_error = getaddrinfo(argv[1], DEFAULT_PORT, &client_info, &client_address_pointer);
	if (last_error != 0) 
	{
		std::cout << "getaddrinfo failed with error: " << last_error << std::endl;
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = client_address_pointer; ptr != NULL; ptr = ptr->ai_next) 
	{

		// Create a SOCKET for connecting to server
		connect_socket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (connect_socket == INVALID_SOCKET) {
			std::cout << "Create socket failed with error: " << WSAGetLastError() << std::endl;
			WSACleanup();
			return 1;
		}

		// Connect to server.
		last_error = connect(connect_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (last_error == SOCKET_ERROR) {
			closesocket(connect_socket);
			connect_socket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(client_address_pointer);

	if (connect_socket == INVALID_SOCKET)
	{
		std::cout << "Unable to connect to server!" << std::endl;
		WSACleanup();
		return 1;
	}

	/// USER CODE BEGIN

	// Send user name
	last_error = send(connect_socket, user_name, (int)strlen(user_name), 0);

	if (last_error == SOCKET_ERROR) 
	{
		std::cout << "send failed with error: " << WSAGetLastError() << std::endl;
		closesocket(connect_socket);
		WSACleanup();
		return 1;
	}

	std::cout << "Bytes Sent: " << last_error << std::endl;
	last_error = send(connect_socket, send_buffer, (int)strlen(send_buffer), 0);
	// shutdown the connection since no more data will be sent
	last_error = shutdown(connect_socket, SD_SEND);
	if (last_error == SOCKET_ERROR) 
	{
		std::cout << "Shutdown failed with error: " << WSAGetLastError() << std::endl;
		closesocket(connect_socket);
		WSACleanup();
		return 1;
	}

	last_error = 1;

	// Receive until the peer closes the connection
	while (last_error > 0) 
	{

		last_error = recv(connect_socket, receive_buffer, receive_buffer_length, 0);
		if (last_error > 0)
		{
			std::cout << "Text received: " << receive_buffer << std::endl;
		}
		else if (last_error == 0)
		{
			std::cout << "Connection closed" << std::endl;
		}
			
		else
		{
			std::cout << "recv failed with error: " << WSAGetLastError() << std::endl;
		}
	}

	/// USER CODE END

	system("PAUSE");
	
	// cleanup
	closesocket(connect_socket);
	WSACleanup();

	
	return 0;
}