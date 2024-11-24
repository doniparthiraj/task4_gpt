#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <uuid/uuid.h>
#include <errno.h>
#include <sys/wait.h>


#define PORT 1234
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

// Structure to hold client details
typedef struct {
    int socket;
    char id[37]; // UUID4 string length
} client_t;

client_t clients[MAX_CLIENTS];
int client_count = 0;

void remove_newline(char* str) 
{
    char* newline = strchr(str, '\n');
    if (newline != NULL) 
    {
        *newline = '\0';
    }
}

void log_message(const char* sender_id, const char* receiver_id, const char* message) 
{
    char sender_logfile_name[100], receiver_logfile_name[100];
    sprintf(sender_logfile_name, "%s.log", sender_id);
    sprintf(receiver_logfile_name, "%s.log", receiver_id);

    // Log for sender
    FILE* sender_logfile = fopen(sender_logfile_name, "a");
    if (sender_logfile) 
    {
        fprintf(sender_logfile, "To %s: %s\n", receiver_id, message);
        fclose(sender_logfile);
    }

    // Log for receiver
    FILE* receiver_logfile = fopen(receiver_logfile_name, "a");
    if (receiver_logfile) 
    {
        fprintf(receiver_logfile, "From %s: %s\n", sender_id, message);
        fclose(receiver_logfile);
    }
}

void send_chat_history_to_client(int client_socket, char* client_id) 
{
    remove_newline(client_id);
    char sender[50];
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (clients[i].socket == client_socket)
        {
            strcpy(sender,clients[i].id);
        }
    }
    char logfile_name[100];
    sprintf(logfile_name, "%s.log", sender);
    printf("logfile %s\n",logfile_name);
    FILE* logfile = fopen(logfile_name, "r");
    if (logfile) {
        char line[1024];
        while (fgets(line, sizeof(line), logfile) != NULL) 
        {
            if (strstr(line, client_id) != NULL) 
            { 
                send(client_socket, line, strlen(line), 0);
            }
        }
        fclose(logfile);
    } 
    else 
    {
        send(client_socket, "No chat history found.\n", 23, 0);
    }
}

void delete_client_history(int client_socket, char* client_id) 
{
    remove_newline(client_id);
    char sender[50];
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (clients[i].socket == client_socket) 
        {
            strcpy(sender, clients[i].id);
        }
    }

    char logfile_name[100], temp_file_name[105];
    sprintf(logfile_name, "%s.log", sender);
    sprintf(temp_file_name, "%s_tmp.log", sender); // Temporary file
    // printf("---------------------\n%s\n%s",logfile_name,temp_file_name);
    FILE* logfile = fopen(logfile_name, "r");
    if (logfile) 
    {
        FILE* temp_file = fopen(temp_file_name, "w");
        if (!temp_file) 
        {
            send(client_socket, "Failed to create a temporary file.\n", 34, 0);
            fclose(logfile);
            return;
        }

        char line[1024];
        while (fgets(line, sizeof(line), logfile) != NULL) 
        {
            // printf("%s\n",line);
            // Write to temp file if the line does not contain client_id
            if (strstr(line, client_id) == NULL) 
            {
                printf("%s---%s",line,client_id);
                fputs(line, temp_file);
            }
        }
        fclose(logfile);
        fclose(temp_file);

        // Replace the original file with the modified content
        remove(logfile_name); // Delete the original file
        rename(temp_file_name, logfile_name); // Rename temp file to original file name
        send(client_socket, "all chat belongs to client deleted.\n", 36, 0);
    } 
    else 
    {
        send(client_socket, "No chat history found.\n", 23, 0);
    }
}

void delete_history(int client_socket) 
{
    char sender[50];
    for (int i = 0; i < MAX_CLIENTS; i++) 
    {
        if (clients[i].socket == client_socket)
        {
            strcpy(sender,clients[i].id);
        }
    }
    char logfile_name[100];
    sprintf(logfile_name, "%s.log", sender);
    printf("Attempting to delete logfile: %s\n", logfile_name);

    if (remove(logfile_name) == 0) 
    {
        send(client_socket, "Chat history deleted.\n", 22, 0);
    } 
    else 
    {
        perror("Error deleting log file");
        send(client_socket, "Failed to delete chat history.\n", 30, 0);
    }
}

// Function to add client
void add_client(int client_socket, char* id) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == 0) {
            clients[i].socket = client_socket;
            strcpy(clients[i].id, id);
            break;
        }
    }
}

// Function to remove client
void remove_client(int client_socket) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == client_socket) {
            clients[i].socket = 0;
            memset(clients[i].id, 0, sizeof(clients[i].id));
            break;
        }
    }
}

// Function to get the list of active clients
void get_active_clients(int requester_socket) {
    char buffer[BUFFER_SIZE];
    strcpy(buffer, "Active clients:\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket != 0) {
            strcat(buffer, clients[i].id);
            strcat(buffer, "\n");
        }
    }
    send(requester_socket, buffer, strlen(buffer), 0);
}

// Function to forward message to the intended recipient
void forward_message(char* dest_id, char* message, int sender_socket) {
    char sender[1024];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].socket == sender_socket)
        {
            strcpy(sender,clients[i].id);
        }
    }
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (strcmp(clients[i].id, dest_id) == 0) {
            log_message(sender, clients[i].id, message);
            send(clients[i].socket, message, strlen(message), 0);
            return;
        }
    }
    char* error_message = "Recipient offline.\n";
    send(sender_socket, error_message, strlen(error_message), 0);
}


const char* find_pattern(const char* pat_msg) 
{
    const char* file_name = "FAQs.txt";
    FILE* file = fopen(file_name, "r");
    if (file == NULL) 
    { 
        perror("Error opening file");
        return NULL;
    }

    char* send_msg = NULL;
    char* line = NULL;
    size_t len = 0;
    int pattern_found = 0;

    char new_pat[1024];
    strcpy(new_pat, pat_msg);
    remove_newline(new_pat);

    while (getline(&line, &len, file) != -1) 
    {
        char* separator = strstr(line, " ||| ");
        if (separator != NULL) 
        {
            *separator = '\0'; 
            char* pattern = line;
            char* message = separator + 5; 

            if (strcmp(pattern, new_pat) == 0) 
            {
                send_msg = malloc(strlen("stupidbot>") + strlen(message) + 1);
                if (send_msg != NULL) 
                {
                    strcpy(send_msg, "stupidbot>");
                    strcat(send_msg, message);
                    pattern_found = 1;
                    break;
                } 
                else 
                {
                    perror("Memory allocation error");
                    send_msg = "stupidbot>System Malfunction, I couldn't understand your query.";
                    break;
                }
            }
        }
    }

    if (!pattern_found) 
    {
        send_msg = "stupidbot>System Malfunction, I couldn't understand your query.";
    }

    free(line);  // Free allocated memory for getline
    fclose(file);
    return send_msg;
}


char* send_gpt(char** arr) 
{
    int fd[2];
    pid_t child_pid;
    static char buffer[BUFFER_SIZE + 1];

    // Create a pipe
    if (pipe(fd) == -1) 
    {
        perror("pipe failed");
        exit(1);
    }

    child_pid = fork();
    if (child_pid == -1) 
    {
        perror("fork failed");
        exit(1);
    }

    if (child_pid == 0) 
    {
        close(fd[0]); 
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]); 

        if (execvp(arr[0], arr) == -1) 
        {
            perror("execvp failed");
            exit(1);
        }
    } 
    else 
    {
        close(fd[1]); 
        wait(NULL); 
        int bytes_read = read(fd[0], buffer, BUFFER_SIZE);
        if (bytes_read >= 0)
            buffer[bytes_read] = '\0'; 
        else
            buffer[0] = '\0';

        close(fd[0]);
    }

    return buffer;
}

int main() {
    int server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_size;
    char buffer[BUFFER_SIZE];
    fd_set read_fds, temp_fds;
    int max_sd, sd;

    // Initialize all client_socket[] to 0
    memset(clients, 0, sizeof(clients));

    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure settings
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server listening on port %d...\n", PORT);

    // Clear the socket set
    FD_ZERO(&read_fds);

    // Add server socket to set
    FD_SET(server_socket, &read_fds);
    max_sd = server_socket;

    // Main loop
    while (1) {
        temp_fds = read_fds;

        // Use select() for multiplexing
        int activity = select(max_sd + 1, &temp_fds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            printf("Select error\n");
        }

        // Incoming connection
        if (FD_ISSET(server_socket, &temp_fds)) {
            addr_size = sizeof(client_addr);
            new_socket = accept(server_socket, (struct sockaddr*)&client_addr, &addr_size);
            if (new_socket < 0)
                        {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
            
            printf("New connection: socket fd is %d, ip is: %s, port: %d\n", new_socket, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            // Send new connection greeting message
            char welcome_message[BUFFER_SIZE];
            uuid_t binuuid;
            char struuid[37]; // 36 byte UUID plus trailing '\0'
            uuid_generate_random(binuuid);
            uuid_unparse_lower(binuuid, struuid);
            sprintf(welcome_message, "Welcome to the chat server. Your ID is %s\n", struuid);
            if (send(new_socket, welcome_message, strlen(welcome_message), 0) != strlen(welcome_message)) {
                perror("Send failed");
            }
            
            puts("Welcome message sent successfully");

            // Add new socket to array of sockets
            add_client(new_socket, struuid);

            // Also add new socket to the set
            FD_SET(new_socket, &read_fds);
            if (new_socket > max_sd) {
                max_sd = new_socket;
            }
        }

        // IO operation on some other socket
        for (int i = 0; i < MAX_CLIENTS; i++) 
        {
            sd = clients[i].socket;
            if (FD_ISSET(sd, &temp_fds)) 
            {
                // Check if it was for closing, and also read the incoming message
                int valread;
                if ((valread = recv(sd, buffer, BUFFER_SIZE, 0)) == 0) 
                {
                    // Somebody disconnected, get his details and print
                    getpeername(sd, (struct sockaddr*)&client_addr, (socklen_t*)&addr_size);
                    printf("Host disconnected, ip %s, port %d \n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                    
                    // Close the socket and mark as 0 in list for reuse
                    close(sd);
                    remove_client(sd);
                    FD_CLR(sd, &read_fds);
                } 
                else 
                {
                    buffer[valread] = '\0';

                    if (strncmp(buffer, "/active", 7) == 0) 
                    {
                        get_active_clients(sd);
                    } 
                    else if (strncmp(buffer, "/send", 5) == 0) 
                    {
                        char* token = strtok(buffer + 6, " ");
                        if (token != NULL) 
                        {
                            char dest_id[37];
                            strncpy(dest_id, token, 36);
                            dest_id[36] = '\0';
                            char* message = strtok(NULL, "");
                            if (message != NULL) 
                            {
                                forward_message(dest_id, message, sd);
                            } 
                            else 
                            {
                                char* error_message = "Message format incorrect.\n";
                                send(sd, error_message, strlen(error_message), 0);
                            }
                        }
                    } 
                    else if(strncmp(buffer, "/chatbot_v2 login", 17) == 0) 
                    {
                        char* msg = "gpt2bot> Hi, I am updated bot, I am able to answer any question be it correct or incorrect";
                        send(sd, msg, strlen(msg), 0);
                    }
                    else if(strncmp(buffer, "/chatbot_v2 logout", 18) == 0) 
                    {
                        char* msg = "gpt2bot> Bye! Have a nice day and hope you do not have any complaints about me";
                        send(sd, msg, strlen(msg), 0);
                    }
                    else if(strncmp(buffer, ">", 1) == 0)
                    {
                        // char filename[20];
                        // sprintf(filename, "file_%d", sd);
                        char *recv_msg = buffer + 1;
                        char *arr[4];
                        arr[0] = "python3";
                        arr[1] = "newbot.py";
                        arr[2] = recv_msg;
                        arr[3] = NULL;
                        char *send_msg = send_gpt(arr);
                        send(sd, send_msg, strlen(send_msg), 0);
                    }
                    else if (strncmp(buffer, "/chatbot login", 14) == 0) 
                    {
                        char* msg = "stupidbot> Hi, I am stupid bot, I am able to answer a limited set of your questions";
                        send(sd, msg, strlen(msg), 0);
                    }
                    else if (strncmp(buffer, "/chatbot logout", 15) == 0) 
                    {
                        char* msg = "stupidbot> Bye! Have a nice day and do not complain about me";
                        send(sd, msg, strlen(msg), 0);
                    }
                    else if (strncmp(buffer, "user>", 5) == 0)
                    {
                        // printf("%s--",buffer);
                        char *recv_msg = buffer + 5;
                        // printf("%s\n",recv_msg);
                        const char *send_msg = find_pattern(recv_msg);

                        send(sd, send_msg, strlen(send_msg), 0);
                    }
                    else if (strncmp(buffer,"/history_delete",15) == 0)
                    {
                        char *recv_msg = buffer + 16;
                        delete_client_history(sd,recv_msg);
                    }
                    else if (strncmp(buffer,"/history",8) == 0)
                    {
                        char *recv_msg = buffer + 9;
                        send_chat_history_to_client(sd,recv_msg);
                    }
                    else if (strncmp(buffer,"/delete_all",10) == 0)
                    {
                        delete_history(sd);
                    }
                    else if (strncmp(buffer, "/logout", 7) == 0) 
                    {
                        char* bye_message = "Bye!! Have a nice day\n";
                        send(sd, bye_message, strlen(bye_message), 0);
                        close(sd);
                        remove_client(sd);
                        FD_CLR(sd, &read_fds);
                    } 
                    else 
                    {
                        send(sd, buffer, strlen(buffer), 0);
                    }
                }
            }
        }
    }

    return 0;
}
