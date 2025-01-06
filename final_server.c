#include	"unp.h"
#include  <string.h>
#include  <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>


int cli[12] = {}, mi = 1; //the connfd of player; 
int oppo[12] = {}; //the index of opponent -1 : None
int w = -1; //the index of the player that want to play random -1 : no player
int dir[22][4]; //the number that number i can move (ignore -1)
int watch[12][12] = {}; //if j-th player is watching i-th player
int ps[12] = {};
int state[12] = {}; //1:online 0:leave 2:playing 3:watching
char name[12][MAXLINE]; //player nickname
char board[12][22] = {}; //0 : None 1 : Black(first) 2 : White(second)
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
//int stat[12] = {}; // 0 : None 1 : Online 2 : Fight (Client)

//make the board direction and other
void 
init(){
    mi = 1;
    for(int i = 0; i < 12; ++i){
        state[i] = 0;
        cli[i] = 0;
        oppo[i] = -1;
        ps[i] = -1;
        w = -1;
        for(int j = 0; j < 12; ++j){
            watch[i][j] = 0;
        }
        for(int j = 0; j < 22; ++j){
            board[i][j] = 0;
        }
    }
    dir[1][0] = 2;
    dir[1][1] = 4;
    dir[1][2] = 5;
    dir[1][3] = -1;

    dir[2][0] = 1;
    dir[2][1] = 3;
    dir[2][2] = 4;
    dir[2][3] = -1;

    dir[3][0] = 2;
    dir[3][1] = 4;
    dir[3][2] = 15;
    dir[3][3] = -1;

    dir[4][0] = 1;
    dir[4][1] = 2;
    dir[4][2] = 3;
    dir[4][3] = 17;

    dir[5][0] = 1;
    dir[5][1] = 6;
    dir[5][2] = 8;
    dir[5][3] = -1;

    dir[6][0] = 5;
    dir[6][1] = 7;
    dir[6][2] = 8;
    dir[6][3] = -1;

    dir[7][0] = 6;
    dir[7][1] = 8;
    dir[7][2] = 9;
    dir[7][3] = -1;

    dir[8][0] = 5;
    dir[8][1] = 6;
    dir[8][2] = 7;
    dir[8][3] = 18;

    dir[9][0] = 7;
    dir[9][1] = 10;
    dir[9][2] = 12;
    dir[9][3] = -1;

    dir[10][0] = 9;
    dir[10][1] = 11;
    dir[10][2] = 12;
    dir[10][3] = -1;

    dir[11][0] = 10;
    dir[11][1] = 12;
    dir[11][2] = 13;
    dir[11][3] = -1;

    dir[12][0] = 9;
    dir[12][1] = 10;
    dir[12][2] = 11;
    dir[12][3] = 19;

    dir[13][0] = 11;
    dir[13][1] = 14;
    dir[13][2] = 16;
    dir[13][3] = -1;

    dir[14][0] = 13;
    dir[14][1] = 15;
    dir[14][2] = 16;
    dir[14][3] = -1;

    dir[15][0] = 3;
    dir[15][1] = 14;
    dir[15][2] = 16;
    dir[15][3] = -1;

    dir[16][0] = 13;
    dir[16][1] = 14;
    dir[16][2] = 15;
    dir[16][3] = 20;

    dir[17][0] = 4;
    dir[17][1] = 18;
    dir[17][2] = 20;
    dir[17][3] = 21;

    dir[18][0] = 8;
    dir[18][1] = 17;
    dir[18][2] = 19;
    dir[18][3] = 21;

    dir[19][0] = 12;
    dir[19][1] = 18;
    dir[19][2] = 20;
    dir[19][3] = 21;

    dir[20][0] = 16;
    dir[20][1] = 17;
    dir[20][2] = 19;
    dir[20][3] = 21;

    dir[21][0] = 17;
    dir[21][1] = 18;
    dir[21][2] = 19;
    dir[21][3] = 20;
}

// board_init
void board_init(int index){
    board[index][1] = 2;
    board[index][2] = 2;
    board[index][3] = 2;
    board[index][4] = 2;
    board[index][5] = 2;
    board[index][6] = 0;
    board[index][7] = 1;
    board[index][8] = 0;
    board[index][9] = 1;
    board[index][10] = 1;
    board[index][11] = 1;
    board[index][12] = 1;
    board[index][13] = 1;
    board[index][14] = 0;
    board[index][15] = 2;
    board[index][16] = 0;
    board[index][17] = 0;
    board[index][18] = 0;
    board[index][19] = 0;
    board[index][20] = 0;
    board[index][21] = 0;
}

//check if the chess be eaten 
int 
eat(int board_res[], int index, int x, int vis[]){
    //int res = 1; //0 : safe 1 : be eaten
    vis[x] = 1;
    printf("x = %d\n", x);
    for(int i = 0; i < 4; ++i){
        int nxt = dir[x][i];
        if(nxt != -1){
            if(board_res[nxt] == 0){
                // res = 0;
                // break;
                return 0;
            }
            else if((vis[nxt] == 0) && (board_res[nxt] == board_res[x])){
                vis[nxt] = 1;
                if(eat(board_res, index, nxt, vis) == 0){
                    // res = 0;
                    // break;
                    return 0;
                }
                // if(board_res[nxt] == board_res[x]){
                //     if(eat(board_res, index, nxt, vis) == 0) res = 0;
                // }
            }
        }
    } 
    // if(res == 1) board[index][x] = 0; //be eaten

    // return res;
    return 1;
}

//check if there is anybody win, lose or tie
int 
check(int index, int my){
    //check if there is any chess that be eaten
    //pthread_mutex_lock(&mutex);
    int vis[22] = {};
    int board_res[22];
    for(int i = 1; i <= 21; ++i){
        board_res[i] = board[index][i];
    }
    for(int i = 1; i <= 21; ++i){
        for(int j = 0; j <= 21; ++j) vis[j] = 0;
        if(board_res[i] != 0 && board_res[i] != my){
            printf("check : %d\n", i);
            if(eat(board_res, index, i, vis) == 1) board[index][i] = 0;
        }
    }

    //check final state
    int bnum = 0, wnum = 0;
    for(int i = 1; i <= 21; ++i){
        if(board[index][i] == 1) bnum++;
        else if(board[index][i] == 2) wnum++;
    }
    printf("check over\n");
    if(bnum == 0) return 2; //white win
    else if(wnum == 0) return 1; //black win
    else if(bnum <= 2 && wnum <= 2) return 0; //tie
    return -1; //continue
    //pthread_mutex_unlock(&mutex);
}

//paper scissor stone
void
pss(int index1, int index2){
    //pthread_mutex_lock(&mutex);
    char name1[MAXLINE], name2[MAXLINE];
    char sendline[MAXLINE], recv1[MAXLINE], recv2[MAXLINE];
    int connfd1, connfd2;
    printf("name : %s, %s\n", name[index1], name[index2]);
    ps[index1] = -1;
    ps[index2] = -1;
    connfd1 = cli[index1];
    connfd2 = cli[index2];
    
    //int n1, n2;
    sprintf(sendline, "pss\n"); //tell client it's time to paper scissor stone
    Writen(connfd1, sendline, strlen(sendline));
    Writen(connfd2, sendline, strlen(sendline));

}

void 
update(char *sendline, int index) {
    char res[MAXLINE];
    // pthread_mutex_lock(&mutex);

    // if (index < 1 || index > 10 || cli[index] == 0) {
    //     pthread_mutex_unlock(&mutex);
    //     return;
    // }

    if(strcmp(sendline, "surrender\n") == 0){
        //printf("yes\n");
        for(int i = 1; i <= 10; ++i){
            if(cli[i] > 0){
                sprintf(res, "update\n%s\nonline\n", name[index]);
                Writen(cli[i], res, strlen(res));
            }
        }
        return;
    }

    sprintf(res, "update\n%s\n%s", name[index], sendline);

    for (int i = 1; i <= 10; ++i) {
        if (cli[i] > 0) {
            //printf("update send %s: %s",name[i], res);
            Writen(cli[i], res, strlen(res));
        }
    }

    if(strcmp(sendline, "online\n") == 0){
        for(int i = 1; i <= 10; ++i){
            if(cli[i] > 0 && i != index){
                if(state[i] == 1){
                    sprintf(res, "update\n%s\nonline\n", name[i]);
                }
                else if(state[i] == 2){
                    sprintf(res, "update\n%s\nplaying\n", name[i]);
                }
                else if(state[i] == 3){
                    sprintf(res, "update\n%s\nwatching\n", name[i]);
                }
                Writen(cli[index], res, strlen(res));
            }
        }

    }
    //pthread_mutex_unlock(&mutex);
}

void
speak(int index, char *sendline){
    char res[MAXLINE];
    //pthread_mutex_lock(&mutex);

    if (index < 1 || index > 10 || cli[index] == 0) {
        pthread_mutex_unlock(&mutex);
        return;
    }

    sprintf(res, "speak\n%s", sendline);

    for (int i = 1; i <= 10; ++i) {
        if (cli[i] > 0 && i != index) {
            Writen(cli[i], res, strlen(res));
        }
    }

    //pthread_mutex_unlock(&mutex);
}

void
bye(int connfd){
    //pthread_mutex_lock(&mutex);
    for(int i = 1; i <= 10; ++i){
        if(cli[i] == connfd){
            state[i] = 0;
            cli[i] = 0;
            oppo[i] = -1;
            board_init(i);
            for(int j = 0; j < 12; ++j){
                watch[i][j] = 0;
                watch[j][i] = 0;
            }
            if(i < mi) mi = i;
            break;
        }
    }
    //pthread_cond_signal(&cond);
    //pthread_mutex_unlock(&mutex);
}

//client
void*
client(void *arg){
    int connfd = *(int *)arg; 
    free(arg);
    
    int index = -1;
    char recvline[MAXLINE], sendline[MAXLINE];

    //announce everybody
    pthread_mutex_lock(&mutex);
    for(int i = 1; i <= 10; ++i){
        if(cli[i] == connfd){
            index = i;
            break;   
        }
    }
    pthread_mutex_unlock(&mutex);
    state[index] = 1;
    sprintf(sendline, "online\n");
    update(sendline, index);

    //the clinet is speaking
    int n;
    while((n = readline(connfd, recvline, MAXLINE)) > 0){
        printf("(%s)%s", name[index], recvline);
        recvline[n] = '\0';
        //if client want to speak : "speak" "the content that the player want to say"
        if(strcmp(recvline, "speak\n") == 0){
            printf("speak succesed\n");
            n = readline(connfd, recvline, MAXLINE);
            recvline[n] = '\0';
            printf("content : %s", recvline);
            sprintf(sendline, "(%s) %s", name[index], recvline);
            speak(index, sendline);
        }
        //if client want to surrender
        if(strcmp(recvline, "surrender\n") == 0){
            sprintf(sendline, "win\n");
            if(cli[oppo[index]] > 0) Writen(cli[oppo[index]], sendline, strlen(sendline));
            state[index] = 1;
            sprintf(sendline, "surrender\n");
            update(sendline, index);
            oppo[oppo[index]] = -1;
            oppo[index] = -1;
        }
        //if client pps
        if(strcmp(recvline, "pss\n") == 0){
            //pthread_mutex_lock(&mutex);
            char res[MAXLINE];
            n = readline(connfd, res, MAXLINE);
            res[n - 1] = '\0';
            int a = atoi(res);
            ps[index] = a;
            // printf("%s pss : %d\n", name[index], a);
            // printf("oppo pss : %d\n", ps[oppo[index]]);
            if(a > 0 && ps[oppo[index]] > 0){
                int b = ps[oppo[index]], connfd2 = cli[oppo[index]];
                printf("a : %d, b : %d\n", a, b);
                sprintf(sendline, "winpss\n");

                char board_state[MAXLINE];
                board_state[0] = '\0';
                for(int i = 1; i < 22; ++i){
                    sprintf(board_state + strlen(board_state), "%d", board[index][i]);
                }
                sprintf(board_state + strlen(board_state), "%s", "\n");

                char turn[MAXLINE];
                sprintf(turn, "turn\n");
                char noturn[MAXLINE];
                sprintf(noturn, "noturn\n");
                char bo[MAXLINE];
                sprintf(bo, "board\n");
                char na[MAXLINE];
                char pla[MAXLINE];
                sprintf(pla, "playing\n");

                if(a == b){
                    ps[index] = -1;
                    ps[oppo[index]] = -1;
                    pss(index, oppo[index]);
                }
                else{
                    if(a == 1){
                        if(b == 2){
                            state[index] = 2;
                            state[oppo[index]] = 2;
                            update(pla, index);
                            update(pla, oppo[index]);
                            sprintf(na, "%s\n", name[oppo[index]]);
                            Writen(connfd, sendline, strlen(sendline));
                            Writen(connfd2, sendline, strlen(sendline));
                            Writen(connfd, na, strlen(na));
                            Writen(connfd2, na, strlen(na));
                            Writen(connfd, bo, strlen(bo));
                            Writen(connfd2, bo, strlen(bo));
                            Writen(connfd, board_state, strlen(board_state));
                            Writen(connfd2, board_state, strlen(board_state));
                            Writen(connfd2, turn, strlen(turn));
                            Writen(connfd, noturn, strlen(noturn));
                            }
                        else if(b == 3){
                            update(pla, index);
                            update(pla, oppo[index]);
                            state[index] = 2;
                            state[oppo[index]] = 2;
                            sprintf(na, "%s\n", name[index]);
                            Writen(connfd, sendline, strlen(sendline));
                            Writen(connfd2, sendline, strlen(sendline));
                            Writen(connfd, na, strlen(na));
                            Writen(connfd2, na, strlen(na));
                            Writen(connfd, bo, strlen(bo));
                            Writen(connfd2, bo, strlen(bo));
                            Writen(connfd, board_state, strlen(board_state));
                            Writen(connfd2, board_state, strlen(board_state));
                            Writen(connfd, turn, strlen(turn));
                            Writen(connfd2, noturn, strlen(noturn));
                        }
                        //else pss(index, oppo[index]);
                    }
                    else if(a == 2){
                        if(b == 3){
                            update(pla, index);
                            update(pla, oppo[index]);
                            state[index] = 2;
                            state[oppo[index]] = 2;
                            sprintf(na, "%s\n", name[oppo[index]]);
                            Writen(connfd, sendline, strlen(sendline));
                            Writen(connfd2, sendline, strlen(sendline));
                            Writen(connfd, na, strlen(na));
                            Writen(connfd2, na, strlen(na));
                            Writen(connfd, bo, strlen(bo));
                            Writen(connfd2, bo, strlen(bo));
                            Writen(connfd, board_state, strlen(board_state));
                            Writen(connfd2, board_state, strlen(board_state));
                            Writen(connfd2, turn, strlen(turn));
                            Writen(connfd, noturn, strlen(noturn));
                        }
                        else if(b == 1){
                            update(pla, index);
                            update(pla, oppo[index]);
                            state[index] = 2;
                            state[oppo[index]] = 2;
                            sprintf(na, "%s\n", name[index]);
                            Writen(connfd, sendline, strlen(sendline));
                            Writen(connfd2, sendline, strlen(sendline));
                            Writen(connfd, na, strlen(na));
                            Writen(connfd2, na, strlen(na));
                            Writen(connfd, bo, strlen(bo));
                            Writen(connfd2, bo, strlen(bo));
                            Writen(connfd, board_state, strlen(board_state));
                            Writen(connfd2, board_state, strlen(board_state));
                            Writen(connfd, turn, strlen(turn));
                            Writen(connfd2, noturn, strlen(noturn));
                            
                        }
                        //else pss(index, oppo[index]);
                    }
                    else if(a == 3){
                        if(b == 1){
                            update(pla, index);
                            update(pla, oppo[index]);
                            state[index] = 2;
                            state[oppo[index]] = 2;
                            sprintf(na, "%s\n", name[oppo[index]]);
                            Writen(connfd, sendline, strlen(sendline));
                            Writen(connfd2, sendline, strlen(sendline));
                            Writen(connfd, na, strlen(na));
                            Writen(connfd2, na, strlen(na));
                            Writen(connfd, bo, strlen(bo));
                            Writen(connfd2, bo, strlen(bo));
                            Writen(connfd, board_state, strlen(board_state));
                            Writen(connfd2, board_state, strlen(board_state));
                            Writen(connfd2, turn, strlen(turn));
                            Writen(connfd, noturn, strlen(noturn));
                        }
                        else if(b == 2){
                            update(pla, index);
                            update(pla, oppo[index]);
                            state[index] = 2;
                            state[oppo[index]] = 2;
                            sprintf(na, "%s\n", name[index]);
                            Writen(connfd, sendline, strlen(sendline));
                            Writen(connfd2, sendline, strlen(sendline));
                            Writen(connfd, na, strlen(na));
                            Writen(connfd2, na, strlen(na));
                            Writen(connfd, bo, strlen(bo));
                            Writen(connfd2, bo, strlen(bo));
                            Writen(connfd, board_state, strlen(board_state));
                            Writen(connfd2, board_state, strlen(board_state));
                            Writen(connfd, turn, strlen(turn));
                            Writen(connfd2, noturn, strlen(noturn));
                        }
                        //else pss(index, oppo[index]);
                    }
                }
                ps[index] = -1;
                ps[oppo[index]] = -1;
            }
            //pthread_mutex_unlock(&mutex);
        }
        //if client want to play with special people : "special" "the name of the player who want to play with"
        if(strcmp(recvline, "special\n") == 0){
            //pthread_mutex_lock(&mutex);
            n = readline(connfd, recvline, MAXLINE);
            recvline[n - 1] = '\0';
            printf("oppo : %s\n", recvline);
            int other_connfd = -1;
            for(int i = 0; i < 11; ++i){
                if(strcmp(name[i], recvline) == 0){
                    other_connfd = cli[i];
                    break;
                }
            }
            if(other_connfd == -1){
                printf("wrong name\n");
                continue;
            }
            sprintf(sendline, "invite\n");
            Writen(other_connfd, sendline, strlen(sendline));
            sprintf(sendline, "%s\n", name[index]);
            Writen(other_connfd, sendline, strlen(sendline));
            //pthread_mutex_unlock(&mutex);
        }
      
        //if client want to answer the invition : "answer" "the name of the player who want to answer" "1 : agree 0 : not agree"
        if(strcmp(recvline, "answer\n") == 0){
            //pthread_mutex_lock(&mutex);
            n = readline(connfd, recvline, MAXLINE);
            recvline[n - 1] = '\0';
            int other_connfd = -1; //who you agree or not agree
            for(int i = 0; i < 11; ++i){
                if(strcmp(name[i], recvline) == 0){
                    other_connfd = cli[i];
                    break;
                }
            } 
            n = readline(connfd, recvline, MAXLINE);
            recvline[n] = '\0';


            //int answer = atoi(recvline); //what is the answer
            if(strcmp(recvline ,"1\n") == 0){
                for(int i = 0; i < 12; ++i){
                    if(cli[i] == other_connfd){
                        oppo[index] = i;
                        oppo[i] = index;
                        break;
                    }
                }
                // sprintf(sendline, "(%s) agree\n", name[oppo[index]]);
                // Writen(connfd, sendline, strlen(sendline));
                pss(index, oppo[index]);
            }
            else{
                // for(int i = 0; i < 12; ++i){
                //     if(cli[i] == other_connfd){
                //         oppo[index] = i;
                //     }
                // }
                sprintf(sendline, "inviteno\n");
                Writen(other_connfd, sendline, strlen(sendline));
                sprintf(sendline, "%s\n", name[index]);
                Writen(other_connfd, sendline, strlen(sendline));
                oppo[oppo[index]] = -1;
                oppo[index] = -1;
            }
            //pthread_mutex_unlock(&mutex);
        }
        //if client want to play with random people : "random"
        if(strcmp(recvline, "random\n") == 0){
            pthread_mutex_lock(&mutex);
            if(w == -1){
                w = index;
                sprintf(sendline, "wait\n");
                Writen(connfd, sendline, strlen(sendline));
            } 
            else{
                oppo[index] = w;
                oppo[w] = index;
                // sprintf(sendline, "find another player\n");
                // Writen(connfd, sendline, strlen(sendline));
                // Writen(cli[w], sendline, strlen(sendline));
                printf("go to pss\n");
                //printf("id1 : %d id2 : %d\n", index, w);
                w = -1;
                pthread_mutex_unlock(&mutex);
                pss(index, oppo[index]);
            }
            //printf("w = %d\n", w);
            pthread_mutex_unlock(&mutex);
        }
        //!
        //if client want to watch somebody : "watch" "name"
        if(strcmp(recvline, "watch\n") == 0){
            //pthread_mutex_lock(&mutex);
            n = readline(connfd, recvline, MAXLINE);
            recvline[n - 1] = '\0';
            sprintf(sendline, "watching\n");
            state[index] = 3;
            update(sendline, index);
            for(int i = 0; i < 12; ++i){
                if(strcmp(name[i], recvline) == 0){
                    watch[i][index] = 1;
                    sprintf(sendline, "winpss\n");
                    Writen(connfd, sendline, strlen(sendline));
                    sprintf(sendline, "watch\n");
                    Writen(connfd, sendline, strlen(sendline));
                    sprintf(sendline, "board\n");
                    Writen(connfd, sendline, strlen(sendline));
                    sendline[0] = '\0';
                    for(int j = 1; j < 22; ++j){
                        sprintf(sendline + strlen(sendline), "%d", board[i][j]);
                    }
                    sprintf(sendline + strlen(sendline), "%s", "\n");
                    Writen(connfd, sendline, strlen(sendline));
                    sprintf(sendline, "noturn\n");
                    Writen(connfd, sendline, strlen(sendline));
                    break;
                }
            }
            //pthread_mutex_unlock(&mutex);
        }

        //if client want to move : "move" "a" move to "b"
        if(strcmp(recvline, "move\n") == 0){
            int a = -1, b = -1;
            n = readline(connfd, recvline, MAXLINE);
            recvline[n - 1] = '\0';
            a = atoi(recvline);

            n = readline(connfd, recvline, MAXLINE);
            recvline[n - 1] = '\0';
            b = atoi(recvline);

            printf("move : %d to %d\n", a, b);

            int ch = 0;
            for(int i = 0; i < 4; ++i){
                if(dir[a][i] == b){
                    ch = 1;
                    break;
                }
            }
            if(ch == 0){
                sprintf(sendline, "wrong\n");
                Writen(connfd, sendline, strlen(sendline));
                sprintf(sendline, "board\n");
                Writen(connfd, sendline, strlen(sendline));
                sendline[0] = '\0';
                for(int i = 1; i < 22; ++i){
                    sprintf(sendline + strlen(sendline), "%d", board[index][i]);
                }
                sprintf(sendline + strlen(sendline), "%s", "\n");
                Writen(connfd, sendline, strlen(sendline));
                sprintf(sendline, "turn\n");
                Writen(connfd, sendline, strlen(sendline));
                continue;
            } 

            // sendline[0] = '\0';
            // for(int i = 1; i < 22; ++i){
            //     sprintf(sendline + strlen(sendline), "%d", board[index][i]);
            // }
            // sprintf(sendline + strlen(sendline), "%s", "\n");
            // printf("board : %s", sendline);
            // return;

            //pthread_mutex_lock(&mutex);
            char color = board[index][a];
            board[index][b] = board[index][a];
            board[index][a] = 0;
            // sendline[0] = '\0';
            // for(int i = 1; i < 22; ++i){
            //     sprintf(sendline + strlen(sendline), "%d", board[index][i]);
            // }
            // sprintf(sendline + strlen(sendline), "%s", "\n");
            // printf("after move board : %s", sendline);
            //pthread_mutex_unlock(&mutex);

            int res = check(index, color);

            //printf("I am back\n");

            //pthread_mutex_lock(&mutex);
            for(int i = 1; i <= 21; ++i){
                board[oppo[index]][i] = board[index][i];
            }
            //pthread_mutex_unlock(&mutex);

            //printf("here is ok, too\n");
            //return board to two player "board" and the player that are wathcing
            //pthread_mutex_lock(&mutex);
            sendline[0] = '\0';
            for(int i = 1; i <= 21; ++i){
                sprintf(sendline + strlen(sendline), "%d", board[index][i]);
            }
            sprintf(sendline + strlen(sendline), "%s", "\n");
            //printf("after eat board : %s", sendline);
            //pthread_mutex_unlock(&mutex);

            char bo[MAXLINE];
            sprintf(bo, "board\n");

            Writen(connfd, bo, strlen(bo));
            Writen(connfd, sendline, strlen(sendline));
            Writen(cli[oppo[index]], bo, strlen(bo));
            Writen(cli[oppo[index]], sendline, strlen(sendline));
             for(int i = 0; i < 12; ++i){
                if(cli[i] > 0 && watch[index][i]){
                    Writen(cli[i], bo, strlen(bo));
                    Writen(cli[i], sendline, strlen(sendline));
                    sprintf(sendline, "noturn\n");
                    Writen(cli[i], sendline, strlen(sendline));
                }
            }

            //tell the next player whether is your turn
            char turn[MAXLINE];
            sprintf(turn, "turn\n");
            char noturn[MAXLINE];
            sprintf(noturn, "noturn\n");

            Writen(cli[index], noturn, strlen(noturn));
            Writen(cli[oppo[index]], turn, strlen(turn));

            //return the result
            //pthread_mutex_lock(&mutex);
            if(res == 0){
                //tie
                sprintf(sendline, "tie\n");
                Writen(connfd, sendline, strlen(sendline));
                Writen(cli[oppo[index]], sendline, strlen(sendline));
                oppo[oppo[index]] = -1;
                oppo[index] = -1;
            }
            else if(res == -1){
                continue;
                // sprintf(sendline, "your turn\n");
                // Writen(cli[oppo[index]], sendline, strlen(sendline));
            }
            else if(res == color){
                //win
                sprintf(sendline, "win\n");
                Writen(cli[index], sendline, strlen(sendline));
                sprintf(sendline, "lose\n");
                Writen(cli[oppo[index]], sendline, strlen(sendline));
                oppo[oppo[index]] = -1;
                oppo[index] = -1;
            }
            else{
                //lose
                sprintf(sendline, "lose\n");
                Writen(cli[index], sendline, strlen(sendline));
                sprintf(sendline, "win\n");
                Writen(cli[oppo[index]], sendline, strlen(sendline));
                oppo[oppo[index]] = -1;
                oppo[index] = -1;
            }
            //pthread_mutex_unlock(&mutex);
        }
    }

    //the clinet disconnected
    sprintf(sendline, "leave\n");
    update(sendline, index);
    bye(connfd);
    Close(connfd);
    return NULL;
}

int
main(int argc, char **argv)
{
    init();
    for(int i = 0; i < 12; ++i) board_init(i);
	//TCP server
	int					listenfd, *connfd_ptr;
	pid_t				childpid;
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;
    pthread_t tid;

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(SERV_PORT + 5);

	Bind(listenfd, (SA *) &servaddr, sizeof(servaddr));

	Listen(listenfd, LISTENQ);
	
	char recvline[MAXLINE], sendline[MAXLINE];

	for(;;){
		clilen = sizeof(cliaddr);
        connfd_ptr = malloc(sizeof(int));
        *connfd_ptr = Accept(listenfd, (SA *)&cliaddr, &clilen);

        int connfd = *(int *)connfd_ptr;
		
        int n = read(connfd, recvline, MAXLINE);
        if (n <= 0) {
            close(connfd);
            continue;
        }
        recvline[n] = '\0';
        printf("%s\n", recvline);
        //pthread_mutex_lock(&mutex);
        if(mi == 11){
            sprintf(sendline, "full\n");
            Writen(connfd, sendline, strlen(sendline));
            Close(connfd);
            //pthread_cond_wait(&cond, &mutex);
            //pthread_mutex_unlock(&mutex);
            continue;
        }

        int flag = 1;
        for(int i = 0; i < 12; ++i){
            if(cli[i] > 0 && (strcmp(name[i], recvline) == 0)) flag = 0;
        }
        if(flag){
            strcpy(name[mi], recvline);
            cli[mi] = *connfd_ptr;
            sprintf(sendline, "connect\n");
            Writen(connfd, sendline, strlen(sendline));


            for(int i = 1; i <= 11; ++i){
                if(cli[i] == 0){
                    mi = i;
                    break;
                }
            }
            printf("mi = %d\n", mi);
            
            pthread_create(&tid, NULL, client, connfd_ptr);
        }
        else{
            sprintf(sendline, "rename\n");
            Writen(connfd, sendline, strlen(sendline));
            Close(connfd);
        }
        // if(flag == 1){
        //     strcpy(name[mi], recvline);
        //     cli[mi] = connfd;
        //     sprintf(sendline, "connect");
        //     Writen(connfd, sendline, strlen(sendline));

        //     for(int i = 1; i <= 11; ++i){
        //         if(cli[i] == 0){
        //             mi = i;
        //             break;
        //         }
        //     }

        //     pthread_create(&tid, NULL, client, connfd_ptr);
        // }
        // else{
        //     sprintf(sendline, "rename\n");
        //     Writen(connfd, sendline, strlen(sendline));
        //     Close(connfd);
        // }
        //pthread_mutex_unlock(&mutex);
	}
}