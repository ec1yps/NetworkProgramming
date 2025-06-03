#define CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

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

#define DEFAULT_ADDR			"127.0.0.1"
#define DEFAULT_PORT			27015
#define DEFAULT_BUFFER_LENGTH	1500

void main()
{
	setlocale(LC_ALL, "");

	WSAData wsaData;
	
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0)
	{
		cout << "Error: WSAstartup failed: " << iResult << endl;
		return;
	}

	SOCKET ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET)
	{
		cout << "Error: Socket creation failed: " << WSAGetLastError() << endl;
		WSACleanup();
		return;
	}

	//https://learn.microsoft.com/en-us/windows/win32/api/winsock2/nf-winsock2-listen
	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = inet_addr(DEFAULT_ADDR);
	service.sin_port = htons(DEFAULT_PORT);

	iResult = bind(ListenSocket, (SOCKADDR*)&service, sizeof(service));
	if (iResult == SOCKET_ERROR)
	{
		cout << "Error: bind failed with code " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		cout << "Error: Listen failed with code " << WSAGetLastError() << endl;
		closesocket(ListenSocket);
		WSACleanup();
		return;
	}

	CHAR recvbuffer[DEFAULT_BUFFER_LENGTH] = {};
	int recv_buffer_length = DEFAULT_BUFFER_LENGTH;
	SOCKET ClientSocket = accept(ListenSocket, NULL, NULL);

	do
	{
		iResult = recv(ClientSocket, recvbuffer, recv_buffer_length, 0);
		if (iResult > 0)
		{
			cout << "Bytes received: " << iResult << endl;
			cout << "Message: " << recvbuffer << endl;
			INT iSendResult = send(ClientSocket, recvbuffer, iResult, 0);
			if (iSendResult == SOCKET_ERROR)
			{
				cout << "Error: Send failed with code " << WSAGetLastError() << endl;
				closesocket(ClientSocket);
				closesocket(ListenSocket);
				WSACleanup();
				return;
			}
			cout << "Bytes sent: " << iSendResult << endl;
		}
		else if (iResult == 0)
		{
			cout << "Connection closing" << endl;
		}
		else
		{
			cout << "Error: recv() failed with code " << WSAGetLastError() << endl;
			closesocket(ClientSocket);
			closesocket(ListenSocket);
			WSACleanup();
			return;
		}
	} while (iResult > 0);
}