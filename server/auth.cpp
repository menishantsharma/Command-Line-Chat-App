#include <iostream>
#include <openssl/sha.h>
#include <fstream>
#include <sstream>
#include <iomanip>

using namespace std;

string hash_password(string password) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(password.c_str()), password.length(), hash);
    std::ostringstream oss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << setw(2) << setfill('0') << std::hex << static_cast<int>(hash[i]);
    }
    return oss.str();
}

bool register_user(string username, string password) {
    ifstream infile("users.txt");
    string line, stored_username;

    if(!infile.is_open()) return false;
    
    while(getline(infile, line)) {
        istringstream iss(line);
        iss >> stored_username;
        if(username == stored_username) return false;
    }

    infile.close();

    ofstream outfile("users.txt", ios::app);
    if(!outfile) return false;
    outfile << username << " " << hash_password(password) << endl;

    outfile.close();
    return true;
}

bool login_user(string username, string password) {
    ifstream infile("users.txt");

    string line, stored_username, stored_password;
    password = hash_password(password);

    while(getline(infile, line)) {
        istringstream iss(line);
        iss >> stored_username;
        iss >> stored_password;

        if(stored_username == username) {
            infile.close();
            return password == stored_password;
        }
    }

    infile.close();
    return false;
}