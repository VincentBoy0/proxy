#ifndef PREPARE_H
#define PREPARE_H

#include <string>
#include <vector>

using namespace std;

// Function declarations
bool isSubstring(const string &str1, const string &str2);
bool checkBlackList(const string &request, const multiset<string> &ban);
tuple<string, string, int> get_Host_Port(const string& request);
void printBlackList(const multiset<string>& blackList);
bool createProxyServer(SOCKET &proxyServer, int port);


#endif // BAN_LIST_UTILS_H