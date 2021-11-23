#include <iostream>
#include <string>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

int main()
{
	string ipAddress = "127.0.0.1";			// IP Address of the server
	int port = 7414;						// Listening port # on the server

	// Initialize WinSock
	WSAData data;
	WORD ver = MAKEWORD(2, 2);
	int wsResult = WSAStartup(ver, &data);
	if (wsResult != 0)
	{
		cerr << "Can't start Winsock, Err #" << wsResult << endl;
		return 0;
	}

	// Create socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET)
	{
		cerr << "Can't create socket, Err #" << WSAGetLastError() << endl;
		WSACleanup();
		return 0;
	}

	// Fill in a hint structure
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(port);
	inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

	// Connect to server
	int connResult = connect(sock, (sockaddr*)&hint, sizeof(hint));
	if (connResult == SOCKET_ERROR)
	{
		cerr << "Can't connect to server, Err #" << WSAGetLastError() << endl;
		closesocket(sock);
		WSACleanup();
		return 0;
	}

	// Do-while loop to send and receive data
	char buf[4096];
	string userInput;

	int bytesReceived = recv(sock, buf, 4096, 0);
	string initial_msg(buf, 0, bytesReceived); //連線成功或額滿訊息
	cout << initial_msg;
	if (initial_msg == "Too many clients on the server. Wait a few minutes.\r\n") {
		system("pause");
		return 0;
	}

	while(true)
	{
		// Prompt the user for some text
		cout << "Input : ";
		getline(cin, userInput);
		
		if (!userInput.empty())		// Make sure the user has typed in something
		{
			// Send the text
			int sendResult = send(sock, userInput.c_str(), userInput.size() + 1, 0);
			if (sendResult != SOCKET_ERROR)
			{
				// Wait for response
				ZeroMemory(buf, 4096);
				int bytesReceived = recv(sock, buf, 4096, 0);
				if (bytesReceived <= 0) break;
				// Echo response to console
				cout << "OUTPUT : " << string(buf, 0, bytesReceived);
			}
			else {
				cout << "SOCKET ERROR!!!\n";
				break;
			}
		}

	}

	// Gracefully close down everything
	closesocket(sock);
	WSACleanup();
	system("pause");
	return 0;
}