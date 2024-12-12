#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define PORT 8080           // Port for the proxy server
#define BUFFER_SIZE 10000  // Size of the buffer for receiving data
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
    if (pos != std::string::npos) {
        port = std::stoi(host.substr(pos + 1));
        host = host.substr(0, pos);
    }
    return make_pair(host, port);
}
bool isSubstring(const string& str1, const string& str2) {
    return str2.find(str1) != string::npos;
}
bool checkBanList(const string &request, vector<string> ban) {
    for(auto s: ban) {
        if (isSubstring(s, request)) return true;        
    }
    return false;
}
void handleClient(SOCKET clientSocket, vector<string> ban) {
    char buffer[BUFFER_SIZE];
    string request;

    int bytes_read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytes_read <= 0) {
        cerr << "Failed to read from client.\n";
        return;
    }
    request = string(buffer, bytes_read);
    cerr << "REQUEST: " << request << '\n';
    if (checkBanList(request, ban)) return; // if request is in ban list
    auto [host, port] = get_Host_Port(request);

    // Create socket to target server
    SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (remoteSocket == INVALID_SOCKET) {
        cerr << "Failed to create server socket, error: " << WSAGetLastError() << '\n';
        return;
    }

    struct hostent* he = gethostbyname(host.c_str());
    if (he == nullptr) {
        cerr << "Host resolution failed!\n";
        closesocket(remoteSocket);
        return;
    }
    
    // cerr << "HOST: " << host << "\nPORT: " << port << '\n';

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    // Connect to server
    if (connect(remoteSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Failed to connect to server, error: " << WSAGetLastError() << '\n';
        closesocket(remoteSocket);
        return;
    }
    
    string connect_response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    int bytesSent = send(clientSocket, connect_response.c_str(), connect_response.size(), 0);

    // Notify client connection established
    while (true) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        FD_SET(remoteSocket, &readfds);

        int max_fd = max(clientSocket, remoteSocket) + 1;
        struct timeval timeout = {10, 0}; // 10 seconds timeout
        int activity = select(max_fd, &readfds, nullptr, nullptr, &timeout);
        if (activity < 0) {
            cerr << "Select error.\n";
            break;
        } else if (activity == 0) {
            // Timeout, check again
            continue;
        }

        int bytes_read;
        int bytes_sent;

        // Check if there's data to read from the client
        if (FD_ISSET(clientSocket, &readfds)) {
            bytes_read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            if (bytes_read <= 0) {
                cerr << "Client disconnected or read error.\n";
                break;
            }

            // Forward data to server
            bytes_sent = send(remoteSocket, buffer, bytes_read, 0);
            if (bytes_sent <= 0) {
                cerr << "Error sending data to server.\n";
                break;
            }
        }
        // Check if there's data to read from the server
        if (FD_ISSET(remoteSocket, &readfds)) {
            bytes_read = recv(remoteSocket, buffer, BUFFER_SIZE, 0);
            if (bytes_read <= 0) {
                cerr << "Error reading data from server.\n";
                break;
            }
            // Forward data to client
            bytes_sent = send(clientSocket, buffer, bytes_read, 0);
            if (bytes_sent <= 0) {
                cerr << "Error sending data to client.\n";
                break;
            }
        }
    }

    // Clean up
    closesocket(remoteSocket);
    closesocket(clientSocket);
}

vector<string> readBanFile(const string &file) {
    vector<string> ban;
    ifstream fi(file);
    string s;
    while(getline(fi, s)) {
        ban.push_back(s);
    }
    // fi.close();
    return ban;
}
int main() {
    vector<string> ban = readBanFile("ban.txt");
    // for(string s: ban) cout << s << endl;
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        cerr << "Winsock initialization failed!\n";
        return -1;
    }

    SOCKET proxyServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (proxyServer == INVALID_SOCKET) {
        cerr << "Failed to create server socket, error: " << WSAGetLastError() << '\n';
        WSACleanup();
        return -1;
    }

    sockaddr_in proxyAddress{};
    proxyAddress.sin_family = AF_INET;
    InetPton(AF_INET, "127.0.0.1", &proxyAddress.sin_addr.s_addr);
    proxyAddress.sin_port = htons(PORT);

    if (bind(proxyServer, (SOCKADDR*)&proxyAddress, sizeof(proxyAddress)) == SOCKET_ERROR) {
        cerr << "Failed to bind socket, error: " << WSAGetLastError() << '\n';
        closesocket(proxyServer);
        WSACleanup();
        return -1;
    }

    if (listen(proxyServer, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "Failed to listen on socket, error: " << WSAGetLastError() << '\n';
        closesocket(proxyServer);
        WSACleanup();
        return -1;
    }

    cout << "Proxy server is listening on port " << PORT << "...\n";

    while (true) {
        sockaddr_in clientAddress{};
        int client_len = sizeof(clientAddress);
        SOCKET clientSocket = accept(proxyServer, (SOCKADDR*)&clientAddress, &client_len);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "Failed to accept connection, error: " << WSAGetLastError() << '\n';
            continue;
        }
        cerr << "Accepted connection from client.\n";

        handleClient(clientSocket, ban);
        closesocket(clientSocket);
    }

    closesocket(proxyServer);
    WSACleanup();
    return 0;
}
