## Project Overiew

In simple terms, this is a cli chat applications where we can chat, create groups, block user, promote admins and much more.

## Dependencies

- pthread (POSIX Threads Library)
- OpenSSL (for libcrypto)

## How to Run

Open two different terminal one to run server and one to run client.

# Terminal 1

How to run server

cd server
g++ server.cpp user.cpp auth.cpp offline_messages.cpp groups.cpp block.cpp -o server -lpthread -lcrypto
./server 9999

# Terminal 2

How to compiler client

cd client
g++ client.cpp user.cpp -o client -pthread
./client 127.0.0.1 9999