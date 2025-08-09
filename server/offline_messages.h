#ifndef OFFLINE_MESSAGES_H
#define OFFLINE_MESSAGES_H

#include<iostream>
using namespace std;

void store_offline_message(string username, string message);
vector<string> retrieve_offline_messages(string username);

#endif