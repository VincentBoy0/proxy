// ----------------------------- Libraries---------------------------------------//
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <vector>
#include <iostream>
#include <sstream>
#include <set>
#include "prepare.h"
#pragma comment(lib, "ws2_32.lib")
using namespace std;
const char g_szClassName[] = "ProxyServerUI";
const int BUFFER_SIZE = 4096;
multiset<string> blackList;
// -----------------------------UI controls----------------------------------------//
HWND hStartButton, hStopButton, hPortEdit, hStatusText;

//--------------------------- Winsock variables-----------------------------------//
SOCKET proxyServer = INVALID_SOCKET;
bool isRunning = false;
thread proxyThread;

void UpdateStatus(const string& message) {
    SetWindowText(hStatusText, message.c_str());
}

void handleClient(SOCKET clientSocket, const multiset<string>& ban) {
    char buffer[BUFFER_SIZE];
    string request;

    int bytes_read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
    if (bytes_read <= 0) return;

    request = string(buffer, bytes_read);
    if (checkBlackList(request, blackList)) return; 
    auto [host, port] = get_Host_Port(request);

    // Create socket to target server
    SOCKET remoteSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (remoteSocket == INVALID_SOCKET) return;


    // ------------------Change host name to IP address------------------------------
    struct hostent* he = gethostbyname(host.c_str());
    if (he == nullptr) {
        closesocket(remoteSocket);
        return;
    }
    
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(remoteSocket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        closesocket(remoteSocket);
        return;
    }
    
    //-------------- send respone to browser that connection had established---------------------//
    string connect_response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    int bytesSent = send(clientSocket, connect_response.c_str(), connect_response.size(), 0);

    while (isRunning) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        FD_SET(remoteSocket, &readfds);

        int max_fd = max(clientSocket, remoteSocket) + 1;
        struct timeval timeout = {10, 0}; // 10 seconds timeout
        int activity = select(max_fd, &readfds, nullptr, nullptr, &timeout);
        if (activity < 0) break;
        else if (activity == 0) continue;

        int bytes_read;
        int bytes_sent;

        if (FD_ISSET(clientSocket, &readfds)) {
            bytes_read = recv(clientSocket, buffer, BUFFER_SIZE, 0);
            if (bytes_read <= 0) break; 
            bytes_sent = send(remoteSocket, buffer, bytes_read, 0);
            if (bytes_sent <= 0) break;
        }
        if (FD_ISSET(remoteSocket, &readfds)) {
            bytes_read = recv(remoteSocket, buffer, BUFFER_SIZE, 0);
            if (bytes_read <= 0) break;
            bytes_sent = send(clientSocket, buffer, bytes_read, 0);
            if (bytes_sent <= 0) break;
        }
    }

    // -----------------------------------Clean up-------------------------------------//
    closesocket(remoteSocket);
    closesocket(clientSocket);
}
void ProxyServer(unsigned short port) {
    UpdateStatus("Proxy server is listening on port " + to_string(port));

    while (isRunning) {
        sockaddr_in clientAddress{};
        int client_len = sizeof(clientAddress);
        SOCKET clientSocket = accept(proxyServer, (SOCKADDR*)&clientAddress, &client_len);
        if (clientSocket == INVALID_SOCKET) continue;

        handleClient(clientSocket, blackList);
        closesocket(clientSocket);
    }

    closesocket(proxyServer);
    WSACleanup();
    UpdateStatus("Proxy server stopped.");
}

void StartProxy(HWND hwnd) {
    if (isRunning) {
        MessageBox(hwnd, "Proxy is already running!", "Info", MB_OK | MB_ICONINFORMATION);
        return;
    }
    isRunning = true;
    char portStr[10];
    GetWindowText(hPortEdit, portStr, sizeof(portStr));
    unsigned short port = (unsigned short)atoi(portStr);
    UpdateStatus("Proxy is running...");
    if (port == 0) {
        MessageBox(hwnd, "Invalid port number!", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    createProxyServer(proxyServer, port);
    proxyThread = thread(ProxyServer, port);
    proxyThread.detach();
}

void StopProxy() {
    if (!isRunning) {
        UpdateStatus("Proxy is not running.");
        return;
    }

    isRunning = false;
    closesocket(proxyServer);
}

void ProcessBlacklist(const char* blacklist, HWND hwnd, HWND hBlacklistView) {
    if (strlen(blacklist) == 0) {
        MessageBox(hwnd, "Blacklist cannot be empty.", "Error", MB_ICONERROR);
        return;
    }

    string blacklistData = blacklist;

    stringstream ss(blacklistData);
    string entry;

    while (getline(ss, entry, ',')) {
        entry.erase(entry.find_last_not_of(" \t\n\r\f\v") + 1);
        entry.erase(0, entry.find_first_not_of(" \t\n\r\f\v"));

        if (!entry.empty()) {
            SendMessage(hBlacklistView, LB_ADDSTRING, 0, (LPARAM)entry.c_str());
            blackList.insert(entry);
        }
    }
    // Clear the blacklist edit box after submission
    SetWindowText(hwnd, "");
    MessageBox(hwnd, "Blacklist submitted successfully!", "Info", MB_ICONINFORMATION);
}
void DeleteBlacklistItem(HWND hBlacklistView, HWND hwnd) {
    // Get the selected item index
    int selectedIndex = SendMessage(hBlacklistView, LB_GETCURSEL, 0, 0);
    if (selectedIndex == LB_ERR) {
        MessageBox(hwnd, "Please select an item to delete.", "Error", MB_ICONERROR);
        return;
    }
    char selectedText[256];  // Buffer to hold the text
    SendMessage(hBlacklistView, LB_GETTEXT, selectedIndex, (LPARAM)selectedText);

    blackList.erase(blackList.find(selectedText));

    // Remove the selected item from the List Box
    SendMessage(hBlacklistView, LB_DELETESTRING, selectedIndex, 0);

    MessageBox(hwnd, "Selected item deleted successfully!", "Info", MB_ICONINFORMATION);
}

// -----------------------Window procedure function-----------------------------------------//
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hBlacklistEdit, hSubmitButton, hBlacklistView, hDeleteButton;

    switch (msg) {
    case WM_CREATE:
        // Create UI elements
        CreateWindow("STATIC", "Port:", WS_CHILD | WS_VISIBLE, 20, 20, 40, 20, hwnd, NULL, NULL, NULL);
        hPortEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 70, 20, 100, 20, hwnd, NULL, NULL, NULL);

        hStartButton = CreateWindow("BUTTON", "Start", WS_CHILD | WS_VISIBLE, 200, 20, 80, 30, hwnd, (HMENU)1, NULL, NULL);
        hStopButton = CreateWindow("BUTTON", "Stop", WS_CHILD | WS_VISIBLE, 290, 20, 80, 30, hwnd, (HMENU)2, NULL, NULL);

        hStatusText = CreateWindow("STATIC", "Status: Idle", WS_CHILD | WS_VISIBLE, 20, 60, 350, 20, hwnd, NULL, NULL, NULL);

        // Add blacklist input box and submit button
        CreateWindow("STATIC", "Blacklist:", WS_CHILD | WS_VISIBLE, 20, 100, 70, 20, hwnd, NULL, NULL, NULL);
        hBlacklistEdit = CreateWindow("EDIT", "", WS_CHILD | WS_VISIBLE | WS_BORDER, 100, 100, 200, 20, hwnd, NULL, NULL, NULL);

        hSubmitButton = CreateWindow("BUTTON", "Submit", WS_CHILD | WS_VISIBLE, 320, 100, 80, 30, hwnd, (HMENU)3, NULL, NULL);

        // Add a list box to view the blacklist
        hBlacklistView = CreateWindow("LISTBOX", "", WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY, 20, 140, 380, 150, hwnd, NULL, NULL, NULL);

        // Add a delete button to remove items from the blacklist
        hDeleteButton = CreateWindow("BUTTON", "Delete", WS_CHILD | WS_VISIBLE, 320, 300, 80, 30, hwnd, (HMENU)4, NULL, NULL);
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { // Start button
            StartProxy(hwnd);
        }
        else if (LOWORD(wParam) == 2) { // Stop button
            StopProxy();
        }
        else if (LOWORD(wParam) == 3) { // Submit blacklist button
            char blacklistBuffer[256];
            GetWindowText(hBlacklistEdit, blacklistBuffer, sizeof(blacklistBuffer));
            ProcessBlacklist(blacklistBuffer, hwnd, hBlacklistView);
        }
        else if (LOWORD(wParam) == 4) { // Delete blacklist item button
            DeleteBlacklistItem(hBlacklistView, hwnd);
        }
        break;

    case WM_DESTROY:
        StopProxy(); // Ensure the proxy stops when the window closes
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}


// ------------------------------------Entry point----------------------------------//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASSEX wc;
    HWND hwnd;
    MSG Msg;

    // --------------------------Register window class-------------------------------//
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = g_szClassName;
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "Window Registration Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    // -------------------------------------Create window-----------------------------------//
    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        g_szClassName,
        "Proxy Server UI",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 450, 400,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        MessageBox(NULL, "Window Creation Failed!", "Error", MB_ICONEXCLAMATION | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    while (GetMessage(&Msg, NULL, 0, 0) > 0) {
        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return Msg.wParam;
}
