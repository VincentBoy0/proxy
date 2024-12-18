#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include "prepare.h"
using namespace std;
bool isSubstring(const string &str1, const string &str2) {
    return str2.find(str1) != string::npos;
}
bool checkBanList(const string &request, const vector<string> &ban) {
    for(string s: ban) { 
        if (isSubstring(s, request)) return true;        
    }
    return false;
}
vector<string> readBanFile(const string &file) {
    vector<string> ban;
    ifstream fi(file);
    string s;
    while(getline(fi, s)) {
        ban.push_back(s);
    }
    return ban;
}

// ------------------------Get Host and Port from HTTP/HTTPS request-----------------------//
pair<string, int> get_Host_Port(string request) {
    string first_line = request.substr(0, request.find("\r\n"));
    string method = first_line.substr(0, first_line.find(" "));
    string url = first_line.substr(first_line.find(" ") + 1);
    string protocol = first_line.substr(first_line.rfind(" ") + 1);
    string host;
    int port = 80;
    size_t pos = url.find("://");
    if (pos != std::string::npos) {
        url = url.substr(pos + 3); // Remove protocol
    }
    pos = url.find('/');
    if (pos != std::string::npos) {
        host = url.substr(0, pos);
        url = url.substr(pos);
    } else {
        host = url;
        url = "/";
    }
    pos = host.find(':');
    if (pos != string::npos) {
        port = std::stoi(host.substr(pos + 1));
        host = host.substr(0, pos);
    }
    return make_pair(host, port);
}
// ---------------------Proxy server logic----------------------------------//
bool createProxyServer(SOCKET &proxyServer, int port) {
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        cerr << "Winsock initialization failed!\n";
        return 0;
    }

    proxyServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (proxyServer == INVALID_SOCKET) {
        cerr << "Failed to create server socket, error: " << WSAGetLastError() << '\n';
        WSACleanup();
        return 0;
    }

    sockaddr_in proxyAddress{};
    proxyAddress.sin_family = AF_INET;
    InetPton(AF_INET, "127.0.0.1", &proxyAddress.sin_addr.s_addr);
    proxyAddress.sin_port = htons(port);

    if (bind(proxyServer, (SOCKADDR*)&proxyAddress, sizeof(proxyAddress)) == SOCKET_ERROR) {
        closesocket(proxyServer);
        WSACleanup();
        return 0;
    }

    if (listen(proxyServer, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(proxyServer);
        WSACleanup();
        return 0;
    }
    return 1;
}