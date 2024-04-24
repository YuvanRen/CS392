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
#include <stdbool.h>
/*
    Yuvan Rengifo  
    I pledge my honor that I have abided by the Stevens Honor System.

*/

#define DEFAULT_FILE "questions.txt"
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 25555
#define MAX_CONNECTIONS 2

#define YELLOW "\x1B[38;5;227m"
#define RED "\x1B[38;5;196m"  /* For errors */
#define BLUE "\x1B[1;34m"
#define GREEN "\x1B[32m"
#define NGREEN "\x1B[92m"
#define DEFAULT "\x1b[0m"

void print_usage(char *prog) {
    printf("Usage: %s [-f question_file] [-i IP_address] [-p port_number] [-h]\n\n", prog);
    printf(" -f question_file       Default to \"questions.txt\";\n");
    printf(" -i IP_address          Default to \"127.0.0.1\";\n");
    printf(" -p port_number         Default to 25555;\n");
    printf(" -h                     Display this help info.\n");
}

struct Entry{
    char prompt[1024];
    char options[3][50];
    int answer_idx;
}Entry;

struct Player{
    int fd;
    int score;
    char name[128];
}Player;

int read_questions(struct Entry* arr, char* filename){
    int count = 0;  /* Questions count*/
    FILE* fp = fopen(filename, "r");

    /* Failed to open file */
    if (!fp) {
        fprintf(stderr, "%sError: Can't open file. %s\n", RED, DEFAULT);
        exit(EXIT_FAILURE);
    }

    char line[1024];

    while (fgets(line, sizeof(line), fp) != NULL) {
        /* Skip any empty lines */ 
        if (strcmp(line, "\n") == 0) continue;

        /* Copy the question prompt */ 
        strcpy(arr[count].prompt, line);

        /* Read the options line */ 
        if (fgets(line, sizeof(line), fp) == NULL) break;
        sscanf(line, "%s %s %s", arr[count].options[0], arr[count].options[1], arr[count].options[2]);

        /* Read the answer index line */ 
        if (fgets(line, sizeof(line), fp) == NULL) break;
        arr[count].answer_idx = atoi(line);

        /* Increment question count */
        count++;

        /* Too many questions*/
        if (count >= 50) break;
    }

    fclose(fp);
    return count; 
}

/* Helper to display messages to players mid game*/
void display(struct Player* players, int total_players, const char *message) {
    for (int i = 0; i < total_players; i++) {
        if (players[i].fd != -1) {
            write(players[i].fd, message, strlen(message));
        }
    }
}

int main(int argc, char *argv[]) {
    char *question_file = DEFAULT_FILE;
    char *ip_address = DEFAULT_IP;
    int port_number = DEFAULT_PORT;
    int opt;

    /* Parsing Arguments*/

    /* First argument must be an opt*/
    if (argc > 1 && argv[1][0] != '-') {
        fprintf(stderr,"%sUnknown option '%s' received. %s\n", RED,argv[1],DEFAULT);
        exit(EXIT_FAILURE);
    }

    opterr = 0; /* Not printing built in error*/
    while ((opt = getopt(argc, argv, "hf:i:p:")) != -1) {
        switch (opt) {

            case 'h':
                print_usage(argv[0]);
                return EXIT_SUCCESS;

            case 'f':
                question_file = optarg;
                break;

            case 'i':
                ip_address = optarg;
                break;

            case 'p':
                port_number = atoi(optarg);
                break;

            case '?':
                if (optopt == 'f' || optopt == 'i' || optopt == 'p')  {
                    fprintf(stderr,"%sError: Option '-%c' requires an argument.%s\n",RED, optopt, DEFAULT);
                } else{
                    fprintf(stderr,"%sUnknown option '-%c' received%s.\n",RED, optopt, DEFAULT);
                }
                exit(EXIT_FAILURE);
                }
        }

    /* IPC stuff setup*/    
    int server_fd;
    int client_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in in_addr;
    socklen_t addrlen = sizeof(in_addr);

/* Creating and Setting up Socket */
    if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Error Creating Socket");
        exit(1);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family        = AF_INET;
    server_addr.sin_port          = htons(port_number);

    /* Make sure ip is valid */
    if (inet_pton(AF_INET, ip_address, &server_addr.sin_addr) <= 0) {
        perror("Invalid address");
        exit(1);
    }

/* Bind file descriptor with adress structure*/
    if(bind(server_fd, (struct sockaddr *) &server_addr, sizeof(server_addr))){
                perror("Error binding");
                exit(1);
            }
    
 /* Listen at most 2 connections*/
    if(listen(server_fd, MAX_CONNECTIONS) != 0){
        perror("Listening error");
        exit(1);
    }

/* Succeeds :D */
    char* Indent = "                ";
    printf("%s%s+------------------------+\n", YELLOW,Indent);
    printf("%s%s| Welcome to 392 Trivia! |\n",YELLOW,Indent);
    printf("%s%s+------------------------+%s\n", YELLOW,Indent, DEFAULT);


/* Read questions database*/
    struct Entry questions[50]; /* Max 50 questions */
    int quesNum = read_questions(questions, question_file); /* get number of questions*/
    if (quesNum < 0) {
        fprintf(stderr, "%sFailed to read questions from the file%s\n",RED, DEFAULT);
        exit(EXIT_FAILURE);
    }

/* Accepting connections to enable comms*/
    fd_set myset;
    FD_ZERO(&myset);
    FD_SET(server_fd,&myset);
    struct Player players[MAX_CONNECTIONS];
    int player_c = 0;
    int game_started = 0;
    int max_fd = server_fd;

    /* Inititate every players attributes*/
    for(int i = 0; i < MAX_CONNECTIONS; i ++) {
        players[i].fd = -1;
        strcpy(players[i].name, "null");
        players[i].score =0;
    }

    char buffer[1024];
    int recvbytes = 0;
    char* getName = "Please type your name:\n";


    /* To accept connections*/
    while(1){

        if (game_started) {
            break;  /* Game already started*/
        }

        /* Re-initiation */
        FD_SET(server_fd,&myset);
        max_fd = server_fd;

        for(int i = 0; i< MAX_CONNECTIONS; i++){
            if(players[i].fd != -1 ){ 
                FD_SET(players[i].fd,&myset);
                if(players[i].fd > max_fd) max_fd = players[i].fd;
            }
        }

        /* Monitor fds*/
        select(max_fd+1,&myset,NULL, NULL, NULL);

        /* Incoming connection */
        if(FD_ISSET(server_fd, &myset)){
            client_fd = accept(server_fd,
                                (struct sockaddr*)&in_addr,
                                &addrlen);
            if(player_c < MAX_CONNECTIONS){
                printf("%sNew connection detected!%s\n",GREEN,DEFAULT);
                player_c++;
                for(int i = 0; i < MAX_CONNECTIONS; i++){
                    if(players[i].fd == -1){
                        players[i].fd = client_fd;
                        write(players[i].fd, getName, strlen(getName));
                        break;
                    } 

                }
            }else{
                printf("%sMax connections reached!%s\n",RED,DEFAULT);
                close(client_fd);
            }
        }
        /* Check which player is ready for IO*/
        for(int i =0; i< MAX_CONNECTIONS;i++){
            if(players[i].fd != -1 && FD_ISSET(players[i].fd, &myset)){
                 /* Request players name*/
                    /* read name */
                        /* && */
               /* Check for lost connection*/
                recvbytes = read(players[i].fd, buffer, 1024);
                if(recvbytes == 0){
                    printf("%sConnection lost!%s\n",RED,DEFAULT);
                    close(players[i].fd);
                    players[i].fd = -1;
                    player_c --;

                }else{
                buffer[recvbytes] = 0; /* Null terminate*/
                printf("Hi %s!\n", buffer);

                /* Set players name*/
                strcpy(players[i].name,buffer);
                }
            }
        }

        /* All players are ready to begin game*/
        int flag = 1;
        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            /* Check if any player has still "null" as the name */ 
            if (strcmp(players[i].name, "null") == 0) {
                flag = 0; /* Not all players have a name */
                break;
            }
        }
        if (player_c == MAX_CONNECTIONS && flag == 1 ){
            printf("%s%s  THE GAME STARTS NOW! :D%s\n\n", BLUE, Indent,DEFAULT);
            game_started = 1; /* Get out of accepting loop*/
        } 
    } 

/* Start Game logic */
int quesIndx = 0;
/* Go through each question*/
    while(quesIndx < quesNum){
    /* Question and Options to screen */
    
        printf("%sQuestion %d: %s%s\n", YELLOW, quesIndx + 1, questions[quesIndx].prompt,DEFAULT);
        printf("1: %s\n2: %s\n3: %s\n\n", questions[quesIndx].options[0], 
                                        questions[quesIndx].options[1], 
                                        questions[quesIndx].options[2]);

    /* Question and Options to players */
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (players[i].fd != -1) {
            snprintf(buffer, sizeof(buffer), "Question %d: %s\n", quesIndx + 1, questions[quesIndx].prompt);
            write(players[i].fd, buffer, strlen(buffer));

            for (int j = 0; j < 3; j++) {
                snprintf(buffer, sizeof(buffer), "Press %d: %s\n", j + 1, questions[quesIndx].options[j]);
                write(players[i].fd, buffer, strlen(buffer));
            }
        }
    }
    /* Set up for select*/
    fd_set read_fds;
    FD_ZERO(&read_fds);
    int max_fd = server_fd;

        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (players[i].fd != -1) {
                FD_SET(players[i].fd, &read_fds);
                if (players[i].fd > max_fd) {
                    max_fd = players[i].fd;
                }
            }
        }
        select(max_fd + 1, &read_fds, NULL, NULL,NULL);
         int active_players = 0;
         int remaining_player=0;


/* Players ready for IO*/     /* Read answers and check for any lost connection*/
        for(int i = 0; i < MAX_CONNECTIONS; i++){
            if(players[i].fd != -1 && FD_ISSET(players[i].fd, &read_fds)){
        /* read players answers*/
        memset(buffer, 0, sizeof(buffer));  // Clear buffer
                int ans_bytes = read(players[i].fd, buffer, sizeof(buffer) - 1);

/* Check who gets answer correct first and assign points */
                if(ans_bytes > 0){
                    buffer[ans_bytes] = 0; /* Terminator*/
                    int choice = atoi(buffer) - 1;

            /* A player got correct answer*/
                    if(choice == questions[quesIndx].answer_idx){
                        players[i].score++;
            /* Not correct */
                    }else{
                        players[i].score--;
                    }

        /* Print correct answer to screen*/
                    printf("%s\nCorrect answer was: \n   %s%s%s\n\n", BLUE,DEFAULT,Indent,questions[quesIndx].options[questions[quesIndx].answer_idx]);

        /* Send out correct answer */
                     for (int k = 0; k < MAX_CONNECTIONS; k++) {
                        if (players[k].fd != -1) {
                            snprintf(buffer, sizeof(buffer), "%sCorrect answer was: %s%s\n\n", GREEN,questions[quesIndx].options[questions[quesIndx].answer_idx] ,DEFAULT);                    
                            write(players[k].fd, buffer, strlen(buffer));
                        }
                    }
                    break;

             }/* Disconnected player handler*/
                else if(ans_bytes == 0){
                    printf("%sConnection lost!%s\n",RED,DEFAULT);
                    close(players[i].fd);
                    players[i].fd = -1;
     /*-----------------------------------------------------------------------*/
            /* Check for current active players */
                        for (int j = 0; j < MAX_CONNECTIONS; j++) {
                             if (players[j].fd != -1) {
                                active_players++;
                                remaining_player = j;
                                }
                            }
            /* Forfeit winning message*/
                        if(active_players == 1){
                            printf("%s%s+------------------------+\n", RED,Indent);
                            printf("%s%s|      GAME OVER !       |\n",RED,Indent);
                            printf("%s%s+------------------------+%s\n\n", RED,Indent, DEFAULT);

                            printf("%s%s Wins by forfeit!\n",NGREEN, players[remaining_player].name);
                            goto end;
                        }
            /* All players disconnected*/
                        else if(active_players == 0){

                            printf("%s%s+------------------------+\n", RED,Indent);
                            printf("%s%s|      GAME OVER !       |\n",RED,Indent);
                            printf("%s%s+------------------------+%s\n\n", RED,Indent, DEFAULT);
                        
                            printf("%sNo more players in the server :(%s\n\n", RED, DEFAULT);
                            goto end;
                        }
     /*-----------------------------------------------------------------------*/
                } /* answer checker closure*/
        } /* if io closure*/
    } /* IO for loop closure*/
    /* Next question*/
     quesIndx++; 
}


/* GAME OVER*/
    printf("%s%s+------------------------+\n", RED,Indent);
    printf("%s%s|      GAME OVER !       |\n",RED,Indent);
    printf("%s%s+------------------------+%s\n\n", RED,Indent, DEFAULT);


/* Determine Winner*/
    int max = 0;
    int winner_idx = 0;
    bool tie = false;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (players[i].fd != -1 && players[i].score > max) {
            max = players[i].score;
            winner_idx = i;

        } else if ((players[i].fd != -1 && max == players[i].score)){
                tie = true;
                printf("%sTIE!\n",RED);
                printf("%sUsers with same score:%d%s\n",YELLOW, max,DEFAULT);
                for(int j = 0; j < MAX_CONNECTIONS; j++) {
                      if (players[i].fd != -1 && players[i].score == max) {
                        printf("%s\n",players[j].name);
                }
            }
         printf("%sTHANK YOU FOR PLAYING! :D\n",BLUE);
         goto end;
      }
    }
/* Winner message */             
    printf("%sCongrats, %s!.\n%sYou win with a score of:%s %d\n", NGREEN,players[winner_idx].name,BLUE,DEFAULT,players[winner_idx].score);


/* Print Scoreboard */
    printf("%s+------------------------+\n", RED);
    printf("%s|      Scoreboard        |\n",RED);
    printf("%s+------------------------+%s\n", RED,DEFAULT);

    for(int j = 0; j < MAX_CONNECTIONS; j++) {
        printf("%sUser:%s%s\n",YELLOW,DEFAULT,players[j].name);
        printf("%sScore:\n", YELLOW);
        printf("%s%d\n\n",NGREEN,players[j].score);
        }
end:
/* Close all connections*/
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (players[i].fd != -1) {
            close(players[i].fd);
            players[i].fd = -1;
        }
    }
    close(server_fd);
    return 0;
}
