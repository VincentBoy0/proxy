#ifndef PREPARE_H
#define PREPARE_H

#include <string>
#include <vector>

using namespace std;

// Function declarations
bool isSubstring(const string &str1, const string &str2);
bool checkBanList(const string &request, const vector<string> &ban);
vector<string> readBanFile(const string &file);
pair<string, int> get_Host_Port(string request);
bool createProxyServer(SOCKET &proxyServer, int port);


#endif // BAN_LIST_UTILS_H