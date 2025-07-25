﻿#define CRT_SECURE_NO_WARNINGS

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif // !WIN32_LEAN_AND_MEAN

#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <iphlpapi.h>
#include <iostream>
using namespace std;

#pragma comment(lib, "WS2_32.lib")

#define DEFAULT_PORT			"27015"
#define DEFAULT_BUFFER_LENGTH	1500
#define SZ_SORRY	"Sorry, but all is busy"

VOID HandleClient(SOCKET ClientSocket);
CONST INT MAX_CLIENTS = 3;
SOCKET clients[MAX_CLIENTS] = {};
DWORD dwThreadIDs[MAX_CLIENTS] = {};
HANDLE hThreads[MAX_CLIENTS] = {};

INT g_connected_clients_count = 0;

void main()
{
	setlocale(LC_ALL, "");

	// Инициализация WinSock
	WSAData wsaData;
	
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		cout << "Error: WSAstartup failed: " << iResult << endl;
		return;
	}

	// Проверка заянятости порта, на котором нужно запустить сервер
	addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;	// TCP/IPv4
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	addrinfo* result = NULL;
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0)
	{
		WSACleanup();
		cout << "Error: getaddrinfo failed: " << iResult << endl;
		return;
	}
	cout << hints.ai_addr << endl;

	// Создание Сокета, который будет прослушивать Сервер
	SOCKET ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET)
	{
		cout << "Error: Socket creation failed: " << WSAGetLastError() << endl;
		freeaddrinfo(result);
		WSACleanup();
		return;
	}

	// Связка Сокета с сетевой картой, которую он будет прослушивать
	//strcpy(result->ai_addr->sa_data,"127.0.0.1");
	iResult = bind(ListenSocket, result->ai_addr, result->ai_addrlen);
	if (iResult == SOCKET_ERROR)
	{
		cout << "Error: bind failed with code " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		freeaddrinfo(result);
		WSACleanup();
		return;
	}

	// Запуск Сокета
	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		cout << "Error: Listen failed with code " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		freeaddrinfo(result);
		WSACleanup();
		return;
	}

	while (true)
	{
		SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);
		if (g_connected_clients_count < MAX_CLIENTS)
		{
			//HandleClient(ClientSocket);
			clients[g_connected_clients_count] = ClientSocket;
			hThreads[g_connected_clients_count] = CreateThread(
				NULL,
				0,
				(LPTHREAD_START_ROUTINE)HandleClient,
				(LPVOID)clients[g_connected_clients_count],
				0,
				&dwThreadIDs[g_connected_clients_count]
			);
			g_connected_clients_count++;
		}
		else
		{
			CHAR receive_buffer[DEFAULT_BUFFER_LENGTH] = {};
			INT iResult = recv(ClientSocket, receive_buffer, DEFAULT_BUFFER_LENGTH, 0);
			if (iResult > 0)
			{
				cout << "Bytes received: " << iResult << endl;
				cout << "Message: " << receive_buffer << endl;
				//CONST CHAR SZ_SORRY[] = "Sorry, but all is busy";
				INT iSendResult = send(ClientSocket, SZ_SORRY, strlen(SZ_SORRY), 0);
				closesocket(ClientSocket);
			}
		}
	}

	WaitForMultipleObjects(MAX_CLIENTS, hThreads, TRUE, INFINITE);
	
	closesocket(ListenSocket);
	freeaddrinfo(result);
	WSACleanup();
}

VOID WINAPI HandleClient(SOCKET ClientSocket)
{
	SOCKADDR_IN peer;
	ZeroMemory(&peer, sizeof(peer));
	// Зацикливание Сокета на получение соединений от клиентов
	INT iResult = 0;
	CHAR recvbuffer[DEFAULT_BUFFER_LENGTH] = {};
	int recv_buffer_length = DEFAULT_BUFFER_LENGTH;
	CHAR address[INET_ADDRSTRLEN] = {};
	INT address_length = 16;
	do
	{
		ZeroMemory(recvbuffer, sizeof(recvbuffer));
		iResult = recvfrom(ClientSocket, recvbuffer, recv_buffer_length, 0, (SOCKADDR*)&peer, &address_length);
		if (iResult > 0)
		{
			getpeername(ClientSocket, (SOCKADDR*)&peer, &address_length);
			inet_ntop(AF_INET, &peer.sin_addr, address, INET_ADDRSTRLEN);
			cout << "Client IP-address: " << address << endl;
			cout << "Client port: " << ntohs(peer.sin_port) << endl;
			cout << "Bytes received: " << iResult << endl;
			cout << "Message: " << recvbuffer << endl;
			CHAR sz_responce[] = "Hello, I'm Server! Nice to meet you!";
			//INT iSendResult = send(ClientSocket, sz_responce, sizeof(sz_responce), 0);
			for (int i = 0; i < g_connected_clients_count; i++)
			{
				INT iSendResult = send(clients[i], recvbuffer, strlen(recvbuffer), 0);
				if (iSendResult == SOCKET_ERROR)
				{
					cout << "Error: Send failed with code " << WSAGetLastError() << endl;
					closesocket(ClientSocket);
					//closesocket(ListenSocket);
					//freeaddrinfo(result);
					//WSACleanup();
					//return;
				}
			}
			//cout << "Bytes sent: " << iSendResult << endl;
		}
		else if (iResult == 0)
		{
			cout << "Connection closing" << endl;
			closesocket(ClientSocket);
		}
		else
		{
			cout << "Error: recv() failed with code " << WSAGetLastError() << endl;
			closesocket(ClientSocket);
			//return;
		}
		cout << "======================================================================" << endl;
	} while (iResult > 0);
}