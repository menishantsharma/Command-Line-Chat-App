#include <iostream>
#include <sys/socket.h>
#include <limits>
#include <vector>
#include "user.h"

using namespace std;

void register_user(int client_sock) {
    string username;
    string password;

    cout << "Enter Username: ";
    cin >> username;

    cout << "Enter password: ";
    cin >> password;

    string command = "REGISTER " + username + " " + password;
    send(client_sock, command.c_str(), command.size(), 0);
}

void login_user(int client_sock) {
    string username;
    cout << "Enter username: ";
    cin >> username;

    string password;
    cout << "Enter password: ";
    cin >> password;
    
    string command = "LOGIN " + username + " " + password;
    send(client_sock, command.c_str(), command.size(), 0);
}   

void chat_with_user(int client_sock) {
    string recipient;
    string message;

    cout << "Enter the recipient username: ";
    cin >> recipient;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    while(1) {
        cout << "Enter message or exit: ";
        getline(cin, message);

        if(message == "exit") break;

        string command = "CHAT " + recipient + " " + message;

        send(client_sock, command.c_str(), command.size(), 0);
    }
}