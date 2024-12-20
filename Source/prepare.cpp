#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <set>
#include <tuple>
#include "prepare.h"
using namespace std;
bool isSubstring(const string &str1, const string &str2) {
    return str2.find(str1) != string::npos;
}
bool checkBlackList(const string &request, const multiset<string> &ban) {
    for(string s: ban) {
        if (isSubstring(s, request)) return true;        
    }
    return false;
}

// ------------------------Get Host and Port from HTTP/HTTPS request-----------------------//
tuple<string, string, int> get_Host_Port(const string& request) {
    string first_line = request.substr(0, request.find("\r\n"));
    string method = first_line.substr(0, first_line.find(" "));
    string url = first_line.substr(first_line.find(" ") + 1);
    string protocol = first_line.substr(first_line.rfind(" ") + 1);
    string host;
    int port = 80;  // Default port

    // Remove protocol if present
    size_t pos = url.find("://");
    if (pos != string::npos) {
        url = url.substr(pos + 3);  // Remove protocol (e.g., "http://")
    }

    // Find the host and port
    pos = url.find('/');
    if (pos != string::npos) {
        host = url.substr(0, pos);  // Extract host
        url = url.substr(pos);      // Remaining part of the URL (after host)
    } else {
        host = url;  // If no '/', the entire url is the host
        url = "/";
    }

    // Extract port if available
    pos = host.find(':');
    if (pos != string::npos) {
        port = stoi(host.substr(pos + 1));  // Extract port after ':'
        host = host.substr(0, pos);               // Remove port from host
    }

    return make_tuple(method, host, port);
}
void printBlackList(const multiset<string>& blackList) {
    cout << "Black List: \n";
    for(string s: blackList) cout << s << endl;
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