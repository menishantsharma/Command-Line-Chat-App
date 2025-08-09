#include<fstream>
#include<iostream>
#include<vector>

using namespace std;

vector<string> load_block_list(string username) {
    vector<string>users;
    ifstream infile("block_lists/" + username + "_block_list.txt", ios::in);
    string line;

    while(getline(infile, line)) users.push_back(line);
    infile.close();
    return users;
}

bool is_user_blocked(string username, string blocked_user) {
    vector<string>block_list = load_block_list(username);
    for(auto &user: block_list) if(user == blocked_user) return true;
    return false;
}

bool add_to_block_list(string username, string blocked_user) {
    if(is_user_blocked(username, blocked_user)) return false;
    ofstream file("block_lists/" + username + "_block_list.txt", ios::app);
    file << blocked_user << endl;
    return true;
}

bool remove_from_block_list(string username, string blocked_user) {
    if(!is_user_blocked(username, blocked_user)) return false;
    vector<string>users = load_block_list(username);

    ofstream file("block_lists/" + username + "_block_list.txt", ios::trunc);
    for(auto &user: users) if(user != blocked_user) file << user << endl;
    file.close();
    return true;
}

