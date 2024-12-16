#include <bits/stdc++.h>
#include "UI.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 10000

using namespace std;

// Global variables
mutex banListMutex;
vector<string> banList;

// Function to read the ban list from a file
void loadBanList(const string& file) {
    lock_guard<mutex> guard(banListMutex);
    ifstream infile(file);
    banList.clear();
    string line;
    while (getline(infile, line)) {
        banList.push_back(line);
    }
    infile.close();
}

// Function to save the ban list to a file
void saveBanList(const string& file) {
    lock_guard<mutex> guard(banListMutex);
    ofstream outfile(file);
    for (const string& entry : banList) {
        outfile << entry << endl;
    }
    outfile.close();
}

// Function to check if a request is banned
bool isBanned(const string& request) {
    lock_guard<mutex> guard(banListMutex);
    for (const string& banned : banList) {
        if (request.find(banned) != string::npos) {
            return true;
        }
    }
    return false;
}
// Function to display the UI menu
void displayMenu() {
    cout << "\nProxy Server Menu" << endl;
    cout << "1. View ban list" << endl;
    cout << "2. Add to ban list" << endl;
    cout << "3. Remove from ban list" << endl;
    cout << "4. Save ban list" << endl;
    cout << "5. Exit" << endl;
    cout << "Enter your choice: ";
}

// Function to handle the user interface
void handleUI() {
    string banFile = "ban.txt";
    loadBanList(banFile);

    while (true) {
        displayMenu();
        int choice;
        cin >> choice;

        switch (choice) {
            case 1: {
                lock_guard<mutex> guard(banListMutex);
                cout << "Ban List:" << endl;
                for (const string& entry : banList) {
                    cout << "- " << entry << endl;
                }
                break;
            }
            case 2: {
                cout << "Enter string to ban: ";
                string toBan;
                cin >> toBan;
                {
                    lock_guard<mutex> guard(banListMutex);
                    banList.push_back(toBan);
                }
                cout << "Added to ban list." << endl;
                break;
            }
            case 3: {
                cout << "Enter string to remove from ban list: ";
                string toRemove;
                cin >> toRemove;
                {
                    lock_guard<mutex> guard(banListMutex);
                    // banList.erase(remove(banList.begin(), banList.end(), toRemove), banList.end());
                    banList.erase(
                        std::remove_if(banList.begin(), banList.end(),
                                    [&toRemove](const string& entry) { return entry == toRemove; }),
                        banList.end());

                }
                cout << "Removed from ban list." << endl;
                break;
            }
            case 4: {
                saveBanList(banFile);
                cout << "Ban list saved to " << banFile << "." << endl;
                break;
            }
            case 5:
                cout << "Exiting..." << endl;
                return;
            default:
                cout << "Invalid choice. Try again." << endl;
        }
    }
}
