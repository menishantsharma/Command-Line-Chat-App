#ifndef GROUPS_H
#define GROUPS_H

#include<iostream>
#include<vector>
using namespace std;

bool group_exits(string group_name);
void create_group(string group_name);
void add_member_to_group(string group_name, string username);
vector<string>get_group_members(string group_name);
bool is_user_in_group(string group_name, string username);
void remove_member_from_group(string group_name, string username);

vector<string>get_group_admins(string group_name);
bool is_user_admin(string group_name, string username);
void add_admin_to_group(string group_name, string username);
void remove_admin_from_group(string group_name, string username);

#endif