
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

bool server_shutting_down = false; 
void receive_messages(int client_socket) { // Function to receive messages from server
    char buffer[1024];
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // Clear buffer
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) { // Server disconnected
            cout << "Disconnected from server.\n";
            close(client_socket);
            break;
        }

        string message(buffer);
        if(message == "Server is shutting down.\n"){
          cout << "\nServer is shutting down.\n";
          server_shutting_down = true;
          break;
        }
        cout << "\r" << message << "\nEnter message: " << flush;
    }
}
void send_messages(int client_socket) { // Function to send messages to server
    string message;
    while (true) {
          if (server_shutting_down) {  // Stop sending messages if server is shutting down
            break;
        }
        cout << "Enter message: ";
        getline(cin, message);

        if (message == "exit") {
            if (send(client_socket, message.c_str(), message.size(), 0) == -1) {
                cerr << "Error sending exit message.\n";
            }
            break; // Exit the loop if "exit" is entered
        }

        // Send message to server
        if (send(client_socket, message.c_str(), message.size(), 0) == -1) {
            cerr << "Error sending message.\n";
            break;
        }
    }
}

int main(int argc, char *argv[]) {
    if(argc != 3){
        cerr << "Usage: " << argv[0] << " <IP> <PORT> " << endl;
        return 1;
    }

    const char *ip = argv[1];
    int port = stoi(argv[2]);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0); // Create socket
    if (client_socket == -1) {
        cerr << "Error creating socket.\n";
        return 1;
    }

    sockaddr_in server_addr{}; // Server address structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip); // Connect to localhost

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) { // Connect to server
        cerr << "Error connecting to server.\n";
        return 1;
    }

    string client_name; 
    cout << "Enter your name: ";
    getline(cin, client_name);
    
    // Send client name to the server
    if (send(client_socket, client_name.c_str(), client_name.size(), 0) == -1) {
        cerr << "Error sending client name.\n";
        close(client_socket);
        return 1;
    }

    // Start receiver thread to handle incoming messages
    thread receiver(receive_messages, client_socket); 
    receiver.detach(); // Detach receiver thread to run in background

    // Start sender thread to handle user input and send messages
    thread sender(send_messages, client_socket); 
    sender.join(); // Wait for sender thread to finish

    close(client_socket); // Close socket
    return 0;
}