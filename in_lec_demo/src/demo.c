#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9999
#define BUFFER_SIZE 1024

char *generate_random_string(int length) {
    // Define the characters to choose from
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char *random_string = malloc(length + 1); // +1 for the null terminator

    if (random_string) {
        for (int i = 0; i < length; i++) {
            int key = rand() % (sizeof(charset) - 1); // Exclude the null terminator
            random_string[i] = charset[key];
        }
        random_string[length] = '\0'; // Null-terminate the string
    }

    return random_string;
}


// int run_rdma_client(char * file_contents);

void function1() {
    printf("Read!\n");
    const char *params[] = {
    	"./bin/demo",
	"-a", "192.168.1.100",
	"-s", "AAAA"
    };
    int h_argc = 5;
    rdma_client(h_argc, (char**)params);
}

void function2() {
    printf("Write!\n");
    const char *params[] = {
    	"./bin/demo",
	"-a", "192.168.1.100",
	"-s", "AAAA"
    };
    int h_argc = sizeof(params) / sizeof(params[0]);
    rdma_client(h_argc, (char**)params);
}



void handle_request(int client_socket) {
    char buffer[BUFFER_SIZE];
    read(client_socket, buffer, sizeof(buffer) - 1);
    
    const char *response_template = 
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: text/html\r\n"
    "Connection: close\r\n"
    "\r\n"
    "<html>"
    "<head>"
    "<style>"
    "body {"
    "    font-family: Arial, sans-serif;"
    "    background-color: #f4f4f4;"
    "    text-align: center;"
    "    padding: 50px;"
    "}"
    "h1 {"
    "    color: #333;"
    "}"
    "button {"
    "    background-color: #4CAF50;"
    "    color: white;"
    "    border: none;"
    "    padding: 15px 32px;"
    "    text-align: center;"
    "    text-decoration: none;"
    "    display: inline-block;"
    "    font-size: 16px;"
    "    margin: 4px 2px;"
    "    cursor: pointer;"
    "    border-radius: 5px;"
    "    transition: background-color 0.3s;"
    "}"
    "button:hover {"
    "    background-color: #45a049;"
    "}"
    "</style>"
    "</head>"
    "<body>"
    "<h1>Simple HTTP Server</h1>"
    "<button onclick=\"location.href='/function1'\">Read Some Data</button>"
    "<button onclick=\"location.href='/function2'\">Write Some Data</button>"
    "</body>"
    "</html>";
    
    // Check the request for function calls
    if (strstr(buffer, "GET /function1") != NULL) {
        function1();
    } else if (strstr(buffer, "GET /function2") != NULL) {
        function2();
    }

    // Send the response
    write(client_socket, response_template, strlen(response_template));
    close(client_socket);
}

int main() {
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Create socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Attach socket to the port
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    // address.sin_addr.s_addr = INADDR_ANY;
    address.sin_addr.s_addr = inet_addr("10.29.233.75");
    address.sin_port = htons(PORT);

    // Bind the socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server is listening on port %d\n", PORT);

    while (1) {
        // Accept a new connection
        if ((client_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Handle the request
        handle_request(client_socket);
    }

    return 0;
}
