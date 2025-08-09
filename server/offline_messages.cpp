#include<fstream>
#include<string>
#include<mutex>
#include<iostream>
#include<vector>

using namespace std;
mutex file_mutex;

void store_offline_message(string username, string message) {
    lock_guard<mutex> lock(file_mutex);
    ofstream file("messages/" + username + ".txt", ios::app);

    if(file.is_open()) {
        file << message << "\n";
        file.close();
    }

    else cerr << "Unable to open file." << endl;
}

vector<string> retrieve_offline_messages(string username) {
    lock_guard<mutex> lock(file_mutex);

    vector<string>ol_messages;
    ifstream file("messages/" + username + ".txt", ios::in);
    string line;
    
    while(getline(file, line)) {
        ol_messages.push_back(line);
    }
    file.close();

    ofstream clear_file("messages/" + username + ".txt", std::ios::trunc);
    clear_file.close();

    return ol_messages;
}