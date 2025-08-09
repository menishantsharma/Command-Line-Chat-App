#ifndef USER_H
#define USER_H

void register_user(int client_sock);
void login_user(int client_sock);
void chat_with_user(int client_sock);

#endif