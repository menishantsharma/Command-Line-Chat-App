#ifndef AUTH_H
#define AUTH_H

#include <string>
using namespace std;

bool register_user(string username, string password);
bool login_user(string username, string password);

#endif