#include <iostream>
#include <fstream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctype.h>

#define DEFALT_MAX_CLIENT_NUM 10
#define DEFALT_ACCOUNTS_FILE "account_data.txt"
#pragma comment (lib, "ws2_32.lib")

using namespace std;

std::string judge_triangle(int[]);
string shift_two_decrypt(string);

class Account {
public:
	Account(string _username,string _password):username(_username),password(_password){}
	string username;
	string password;
	bool logined = false;
};

int main(int argc, char* argv[])
{
	int max_client_num = 10;
	if (argc > 1) max_client_num = atoi(argv[1]);
	else max_client_num = DEFALT_MAX_CLIENT_NUM;

	fstream account_data_file;
	if (argc > 2) {
		account_data_file.open(argv[2]);
	}
	else {
		account_data_file.open(DEFALT_ACCOUNTS_FILE);
	}
	
	string account_data;
	vector<Account*> accounts;

	while (getline(account_data_file, account_data)) {
		account_data.erase(std::remove(account_data.begin(), account_data.end(), ' '), account_data.end()); //移除所有空格
		string username, password;
		stringstream ss(account_data);
		getline(ss, username, ',');
		getline(ss, password);
		accounts.push_back(new Account(username, shift_two_decrypt(password)));
	}



	//accounts.push_back(new Account("abc", "123"));
	//accounts.push_back(new Account("frakw", "mew"));
	map<int, Account*> login_map;



	// Initialze winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);

	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0)
	{
		cerr << "Can't Initialize winsock! Quitting" << endl;
		return 0;
	}

	// Create a socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET)
	{
		cerr << "Can't create a socket! Quitting" << endl;
		return 0;
	}

	// Bind the ip address and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(7414);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; // Could also use inet_pton .... 

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// Tell Winsock the socket is for listening 
	listen(listening, SOMAXCONN);

	// Create the master file descriptor set and zero it
	fd_set master;
	FD_ZERO(&master);

	// Add our first socket that we're interested in interacting with; the listening socket!
	// It's important that this socket is added for our server or else we won't 'hear' incoming
	// connections 
	FD_SET(listening, &master);

	// this will be changed by the \quit command (see below, bonus not in video!)
	bool running = true;

	cout << "The server is ready to provide service.\n";
	cout << "The maximum number of connections is "<< max_client_num <<".\n";


	while (running)
	{
		// Make a copy of the master file descriptor set, this is SUPER important because
		// the call to select() is _DESTRUCTIVE_. The copy only contains the sockets that
		// are accepting inbound connection requests OR messages. 

		// E.g. You have a server and it's master file descriptor set contains 5 items;
		// the listening socket and four clients. When you pass this set into select(), 
		// only the sockets that are interacting with the server are returned. Let's say
		// only one client is sending a message at that time. The contents of 'copy' will
		// be one socket. You will have LOST all the other sockets.

		// SO MAKE A COPY OF THE MASTER LIST TO PASS INTO select() !!!

		fd_set copy = master;

		// See who's talking to us
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);

		// Loop through all the current connections / potential connect
		for (int i = 0; i < socketCount; i++)
		{
			// Makes things easy for us doing this assignment
			SOCKET sock = copy.fd_array[i];

			// Is it an inbound communication?
			if (sock == listening)
			{

				// Accept a new connection
				SOCKET client = accept(listening, nullptr, nullptr);

				// Add the new connection to the list of connected clients
				FD_SET(client, &master);

				//cout << master.fd_count << endl;
				if (master.fd_count-1 > max_client_num) {
					string fullClientMsg = "Too many clients on the server. Wait a few minutes.\r\n";
					send(client, fullClientMsg.c_str(), fullClientMsg.size() + 1, 0);
					// Drop the client
					closesocket(client);
					FD_CLR(client, &master);
				}
				else {
					// Send a welcome message to the connected client
					string acceptMsg = "Accept " + to_string(master.fd_count - 1) + " connection.\r\n";
					cout << acceptMsg;
					string conectedtMsg = "Successful connect to server.\r\n";
					send(client, conectedtMsg.c_str(), conectedtMsg.size() + 1, 0);
				}

			}
			else // It's an inbound message
			{
				char buf[4096];
				ZeroMemory(buf, 4096);

				// Receive message
				int bytesIn = recv(sock, buf, 4096, 0);
				if (bytesIn <= 0)
				{
					// Drop the client
					if (login_map.find(sock) != login_map.end()) {
						cout << "Close the TCP socket connection of client '" << login_map[sock]->username << "'.\n";
						login_map[sock]->logined = false;
						login_map.erase(sock);
					}
					closesocket(sock);
					FD_CLR(sock, &master);
				}
				else
				{
					string line = string(buf, bytesIn);
					char back;
					while (back = line.back(), back == '\r' || back == '\n' || back == ' ' || back == EOF || back == '\0') { line.pop_back(); }
					string command = "";
					string input = "";


					bool get_last_space = false;
					for (int i = line.length() - 1; i >= 0; i--) {
						if (!get_last_space && line[i] == ' ') { get_last_space = true; continue; }
						if (get_last_space) {
							input.insert(input.begin(), line[i]);
						}
						else {
							command.insert(command.begin(), line[i]);
						}
					}
					
					if (command == "LOGIN") {
						if (login_map.find(sock) != login_map.end()) {
							string already_login = "You already login\r\n";
							send(sock, already_login.c_str(), already_login.size() + 1, 0);
							continue;
						}
						bool  get_colon = false;
						string username = "";
						string password = "";
						for (int i = 0; i < input.length(); i++) {
							if (!get_colon && input[i] == ':') { get_colon = true; continue; }
							if (get_colon) {
								password.push_back(input[i]);
							}
							else {
								username.push_back(input[i]);
							}
						}
						bool login_success = false;
						for (int i = 0; i < accounts.size(); i++) {
							if (username == accounts[i]->username && password == accounts[i]->password) {
								login_success = true;
								if (accounts[i]->logined) {
									string login_from_other_client = "This account already login from other client.\r\n";
									send(sock, login_from_other_client.c_str(), login_from_other_client.size() + 1, 0);
									goto next_req;
								}
								accounts[i]->logined = true;
								login_map.insert(make_pair(sock, accounts[i]));
								break;
							}
						}
						if (login_success) {
							string valid = "valid\r\n";
							cout << "Client '" << username << "' logins successfully\n";
							//Sleep(500);
							send(sock, valid.c_str(), valid.size() + 1, 0);
						}
						else {
							string invalid = "invalid\r\n";
							cout << "Client '" << username << "' is unregistered or enter incorrect password.\n";
							send(sock, invalid.c_str(), invalid.size() + 1, 0);
						}
					}
					else if (command == "SPO") {

						stringstream ss(input);
						int triangle[3], index = 0;
						for (int i = 0; i < 3; i++)
						{
							string substr;
							try {
								getline(ss, substr, ',');
								triangle[index++] = stoi(substr);
							}
							catch (...) {
								string input_err = "Input Error!!!\r\n";
								send(sock, input_err.c_str(), input_err.size() + 1, 0);
								goto next_req;
							}
						}
						if (login_map.find(sock) == login_map.end()) {
							string permission_denied = "Permission Denied\r\n";
							send(sock, permission_denied.c_str(), permission_denied.size() + 1, 0);
							cout << "No permission to process the triangle problem with side length of " << triangle[0] << ", " << triangle[1] << ", and " << triangle[2] << ". Please try again.\n";
							continue;
						}

						string triangle_result = judge_triangle(triangle);
						send(sock, triangle_result.c_str(), triangle_result.size() + 1, 0);
					}
					else {
						string unknown_command = "unknown command\r\n";
						send(sock, unknown_command.c_str(), unknown_command.size() + 1, 0);						
					}
				}
			}
			next_req:;
		}
	}

	// Remove the listening socket from the master file descriptor set and close it
	// to prevent anyone else trying to connect.
	FD_CLR(listening, &master);
	closesocket(listening);

	// Message to let users know what's happening.
	string msg = "Server is shutting down. Goodbye\r\n";

	while (master.fd_count > 0)
	{
		// Get the socket number
		SOCKET sock = master.fd_array[0];

		// Send the goodbye message
		send(sock, msg.c_str(), msg.size() + 1, 0);

		// Remove it from the master file list and close the socket
		FD_CLR(sock, &master);
		closesocket(sock);
	}

	// Cleanup winsock
	WSACleanup();

	for (int i = 0; i < accounts.size(); i++) {
		delete accounts[i];
	}

	system("pause");
	return 0;
}

string judge_triangle(int triangle[]) {
	for (int i = 0; i < 3; i++) {
		if (triangle[i] <= 0) return "not a triangle\r\n";
	}
	sort(triangle, triangle + 3);
	int a = triangle[0], b = triangle[1], c = triangle[2]; // c is biggest
	if (a + b <= c)return "not a triangle\r\n";
	if (c * c == a * a + b * b) return "Right Triangle\r\n";
	if (c * c > a * a + b * b) return "Obtuse Triangle\r\n";
	if (c * c < a * a + b * b) return "Acute Triangle\r\n";
}

string shift_two_decrypt(string input) {
	for (int i = 0; i < input.length();i++) {
		if (isdigit(input[i])) {
			if (input[i] == '0') input[i] = '8';
			else if (input[i] == '1') input[i] = '9';
			else input[i] -= 2;
		}
		else if (isupper(input[i]) && isalpha(input[i])) {
			if (input[i] == 'A') input[i] = 'Y';
			else if (input[i] == 'B') input[i] = 'Z';
			else input[i] -= 2;
		}
		else if (islower(input[i]) && isalpha(input[i])) {
			if (input[i] == 'a') input[i] = 'y';
			else if (input[i] == 'b') input[i] = 'z';
			else input[i] -= 2;
		}
	}
	return input;
}

// Send message to other clients, and definiately NOT the listening socket

//for (int i = 0; i < master.fd_count; i++)
//{
//	SOCKET outSock = master.fd_array[i];
//	if (outSock != listening && outSock != sock)
//	{
//		ostringstream ss;
//		ss << "SOCKET #" << sock << ": " << buf << "\r\n";
//		string strOut = ss.str();

//		send(outSock, strOut.c_str(), strOut.size() + 1, 0);
//	}
//}