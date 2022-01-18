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
#include <regex>
#include <limits>

#include "jsonxx.h"

#define DEFALT_MAX_CLIENT_NUM 10
#pragma comment (lib, "ws2_32.lib")

using namespace std;

bool valid_IP_address(string IP);
bool is_valid_domain_name(string str);

class DomainName {
public:
	DomainName() {}
	DomainName(string _domain_name, string _ip) :domain_name(_domain_name), ip(_ip) {};
	string domain_name = "";
	string ip = "";
	int recently_used = 0;
private:
};





class DNRecord {
public:
	DNRecord(string _json_file_name, int _max_record, bool _LRU_open) :json_file_name(_json_file_name), max_record(_max_record), LRU_open(_LRU_open) {

		json_file.open(_json_file_name);
		json.parse(json_file);
		for (int i = 0; i < json.size(); i++) {
			jsonxx::Object obj = json.get<jsonxx::Object>(i);
			if (obj.get<jsonxx::String>("type") == "A") {
				database.push_back(DomainName(obj.get<jsonxx::String>("domain_name"), obj.get<jsonxx::String>("ip_address")));
			}
			else if (obj.get<jsonxx::String>("type") == "CNAME") {
				database.push_back(DomainName(obj.get<jsonxx::String>("alias"), obj.get<jsonxx::String>("canonical name")));
			}
		}
		if (has_upper_dns) {
			init_upper_dns_connection();
		}
	}
	void init_upper_dns_connection() {
		string ipAddress = upper_dns_ip;			// IP Address of the server
		int port = upper_dns_port;						// Listening port # on the server

		// Initialize WinSock
		WSAData data;
		WORD ver = MAKEWORD(2, 2);
		int wsResult = WSAStartup(ver, &data);
		if (wsResult != 0)
		{
			cerr << "Can't start Winsock, Err #" << wsResult << endl;
			return;
		}

		// Create socket
		upper_dns_sock = socket(AF_INET, SOCK_STREAM, 0);
		if (upper_dns_sock == INVALID_SOCKET)
		{
			cerr << "Can't create socket, Err #" << WSAGetLastError() << endl;
			WSACleanup();
			return;
		}

		// Fill in a hint structure
		sockaddr_in hint;
		hint.sin_family = AF_INET;
		hint.sin_port = htons(port);
		inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

		// Connect to server
		int connResult = connect(upper_dns_sock, (sockaddr*)&hint, sizeof(hint));
		if (connResult == SOCKET_ERROR)
		{
			cerr << "Can't connect to server, Err #" << WSAGetLastError() << endl;
			closesocket(upper_dns_sock);
			WSACleanup();
			return;
		}

		// Do-while loop to send and receive data
		char buf[4096];

		int bytesReceived = recv(upper_dns_sock, buf, 4096, 0);
		string initial_msg(buf, 0, bytesReceived); //連線成功或額滿訊息
		cout << initial_msg;
	}
	void add_record(string _domain_name, string _ip) {
		if (database.size() < max_record) {
			database.push_back(DomainName(_domain_name, _ip));
			return;
		}
		if (LRU_open) {
			int min_used = INT_MAX;
			int replace_index = 0;
			for (int i = 0; i < database.size(); i++) {
				if (database[i].recently_used < min_used) {
					min_used = database[i].recently_used;
					replace_index = i;
				}
			}
			database[replace_index] = DomainName(_domain_name, _ip);
		}
		else {
			database.insert(database.begin(), DomainName(_domain_name, _ip));
			database.pop_back();
		}
	}
	string get_ip(string _domain_name) {
		for (int i = 0; i < database.size(); i++) {
			if (database[i].domain_name == _domain_name) {
				database[i].recently_used++;
				return database[i].ip;
			}
		}
		if (has_upper_dns) {
			string ip = get_ip_from_root(_domain_name);
			if (ip != "not found") {
				add_record(_domain_name, ip);
			}
			return ip;
		}
		else {
			return "not found";
		}
	}
	string get_ip_from_root(string _domain_name) {
		char buf[4096];
		// Send the text
		int sendResult = send(upper_dns_sock, _domain_name.c_str(), _domain_name.size() + 1, 0);
		if (sendResult != SOCKET_ERROR)
		{
			// Wait for response
			ZeroMemory(buf, 4096);
			int bytesReceived = recv(upper_dns_sock, buf, 4096, 0);
			//if (bytesReceived <= 0) break;
			// Echo response to console
			return string(buf, 0, bytesReceived);
		}
		else {
			return "Root dns SOCKET ERROR!!!\n";
		}
	}
	void write_to_json() {
		jsonxx::Array new_json;
		for (int i = 0; i < database.size(); i++) {
			jsonxx::Object obj;
			obj << "ip_address" << database[i].ip;
			obj << "domain_name" << database[i].domain_name;
			new_json << obj;
		}
		json_file.seekp(0, ios::beg);
		json_file << new_json.json();
	}
	int max_record = 10;
	bool LRU_open = false;
	vector<DomainName> database;
	string json_file_name;
	fstream json_file;
	jsonxx::Array json;
	bool has_upper_dns = false;
	string upper_dns_ip = "";
	int upper_dns_port = -1;
	int upper_dns_sock;
private:
};

int main()
{
	int max_client_num = 10;
	DNRecord db("./root_DNS_data.json",10,false);
	//cout << db.get_ip("www.google.com");
	//cout << db.get_ip("www.taipower.com.tw");
	//return 0;
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
	cout << "The maximum number of connections is " << max_client_num << ".\n";


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

				if (master.fd_count - 1 > max_client_num) {
					string fullClientMsg = "Too many clients on the server. Wait a few minutes.\r\n";
					send(client, fullClientMsg.c_str(), fullClientMsg.size() + 1, 0);
					// Drop the client
					cout << "Close the TCP socket connection of client\n";
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
					closesocket(sock);
					FD_CLR(sock, &master);
				}
				else
				{
					string domain_name = string(buf, bytesIn);
					char back;
					while (back = domain_name.back(), back == '\r' || back == '\n' || back == ' ' || back == EOF || back == '\0') { domain_name.pop_back(); }



					if (domain_name != "logout") {
						string return_msg;
						if (is_valid_domain_name(domain_name)) {
							return_msg = db.get_ip(domain_name);
						}
						else {
							return_msg = "not valid domain name\r";
						}
						send(sock, return_msg.c_str(), return_msg.size() + 1, 0);
					}
					else if (domain_name == "logout") {
						// Drop the client
						cout << "Close the TCP socket connection of client\n";
						closesocket(sock);
						FD_CLR(sock, &master);
					}
					else {
						string unknown_command = "unknown command\r";
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


	system("pause");
	return 0;
}

bool valid_IP_address(string IP) {
	regex ipv4("(([0-9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])\\.){3}([0- 9]|[1-9][0-9]|1[0-9][0-9]|2[0-4][0-9]|25[0-5])");
	if (regex_match(IP, ipv4))
		return true;
	else
		return false;
}

bool is_valid_domain_name(string str)
{

	// Regex to check valid domain name.
	const regex pattern("^(?!-)[A-Za-z0-9-]+([\\-\\.]{1}[a-z0-9]+)*\\.[A-Za-z]{2,6}$");

	// If the domain name
	// is empty return false
	if (str.empty())
	{
		return false;
	}

	// Return true if the domain name
	// matched the ReGex
	if (regex_match(str, pattern))
	{
		return true;
	}
	else
	{
		return false;
	}
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