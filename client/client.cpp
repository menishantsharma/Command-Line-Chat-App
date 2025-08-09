#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <limits> 
#include <unistd.h>
#include <cstring>
#include <vector>
#include <mutex>
#include <sstream>
#include <thread>
#include <condition_variable>
#include "user.h"

using namespace std;

#define BUFFER_SIZE 128
#define MAX_INBOX_SIZE 1000

mutex mtx;
condition_variable cv;
bool ready = false;


void error(string msg) {
    cout << msg << endl;
    exit(EXIT_FAILURE);
}

vector<string>inbox;
mutex inbox_mtx;

bool isLoggedIn = false;
bool isConnected = true;

void receive_messages(int client_sock) {
    char buffer[BUFFER_SIZE];

    while(isConnected) {
        memset(buffer, 0, BUFFER_SIZE);
        int cnt = recv(client_sock, buffer, BUFFER_SIZE-1, 0);
        if(cnt <= 0) {
            cout << "Connection closed." << endl;
            isConnected = false;
            break;
        }
        
        string message(buffer);

        if(message.substr(0,4) == "CTRL") {
            if(message == "CTRL_LOGGED_IN") isLoggedIn = true;
            lock_guard<mutex> lock(mtx);
            cout << message << endl;
            ready = true;
            cv.notify_one();
        }
        
        else if(message.substr(0,4) == "CHAT" || message.substr(0,5) == "GROUP") {
            lock_guard<mutex> lock(inbox_mtx);
            if(inbox.size() >= MAX_INBOX_SIZE) inbox.erase(inbox.begin());
            inbox.push_back(message);
        }
    }
}

void wait_for_output() {
    unique_lock<mutex> lock(mtx);
    cv.wait(lock, [] { return ready; });
    ready = false;
}

void send_command(int client_sock, string command) {
    send(client_sock, command.c_str(), command.size(), 0);
    wait_for_output();
}

int main(int argc, char *argv[]) {
    if(argc != 3) error("USAGE: ./client <server_ip> <port>");

    char* server_ip = argv[1];
    int port = atoi(argv[2]);
    int ret;

    int client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(client_sock < 0) error("Failed to create socket.");

    sockaddr_in server_addr;
    server_addr.sin_port = htons(port);
    server_addr.sin_family = AF_INET;
    ret = inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    if(ret < 0) error("Invalid address.");

    ret = connect(client_sock, (sockaddr*)&server_addr, sizeof(server_addr));
    if(ret < 0) error("Failed to connect.");

    thread receiverThread(receive_messages, client_sock);
    
    while(isConnected) {
        string command;
        cout << "> ";
        getline(cin, command);

        istringstream iss(command);
        iss >> command;

        for(char &c: command) c = toupper(c);

        if(command == "REGISTER") {
            if(isLoggedIn) {
                cout << "ALREADY_LOGGED_IN" << endl;
                continue;
            }

            string username, password;
            iss >> username >> password;
            command = "REGISTER " + username + " " + password;
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "LOGIN") {
            if(isLoggedIn) {
                cout << "ALREADY_LOGGED_IN" << endl;
                continue;
            }

            string username, password;
            iss >> username >> password;
            command = "LOGIN " + username + " " + password;
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "CHAT") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }

            string recipient, message;
            iss >> recipient;
            getline(iss, message);
            command = "CHAT " + recipient + " " + message;
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "INBOX") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }

            cout << "--------------INBOX--------------\n";
            lock_guard<mutex> lock(inbox_mtx);
            if(inbox.empty()) cout << "INBOX_EMPTY" << endl;
            else for(auto msg: inbox) cout << msg << endl;
            cout << "---------------------------------\n";
        }

        else if(command == "CLEAR_INBOX") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }

            lock_guard<mutex> lock(inbox_mtx);
            inbox.clear();
            cout << "INBOX_CLEARED" << endl;
        }
        
        else if(command == "CREATE_GROUP") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }

            string group_name;
            iss >> group_name;
            command = "CREATE_GROUP " + group_name; 
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "JOIN_GROUP") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }

            string group_name;
            iss >> group_name;
            command = "JOIN_GROUP " + group_name;
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "LEAVE_GROUP") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }

            string group_name;
            iss >> group_name;
            command = "LEAVE_GROUP " + group_name;
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "GROUP_CHAT") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }

            string group_name;
            iss >> group_name;

            string message;
            getline(iss, message);
            command = "GROUP_CHAT " + group_name + " " + message;
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "BLOCK") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }
            string blocked_user;
            iss >> blocked_user;
            command = "BLOCK_USER " + blocked_user;
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "UNBLOCK") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }
            string blocked_user;
            iss >> blocked_user;
            command = "UNBLOCK_USER " + blocked_user;
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "LOGOUT") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }

            isLoggedIn = false;
            command = "LOGOUT";
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "ADD_MEMBER") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }

            string group_name;
            string member_to_add;
            iss >> group_name >> member_to_add;

            command = "ADD_MEMBER " + group_name + " " + member_to_add;
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "REMOVE_MEMBER") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }

            string group_name;
            string member_to_remove;
            iss >> group_name >> member_to_remove;

            command = "REMOVE_MEMBER " + group_name + " " + member_to_remove;
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "PROMOTE") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }

            string group_name;
            string member_to_promote;
            iss >> group_name >> member_to_promote;

            command = "PROMOTE " + group_name + " " + member_to_promote;
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "DEMOTE") {
            if(!isLoggedIn) {
                cout << "LOGIN_REQUIRED" << endl;
                continue;
            }

            string group_name;
            string member_to_demote;
            iss >> group_name >> member_to_demote;

            command = "DEMOTE " + group_name + " " + member_to_demote;
            // send(client_sock, command.c_str(), command.size(), 0);
            send_command(client_sock, command);
        }

        else if(command == "EXIT") {
            isConnected = false;
            shutdown(client_sock, SHUT_RDWR);
            break;
        }

        else if(command == "HELP") {
            cout << "Available Commands:\n";
            cout << "-------------------\n";
            cout << "REGISTER <username> <password>       - Register a new user.\n";
            cout << "LOGIN <username> <password>          - Log in to the server.\n";
            cout << "CHAT <recipient> <message>           - Send a private message to a user.\n";
            cout << "INBOX                                - View your inbox.\n";
            cout << "CLEAR_INBOX                          - Clear all messages in your inbox.\n";
            cout << "CREATE_GROUP <group_name>            - Create a new group.\n";
            cout << "JOIN_GROUP <group_name>              - Join an existing group.\n";
            cout << "LEAVE_GROUP <group_name>             - Leave a group.\n";
            cout << "GROUP_CHAT <group_name> <message>    - Send a message to a group.\n";
            cout << "BLOCK <username>                     - Block a user from messaging you.\n";
            cout << "UNBLOCK <username>                   - Unblock a user.\n";
            cout << "LOGOUT                               - Log out from the server.\n";
            cout << "ADD_MEMBER <group_name> <username>   - Add a member to a group (admin only).\n";
            cout << "REMOVE_MEMBER <group_name> <username> - Remove a member from a group (admin only).\n";
            cout << "PROMOTE <group_name> <username>      - Promote a member to admin (admin only).\n";
            cout << "DEMOTE <group_name> <username>       - Demote an admin to a regular member (admin only).\n";
            cout << "EXIT                                 - Exit the application.\n";
            cout << "HELP                                 - Show this help menu.\n";
            cout << "-------------------\n";
        }
    
        else {
            cout << "INVALID_COMMAND" << endl;
        }
    }

    receiverThread.join();

    close(client_sock);
    cout << "CLIENT_CLOSED" << endl;
    return 0;
}