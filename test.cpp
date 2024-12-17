#include <iostream>
#include <string>
#include <winsock2.h>   // For Windows socket programming
#include <ws2tcpip.h>   // For Windows socket functions

using namespace std;

// Initialize Winsock
void initializeWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup failed" << endl;
        exit(1);
    }
}

// Function to handle HTTP request and respond with a basic HTML page
void handle_client(SOCKET client_socket) {
    char buffer[1024];
    recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    // Print the request (for debugging)
    cout << "Request received: \n" << buffer << endl;
    
    // Send HTTP response (basic HTML form for proxy settings)
    string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    response += "<html><body><h1>Configure Proxy Server</h1>";
    response += "<form method='POST' action='/start-proxy'>";
    response += "Proxy Port: <input type='text' name='port'><br>";
    response += "Target Host: <input type='text' name='host'><br>";
    response += "<button type='submit'>Start Proxy</button>";
    response += "</form></body></html>";
    
    send(client_socket, response.c_str(), response.length(), 0);
    
    // Close the client socket
    closesocket(client_socket);
}

int main() {
    initializeWinsock();

    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);

    // Create a socket
    server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        cerr << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }
    
    // Set server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;  // Listen on all network interfaces
    server_addr.sin_port = htons(8080);        // Port 8080

    // Bind the socket to the address and port
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        cerr << "Bind failed" << endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    // Listen for incoming connections
    if (listen(server_socket, 3) == SOCKET_ERROR) {
        cerr << "Listen failed" << endl;
        closesocket(server_socket);
        WSACleanup();
        return 1;
    }

    cout << "Server listening on http://localhost:8080" << endl;

    // Main server loop
    while (true) {
        // Accept a connection from a client
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            cerr << "Accept failed" << endl;
            continue;
        }

        // Handle the client request
        handle_client(client_socket);
    }

    // Close the server socket (never reached in this loop)
    closesocket(server_socket);
    WSACleanup();

    return 0;
}
