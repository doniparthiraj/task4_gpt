#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdbool.h>

#define PORT 1234
#define BUFFER_SIZE 1024

int main() 
{
    int client_socket;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    bool isChat = 0, isGpt = 0;
    // Create socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) 
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Connect to server
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) 
    {
        perror("Connect failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server.\n");

    pid_t child_pid = fork();
    if (child_pid < 0) 
    {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }

    if (child_pid == 0) 
    {
        while (1) 
        {
            memset(buffer, 0, BUFFER_SIZE); // Clear buffer
            ssize_t receive = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (strncmp(buffer, "stupidbot> bye", 14) == 0)
            {
                printf("%s\n", buffer);
            }
            else if (strncmp(buffer, "stupidbot>", 10) == 0)
            {
                printf("%s\nuser>", buffer);
                fflush(stdout);
            }
            else if (strncmp(buffer, "gpt2bot> bye", 12) == 0)
            {
                printf("%s\n", buffer);
            }
            else if (strncmp(buffer, "gpt2bot>", 8) == 0)
            {
                printf("%s\n>", buffer);
                fflush(stdout);
            }
            
            else if (receive > 0) 
            {
                printf("%s", buffer);
            } 
            else if (receive == 0) 
            {
                printf("Disconnected from server.\n");
                break;
            } 
            else 
            {
                perror("Receive failed");
                break;
            }
        }
    } 
    else 
    { // Parent process for writing messages to server
        while (1) 
        {
            memset(buffer, 0, BUFFER_SIZE);
            fgets(buffer, BUFFER_SIZE, stdin);
            if (strncmp(buffer, "/logout", 7) == 0) 
            {
                send(client_socket, buffer, strlen(buffer), 0);
                break;
            }
            else if (strncmp(buffer, "/chatbot_v2 login", 17) == 0)
            {
                isGpt = 1;
                send(client_socket, buffer, strlen(buffer), 0);
            }
            else if (strncmp(buffer, "/chatbot_v2 logout", 18) == 0)
            {
                send(client_socket, buffer, strlen(buffer), 0);
                printf("\n");
                isGpt = 0;
            }
            else if (strncmp(buffer, "/chatbot login", 14) == 0)
            {
                isChat = 1;
                send(client_socket, buffer, strlen(buffer), 0);
            } 
            else if (strncmp(buffer, "/chatbot logout", 15) == 0)
            {
                send(client_socket, buffer, strlen(buffer), 0);
                printf("\n");
                isChat = 0;
            }
            else if (isGpt == 1)
            {
                char usr[BUFFER_SIZE + 1];
                strcpy(usr, ">");
                strcat(usr, buffer);
                send(client_socket, usr, strlen(usr), 0);
            }
            else if (isChat == 1)
            {
                char usr[BUFFER_SIZE + 5];
                strcpy(usr, "user>");
                strcat(usr, buffer);
                send(client_socket, usr, strlen(usr), 0);
            }
            else 
            {
                if (send(client_socket, buffer, strlen(buffer), 0) < 0) 
                {
                    perror("Send failed");
                    break;
                }
            }
        }

        // Terminate child process when done
        kill(child_pid, SIGKILL);
    }

    // Close the socket before exiting
    close(client_socket);
    printf("Connection closed.\n");

    return 0;
}
