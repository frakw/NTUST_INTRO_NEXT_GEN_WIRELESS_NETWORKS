#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <WS2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define DEFAULT_TRIANGLE_FILE "triangle_data.txt"
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
			vector<string> commands;
			size_t found_txt = userInput.find("txt");
			size_t found_SPO = userInput.find("SPO");
			size_t found_LOGIN = userInput.find("LOGIN");
			if (found_txt != string::npos && found_SPO != string::npos) {//txt檔
				fstream input_file;
				input_file.open(userInput.substr(0, found_txt + 3));
				if (!input_file.is_open()) {
					cout << "OUTPUT : file not found\n";
					continue;
				}
				string command;
				while (getline(input_file, command)) {
					commands.push_back(command);
				}
				commands.push_back("logout");//做完前面的就發出關閉連線的指令
			}
			else if(found_SPO != string::npos){
				commands.push_back(userInput);
				commands.push_back("logout");//做完前面的就發出關閉連線的指令
			}
			else if (found_LOGIN != string::npos) {
				commands.push_back(userInput);
			}
			else {//unknown command
				cout << "OUTPUT : unknown command\n";
				continue;
			}
			
			for (string command : commands) {
				// Send the text
				int sendResult = send(sock, command.c_str(), command.size() + 1, 0);
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
			if (commands.back() == "logout") {
				cout << "OUTPUT : TCP connection closed\n";
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