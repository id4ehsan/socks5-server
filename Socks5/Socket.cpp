#undef UNICODE

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>

// Need to link with Ws2_32.lib, Mswsock.lib, and Advapi32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define _WINSOCK_DEPRECATED_NO_WARNINGS true

#define DEFAULT_BUFLEN 4096
#define DEFAULT_PORT "27015"
#define NODE_NAME nodename

SOCKET connect_to_host(char* host, char* port) {

	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	char *sendbuf = "this is a test";
//	char recvbuf[DEFAULT_BUFLEN];
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	char* nodename = host;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Resolve the server address and port
	iResult = getaddrinfo(nodename, port, &hints, &result);
	if (iResult != 0) {
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	// Attempt to connect to an address until one succeeds
	for (ptr = result; ptr != NULL;ptr = ptr->ai_next) {

		// Create a SOCKET for connecting to server
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("socket failed with error: %ld\n", WSAGetLastError());
			WSACleanup();
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}
	printf("Connected!\n");
	return ConnectSocket;
	//// Send an initial buffer
	//iResult = send(ConnectSocket, sendbuf, (int)strlen(sendbuf), 0);
	//if (iResult == SOCKET_ERROR) {
	//	printf("send failed with error: %d\n", WSAGetLastError());
	//	closesocket(ConnectSocket);
	//	WSACleanup();
	//	return 1;
	//}

	//printf("Bytes Sent: %ld\n", iResult);

	//// shutdown the connection since no more data will be sent
	//iResult = shutdown(ConnectSocket, SD_SEND);
	//if (iResult == SOCKET_ERROR) {
	//	printf("shutdown failed with error: %d\n", WSAGetLastError());
	//	closesocket(ConnectSocket);
	//	WSACleanup();
	//	return 1;
	//}

	//// Receive until the peer closes the connection
	//do {

	//	iResult = recv(ConnectSocket, recvbuf, recvbuflen, 0);
	//	if (iResult > 0)
	//		printf("Bytes received: %d\n", iResult);
	//	else if (iResult == 0)
	//		printf("Connection closed\n");
	//	else
	//		printf("recv failed with error: %d\n", WSAGetLastError());

	//} while (iResult > 0);

	//// cleanup
	//closesocket(ConnectSocket);
	//WSACleanup();
	//return 0;
}
void  print_hex_memory(void* buf, int size) {
	unsigned char* p = (unsigned char*)buf;
	for (int i = 0;i < size;i++) {
		printf("0x%02X ",p[i]);
	}
	printf("\n");
}
void print_error(char* mes, int code) {
	printf("%s: %d\n", mes ,code);
	WSACleanup();
	//return 1;
}
SOCKET create_listen_socket() {
	
	int iResult;

	SOCKET ListenSocket = INVALID_SOCKET;

	struct addrinfo* result;
	struct addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the server address and port
	iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		print_error("getaddrinfo failed with error: %d\n", iResult);
		printf("getaddrinfo failed with error: %d\n", iResult);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Create a SOCKET for connecting to server
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("socket failed with error: %ld\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Setup the TCP listening socket
	iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iResult == SOCKET_ERROR) {
		printf("bind failed with error: %d\n", WSAGetLastError());
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}

	freeaddrinfo(result);

	iResult = listen(ListenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR) {
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return INVALID_SOCKET;
	}
	return ListenSocket;
}
int handshack(SOCKET AcceptSocket) {
	int iResult;
	int iSendResult;

	char recvbuf[257];
	int recvbuflen = 257;

	iResult = recv((SOCKET)AcceptSocket, recvbuf, recvbuflen, 0);
	if (iResult > 0) {
		printf("Bytes received: %d\n", iResult);
		printf("1th received Message is:\n");
		print_hex_memory(recvbuf, iResult);
		char handshakebuf[2];
		handshakebuf[0] = recvbuf[0];
		handshakebuf[1] = recvbuf[2];
		// Echo the buffer back to the sender
		iSendResult = send((SOCKET)AcceptSocket, handshakebuf, 2, 0);
		if (iSendResult == SOCKET_ERROR) {
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket((SOCKET)AcceptSocket);
			WSACleanup();
			return 1;
		}
		printf("Bytes sent: %d\n", iSendResult);
		printf("1th sent Message is:\n");
		print_hex_memory(handshakebuf, iSendResult);
	}
}
char* gethostnamefrom(char* recvbuf) {
	BYTE* host = new BYTE[256];

	

	for (int i = 0;i < recvbuf[4] + 2;i++) {
		host[i] = recvbuf[5 + i];
		//hostname[i] = recvbuf[5 + i];
	}
	//memmove(hostname, recvbuf + recvbuf[4] * sizeof(BYTE), recvbuf[4] * sizeof(char));
	int lenght = recvbuf[4];
	//port = recvbuf[5 + lenght];
	char* hostname = new char[lenght];
	hostname = (char*)host;
	hostname[lenght] = 0;


	print_hex_memory(host, recvbuf[4]);
	//print_hex_memory(hostname, recvbuf[4]+1);

	printf("%s\n", hostname);
	return hostname;

}
int getportnumberfrom(char* recvbuf) {
	char const* portname;
	short portnumber;
	BYTE* port = new BYTE[2];
	for (int i = recvbuf[4];i < recvbuf[4] + 2;i++) {
		port[i - recvbuf[4]] = recvbuf[5 + i];
		//portnumber[i-recvbuf[4]] = recvbuf[5 + i];

	}
	//char* portname = new char[3];
	//portname = (char*)port;
	//portname[2] = 0;

	//Converting byte array(char array) to an integer type(short, int, long)
	portnumber = (port[0] << 8) | (port[1]);

	print_hex_memory(port, 2);
	//print_hex_memory(portnumber, 2);

	printf("%i\n", portnumber);
	return portnumber;
}
int do_proxy(SOCKET AcceptSocket, SOCKET ConnectSocket) {
	int iResult;
	unsigned long int i = 1;
//	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	SOCKET SelectedSocket = INVALID_SOCKET;
	fd_set fdset;
	TIMEVAL timeout;
	timeout.tv_sec = 120;
	timeout.tv_usec = 0;
	ioctlsocket(AcceptSocket, FIONBIO, (i = 1, &i));
	ioctlsocket(ConnectSocket, FIONBIO, (i = 1, &i));
	for (;;) {
		FD_ZERO(&fdset);
		FD_SET((SOCKET)AcceptSocket, &fdset);
		FD_SET((SOCKET)ConnectSocket, &fdset);


		SelectedSocket = select(1 + (((SOCKET)AcceptSocket > ConnectSocket) ? (SOCKET)AcceptSocket : ConnectSocket), &fdset, NULL, NULL, &timeout);
		if (SelectedSocket <= 0) {
			printf("Timed out\n");
			break;
		}
		if (FD_ISSET(AcceptSocket, &fdset)) {
			if ((iResult = recv((SOCKET)AcceptSocket, recvbuf, recvbuflen, 0)) > 0) {
				if (send((SOCKET)ConnectSocket, recvbuf, iResult, 0) != iResult)
					break;
			}
			else break;
		}
		else if (FD_ISSET(ConnectSocket, &fdset)) {
			if ((iResult = recv((SOCKET)ConnectSocket, recvbuf, recvbuflen, 0)) > 0) {
				if (send((SOCKET)AcceptSocket, recvbuf, iResult, 0) != iResult)
					break;
			}
			else break;
		}
		else break;


		//iResult = recv((SOCKET)AcceptSocket, recvbuf, recvbuflen, 0);
		//if (iResult > 0) {
		//	printf("Bytes received: %d\n", iResult);
		//	printf("Read Message From Server Socket is:\n");
		//	print_hex_memory(recvbuf, iResult);
		//}
		//printf("recive from Server Socket done!:\n");
		//iSendResult = send((SOCKET)ConnectSocket, recvbuf, recvbuflen, 0);
		//if (iSendResult == SOCKET_ERROR) {
		//	printf("send to connect failed with error: %d\n", WSAGetLastError());
		//	closesocket((SOCKET)AcceptSocket);
		//	WSACleanup();
		//	return 1;
		//}
		//printf("Wrote to Connect Socket done!:\n");
		//iResult = recv((SOCKET)ConnectSocket, recvbuf, recvbuflen, 0);
		//if (iResult > 0) {
		//	printf("Bytes received: %d\n", iResult);
		//	printf("Read Message From Connect Socket is:\n");
		//	print_hex_memory(recvbuf, iResult);
		//}
		//printf("recive from Connect Socket done!:\n");
		//iSendResult = send((SOCKET)AcceptSocket, recvbuf, recvbuflen, 0);
		//if (iSendResult == SOCKET_ERROR) {
		//	printf("send to socks server failed with error: %d\n", WSAGetLastError());
		//	closesocket((SOCKET)AcceptSocket);
		//	WSACleanup();
		//	return 1;
		//}
		//printf("Wrote to Server Socket done!:\n");

	}
	return 0;
}
DWORD WINAPI client_handler(LPVOID AcceptSocket) {
	
	int iResult;
	int iSendResult;
	char recvbuf[DEFAULT_BUFLEN];
	int recvbuflen = DEFAULT_BUFLEN;
	SOCKET ConnectSocket = INVALID_SOCKET;


	handshack((SOCKET)AcceptSocket);
	
	// Receive until the peer shuts down the connection
		
		//else if (iResult == 0)
		//	printf("Connection closing...\n");
		//else {
		//	printf("recv failed with error: %d\n", WSAGetLastError());
		//	closesocket((SOCKET)AcceptSocket);
		//	WSACleanup();
		//	return 1;
		//}
		iResult = recv((SOCKET)AcceptSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0) {
			printf("Bytes received: %d\n", iResult);
			printf("2th received Message is:\n");
			print_hex_memory(recvbuf, iResult);
		}
		switch (recvbuf[3]) {
			case 0x01:
				printf("IPV4\n");
				
				struct sockaddr_in saServer;
				unsigned long ip;
				ip = (recvbuf[4] << 24) || (recvbuf[5] << 16) || (recvbuf[6] << 8) || recvbuf[4];
				//saServer.sin_family = AF_INET;
				//saServer.sin_addr = inet_pton(AF_INET,);
				saServer.sin_port = htons(3490);
				//SOCKET ConnectSocket = INVALID_SOCKET;
				//ConnectSocket = socket(AF_INET, SOCK_STREAM,
				//	IPPROTO_TCP);
				//if (ConnectSocket == INVALID_SOCKET) {
				//	printf("socket failed with error: %ld\n", WSAGetLastError());
				//	WSACleanup();
				//	return 1;
				//}
				//iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
				//if (iResult == SOCKET_ERROR) {
				//	closesocket(ConnectSocket);
				//	ConnectSocket = INVALID_SOCKET;
				//}
				break;
			case 0x03:
				printf("HostName:\n");
				{
					char* hostname;
					int portnumber;
					hostname = gethostnamefrom(recvbuf);
					printf("%s\n", hostname);
					portnumber = getportnumberfrom(recvbuf);
					std::string s = std::to_string(portnumber);
					char const* portname = s.c_str();  //use char const* as target type
					printf("getportnamefrom output:%s\n", portname);
				
				ConnectSocket  = connect_to_host(hostname, (char*)portname);
				char responsebuf[10];
				responsebuf[0] = 0x05;
				responsebuf[1] = 0x00;
				responsebuf[2] = 0x00;
				responsebuf[3] = 0x01;
				responsebuf[4] = 0x00;
				responsebuf[5] = 0x00;
				responsebuf[6] = 0x00;
				responsebuf[7] = 0x00;
				responsebuf[8] = 0x69;
				responsebuf[9] = 0x87;
				iSendResult = send((SOCKET)AcceptSocket, responsebuf, 10, 0);
				if (iSendResult == SOCKET_ERROR) {
					printf("send failed with error: %d\n", WSAGetLastError());
					closesocket((SOCKET)AcceptSocket);
					WSACleanup();
					return 1;
				}
				printf("Bytes sent: %d\n", iSendResult);
				printf("SOCK5 Response is:\n");
				print_hex_memory(responsebuf, iSendResult);
				
				
				break;}
			case 0x04:
				printf("IPV6\n");
				//ConnectSocket = socket(AF_INET6, SOCK_STREAM,
				//	IPPROTO_TCP);
				//if (ConnectSocket == INVALID_SOCKET) {
				//	printf("socket failed with error: %ld\n", WSAGetLastError());
				//	WSACleanup();
				//	return 1;
				//}
				//iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
				//if (iResult == SOCKET_ERROR) {
				//	closesocket(ConnectSocket);
				//	ConnectSocket = INVALID_SOCKET;
				//}
				break;
			default:
				break;
		}
		do_proxy((SOCKET)AcceptSocket, (SOCKET)ConnectSocket);
		return 0;
}



int __cdecl main(void) {
	
	int iResult;
	
	// Initialize Winsock
	WSADATA wsaData;
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		printf("WSAStartup failed with error: %d\n", iResult);
		return 1;
	}


	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET AcceptSocket = INVALID_SOCKET;


	printf("SOCKS5 Listening server \n ");
	
	ListenSocket = create_listen_socket();
	
	for (;;) {
		printf("Start Of Accept Loop!:\n");
		// Accept a client socket
		AcceptSocket = accept(ListenSocket, NULL, NULL);
		
		if (AcceptSocket == INVALID_SOCKET) {
			printf("accept failed with error: %d\n", WSAGetLastError());
			closesocket(ListenSocket);
			WSACleanup();
			break;
		}
		CreateThread(NULL, 0, client_handler, (LPVOID)AcceptSocket, 0, 0);
		printf("End Of Accept Loop!:\n");
	}
	// No longer need server socket
	closesocket(ListenSocket);


	// shutdown the connection since we're done
	iResult = shutdown(AcceptSocket, SD_SEND);
	if (iResult == SOCKET_ERROR) {
		printf("shutdown failed with error: %d\n", WSAGetLastError());
		closesocket(AcceptSocket);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(AcceptSocket);
	WSACleanup();

	return 0;
}