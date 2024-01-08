//COMP 8567 Advanced System Programming
//Final Project
//Client-Server Model

//Description - This client-server project involves a server and mirror communicating through sockets. 
//Clients, connecting from different machines, can request files using commands like "getfn" for specific files, 
//"getfz" for a range of file sizes, "getft" for specific file types, and "getfdb" or "getfda" based on file 
//creation dates. The server and mirror alternate handling client connections, with the server for the first 4, 
//the mirror for the next 4, and subsequent connections alternating between them. The client receives requested files, 
//storing them in a folder named "f23project" in their home directory.


#include <stdio.h>       // Include standard input-output library
#include <stdlib.h>      // Include standard library
#include <string.h>      // Include string manipulation library
#include <unistd.h>      // Include symbolic constants and types library
#include <sys/socket.h>  // Include socket-related functions and structures
#include <sys/types.h>   // Include various data types used in system calls
#include <netinet/in.h>  // Include Internet address family structures
#include <netdb.h>       // Include network-related functions and structures
#include <stdbool.h>     // Include boolean data type
#include <time.h>        // Include time-related functions
#include <sys/stat.h>    // Include file information functions

// Define constant CR_BUFFSIZE with value 255
#define CR_BUFFSIZE 255
// Define constant for maximum unique extensions                    
#define CR_MAX_EXTS 3
// Define constant for maximum extension length
#define CR_MAX_EXT_LEN 10
// Define constant for maximum extension list length
#define CR_MAX_EXT_LIST_LEN 50
// Define constant string for client folder name
#define CR_CLIENT_FOLDER "f23project"

//this is the Function to handle errors and exit
void error(const char *msg) {

    perror(msg);    // this is the Print error message to standard error
    exit(1);        //this will Exit the program with status 1
}

// this is the Function to validate a comma-separated list of file extensions
bool cr_valid_ext_fun(const char *cr_extList) {
    
    //here it Initializes the count of unique extensions to zero
    int cr_uniquecnt = 0;
    bool flag = true;

    //this is an Array to store encountered extensions
    char cr_encntrdExt[CR_MAX_EXTS][CR_MAX_EXT_LEN];

    //this will Tokenize the input extension list using ',' as a delimiter
    char *token = strtok((char *)cr_extList, ",");
    while (token != NULL) {
        
        // this will Check if the current extension has already been encountered
        for (int i = 0; i < cr_uniquecnt; ++i) {
            if (strcmp(cr_encntrdExt[i], token) == 0) {
                return false; // Return false if the extension is not unique
            }
        }

        //this will Check if the current extension exceeds the maximum allowed length
        if (strlen(token) > CR_MAX_EXT_LEN) {
            return false; // Return false if the extension length is too long
        }
    

        //this will Check if there is space to store another unique extension
        if (cr_uniquecnt < CR_MAX_EXTS) {

            // this will Copy the current extension to the cr_encntrdExt array
            strcpy(cr_encntrdExt[cr_uniquecnt], token);

            // this will Increment the count of unique extensions
            cr_uniquecnt++;

        } else {

            //here Return false if the maximum unique extensions limit is reached
            return false; 
        }

        //this will Move to the next token in the extension list
        token = strtok(NULL, ",");
    }

    //this Returns true if at least one unique extension was encountered
    return cr_uniquecnt > 0;
}

// this is the Function to send a command to the server using a given socket
void cr_postCommandToServer(int cr_socketfDescriptor, const char *command) {
    
    //this will Calculate the length of the command string
    size_t cr_cmdLngth = strlen(command);

    //this will Attempt to send the command to the server and store the number of bytes sent
    ssize_t cr_sntBytes = send(cr_socketfDescriptor, command, cr_cmdLngth, 0);

    //this will Check for errors in the send operation
    if (cr_sntBytes < 0) {
        
        perror("System Failed to send command to server\n");
        exit(4); //this Exits the program with status 4
    } else if ((size_t)cr_sntBytes != cr_cmdLngth) {
        
        fprintf(stderr, "Failed to send the entire command to the server\n\n");
        exit(4); //this will Exit the program with status 4
    }

    //this will Print a message indicating the successful transmission of the command
    printf("Command successfully sent to the server: %s\n", command);
    printf("---------------------------------------------------------------");
}

//this is Function to handle the "getft" command and send it to the server
bool cr_manage_getft_cmd(int cr_socketfDescriptor, const char *cr_usr_input_cmd) {
    char extensions[CR_MAX_EXT_LIST_LEN]; // cr_buffer to store file extensions

    //this will Parse the user command, extracting the file extensions
    int cr_res = sscanf(cr_usr_input_cmd, "getft %[^\n]", extensions);

    //it will Check if the command format is valid
    if (cr_res != 1) {
        fprintf(stderr, "Error: Invalid format for the getft command.\n"); // Print error message to standard error
        return false; // Return false to indicate failure
    }

    //this will Validate the extracted file extensions
    if (!cr_valid_ext_fun(extensions)) {
        fprintf(stderr, "Error: Invalid extension list. Please include at least one and up to %d unique extensions.\n", CR_MAX_EXTS);  // cr_buffer for the modified command
        return false; // Return false to indicate failure
    }

    //this will Create a modified command with the validated extensions
    char cr_modified_cmd[CR_MAX_EXT_LIST_LEN + 10]; 
    snprintf(cr_modified_cmd, sizeof(cr_modified_cmd), "getft %s", extensions);

    //it will Send the modified command to the server
    cr_postCommandToServer(cr_socketfDescriptor, cr_modified_cmd);

    //this Returns true to indicate successful handling of the "getft" command
    return true;
}

//this is the Function to handle the "getfz" command with specified size range
bool cr_manage_getfz_cmd(int cr_socketfDescriptor, const char *cr_usr_input_cmd) {
    long cr_size1, cr_size2; //these Variables to store the specified size range

    //it Parses the user command to extract two long integers representing the size range
    int cr_res = sscanf(cr_usr_input_cmd, "getfz %ld %ld", &cr_size1, &cr_size2);

    //this Checks for invalid command format or sizes
    if (cr_res != 2 || cr_size1 < 0 || cr_size2 < 0 || cr_size1 > cr_size2) {
        fprintf(stderr, "Error: The format of the getfz command is invalid, or the provided sizes are not valid. Please use the following format: getfz <size1> <size2>");
        return false;  // Return false to indicate an error
    }

    //this will Create a modified command with the extracted size range
    char cr_modified_cmd[30];  
    snprintf(cr_modified_cmd, sizeof(cr_modified_cmd), "getfz %ld %ld", cr_size1, cr_size2);

    //it Sends the modified command to the server
    cr_postCommandToServer(cr_socketfDescriptor, cr_modified_cmd);

    //this will Read the server's response and print it to the console
    char cr_buffer[255];
    bzero(cr_buffer, 255);
    if (read(cr_socketfDescriptor, cr_buffer, 255) < 0)
        error("Error: Unable to read from the socket\n"); // Handle error if reading from socket fails
    printf("\nSent the following command to the server: %s\n", cr_buffer); // Print the server's response

    return true; //it will Return true to indicate successful execution
}

//this is the Function to handle the getfdb command and send a modified command to the server
bool cr_manage_getfdb_cmd(int cr_socketfDescriptor, const char *cr_usr_input_cmd) {

    //this will Declare a character array to store the date string
    char cr_date_str[64];

    //this will Parse the cr_usr_input_cmd to extract the date string using sscanf
    int cr_res = sscanf(cr_usr_input_cmd, "getfdb %[^\n]", cr_date_str);

    //it Checks if the sscanf successfully parsed one item
    if (cr_res != 1) {
        fprintf(stderr, "Error: The format of the getfdb command is invalid.\n"); // Print an error message to standard error
        return false;  // Return false to indicate an error
    }

    //this will Create a modified command with the extracted date string
    char cr_modified_cmd[74];  // Declare a character array to store the modified command
    snprintf(cr_modified_cmd, sizeof(cr_modified_cmd), "getfdb %s", cr_date_str);

    //this  Sends the modified command to the server
    cr_postCommandToServer(cr_socketfDescriptor, cr_modified_cmd);

    return true; // Return true to indicate successful handling of the getfdb command
}

//this is the Function to handle the getfda command and communicate with the server
bool cr_manage_getfda_cmd(int cr_socketfDescriptor, const char *cr_usr_input_cmd) {
    char cr_date_str[64]; //this is the cr_buffer to store the extracted date string

    //this Extracts the date from the user command using a format specifier
    int cr_res = sscanf(cr_usr_input_cmd, "getfda %[^\n]", cr_date_str);

    //this will Check if the extraction was successful
    if (cr_res != 1) {
        fprintf(stderr, "Error: Incorrect format for the getfda command.\n"); // Print error message to standard error
        return false; // Return false to indicate command handling failure
    }
 
    //this Creates the modified command with the validated date
    char cr_modified_cmd[74];  // Extra space for "getfda " and null terminator
    snprintf(cr_modified_cmd, sizeof(cr_modified_cmd), "getfda %s", cr_date_str);

    //it will Send the modified command to the server using the cr_postCommandToServer function
    cr_postCommandToServer(cr_socketfDescriptor, cr_modified_cmd);

    return true; //this will Return true to indicate successful command handling
}


// this is the Function to send a 'getfn' command to the server with a specified cr_filename
void cr_send_getfn_cmd(int cr_socketfDescriptor, const char *cr_filename) {

    //this will Create a command string with the format "getfn <cr_filename>"
    char command[255];
    snprintf(command, sizeof(command), "getfn %s", cr_filename);

    //it Writes the command to the server socket
    if (write(cr_socketfDescriptor, command, strlen(command)) < 0)
        error("Failed to send data to the server\n"); //it Prints error message and exit if writing fails

    //this Reads the server's response into a cr_buffer
    char cr_buffer[255];
    bzero(cr_buffer, 255); //this will Clear the cr_buffer before reading
    if (read(cr_socketfDescriptor, cr_buffer, 255) < 0)
        error("Encountered an issue while reading from the socket"); // Print error message and exit if reading fails
    
    // Print the server's response to the console
    printf("Received response from server: %s", cr_buffer);
}


// Main function for the client application
int main(int argc, char *argv[]) {
    int cr_socketfDescriptor, cr_portnumber, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    char cr_buffer[255];
    char command[255];

    // Check if the correct number of command-line arguments is provided
    if (argc < 3) {
        fprintf(stderr, "Please use the following format: %s [hostname] [port]", argv[0]); // Print usage information to standard error
        exit(0); // Exit the program with status 0
    }

    // Extract the port number from the command-line arguments
    cr_portnumber = atoi(argv[2]);

    // Create a socket for communication
    cr_socketfDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (cr_socketfDescriptor < 0)
        error("ERROR OPENING SOCKET"); // Print an error message and exit if socket creation fails

    // Get server information based on the provided hostname
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr, "Error: Unable to resolve the host."); // Print an error message if the host is not found
    }

    // Initialize the server address structure
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(cr_portnumber);

    // Connect to the server
    if (connect(cr_socketfDescriptor, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) > 0)
        error("the connection will get failed"); // Print an error message and exit if connection fails

// Main loop for user input and command execution
while (1) {
    
    printf("\nPlease enter a command: ");
    fgets(command, sizeof(command), stdin);

    // Remove newline character from the entered command
    size_t len = strlen(command);
    if (len > 0 && command[len - 1] == '\n') {
        command[len - 1] = '\0';
    } else {
        int c;
        while ((c=getchar()) != '\n' && c != EOF);
    }

    // Check for specific commands and handle them accordingly
    if (strncmp("getft", command, 5) == 0) {
        // Handle 'getft' command
        if (!cr_manage_getft_cmd(cr_socketfDescriptor, command)) {
            fprintf(stderr, "Failed to execute the 'getft' command\n");
        }
    }

   
    else if (strncmp("getfdb", command, 6) == 0) {
        // Handle 'getfdb' command
        if (!cr_manage_getfdb_cmd(cr_socketfDescriptor, command)) {
            fprintf(stderr, "Failed to execute the 'getfdb' command\n");
        }
    }

    else if (strncmp("getfda", command, 6) == 0) {
        // Handle 'getfda' command
        if (!cr_manage_getfda_cmd(cr_socketfDescriptor, command)) {
            fprintf(stderr, "Failed to execute the 'getfda' command\n");
        }
    }

    else if (strncmp("getfn", command, 5) == 0) {
        // // Handle 'getfn' command
        char cr_filename[255];
        if (sscanf(command, "getfn %[^\n]", cr_filename) == 1) {
            // Send the getfn command to the server
            cr_send_getfn_cmd(cr_socketfDescriptor, cr_filename);
        } else {
            fprintf(stderr, "Failed to execute the 'getfn' command\n");
        }
    }

    // Handle 'getfz' command
    else if (strncmp("getfz", command, 5) == 0) {
        if (!cr_manage_getfz_cmd(cr_socketfDescriptor, command)) {
            fprintf(stderr, "Failed to execute the 'getfz' command\n");
        }
    }

    else {
        // Send a general command to the server
        n = write(cr_socketfDescriptor, command, strlen(command));
        if (n < 0)
            error("Failed to send command to server: %s\n"); // Print an error message and exit if writing to the socket fails

        // Read the server's response
        bzero(cr_buffer, 255);
        n = read(cr_socketfDescriptor, cr_buffer, 255);
        if (n < 0)
            error("Failed to read from the socket\n"); // Print an error message and exit if reading from the socket fails
        
        // Print the server's response
        printf("Command successfully sent to the server: \"%s\"\n", cr_buffer);
        printf("--------------------------------------------------------------");

        // Check if the user entered the 'quitc' command to break out of the loop
        int i = strncmp("quitc", command, 5);
        if (i == 0)
            break;
    }
}

    // Close the socket connection
    close(cr_socketfDescriptor);

    // Exit the program with status 0
    return 0;
}