#ifndef BLOCK_H
#define BLOCK_H

#include <iostream>
using namespace std;

bool add_to_block_list(string username, string blocked_user);
bool remove_from_block_list(string username, string blocked_user);
bool is_user_blocked(string username, string blocked_user);

#endif