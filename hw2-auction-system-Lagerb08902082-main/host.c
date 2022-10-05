#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>

void get_player_list(char player_char[], int player[], int player_number)
{
    for (int i = 0; i < player_number; i++)
    {
        player[i] = atoi(&player_char[2 * i]);
    }
    return;
}

bool compare_playerlist(int last[], int new[], int size)
{
    for (int i = 0; i < size; i++)
    {
        if (last[i] != new[i])
        {
            return false;
        }
    }
    return true;
}

int get_player_num(int depth)
{
    if (depth == 0)
    {
        return 8;
    }
    else if (depth == 1)
    {
        return 4;
    }
    else if (depth == 2)
    {
        return 2;
    }
    return 0;
}

void init_player_list(int last_list[], int new_list[], int end_player[], int player_num)
{
    for (int i = 0; i < player_num; i++)
    {
        last_list[i] = 0;
        new_list[i] = 0;
        end_player[i] = -1;
    }
    return;
}

int get_score(int round, int fd)
{
    char c[1];
    int rec_line = 0;
    while (rec_line < round - 1)
    {
        read(fd, c, sizeof(c));
        if (c[0] == '\n')
        {
            rec_line++;
        }
    }
    char score_c[100];
    read(fd, c, sizeof(c));
    int line_length = 0;
    int flag = 0;
    while (c[0] != '\n')
    {
        if (flag == 1)
        {
            score_c[line_length] = c[0];
            line_length++;
        }
        if (c[0] == ' ')
        {
            flag = 1;
        }
        read(fd, c, sizeof(c));
    }
    score_c[line_length] = '\0';
    int score = atoi(score_c);
    return score;
}

void set_last_player(int last[], int new[], int player_num)
{
    for (int i = 0; i < player_num; i++)
    {
        last[i] = new[i];
    }
    return;
}
void exec_player(int outfd, int infd, int playerID)
{
    dup2(outfd, STDOUT_FILENO);
    dup2(infd, STDIN_FILENO);
    close(outfd);
    close(infd);
    char player_id[3];
    snprintf(player_id, 3, "%d", playerID);
    execl("./player", "./player", player_id, NULL);
    return;
}
void exec_child(int outfd, int infd, char hostID[], char Key[], int depth)
{
    dup2(outfd, STDOUT_FILENO);
    dup2(infd, STDIN_FILENO);
    close(outfd);
    close(infd);
    char next_depth[2];
    snprintf(next_depth, 2, "%d", depth + 1);
    execl("./host", "./host", hostID, Key, next_depth, NULL);
    return;
}
void r_s(int score[], int final_rank[])
{
    int rank = 1;
    int num = 0;
    for(int i = 10; i >= 0; i--)
    {   
        for(int j = 1; j <= 8; j++)   
        {
            if(score[j] == i)
            {
                final_rank[j] = rank;
                num++;
            }
        }
        rank += num;
        num = 0;
    }
    return;
}

int main(int argc, char *argv[])
{
    // -------------------------------prepare
    // get identity
    int host_id = atoi(argv[1]);
    int key = atoi(argv[2]);
    int depth = atoi(argv[3]);
    // get player number
    int player_num = get_player_num(depth);

    if (depth == 0)
    {
        // get fifoname
        char myfifo[20];
        snprintf(myfifo, 12, "fifo_%d.tmp", host_id);
        // char outfifo[20];
        // snprintf(outfifo, 12, "fifo_0.tmp");

        // fprintf(stderr, "in fifo = %s\nout fifo = %s\n", myfifo, outfifo);
        
        // open fifo
        // int infd = open(myfifo, O_RDONLY);
        int outfd = open("fifo_0.tmp", O_RDWR);
        // FILE* fifo_wr = fopen(outfifo, "r+");
                    FILE* fifo_rd = fopen(myfifo, "r+");

        // prepare buffer

        // fork child
        int to_child[2];
        int from_child[2];
        pipe(to_child);
        pipe(from_child);

        int to_child2[2];
        int from_child2[2];
        pipe(to_child2);
        pipe(from_child2);
        pid_t pid;
        pid = fork();
        if (pid < 0)
        {
            return -1;
        }
        else if (pid == 0)
        {
            exec_child(from_child[1], to_child[0], argv[1], argv[2], depth);
        }
        else if (pid > 0)
        {
            FILE* f_child = fdopen(from_child[0], "r");
            pid_t pid2;
            pid2 = fork();
            if (pid2 < 0)
            {
                return -1;
            }
            else if (pid2 == 0)
            {
                // sleep(1000);
                exec_child(from_child2[1], to_child2[0], argv[1], argv[2], depth);
            }
            else if (pid2 > 0)
            {
                FILE* f_child2 = fdopen(from_child2[0], "r");
                while (true)
                {
                    // fprintf(stderr, "go to loop\n");
                    int score[12 + 1];
                    for(int i = 0; i <= 12; i++)
                    {
                        score[i] = 0;
                    }
                    char player_buf[100];
                    // fprintf(stderr, "get list\n");
                    fgets(player_buf, 100, fifo_rd);

                    // read(infd, player_buf, 100);

                    // fprintf(stderr, "get list end\n");
                    // fprintf(stderr, "1: %s", player_buf);

                    int lp[4];
                    int rp[4];
                    sscanf(player_buf, "%d %d %d %d %d %d %d %d", &lp[0], &lp[1], &lp[2], &lp[3], &rp[0], &rp[1], &rp[2], &rp[3]);

                    int p_board[9];
                    for(int i = 1; i <= 4; i++)
                    {
                        p_board[i] = lp[i - 1];
                    }
                    for(int i = 5; i <= 8; i++)
                    {
                        p_board[i] = rp[i - 5];
                    }

                    // fprintf(stderr, "2: %s", player_buf);

                    char left[100];
                    char right[100];
                    sprintf(left ,"%d %d %d %d\n", lp[0], lp[1], lp[2], lp[3]);
                    sprintf(right, "%d %d %d %d\n", rp[0], rp[1], rp[2], rp[3]);

                    // fprintf(stderr, "3: %s", player_buf);
                    // fprintf(stderr, "3: %s", left);
                    // fprintf(stderr, "3: %s", right);

                    write(to_child[1], left, strlen(left));
                    write(to_child2[1], right, strlen(right));
                    
                    if(lp[0] == -1)
                    {
                        // fprintf(stderr, "host end\n");
                        sleep(2);
                        exit(-1);
                        return -1;
                    }

                    // maybe can sleep ??
                    // fprintf(stderr, "go to loop\n");
                    for(int i = 0; i < 10; i++)
                    {   
                        // fprintf(stderr, "loop %d\n", i);
                        int lplayer, lscore;
                        int rplayer, rscore;
                        // fscanf(f_child, "%d %d", &lplayer, &lscore);
                        // fscanf(f_child2, "%d %d", &rplayer, &rscore);
                        char lps[100];
                        char rps[100];
                        // fprintf(stderr, "get score\n");

                        fgets(lps, 100, f_child);
                        fgets(rps, 100, f_child2);
                        sscanf(lps, "%d %d", &lplayer, &lscore);
                        sscanf(rps, "%d %d", &rplayer, &rscore);
                        // fprintf(stderr, "%d %d\n", lplayer, lscore);
                        // fprintf(stderr, "%d %d\n", rplayer, rscore);
                        if(lscore > rscore)
                        {
                            score[lplayer]++;
                        }
                        else
                        {
                            score[rplayer]++;
                        }
                    }
                    int eight_score[9];
                    for(int i = 1; i <= 8; i++)
                    {
                        eight_score[i] = score[p_board[i]];
                    }
                    // fprintf(stderr, "loop end\n");
                    int final_rank[9];
                    r_s(eight_score, final_rank);
                    // write rank to fifo'
                    char rank_c[100];
                    char output[100] = {'\0'};
                    for(int i = 1; i <= 8; i++)
                    {
                        // fprintf(rank_c, "%d %d\n", i, final_rank[i]);
                        // write(fifo_wr, rank_c, sizeof(rank_c));
                        snprintf(output, 100, "%d %d\n", p_board[i], final_rank[i]);
                        // snprintf(output, "%d %d\n", i, final_rank[i]);
                        write(outfd, output, strlen(output));
                        // fprintf(fifo_wr, "%d %d\n", i, final_rank[i]);

                        // fprintf(stderr, "%d %d\n", p_board[i], final_rank[i]);
                    }
                    // fprintf(stderr, "loop end\n");
                    // sleep(100);
                }
            }
        }
    }
    else if (depth == 1)
    {
        // sleep(100);
        int to_child[2];
        int from_child[2];
        pipe(to_child);
        pipe(from_child);

        int to_child2[2];
        int from_child2[2];
        pipe(to_child2);
        pipe(from_child2);
        pid_t pid;
        pid = fork();
        if (pid < 0)
        {
            return -1;
        }
        else if (pid == 0)
        {
            exec_child(from_child[1], to_child[0], argv[1], argv[2], depth);
        }
        else if (pid > 0)
        {
            FILE* f_child = fdopen(from_child[0], "r");
            pid_t pid2;
            pid2 = fork();
            if (pid2 < 0)
            {
                return -1;
            }
            else if (pid2 == 0)
            {
                // sleep(1000);
                exec_child(from_child2[1], to_child2[0], argv[1], argv[2], depth);
            }
            else if (pid2 > 0)
            {
                FILE* f_child2 = fdopen(from_child2[0], "r");
                while(true)
                {
                    char player_c[100];
                    // fgets(player_c, 100, stdin); 
                    read(STDIN_FILENO, player_c, 100);
                    int player[4];
                    sscanf(player_c, "%d %d %d %d", &player[0], &player[1], &player[2], &player[3]);
                    // fprintf(stderr, "total %s\n", player_c); 
                    char lplayers[100];
                    char rplayers[100];
                    snprintf(lplayers, 100, "%d %d\n", player[0], player[1]);
                    snprintf(rplayers, 100, "%d %d\n", player[2], player[3]);
                    write(to_child[1], lplayers, 100);
                    write(to_child2[1], rplayers, 100);

                    if(player[0] == -1)
                    {
                        sleep(1);
                        exit(-1);
                        return -1;
                    }

                    // fprintf(stderr, "left %d %d\n", player[0], player[1]);
                    // fprintf(stderr, "right %d %d\n", player[2], player[3]);
                    for(int i = 0; i < 10; i++)
                    {
                        int lplayer, lscore;
                        int rplayer, rscore;
                        char lps[100];
                        char rps[100];
                        fgets(lps, 100, f_child);
                        fgets(rps, 100, f_child2);
                        sscanf(lps, "%d %d", &lplayer, &lscore);
                        sscanf(rps, "%d %d", &rplayer, &rscore);
                        if (lscore > rscore)
                        {
                            printf("%d %d\n", lplayer, lscore);
                            fflush(stdout); 
                        }
                        else
                        {
                            printf("%d %d\n", rplayer, rscore);   
                            fflush(stdout); 
                        }
                        // fprintf(stderr, "thrown\n");
                    }
                }
            }
        }
    }
    else if (depth == 2)
    {
        int player1;
        int player2;
        while(true)
        {
            char p_2[100];
            // fgets(p_2, 100, stdin);
            read(STDIN_FILENO, p_2, 100);
            sscanf(p_2, "%d %d", &player1, &player2);

            if(player1 == -1)
            {
                exit(-1);
                return -1;
            }

            // fprintf(stderr, "player is %d %d\n", player1, player2);

            
            int to_player[2];
            int from_player[2];
            pipe(to_player);
            pipe(from_player);
            pid_t pid;
            pid = fork();
            if (pid < 0)
            {
                return -1;
            }
            else if (pid == 0)
            {
                exec_player(from_player[1], to_player[0], player1);
                // exit(-1);
            }
            else if (pid > 0)
            {
                FILE* f_p = fdopen(from_player[0], "r");
                int to_player2[2];
                int from_player2[2];
                pipe(to_player2);
                pipe(from_player2);
                pid_t pid2;
                pid2 = fork();
                
                if (pid2 < 0)
                {
                    return -1;
                }
                else if (pid2 == 0)
                {
                    exec_player(from_player2[1], to_player2[0], player2);
                    exit(-1);
                }
                else if (pid2 > 0)
                {
                    FILE* f_p2 = fdopen(from_player2[0], "r");
                    for(int i = 0; i < 10; i++)
                    {
                        // fprintf(stderr, "loop == %d\n");
                        char id_s[100];
                        char id_s2[100];
                        // fprintf(stderr, "non0000");

                        fgets(id_s, 100, f_p);
                        fgets(id_s2, 100, f_p2);
                        int p1;
                        int score;
                        int p2;
                        int score2;
                        // fprintf(stderr, "non");
                        sscanf(id_s, "%d %d", &p1, &score);
                        sscanf(id_s2, "%d %d", &p2, &score2);
                        if(score > score2)
                        {
                            printf("%d %d\n", p1, score);
                            fflush(stdout); 
                        }
                        else
                        {
                            printf("%d %d\n", p2, score2);
                            fflush(stdout); 
                        }
                        // fprintf(stderr, "leaf thrown\n");
                    }
                    wait(NULL);
                    wait(NULL);
                }
            }
        }
    }
    return 0;
}