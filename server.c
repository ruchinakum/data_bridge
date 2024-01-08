//COMP 8567 Advanced System Programming
//Final Project
//Client-Server Model

//Description - This client-server project involves a server and mirror communicating through sockets. 
//Clients, connecting from different machines, can request files using commands like "cr_getfn_cmd" for specific files, 
//"getfz" for a range of file sizes, "getft" for specific file types, and "getfdb" or "getfda" based on file 
//creation dates. The server and mirror alternate handling client cr_conns, with the server for the first 4, 
//the mirror for the next 4, and subsequent cr_conns alternating between them. The client receives requested files, 
//storing them in a folder named "f23project" in their home directory.

//Submitted to: Dr Prashanth Ranga

//Submitted by:
//Name: Ruchibahen Surendrasinh Nakum
//Student ID: 110122972
//Section: 1

//Name: Chandana Sree Sunkara
//Student ID: 110100976
//Section: 1

// Required header files for standard input/output and string functions
#include <stdio.h>   // Standard input/output functions
#include <stdlib.h>  // General-purpose standard library functions
#include <string.h>  // String manipulation functions
#include <unistd.h>  // Standard symbolic constants and types

// Header files required for socket programming
#include <sys/socket.h>  // Core socket functions and data structures
#include <sys/types.h>   // Data types used in socket functions
#include <netinet/in.h>  // Internet address family and socket address structures

// Header files for function control and address-related structures
#include <sys/stat.h>   // File status and information functions
#include <fcntl.h>      // File control options
#include <time.h>       // Time-related functions
#include <stdbool.h>    // Boolean data type and related functions
#include <arpa/inet.h>  // Functions for manipulating Internet addresses
#include <netinet/in.h>  // Internet address family and socket address structures

// Defining the cr_buffer size
#define CR_BUFFSIZE 1024

// Defining the folder name for storing files on the client side
#define CR_CLIENT_FOLDER "f23project"

// Variable to keep track of the total number of connected clients
int cr_total_clients = 0;

// Function to handle and display errors with a provided message
void error(const char *msg) {
    perror(msg);  // perror prints the error message to stderr along with the description of the last error that occurred
    exit(1);      // Terminate the program with an exit status of 1, indicating an error
}


// Function to send either a tar.gz file or a response message to the client
void cr_send_response_to_client(int cr_clientsocket, const char *cr_file_pth, const char *cr_resp_msg) {
    // Check if parameters are correct
    if ((cr_file_pth == NULL && cr_resp_msg == NULL) || (cr_file_pth != NULL && cr_resp_msg != NULL)) {
        // Ensure either a file path or a response message is provided, but not both
        fprintf(stderr, "Error: Incorrect parameters for cr_send_response_to_client function\n");
        exit(EXIT_FAILURE);
    }

    // Set the flag based on the type of response
    int flag = (cr_file_pth != NULL);

    // Send the flag to indicate the type of response
    if (send(cr_clientsocket, &flag, sizeof(flag), 0) < 0) {
        perror("Error sending response flag to client");
        exit(EXIT_FAILURE);
    }

    // Introduce a delay to simulate processing time
    sleep(1);

    if (cr_file_pth != NULL) {
        // Open the tar.gz file
        FILE *file = fopen(cr_file_pth, "rb");
        if (file == NULL) {
            // Handle file opening error
            perror("Error opening tar.gz file");
            exit(EXIT_FAILURE);
        }

        // Calculate the file size
        fseek(file, 0, SEEK_END);
        long cr_size_of_file = ftell(file);
        fseek(file, 0, SEEK_SET);

        // Send the file size to the client
        if (send(cr_clientsocket, &cr_size_of_file, sizeof(cr_size_of_file), 0) < 0) {
            // Handle file size sending error
            perror("Error sending file size to client");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Introduce a delay to simulate processing time
        sleep(1);

        // Dynamically allocate memory to store the file content
        char *cr_content_of_file = (char *)malloc(cr_size_of_file);
        if (cr_content_of_file == NULL) {
            // Handle memory allocation error
            perror("Error allocating memory for file content");
            fclose(file);
            exit(EXIT_FAILURE);
        }

        // Read the file into memory
        if (fread(cr_content_of_file, 1, cr_size_of_file, file) != (size_t)cr_size_of_file) {
            // Handle file reading error
            perror("Error reading file into memory");
            fclose(file);
            free(cr_content_of_file);
            exit(EXIT_FAILURE);
        }

        // Construct the full path to the client's folder
        char cr_client_folder_pth[CR_BUFFSIZE];
        snprintf(cr_client_folder_pth, sizeof(cr_client_folder_pth), "%s/%s", getenv("HOME"), CR_CLIENT_FOLDER);

        // Check if the folder exists; if not, create it
        struct stat st = {0};
        if (stat(cr_client_folder_pth, &st) == -1) {
            mkdir(cr_client_folder_pth, 0700);
        }

        // Construct the full path to the destination file in the client's folder
        char cr_dest_file_pth[CR_BUFFSIZE];
        snprintf(cr_dest_file_pth, sizeof(cr_dest_file_pth), "%s/%s/%s", getenv("HOME"), CR_CLIENT_FOLDER, cr_file_pth);

        // Open the destination file in the client's folder
        FILE *cr_dest_file = fopen(cr_dest_file_pth, "wb");
        if (cr_dest_file == NULL) {
            // Handle destination file opening error
            perror("Error opening destination file in client's folder");
            fclose(file);
            free(cr_content_of_file);
            exit(EXIT_FAILURE);
        }

        // Write the file content to the destination file
        fwrite(cr_content_of_file, 1, cr_size_of_file, cr_dest_file);

        // Close the destination file
        fclose(cr_dest_file);

        // Print success message
        printf("File sent successfully to the client's folder: %s\n", CR_CLIENT_FOLDER);
        printf("***********************************************************************\n");

        // Clean up
        fclose(file);
        free(cr_content_of_file);
    } else {
        // Send the response message to the client
        if (send(cr_clientsocket, cr_resp_msg, strlen(cr_resp_msg) + 1, 0) < 0) {
            // Handle response message sending error
            perror("Error sending response message to client");
            exit(EXIT_FAILURE);
        }

        // Introduce a delay to simulate processing time
        sleep(1);

        // Print success message
        printf("Response message sent successfully to the client\n");
    }
}

// Function to handle getfz command received from the client
void cr_manage_getfz_cmd_from_client(int cr_clientsocket, const char *command) {
    long cr_size1, cr_size2;
    int cr_size;

    // Extract cr_size1 and cr_size2 from the command
    int cr_res = sscanf(command, "getfz %ld %ld", &cr_size1, &cr_size2);

    if (cr_res != 2 || cr_size1 < 0 || cr_size2 < 0 || cr_size1 > cr_size2) {
        // Check if the command format is invalid or if sizes are negative or out of order
        // If so, send an error message to the client and exit the function
        cr_send_response_to_client(cr_clientsocket, NULL, "Invalid command format or invalid sizes. Please use: getfz cr_size1 cr_size2");
        return;
    }

    // Remove any existing temp.tar.gz file
    system("rm -f temp.tar.gz");

    // Construct a find command to locate files with specified sizes
    char cr_search_cmd[CR_BUFFSIZE];
    snprintf(cr_search_cmd, sizeof(cr_search_cmd), "find ~/ -type f -size +%ldc -a -size -%ldc -exec tar czf temp.tar.gz {} +", cr_size1, cr_size2);

    // Execute the find and tar command
    if (system(cr_search_cmd) == 0) {
        // If the command execution is successful, send the generated tar.gz file to the client
        cr_send_response_to_client(cr_clientsocket, "temp.tar.gz", NULL);
    } else {
        // If an error occurs during command execution, report an error message to the client
        cr_send_response_to_client(cr_clientsocket, NULL, "Error generating temp.tar.gz file");
    }
}



// Function to handle getft command received from the client
void cr_manage_getft_cmd_from_client(int cr_clientsocket, const char *command) {
    // Remove any existing temp.tar.gz file
    system("rm -f temp.tar.gz");

    // Extract the extension list from the command
    char cr_exts_list[255];
    sscanf(command, "getft %[^\n]", cr_exts_list);

    printf("Received extension list: %s\n", cr_exts_list);

    // Construct a find command to locate files with specified extensions
    char cr_search_cmd[CR_BUFFSIZE];
    snprintf(cr_search_cmd, sizeof(cr_search_cmd), "find ~/ -type f \\( -name \"*.%s\"", cr_exts_list);

    // Tokenize the extension list
    char *token = strtok(cr_exts_list, " ");
    while (token != NULL) {
        snprintf(cr_search_cmd + strlen(cr_search_cmd), sizeof(cr_search_cmd) - strlen(cr_search_cmd), " -o -name \"*.%s\"", token);
        token = strtok(NULL, " ");
    }

    // Close the parenthesis for the find command
    snprintf(cr_search_cmd + strlen(cr_search_cmd), sizeof(cr_search_cmd) - strlen(cr_search_cmd), " \\)");

    // Construct the tar command to compress identified files
    char tar_command[CR_BUFFSIZE * 2];
    snprintf(tar_command, sizeof(tar_command), "%s -exec tar czf temp.tar.gz {} +", cr_search_cmd);

    printf("Find command: %s\n", cr_search_cmd);
    printf("Tar command: %s\n", tar_command);

    // Execute the find and tar command
    if (system(tar_command) == 0) {
        // If the command execution is successful, send the generated tar.gz file to the client
        cr_send_response_to_client(cr_clientsocket, "temp.tar.gz", NULL);
    } else {
        // If an error occurs during command execution, report an error message to the client
        cr_send_response_to_client(cr_clientsocket, NULL, "Error generating temp.tar.gz file");
    }
}


// Function to handle getfdb command received from the client
void cr_manage_getfdb_cmd_from_client(int cr_clientsocket, const char *command) {
    // Remove any existing temp.tar.gz file
    system("rm -f temp.tar.gz");
    int cr_client;

    // Extract the date from the command
    char cr_date_str[64];
    sscanf(command, "getfdb %[^\n]", cr_date_str);

    // Convert the date string to time_t
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));

    if (sscanf(cr_date_str, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) != 3) {
        // Check if the date format is invalid; if so, send an error message to the client and exit the function
        cr_send_response_to_client(cr_clientsocket, NULL, "Invalid date format. Please use YYYY-MM-DD.");
        return;
    }

    tm.tm_year -= 1900;  // Adjust for year starting from 1900
    tm.tm_mon--;         // Adjust for months starting from January

    time_t cr_target_date = mktime(&tm);

    // Construct a find command to locate files with creation date <= cr_target_date
    char cr_search_cmd[CR_BUFFSIZE];
    snprintf(cr_search_cmd, sizeof(cr_search_cmd), "find ~/ -type f ! -newermt \"%s\" -exec tar czf temp.tar.gz {} +", cr_date_str);

    // Execute the find and tar command
    if (system(cr_search_cmd) == 0) {
        // If the command execution is successful, send the generated tar.gz file to the client
        cr_send_response_to_client(cr_clientsocket, "temp.tar.gz", NULL);
    } else {
        // If an error occurs during command execution, report an error message to the client
        cr_send_response_to_client(cr_clientsocket, NULL, "Error generating temp.tar.gz file");
    }
}


// Function to handle getfda command received from the client
void cr_manage_getfda_cmd_from_client(int cr_clientsocket, const char *command) {
    // Remove any existing temp.tar.gz file
    system("rm -f temp.tar.gz");

    // Extract the date from the command
    char cr_date_str[64];
    sscanf(command, "getfda %[^\n]", cr_date_str);

    // Convert the date string to time_t
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));

    if (sscanf(cr_date_str, "%d-%d-%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday) != 3) {
        // Check if the date format is invalid; if so, send an error message to the client and exit the function
        cr_send_response_to_client(cr_clientsocket, NULL, "Invalid date format. Please use YYYY-MM-DD.");
        return;
    }

    tm.tm_year -= 1900;  // Adjust for year starting from 1900
    tm.tm_mon--;         // Adjust for months starting from January

    time_t cr_target_date = mktime(&tm);

    // Construct a find command to locate files with creation date >= cr_target_date
    char cr_search_cmd[CR_BUFFSIZE];
    snprintf(cr_search_cmd, sizeof(cr_search_cmd), "find ~/ -type f -newermt \"%s\" -exec tar czf temp.tar.gz {} +", cr_date_str);

    // Execute the find and tar command
    if (system(cr_search_cmd) == 0) {
        // If the command execution is successful, send the generated tar.gz file to the client
        cr_send_response_to_client(cr_clientsocket, "temp.tar.gz", NULL);
    } else {
        // If an error occurs during command execution, report an error message to the client
        cr_send_response_to_client(cr_clientsocket, NULL, "Error generating temp.tar.gz file");
    }
}


// Function to get file information based on the filename
void cr_getfn_cmd(int cr_new_socket_descriptor, const char *filename) {
    char cr_search_cmd[512];  // cr_buffer for the find command
    char cr_buffer[1024];       // General-purpose cr_buffer
    char cr_file_pth[256];     // cr_buffer to store the file path

    // Construct the find command to search for the file in the user's home directory
    snprintf(cr_search_cmd, sizeof(cr_search_cmd), "find $HOME -type f -name '%s' 2>/dev/null | head -n 1", filename);

    FILE *pipe = popen(cr_search_cmd, "r");  // Open a pipe to execute the find command
    if (!pipe) {
        error("Error opening pipe");  // Handle error if opening the pipe fails
    }

    // Read the output of the find command
    if (fgets(cr_file_pth, sizeof(cr_file_pth), pipe) != NULL) {
        // Remove newline character from the end of the file path
        size_t len = strlen(cr_file_pth);
        if (len > 0 && cr_file_pth[len - 1] == '\n') {
            cr_file_pth[len - 1] = '\0';
        }

        // Get file information
        struct stat cr_file_statistics;
        if (stat(cr_file_pth, &cr_file_statistics) < 0) {
            error("Error getting file information");  // Handle error if getting file information fails
        }

        // Prepare file information string
        char file_info[1024];
        snprintf(file_info, sizeof(file_info), "File Information:\nPath: %s\nSize: %ld bytes\nDate Created: %sPermissions: %o\n",
                 cr_file_pth,
                 (long)cr_file_statistics.st_size,
                 ctime(&cr_file_statistics.st_ctime),
                 cr_file_statistics.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO));
                 
        // Remove newline character from the end of the file information string
        len = strlen(file_info);
        if (len > 0 && file_info[len - 1] == '\n') {
            file_info[len - 1] = '\0';
        }

        // Send file information to the client
        write(cr_new_socket_descriptor, file_info, strlen(file_info));
    } else {
        // File not found
        char not_found_msg[] = "File not found";
        write(cr_new_socket_descriptor, not_found_msg, strlen(not_found_msg));
    }

    // Close the pipe
    pclose(pipe);
}


// Function to handle client requests
void pclientrequest(int cr_new_socket_descriptor) {
    char cr_buffer[255];  // cr_buffer for storing incoming messages
    int n;

    while (1) {
        bzero(cr_buffer, 255);  // Clear the cr_buffer
        n = read(cr_new_socket_descriptor, cr_buffer, 255);  // Read data from the client socket

        if (n < 0) {
            error("Error reading from socket");  // Handle error if reading from the socket fails
        } else if (n==0){
            printf("Client disconnected\n");
            break;
        }

        cr_buffer[n] = '\0';
        printf("Client: %s\n", cr_buffer);  // Print the received command from the client

        // Parse the received command
        char command[10];
        sscanf(cr_buffer, "%s", command);

        // Implement logic for each command
        if (strcmp(command, "getfn") == 0) {
            char filename[100];
            sscanf(cr_buffer, "%*s %s", filename);

            // Call the cr_getfn_cmd function to handle the "cr_getfn_cmd" command
            cr_getfn_cmd(cr_new_socket_descriptor, filename);
        } else if (strcmp(command, "getft") == 0) {
            // Call the cr_manage_getft_cmd_from_client function to handle the "getft" command
            cr_manage_getft_cmd_from_client(cr_new_socket_descriptor, cr_buffer);
        } else if (strcmp(command, "getfdb") == 0) {
            // Call the cr_manage_getfdb_cmd_from_client function to handle the "getfdb" command
            cr_manage_getfdb_cmd_from_client(cr_new_socket_descriptor, cr_buffer);
        } else if (strcmp(command, "getfda") == 0) {
            // Call the cr_manage_getfda_cmd_from_client function to handle the "getfda" command
            cr_manage_getfda_cmd_from_client(cr_new_socket_descriptor, cr_buffer);
        } else if (strcmp(command, "getfz") == 0) {
            // Call the cr_manage_getfz_cmd_from_client function to handle the "getfz" command
            cr_manage_getfz_cmd_from_client(cr_new_socket_descriptor, cr_buffer);
        } else if (strcmp(command, "quitc") == 0) {
            printf("Client requested to quit. Exiting...\n");
            printf("****************************************");
            break;
        } else {
            // Handle unknown command
            char error_msg[] = "Unknown command. Please enter a valid command.";
            write(cr_new_socket_descriptor, error_msg, strlen(error_msg));
        }
    }

    close(cr_new_socket_descriptor);  // Close the client socket
    exit(EXIT_SUCCESS);  // Exit the child process
}

// Function to connect the client request to the mirror server
int cr_connect_client_mirror(char *argv[]) {
    // Variables for socket, port number, and address structure
    int cr_clientSocket, cr_portnum;
    socklen_t len;
    struct sockaddr_in servAdd;

    // Creating a socket
    if ((cr_clientSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Error: Unable to create socket\n");
        exit(1);
    }

    // Creating the address structure
    servAdd.sin_family = AF_INET;
    sscanf(argv[3], "%d", &cr_portnum);
    servAdd.sin_port = htons(cr_portnum);

    // Converting the IP address from string to binary
    if (inet_pton(AF_INET, argv[2], &servAdd.sin_addr) < 0) {
        fprintf(stderr, "Error: inet_pton() has failed\n");
        exit(2);
    }

    // Connecting to the mirror server
    if (connect(cr_clientSocket, (struct sockaddr *) &servAdd, sizeof(servAdd)) < 0) {
        fprintf(stderr, "Error: connect() has failed\n");
        exit(3);
    }

    // Returning the connected socket for communication with the mirror server
    return cr_clientSocket;
}


// Function to check the cr_conn with the mirror server
void cr_check_connected_mirror(char *argv[]) {
    // Obtain a socket cr_conn to the mirror server
    int cr_conn = cr_connect_client_mirror(argv);

    // Check if the cr_conn was successful
    if (cr_conn == -1) {
        fprintf(stderr, "Error: Unable to connect to the mirror server\n");
        exit(1);
    } else {
        printf("Mirror and Server are successfully connected.\n");  // Print a success message if the cr_conn is established
        printf("-------------------------------------");
        close(cr_conn);  // Close the cr_conn as it's only a check, not for actual communication
    }
}


// Function to route commands from the client to the mirror server
void cr_sending_cmd_to_mirror(char *argv[], int cr_clientSocket) {
    // Connect to the mirror server
    int cr_conn = cr_connect_client_mirror(argv);
    char cr_resp_buff[50000];
    printf("Connected to mirror successfully\n", cr_conn);
    printf("------------------------------------------\n");

    bool keep_cr_conn = true;
    while (keep_cr_conn) {
        char command_str[1000];

        // Receive the command from the client
        ssize_t cr_bytes_got = recv(cr_clientSocket, command_str, sizeof(command_str) - 1, 0);
        cr_resp_buff[cr_bytes_got] = '\0';
        printf("Mirror response: %s\n", cr_resp_buff);
        printf("-------------------------------------\n");

        // Check for receive errors
        if (cr_bytes_got < 0) {
            fprintf(stderr, "recv() failed\n");
            exit(5);
        }

        command_str[cr_bytes_got] = '\0';

        // Send the command to the mirror server
        if (send(cr_conn, command_str, strlen(command_str), 0) < 0) {
            fprintf(stderr, "send() failed\n");
            exit(0);
        }

        printf("Command %s routed to mirror\n\n", command_str);

        // Check for exit conditions
        if (strcmp(command_str, "quitc") == 0 || strcmp(command_str, " ") == 0 || strcmp(command_str, "") == 0) {
            printf("Exit\n");
            close(cr_conn);
            close(cr_clientSocket);
            keep_cr_conn = false;
            break;
        }

        // Receive the response from the mirror server
        cr_bytes_got = recv(cr_conn, cr_resp_buff, 50000, 0);
        cr_resp_buff[cr_bytes_got] = '\0';

        if (cr_bytes_got > 0) {
            // Check for TAR_FILE_FLAG
            if (strcmp(cr_resp_buff, "TAR_FILE_FLAG") == 0) {
                // Notify the client about the TAR_FILE_FLAG
                send(cr_clientSocket, cr_resp_buff, cr_bytes_got, 0);

                memset(cr_resp_buff, 0, sizeof(cr_resp_buff));

                // Receive the size of the tar file
                cr_bytes_got = recv(cr_conn, cr_resp_buff, sizeof(cr_resp_buff) - 1, 0);
                cr_resp_buff[cr_bytes_got] = '\0';

                // Notify the client about the tar file size
                send(cr_clientSocket, cr_resp_buff, cr_bytes_got, 0);

                // Allocate memory for the tar file content
                long tarfile_bytes_buff = atol(cr_resp_buff);
                char *tarfile_buff = (char *)malloc(tarfile_bytes_buff);
                
                // Check for memory allocation failure
                if (tarfile_buff == NULL) {
                    fprintf(stderr, "No memory allocated\n");
                    return;
                }

                ssize_t cr_bytes_got = 0;
                size_t total_cr_bytes_got = 0;

                // Receive the tar file content
                while (total_cr_bytes_got < tarfile_bytes_buff) {
                    cr_bytes_got = recv(cr_conn, tarfile_buff + total_cr_bytes_got, tarfile_bytes_buff, 0);
                    if (cr_bytes_got <= 0) {
                        if (cr_bytes_got == 0) {
                            printf("Server terminated connection\n");
                        } else {
                            fprintf(stderr, "Unable to get data\n");
                        }
                        free(tarfile_buff);
                        return;
                    }
                    total_cr_bytes_got += cr_bytes_got;
                }

                // Add a null terminator to the tar file content
                tarfile_buff[sizeof(tarfile_buff)] = '\0';
                sleep(2);

                // Send the tar file content to the client
                send(cr_clientSocket, tarfile_buff, total_cr_bytes_got, 0);
                printf("Response routed to client\n");
            }
            // Check for MESSAGE_FLAG
            else if (strcmp(cr_resp_buff, "MESSAGE_FLAG") == 0) {
                // Notify the client about the MESSAGE_FLAG
                send(cr_clientSocket, cr_resp_buff, cr_bytes_got, 0);
                memset(cr_resp_buff, 0, sizeof(cr_resp_buff));

                // Receive and send the message to the client
                cr_bytes_got = recv(cr_conn, cr_resp_buff, 50000, 0);
                cr_resp_buff[cr_bytes_got] = '\0';
                send(cr_clientSocket, cr_resp_buff, cr_bytes_got, 0);
                printf("Message sent to client!\n");
            }
            // Handle incompatible responses
            // else {
            //     fprintf(stderr, "Incompatible response from mirror\n");
            // }
        }
    }
    exit(0);
}


// Function to handle a client at the mirror server
void cr_handle_client_at_mirror(char *argv[], int cr_conn) {
    // Fork a child process to handle the client
    int pid = fork();

    // Check for fork failure
    if (pid == 0) {
        // Child process: Route commands between the client and the mirror
        cr_sending_cmd_to_mirror(argv, cr_conn);
    } else if (pid == -1) {
        // Forking error: Print an error message
        fprintf(stderr, "An error occurred while connecting to the client\n");
    }
    // Parent process continues to wait for other client cr_conns
}


// Function to handle a client at the server
void cr_handle_client_at_server(int cr_conn) {
    // Fork a child process to handle the client
    int pid = fork();

    // Check for fork failure
    if (pid == 0) {
        // Child process: Handle the client request
        pclientrequest(cr_conn);
    } else if (pid == -1) {
        // Forking error: Print an error message
        fprintf(stderr, "An error occurred while connecting to the client\n");
    } else {
        // Parent process
        // Note: This loop waits for any terminated child processes (non-blocking)
        while (waitpid(-1, NULL, WNOHANG) > 0);
        // The parent continues to wait for other client cr_conns
    }
}


// Main function
int main(int argc, char *argv[]) {
    // Check if the correct number of command-line arguments is provided
    if (argc != 4) {
        fprintf(stderr, "Please provide: %s <Port#> <Mirror IP> <Mirror Port#>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Check the cr_conn to the mirror server
    cr_check_connected_mirror(argv);

    int listen_to, cr_conn, cr_portnum;
    socklen_t socket_length;
    struct sockaddr_in server_add;

    // Create a socket for the server
    if ((listen_to = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Unable to create socket\n");
        exit(1);
    }

    // Set up the server address struct
    server_add.sin_family = AF_INET;
    server_add.sin_addr.s_addr = htonl(INADDR_ANY);
    sscanf(argv[1], "%d", &cr_portnum);
    server_add.sin_port = htons(cr_portnum);

    // Bind the socket to the server address
    bind(listen_to, (struct sockaddr *) &server_add, sizeof(server_add));

    // Start listening for client cr_conns
    listen(listen_to, 20);

    // Accept and handle client cr_conns in a loop
    while (1) {
        cr_conn = accept(listen_to, (struct sockaddr*) NULL, NULL);
        if (cr_conn != -1) {
            cr_total_clients++;

            // Determine whether to handle the client at the server or the mirror
            if (cr_total_clients <= 4) {
                printf("\nServer handling: Client count - %d\n", cr_total_clients);
                cr_handle_client_at_server(cr_conn);
            } else if(cr_total_clients > 4 && cr_total_clients<=8){
                printf("\nMirror handling: Client count - %d\n", cr_total_clients);
                cr_handle_client_at_mirror(argv, cr_conn);
            }
            else if(cr_total_clients %2 != 0){
                printf("\nServer handling: Client count - %d\n", cr_total_clients);
                cr_handle_client_at_server(cr_conn);
            }
            else if(cr_total_clients %2 == 0){
                printf("\nMirror handling: Client count - %d\n", cr_total_clients);
                cr_handle_client_at_mirror(argv, cr_conn);
            }
            close(cr_conn);

        }
    }
    close(cr_conn);

    }

        