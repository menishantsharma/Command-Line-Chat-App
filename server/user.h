#ifndef USER_H
#define USER_H

#include <string>
using namespace std;

void register_user_socket(string username, int socket);
void unregister_user_socket(string username);
int get_user_socket(string username);
bool send_message_to_user(int client_sock, string username, string message);
string format_chat_msg(string msg, string sender);
string format_group_msg(string msg, string group_name, string sender);

#endif