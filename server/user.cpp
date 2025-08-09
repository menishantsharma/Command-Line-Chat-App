#include <unordered_map>
#include <mutex>
#include<sys/socket.h>
#include "user.h"

using namespace std;

unordered_map<string, int>clients;
mutex mtx;

void register_user_socket(string username, int client_sock) {
    mtx.lock();
    if(clients.find(username) == clients.end()) clients[username] = client_sock;
    mtx.unlock();
}

void unregister_user_socket(string username) {
    mtx.lock();
    if(clients.find(username) != clients.end()) clients.erase(username);
    mtx.unlock();
}

int get_user_socket(string username) {
    mtx.lock();
    int sock = -1;
    if(clients.find(username) != clients.end()) sock = clients[username];
    mtx.unlock();
    return sock;
}

string format_chat_msg(string msg, string sender) {
    return "CHAT " + sender + ": " + msg; 
}

string format_group_msg(string msg, string group_name, string sender) {
    return "GROUP_" + group_name + " " + sender + ": " + msg;
}

bool send_message_to_user(int client_sock, string recipient, string message) {
    int recipient_sock = get_user_socket(recipient);
    if(recipient_sock == -1) return false;
    send(recipient_sock, message.c_str(), message.size(), 0);
    return true;
}
