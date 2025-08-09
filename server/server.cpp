#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <cstring>
#include <map>
#include <sstream>
#include <mutex>
#include <queue>
#include "auth.h"
#include "groups.h"
#include "user.h"
#include "offline_messages.h"
#include "block.h"

using namespace std;

#define BUFFER_SIZE 128

void error(string msg) {
    cout << msg << endl;
    exit(EXIT_FAILURE);
}

void send_msg_to_client(int client_fd, string msg) {
    send(client_fd, msg.c_str(), msg.size(), 0);
}

void handle_client(int client_sock) {
    char buffer[BUFFER_SIZE];
    string username;

    while(1) {
        memset(buffer, 0, sizeof(buffer));
        int cnt = recv(client_sock, buffer, BUFFER_SIZE, 0);

        if(cnt <= 0) {
            if(!username.empty()) unregister_user_socket(username);
            cout << "CLIENT_CLOSED" << endl;
            close(client_sock);
            return;
        }

        buffer[cnt] = '\0';

        string command(buffer);
        istringstream iss(command);
        iss >> command;

        if(command == "REGISTER") {
            string password;
            iss >> username;
            iss >> password;
            if(username.empty()) send_msg_to_client(client_sock, "CTRL_USERNAME_MISSING");
            else if(password.empty()) send_msg_to_client(client_sock, "CTRL_PASSWORD_MISSING");
            else if(register_user(username, password)) send_msg_to_client(client_sock, "CTRL_REGISTERED");
            else send_msg_to_client(client_sock, "CTRL_USERNAME_TAKEN");
        }

        else if(command == "LOGIN") {
            string password;
            iss >> username;
            iss >> password;

            if(username.empty()) send_msg_to_client(client_sock, "CTRL_USERNAME_MISSING");
            else if(password.empty()) send_msg_to_client(client_sock, "CTRL_PASSWORD_MISSING");
            else if(login_user(username, password)) {
                send_msg_to_client(client_sock, "CTRL_LOGGED_IN");
                register_user_socket(username, client_sock);
                
                vector<string>ol_messages;
                ol_messages = retrieve_offline_messages(username);
                
                for(auto message: ol_messages) {
                    sleep(0.01);
                    send_message_to_user(client_sock, username, message);
                }
            }
            
            else send_msg_to_client(client_sock, "CTRL_LOGIN_FAILED");
        }

        else if(command == "LOGOUT") {
            if(!username.empty()) {
                unregister_user_socket(username);
                send_msg_to_client(client_sock, "CTRL_LOGGED_OUT");
                username.clear();
            }

            else send_msg_to_client(client_sock, "CTRL_LOGOUT_FAILED");
        }

        else if(command == "CHAT") {
            if(!username.empty()) {
                string recipient;
                iss >> recipient;

                string message;
                getline(iss, message);

                message = format_chat_msg(message, username);

                if(recipient.empty()) send_msg_to_client(client_sock, "CTRL_RECIPIENT_MISSING");
                else if(username == recipient) send_msg_to_client(client_sock, "CTRL_RECIPIENT_CANT_BE_YOU");
                else if(is_user_blocked(recipient, username)) send_msg_to_client(client_sock, "CTRL_USER_BLOCKED_YOU");
                else if(send_message_to_user(client_sock, recipient, message)) send_msg_to_client(client_sock, "CTRL_SENT");
                else {
                    store_offline_message(recipient, message);
                    send_msg_to_client(client_sock, "CTRL_USER_OFFLINE_OR_NOT_EXISTS");
                }
            }

            else send_msg_to_client(client_sock, "CTRL_LOGIN_REQUIRED");
        }
        
        else if(command == "CREATE_GROUP") {
            if(!username.empty()) {
                string group_name;
                iss >> group_name;

                if(group_name.empty()) send_msg_to_client(client_sock, "CTRL_GROUP_NAME_MISSING");
                else if(group_exits(group_name)) send_msg_to_client(client_sock, "CTRL_GROUP_NAME_TAKEN");
                else {
                    create_group(group_name);
                    add_member_to_group(group_name, username);
                    add_admin_to_group(group_name, username);
                    send_msg_to_client(client_sock, "CTRL_GROUP_CREATED");
                }
            }
            else send_msg_to_client(client_sock, "CTRL_LOGIN_REQUIRED");
        }

        else if(command == "JOIN_GROUP") {
            if(!username.empty()) {
                string group_name;
                iss >> group_name;

                if(group_name.empty()) send_msg_to_client(client_sock, "CTRL_GROUP_NAME_MISSING");
                else if(!group_exits(group_name)) send_msg_to_client(client_sock, "CTRL_GROUP_NOT_EXISTS");
                else {
                    if(is_user_in_group(group_name, username)) send_msg_to_client(client_sock, "CTRL_ALREADY_IN_GROUP");
                    else {
                        add_member_to_group(group_name, username);
                        send_msg_to_client(client_sock, "CTRL_GROUP_JOINED");
                    }
                }
            }
            else send_msg_to_client(client_sock, "CTRL_LOGIN_REQUIRED");
        }

        else if(command == "LEAVE_GROUP") {
            if(!username.empty()) {
                string group_name;
                iss >> group_name;
                
                if(group_name.empty()) send_msg_to_client(client_sock, "CTRL_GROUP_NAME_MISSING");
                else if(!group_exits(group_name)) send_msg_to_client(client_sock, "CTRL_GROUP_NOT_EXISTS");
                else {
                    if(!is_user_in_group(group_name, username)) send_msg_to_client(client_sock, "CTRL_NOT_IN_GROUP");
                    else {
                        if(is_user_admin(group_name, username)) remove_admin_from_group(group_name, username);
                        remove_member_from_group(group_name, username);
                        send_msg_to_client(client_sock, "CTRL_GROUP_LEFT");
                    }
                }
            }

            else send_msg_to_client(client_sock, "CTRL_LOGIN_REQUIRED");
        }

        else if(command == "GROUP_CHAT") {
            if(!username.empty()) {
                string group_name;
                iss >> group_name;
                string message;
                getline(iss, message);

                if(group_name.empty()) {
                    send_msg_to_client(client_sock, "CTRL_GROUP_NAME_MISSING");
                    continue;
                }

                if(!is_user_in_group(group_name, username)) {
                    send_msg_to_client(client_sock, "CTRL_NOT_MEMBER_OF_GROUP");
                    continue;
                }

                vector<string>members = get_group_members(group_name);

                message = format_group_msg(message, group_name, username);
                
                for(auto &member: members) {
                    if(member == username) continue;
                    if(!send_message_to_user(client_sock, member, message)) store_offline_message(member, message);
                }

                send_msg_to_client(client_sock, "CTRL_GROUP_MESSAGE_SENT");
            }

            else send_msg_to_client(client_sock, "CTRL_LOGIN_REQUIRED");
        }

        else if(command == "ADD_MEMBER") {
            string group_name;
            string member_to_add;
            iss >> group_name;
            iss >> member_to_add;

            if(!username.empty()) {
                if(group_name.empty()) send_msg_to_client(client_sock, "CTRL_GROUP_NAME_MISSING");
                else if(member_to_add.empty()) send_msg_to_client(client_sock, "CTRL_MEMBER_TO_ADD_MISSING");
                else if(!group_exits(group_name)) send_msg_to_client(client_sock, "CTRL_GROUP_NOT_EXISTS");
                else if(is_user_blocked(member_to_add, username)) send_msg_to_client(client_sock, "CTRL_USER_BLOCKED_YOU");
                else if(!is_user_admin(group_name, username)) send_msg_to_client(client_sock, "CTRL_YOU_ARE_NOT_ADMIN");
                else if(is_user_in_group(group_name, member_to_add)) send_msg_to_client(client_sock, "CTRL_ALREADY_IN_GROUP");
                else {
                    add_member_to_group(group_name, member_to_add);
                    send_msg_to_client(client_sock, "CTRL_MEMBER_ADDED");
                }
            }

            else send_msg_to_client(client_sock, "CTRL_LOGIN_REQUIRED");
        }

        else if(command == "REMOVE_MEMBER") {
            string group_name;
            string member_to_remove;
            iss >> group_name;
            iss >> member_to_remove;

            if(!username.empty()) {
                if(group_name.empty()) send_msg_to_client(client_sock, "CTRL_GROUP_NAME_MISSING");
                else if(member_to_remove.empty()) send_msg_to_client(client_sock, "CTRL_MEMBER_TO_REMOVE_MISSING");
                else if(!group_exits(group_name)) send_msg_to_client(client_sock, "CTRL_GROUP_NOT_EXISTS");
                else if(!is_user_admin(group_name, username)) send_msg_to_client(client_sock, "CTRL_YOU_ARE_NOT_ADMIN");
                else if(!is_user_in_group(group_name, member_to_remove)) send_msg_to_client(client_sock, "CTRL_ALREADY_NOT_IN_GROUP");
                else if(is_user_admin(group_name, member_to_remove)) send_msg_to_client(client_sock, "CTRL_CANT_REMOVE_ADMIN");
                else {
                    remove_member_from_group(group_name, member_to_remove);
                    send_msg_to_client(client_sock, "CTRL_MEMBER_REMOVED");
                }
            }

            else send_msg_to_client(client_sock, "CTRL_LOGIN_REQUIRED");
        }

        else if(command == "PROMOTE") {
            string group_name;
            string member_to_promote;
            iss >> group_name;
            iss >> member_to_promote;

            if(!username.empty()) {
                if(group_name.empty()) send_msg_to_client(client_sock, "CTRL_GROUP_NAME_MISSING");
                else if(member_to_promote.empty()) send_msg_to_client(client_sock, "CTRL_MEMBER_TO_PROMOTE_MISSING");
                else if(!group_exits(group_name)) send_msg_to_client(client_sock, "CTRL_GROUP_NOT_EXISTS");
                else if(!is_user_admin(group_name, username)) send_msg_to_client(client_sock, "CTRL_YOU_ARE_NOT_ADMIN");
                else if(!is_user_in_group(group_name, member_to_promote)) send_msg_to_client(client_sock, "CTRL_MEMBER_NOT_IN_GROUP");
                else if(is_user_admin(group_name, member_to_promote)) send_msg_to_client(client_sock, "CTRL_MEMBER_ALREADY_ADMIN");
                else {
                    add_admin_to_group(group_name, member_to_promote);
                    send_msg_to_client(client_sock, "CTRL_MEMBER_PROMOTED");
                }
            }

            else send_msg_to_client(client_sock, "CTRL_LOGIN_REQUIRED");
        }

        else if(command == "DEMOTE") {
            string group_name;
            string member_to_demote;
            iss >> group_name >> member_to_demote;

            if(!username.empty()) {
                if(group_name.empty()) send_msg_to_client(client_sock, "CTRL_GROUP_NAME_MISSING");
                else if(member_to_demote.empty()) send_msg_to_client(client_sock, "CTRL_MEMBER_TO_DEMOTE_MISSING");
                else if(!group_exits(group_name)) send_msg_to_client(client_sock, "CTRL_GROUP_NOT_EXISTS");
                else if(!is_user_admin(group_name, username)) send_msg_to_client(client_sock, "CTRL_YOU_ARE_NOT_ADMIN");
                else if(!is_user_in_group(group_name, member_to_demote)) send_msg_to_client(client_sock, "CTRL_MEMBER_NOT_IN_GROUP");
                else if(!is_user_admin(group_name, member_to_demote)) send_msg_to_client(client_sock, "CTRL_MEMBER_ALREADY_NOT_ADMIN");
                else {
                    remove_admin_from_group(group_name, member_to_demote);
                    send_msg_to_client(client_sock, "CTRL_MEMBER_DEMOTED");
                }
            }
            else send_msg_to_client(client_sock, "CTRL_LOGIN_REQUIRED");
        }

        else if(command == "BLOCK_USER") {
            if(!username.empty()) {
                string blocked_user;
                iss >> blocked_user;

                if(blocked_user.empty()) send_msg_to_client(client_sock, "CTRL_BLOCK_USERNAME_MISSING");
                else if(add_to_block_list(username, blocked_user)) send_msg_to_client(client_sock, "CTRL_USER_BLOCKED");
                else send_msg_to_client(client_sock, "CTRL_USER_ALREADY_BLOCKED");
            }
            else send_msg_to_client(client_sock, "CTRL_LOGIN_REQUIRED");
        }

        else if(command == "UNBLOCK_USER") {
            if(!username.empty()) {
                string blocked_user;
                iss >> blocked_user;

                if(blocked_user.empty()) send_msg_to_client(client_sock, "CTRL_BLOCK_USERNAME_MISSING");
                else if(remove_from_block_list(username, blocked_user)) send_msg_to_client(client_sock, "CTRL_USER_UNBLOCKED");
                else send_msg_to_client(client_sock, "CTRL_USER_ALREADY_UNBLOCKED");
            }
            
            else send_msg_to_client(client_sock, "CTRL_LOGIN_REQUIRED");
        }

        else {
            send_msg_to_client(client_sock, "CTRL_INVALID_COMMAND");
        }
    }
}

int main(int argc, char* argv[]) {
    if(argc != 2) error("USAGE: ./server <port>");

    int port = atoi(argv[1]);
    int ret;

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if(server_sock < 0) error("Failed to create socket.");

    sockaddr_in server_addr;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    
    ret = bind(server_sock, (sockaddr*)&server_addr, sizeof(server_addr));
    if(ret < 0) error("Failed to bind.");

    ret = listen(server_sock, 128);
    if(ret < 0) error("Failed to listen.");

    cout << "Server Started." << endl;

    while(true) {
        int client_socket = accept(server_sock, nullptr, nullptr);
        thread t(handle_client, client_socket);
        t.detach();
    }

    close(server_sock);
    return 0;
}