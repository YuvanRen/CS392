#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
/*
    Yuvan Rengifo  
    I pledge my honor that I have abided by the Stevens Honor System.

*/
#define RED "\x1B[38;5;196m" 
#define DEFAULT "\x1b[0m"
#define YELLOW "\x1B[38;5;227m"

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 25555


void print_usage(char *prog) {
    printf("Usage: %s [-i IP_address] [-p port_number] [-h]\n", prog);
    printf(" -i IP_address          Default to \"127.0.0.1\";\n");
    printf(" -p port_number         Default to 25555;\n");
    printf(" -h                     Display this help info.\n");
}

void parse_connect(int argc, char** argv, int* server_fd) {

    int opt;
    char *ip_address = DEFAULT_IP;
    int port_number = DEFAULT_PORT;
    
    /* First argument must be an opt if one passed*/
    if (argc > 1 && argv[1][0] != '-') {
        fprintf(stderr,"%sUnknown option '%s' received. %s\n", RED,argv[1],DEFAULT);
        exit(EXIT_FAILURE);
    }

    opterr = 0; /* Not printing built in error*/
    while ((opt = getopt(argc, argv, "hi:p:")) != -1) {
        switch (opt) {
            case 'h':
                print_usage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'i':
                ip_address = optarg;
                break;
            case 'p':
                port_number = atoi(optarg);
                break;
            case '?':
                if (optopt == 'i' || optopt == 'p') {
                    fprintf(stderr, "%sError: Option '-%c' requires an argument.%s\n", RED, optopt, DEFAULT);
                } else {
                    fprintf(stderr, "%sUnknown option '-%c' received%s.\n", RED, optopt, DEFAULT);
                }
                exit(EXIT_FAILURE);
        }
    }
/* Creating and Setting up Socket */
    struct sockaddr_in server_addr;

    if((*server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Error Creating Socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_number);

    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }
/* Try to connect to server*/
    if (connect(*server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection Failed");
        exit(1);
    }
}

int main(int argc, char *argv[]) {
    int server_fd;
    fd_set read_fds;
    parse_connect(argc, argv, &server_fd);
    char buffer[1024];
/* SERVER ACCEPTANCE*/
        /* Receive messages from server*/
        int recvbytes = recv(server_fd, buffer, 1024, 0);
        if (recvbytes == 0){
            printf("Server closed the connection.\n");
            close(server_fd);
        }
        else {
            buffer[recvbytes] = 0;
            printf("%s%s%s", YELLOW ,buffer,DEFAULT); fflush(stdout);
        }
        /* Send messages to server*/
        scanf("%s", buffer);
        send(server_fd, buffer, strlen(buffer), 0);

/* GAME*/
while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        FD_SET(STDIN_FILENO, &read_fds);

        int max_fd = (server_fd > STDIN_FILENO) ? server_fd : STDIN_FILENO;


        select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &read_fds)) {  /* Input from server */ 
            int recvbytes = recv(server_fd, buffer, sizeof(buffer) - 1, 0);
            if (recvbytes <= 0) {
                printf("Server closed the connection.\n");
                break;
            }
            buffer[recvbytes] = '\0';
            printf("%s", buffer);
        }

        if (FD_ISSET(STDIN_FILENO, &read_fds)) {  /* Input from user */ 
            if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
                send(server_fd, buffer, strlen(buffer), 0);
            }
        }
    }

    close(server_fd);  
    return 0;
}
