#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "run_qr.hpp"
#include "read_map.h"


pthread_mutex_t map_mutex;
pthread_mutex_t qr_mutex;


int main(int argc, char* argv[]) {
    if (argc != 4) {
        printf("Usage: %s <Server IP> <port number> <cur dir>\n", argv[0]);
        return 1;
    }

    // init start direction
    int cur_dir = atoi(argv[3]);

    if (!((cur_dir == 1) || (cur_dir == 3))){
        printf("Current direction must be 1 or 3.\n");
        return 2;
    }

    int cur_x = -1;
    int cur_y = -1;
    int set_bomb = 0;
    int sock = 0;
    struct sockaddr_in serv_addr;

    // Create Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

	const int PORT = atoi(argv[2]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    // Init thread assets (QR)
    pthread_mutex_init(&qr_mutex, NULL);
    pthread_t thread_qr;
    qr_thread_data_t qr_thread_data;

    // Init data struct for qr thread
    qr_thread_data.sock = sock;
    qr_thread_data.cur_x_ptr = &cur_x;
    qr_thread_data.cur_y_ptr = &cur_y;
    qr_thread_data.set_bomb_ptr = &set_bomb;

    // Create and run QR thread
    int qr_thread_ret;
    qr_thread_ret = pthread_create(&thread_qr, NULL, run_qr, (void*)&qr_thread_data);
    if (qr_thread_ret) {
        printf("Error: unable to create thread, %d\n", qr_thread_ret);
        exit(-1);
    }

    // Init thread assets (map)
    pthread_mutex_init(&map_mutex, NULL);
    pthread_t thread_map;
    map_thread_data_t map_thread_data;
    

    // Init data struct for map thread
    DGIST raw_map={0,};
    map_thread_data.sock = sock;
    map_thread_data.raw_map_ptr = &raw_map;

    // Create and run map thread
    int map_thread_ret;
    map_thread_ret = pthread_create(&thread_map, NULL, read_map, (void*)&map_thread_data);
    if (map_thread_ret) {
        printf("Error: unable to create thread, %d\n", map_thread_ret);
        exit(-1);
    }

    // Init for main algorithm (Greedy)
    int dir[4][2] = {{1,0},{0,1},{-1,0},{0,-1}};

    while(1){
        int next_dir= -1;
        int score_min = -1;

        for(int d = 0;d<4;d++){
            int score;
            int diff = d - cur_dir;
            if (diff < 0){
                diff += 4;
            }
            if (diff != 2){
                int nx = cur_x+dir[d][0];
                int ny = cur_y+dir[d][1];
    
                if((ny >= 0 && nx >= 0) && (ny < 5 && nx < 5)){
                    if (raw_map.map[nx][ny].item.status==1 || raw_map.map[nx][ny].item.status==0){
                        if (raw_map.map[nx][ny].item.status==1){
                            score = raw_map.map[nx][ny].item.score;
                        }
                        else{
                            score = 0;
                        }
                        if (score > score_min){
                            next_dir = d;
                        }
                    }
                }
            }
        }
        
        if (next_dir==-1){ // Surrounded by trap
            // turn_left();
            // turn_left();
            // straight();
            cur_dir -= 2;
            if (cur_dir < 0){
                cur_dir += 4;
            }        
        }

        else{ // Go to next node
            int calc_rot = cur_dir - next_dir;
            if (calc_rot < 0){
                calc_rot += 4;
            }

            if (calc_rot == 0){
                // straight();
            } 

            else if (calc_rot == 1){
                // turn_left();
                // straight();
                cur_dir -= 1;
                if (cur_dir < 0){
                    cur_dir += 4;
                }
            }

            else if (calc_rot == 3){
                // turn_right();
                // straight();
                cur_dir += 1;
                if (cur_dir > 4){
                    cur_dir -= 4;
                }
            }

            if ((cur_x==1 && cur_y==1) || (cur_x==1 && cur_y==3) || (cur_x==3 && cur_y==1) || (cur_x==3 && cur_y==3)){
                set_bomb = 1;
                printf("Set Bomb : 1\n");
            }
            else{
                set_bomb = 0;
                printf("Set Bomb : 0\n");

            }
            sleep(1);
        }
    }

    // Wait until thread terminate
    pthread_join(thread_qr, NULL);
    pthread_join(thread_map, NULL);

    pthread_mutex_destroy(&qr_mutex);
    pthread_mutex_destroy(&map_mutex);

    close(sock);
    return 0;
}