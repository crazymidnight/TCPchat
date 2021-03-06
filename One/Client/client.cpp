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
/// getline() для ввода строки
#include <sstream>

/// Компоновка
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

/// Пример запуска программы: C:\Program Files\Client.exe localhost User
/// Далее набираются сообщения, отправляются клавишей Enter
/// Первый параметр - имя сервера или его IP адрес (localhost or 192.168.1.40)
/// Второй параметр - имя пользователя (User or Anonymous)



/// Отдельный поток для отправки данных
unsigned __stdcall sendData(void* data)
{
	/// Буфер отправки
	std::string send_buffer;
	/// Флаг ошибки
	int last_error;
	/// Сокет для отправки данных
	SOCKET connect_socket = (SOCKET)data;

	/// Бесконечный цикл для отправки сообщений
	while (true)
	{
		/// Очистка буфера отправки
		send_buffer.clear();
		/// Ожидание ввода сообщения пользователем
		std::getline(std::cin, send_buffer);
		/// Отправка сообщения
		last_error = send(connect_socket, send_buffer.c_str(), send_buffer.length(), 0);
		/// Обработка ошибок
		if (last_error == SOCKET_ERROR)
		{
			std::cout << "Shutdown failed with error: " << WSAGetLastError() << std::endl;
			closesocket(connect_socket);
			WSACleanup();
			return 1;
		}
		if (last_error < 0)
		{
			_endthread();
		}
	}

	return 0;
}

/// Главная программа
int __cdecl main(int argc, char **argv)
{
	SOCKET connect_socket = INVALID_SOCKET;

	/// Хранение отладочной информации
	WSADATA wsaData;
	/// Сокет сервера
	/// Указатель на сокет клиента
	struct addrinfo *client_address_pointer = NULL,
		*ptr = NULL,
		client_info;
	/// Строка отправки
	std::string send_buffer;
	/// Имя пользователя получаемой из командой строки при запуске программы
	std::string user_name = argv[2];
	/// Флаг ошибки
	int last_error;
	/// Буфер для приёма сообщений
	char receive_buffer[DEFAULT_BUFLEN] = { 0 };
	/// Длина буфера
	const int buffer_length = DEFAULT_BUFLEN;

	//| Валидация количества параметров командной строки
	if (argc != 3) {
		std::cout << "Usage: " << argv[0] << "server-name user-name" << std::endl;
		return 1;
	}

	/// Инициализация сокета Windows
	last_error = WSAStartup(MAKEWORD(2, 2), &wsaData);
	/// Обработка ошибок
	if (last_error != 0)
	{
		std::cout << "WSAStartup failed with error: " << last_error << std::endl;
		return 1;
	}

	/// Подключение к серверу и работа
	while (true)
	{
		/// Очистка памяти
		ZeroMemory(&client_info, sizeof(client_info));
		/// Установка параметров соединения
		client_info.ai_family = AF_UNSPEC;
		client_info.ai_socktype = SOCK_STREAM;
		client_info.ai_protocol = IPPROTO_TCP;

		/// Определение адреса сервера
		last_error = getaddrinfo(argv[1], DEFAULT_PORT, &client_info, &client_address_pointer);
		/// Обработка ошибок
		if (last_error != 0)
		{
			std::cout << "getaddrinfo failed with error: " << last_error << std::endl;
			WSACleanup();
			return 1;
		}

		/// Установка соединения с сервером 
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

			/// Обработка ошибок
			if (connect_socket == INVALID_SOCKET)
			{
				std::cout << "Unable to connect to server!" << std::endl;
				continue;
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

		/// Освобождение неиспользуемой памяти
		freeaddrinfo(client_address_pointer);

		/// Обработка ошибок
		if (connect_socket == INVALID_SOCKET)
		{
			std::cout << "Unable to connect to server!" << std::endl;
			WSACleanup();
			return 1;
		}

		/// Отправка серверу имя пользователя
		last_error = send(connect_socket, user_name.c_str(), user_name.length(), 0);
		/// Обработка ошибок
		if (last_error == SOCKET_ERROR)
		{
			std::cout << "send failed with error: " << WSAGetLastError() << std::endl;
			closesocket(connect_socket);
			WSACleanup();
			return 1;
		}

		/// Создание отдельного потока для отправки данных серверу
		unsigned thread_send_ID;
		HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, &sendData, (void*)connect_socket, 0, &thread_send_ID);

		/// Бесконечный цикл для принятия сообщений от сервера
		while (true)
		{
			/// Получение сообщения с сервера
			last_error = recv(connect_socket, receive_buffer, buffer_length, 0);
			/// В случае наличия данных
			if (last_error > 0)
			{
				/// Вывод полученного сообщения
				std::cout << receive_buffer << std::endl;

				// Очистка буфера
				memset(receive_buffer, 0, sizeof receive_buffer);
			}
			/// Закрытие соединения
			else if (last_error == 0)
			{
				std::cout << "Connection closed" << std::endl;
			}
			/// Обработка ошибок
			else
			{
				std::cout << "Connection closed, reconnecting ..." << WSAGetLastError() << std::endl;
				// Закрытие соединения
				closesocket(connect_socket);
				WSACleanup();
				break;
			}
		}
	}

	// Закрытие соединения
	closesocket(connect_socket);
	WSACleanup();

	return 0;
}