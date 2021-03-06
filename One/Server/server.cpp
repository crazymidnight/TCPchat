#undef UNICODE
/// Ускорение процесса сборки и исключение ненужных
/// библиотек из проекта
#define WIN32_LEAN_AND_MEAN

/// Максиальное количество записываемых символов в буфер
#define DEFAULT_BUFLEN 512
/// Выбранный порт сервера, по которому идёт подключение
#define DEFAULT_PORT "27015"

/// Подключение библиотеки ws2tcpip.h
#include <winsock2.h>
/// Подключение к серверу
#include <ws2tcpip.h>
/// Библиотека для работы с потоками
#include <process.h>
/// Стандартные потоки ввода-вывода
#include <iostream>
/// Библеотека для работы с векторами (способом хранения данных)
#include <vector>
/// stringstream для формирования сообщения на отправку клиентам
#include <sstream>

/// Компоновка
#pragma comment (lib, "Ws2_32.lib")

/// Хранение информации о всех подключенных соединениях
std::vector<SOCKET> client_socket_vector = { INVALID_SOCKET };

/// Отдельный поток для отправки сообщения всем клиентам
/// Первый параметр: имя пользователя, отправившего сообщение
/// Второй парметр: сообщение
unsigned __stdcall clientSendData(std::string user_name, std::string data)
{
	/// Поток для формирование сообщения на отправку
	std::stringstream send_buffer;
	/// Флаг ошибки
	int last_error;
	/// Размер буфера
	const int buffer_length = DEFAULT_BUFLEN;

	/// Очистка буфера на отправку
	send_buffer.str("");
	/// Формирование сообщения на отправку
	send_buffer << user_name.c_str() << ": " << data;
	/// Цикл для отправки сообщения каждому клиенту
	for (int i = 0; i < client_socket_vector.size(); i++)
	{
		/// Отправка сообщения
		last_error = send(client_socket_vector[i], send_buffer.str().c_str(), buffer_length, 0);
	}

	return 0;
}

/// Отдельный поток для каждого подключенного клиента 
/// для приема сообщений
unsigned __stdcall clientReceiveData(void *data)
{
	/// Данные о подключении
	SOCKET client_socket = (SOCKET)data;
	/// Имя пользователя
	std::string user_name;
	/// Объявление буфера для приема сообщений
	char receive_buffer[DEFAULT_BUFLEN] = { 0 };
	/// Флаг ошибки
	int last_error;
	/// Размер буфера
	const int buffer_length = DEFAULT_BUFLEN;

	/// Прием имя пользователя
	last_error = recv(client_socket, receive_buffer, buffer_length, 0);
	if (last_error > 0)
	{
		std::cout << "User " << receive_buffer << " connected" << std::endl;
		user_name = receive_buffer;
	}

	/// Бесконечный цикл для приема сообщений
	while (true)
	{
		/// Прием сообщения
		last_error = recv(client_socket, receive_buffer, buffer_length, 0);
		/// Если есть данные
		if (last_error > 0)
		{
			/// Рассылка сообщения всем подключенным клиентам
			clientSendData(user_name, receive_buffer);
			/// Очистка буфера
			memset(receive_buffer, 0, sizeof receive_buffer);
		}
	}

	return 0;
}

/// Главная программа
int __cdecl main(void)
{
	/// Хранение отладочной информации
	WSADATA wsaData;
	/// Флаг ошибки
	int last_error;
	
	/// Сокет сервера, по которому идут подключения
	SOCKET listen_socket = INVALID_SOCKET;
	/// Сокет нового подключения
	SOCKET client_socket = INVALID_SOCKET;

	/// Указатель на сокет сервера
	struct addrinfo *server_address_pointer = NULL;
	struct addrinfo server_info;

	/// Инициализация сокета Windows
	last_error = WSAStartup(MAKEWORD(2, 2), &wsaData);
	/// Обработка ошибок
	if (last_error != 0) 
	{
		std::cout << "WSAStartup failed with error: " << last_error << std::endl;
		return 1;
	}

	/// Очистка памяти
	ZeroMemory(&server_info, sizeof(server_info));
	/// Установка параметров соединения
	server_info.ai_family = AF_INET;
	server_info.ai_socktype = SOCK_STREAM;
	server_info.ai_protocol = IPPROTO_TCP;
	server_info.ai_flags = AI_PASSIVE;

	/// Определение адреса сервера
	last_error = getaddrinfo(NULL, DEFAULT_PORT, &server_info, &server_address_pointer);
	/// Обработка ошибок
	if (last_error != 0) 
	{
		std::cout << "getaddrinfo failed with error: " << last_error << std::endl;
		WSACleanup();
		return 1;
	}

	/// Создание реального сокета для подключения к серверу
	listen_socket = socket(server_address_pointer->ai_family, server_address_pointer->ai_socktype, server_address_pointer->ai_protocol);
	/// Обработка ошибок
	if (listen_socket == INVALID_SOCKET) 
	{
		std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
		freeaddrinfo(server_address_pointer);
		WSACleanup();
		return 1;
	}

	/// Настройка TCP слушающего входящие подключения сокета
	last_error = bind(listen_socket, server_address_pointer->ai_addr, (int)server_address_pointer->ai_addrlen);
	/// Обработка ошибок
	if (last_error == SOCKET_ERROR) 
	{
		freeaddrinfo(server_address_pointer);
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}
	
	/// Освобождение неиспользуемой памяти
	freeaddrinfo(server_address_pointer);

	/// Прослушивание сокета
	last_error = listen(listen_socket, SOMAXCONN);
	/// Обработка ошибок
	if (last_error == SOCKET_ERROR) 
	{
		std::cout << "listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listen_socket);
		WSACleanup();
		return 1;
	}

	/// Ожидание подключения нового клиента
	while ((client_socket = accept(listen_socket, NULL, NULL))) 
	{
		/// Обработка ошибок
		if (client_socket == INVALID_SOCKET)
		{
			std::cout << "accept failed with error: " << WSAGetLastError() << std::endl;
			closesocket(listen_socket);
			WSACleanup();
			return 1;
		}
		/// Сохранение информации о подключенном клиенте
		client_socket_vector.push_back(client_socket);
		/// Создание отдельного потока для приема сообщения для нового подключения
		unsigned threadID;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &clientReceiveData, (void*)client_socket, 0, &threadID);
	}

	// Закрытие соединения
	closesocket(listen_socket);
	WSACleanup();
	
	return 0;
}