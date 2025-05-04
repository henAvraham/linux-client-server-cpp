
#include "std_lib_facilities.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <algorithm>
#include <thread>
#include <map>


map<string, int> clients; // Map to store client names and their sockets
bool server_shutting_down = false; //global flag for server shutdown

void broadcast_message(const string &message, int sender_socket){
    for(auto& client : clients){
        if(client.second != sender_socket) { // Don't send to the sender
              send(client.second, message.c_str(), message.size() ,0);
        }
    }
}


void handle_client(int client_socket){
    char buffer[1024];
    string name;
    //name of client
    memset(buffer, 0, sizeof(buffer));
    if(recv(client_socket, buffer, sizeof(buffer), 0) > 0){
        name = buffer;
    }else{
        close(client_socket);
        return;
    }

    cout << "Client connected: " << name << endl;
    clients[name] = client_socket;  // Add client to the map

    while(true){
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(client_socket, buffer, sizeof(buffer),0);

        if(bytes_received <= 0 || string(buffer).find("exit") == 0){
            cout << "Client disconnected: " << name << endl;
            close(client_socket);
            clients.erase(name);  // Remove client from the map
            break;
        }
        string message(buffer);
        if(message.find("send") == 0){
        // Parse the message to extract the recipient and the actual message
            size_t space_pos = message.find(' ',5); //the space of the word after 'send'
            string recipient = message.substr(5, space_pos -5); //cut the message - to know the client name
            string msg = message.substr(space_pos + 1); // cut the message - to know the content in the messsage
              // Search for the recipient in the clients map
            auto it = clients.find(recipient);
            if (it != clients.end()) {  // If recipient is found
                // Construct the "Incoming message" and send it to the recipient
                string incoming_msg = "Incoming message from " + name + ": " + msg;
                send(it->second, incoming_msg.c_str(), incoming_msg.size(), 0);
                cout << name << " sent a message to " << recipient << endl;
            } else {
                // If the recipient is not found, inform the sender
                send(client_socket, "Recipient not found.\n", 21, 0);
            }
        } else {
            // Broadcast the message to all clients except the sender
            cout << name << ": " << message << endl;
            broadcast_message(message, client_socket);
        }
    }
}
  //listen for server shutdown command frome user
    void listen_for_exit(int server_socket){
    string command;
    while (true)
    {
        getline(cin, command);
        if(command == "exit"){
            server_shutting_down = true;
            for(auto& client : clients){
                string shutdown = "Server is shutting down.\n";
                send(client.second, shutdown.c_str(), shutdown.size(),0);
                close(client.second);
            }
            clients.clear();
            cout << "shutting down server and Disconnceted the clients..." << endl;
            close(server_socket);
            exit(0);
        }

    }
    }


int main(int argc, char *argv[]){
    if(argc != 3){
        cerr << "Usage: " << argv[0] << " <IP> <PORT> " << endl;
        return 1;
    }
    const char *ip = argv[1];
    int port = stoi(argv[2]);

    int server_socket = socket(AF_INET, SOCK_STREAM,0);
    if(server_socket == -1){
        cerr << "Failed to create socket." << endl;
        return 1;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    if(bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == -1){
        cerr << "Bind failed." << endl;
        close(server_socket);
        return 1;
    }
    if(listen(server_socket,10) == -1){
        cerr << "Listen faile." << endl;
        close(server_socket);
        return 1;
    }
    cout << "Server listing on " << ip << ": " << port << endl;

    thread exit_thread(listen_for_exit,server_socket);
    exit_thread.detach();

    //start accepting client connections in a separate thread
      while(true){
        sockaddr_in client_addr;
        socklen_t client_size = sizeof(client_addr);
        int client_socket = accept(server_socket,(sockaddr *)&client_addr,&client_size);

        if(server_shutting_down) break;

        if(client_socket == -1){
            cerr << "Failed to accept connection." << endl;
            continue;
        }
        //create a new thread to handle the new client
        thread(handle_client, client_socket).detach();
    }
    
    
    return 0;

    }

