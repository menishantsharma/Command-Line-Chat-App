#include <fstream>
#include <sys/stat.h>
#include<vector>
#include<iostream>

using namespace std;

bool group_exits(string group_name) {
    ifstream group("groups/" + group_name + ".txt");
    return group.good();
}

void create_group(string group_name) {
    ofstream group("groups/" + group_name + ".txt");
    group.close();
}

void add_member_to_group(string group_name, string username) {
    ofstream group("groups/" + group_name + ".txt", ios::app);
    group << username << "\n";
    group.close();
}

vector<string> get_group_members(string group_name) {
    vector<string>members;
    ifstream group("groups/" + group_name + ".txt");
    string line;

    if(group.is_open()) {
        while(getline(group, line)) members.push_back(line);
        group.close();
    }

    return members;
}

bool is_user_in_group(string group_name, string username) {
    vector<string>members = get_group_members(group_name);
    for(auto &member: members) {
        if(member == username) return true;
    }
    return false;
}

void remove_member_from_group(string group_name, string username) {
    ifstream group("groups/" + group_name + ".txt");
    vector<string>members;
    string line;

    while(getline(group, line)) if(line!=username) members.push_back(line);
    group.close();

    ofstream group_out("groups/" + group_name + ".txt", ios::trunc);
    for(auto &member: members) group_out << member << "\n";
    group_out.close();
}

vector<string>get_group_admins(string group_name) {
    ifstream file("admins/" + group_name + ".txt");
    vector<string>admins;
    string line;
    while(getline(file, line)) admins.push_back(line);
    file.close();
    return admins;
}

bool is_user_admin(string group_name, string username) {
    vector<string>admins = get_group_admins(group_name);
    for(auto &admin: admins) if(admin == username) return true;
    return false;
}

void add_admin_to_group(string group_name, string username) {
    ofstream file("admins/" + group_name + ".txt", ios::app);
    file << username << endl;
    file.close();
}

void remove_admin_from_group(string group_name, string username) {
    vector<string>admins = get_group_admins(group_name);
    ofstream file("admins/" + group_name + ".txt", ios::trunc);
    for(auto &admin: admins) {
        if(admin != username) file << admin << endl;
    }

    file.close();
}