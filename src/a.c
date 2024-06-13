#include <stdio.h>          	// 입출력 관련 
#include <stdlib.h>         	// 문자열 변환, 메모리 관련 
#include <string.h>       	    // 문자열 처리 
#include <time.h>         	    // 시간 관련
#include<unistd.h>       		// POSIX 운영체제 API에 대한 액세스 제공 
#include<fcntl.h>			    // 타겟시스템 입출력 장치 관련 
#include<sys/types.h>    		// 시스템에서 사용하는 자료형 정보 
#include<sys/ioctl.h>    		// 하드웨어의 제어와 상태 정보 
#include<sys/stat.h>     		// 파일의 상태에 대한 정보
#include<stdbool.h>
#include<pthread.h>

// Target System
#define fnd "/dev/fnd"			// 7-Segment FND 
#define dot "/dev/dot"			// Dot Matrix
#define tact "/dev/tactsw"    	// Tact Switch
#define led "/dev/led"			// LED 
#define dip "/dev/dipsw"		// Dip Switch
#define clcd "/dev/clcd"		// Character LCD

int turn(int*);
void roll_dice(int*);
void* dot_fnd_thread(void* arg);
// void set_roll_cnt(char);
void set_lcd_bot(int);
void set_turn_score(int);
void calc_score(int*, int*);

// 파일 디스크립터
int tactsw = 0;
int dipsw = 0;
int leds = 0;
int dot_mtx = 0;
int clcds = 0;
int fnds = 0;

// 뮤텍스
pthread_mutex_t dot_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t fnd_mutex = PTHREAD_MUTEX_INITIALIZER;

unsigned char fnd_map[15] = {
    0b11000000, // 0
    0b11111001, // 1
    0b10100100, // 2
    0b10110000, // 3
    0b10011001, // 4
    0b10010010, // 5
    0b10000010, // 6
    0b11111000, // 7
    0b10000000, // 8
    0b10010000, // 9
    0b11000001, // U
    0b10010010, // S
    0b10000110, // E
    0b10100001, // d
    0b11111111 //출력 없음
    };

unsigned char dot_buffer[8];
unsigned char fnd_buffer[4];

int main(){
    if((dipsw = open( dip, O_RDWR )) < 0){printf("dip open error\n"); exit(1);}
    
    memset(dot_buffer, 0xff, sizeof(dot_buffer)); // 도트 버퍼 초기화
    memset(fnd_buffer, 0x00, sizeof(fnd_buffer)); // fnd 버퍼 초기화
    pthread_t dot_thread;
    pthread_create(&dot_thread, NULL, dot_fnd_thread, NULL);

    unsigned char dip_input = 0;
     while(1) {
        usleep(1000000); // 0.01 초 쉬기
        read(dipsw, &dip_input, sizeof(dip_input));
        if(dip_input)printf("?");
     }
    sleep(30);
}


void* dot_fnd_thread(void* arg) {
    while (1) {
        dot_mtx = open(dot, O_RDWR);
        pthread_mutex_lock(&dot_mutex);
        write(dot_mtx, dot_buffer, sizeof(dot_buffer));
        usleep(500000); // 0.5 초
        pthread_mutex_unlock(&dot_mutex);
        close(dot_mtx);


        fnds = open(fnd, O_RDWR);
        pthread_mutex_lock(&fnd_mutex);
        write(fnds, &fnd_buffer, sizeof(fnd_buffer));
        usleep(500000); // 0.5 초
        pthread_mutex_unlock(&fnd_mutex);
        close(fnds);
    }
    return NULL;
}

//1. dot와 mtx는 파일디스크럽터를 같이 열면 출력 오류가 발생한다(노이즈 처럼)
// 추정 : 메모리를 공유하거나 하는듯
//2. dot와 mtx 출력중 read를 빠르게 반복하면 출력 오류가 발생한다(노이즈처럼)
// 추정 : 이건 진짜 모르겠음



// dot와 fnd를 같이 사용X
// dot fnd 출력중에 입력대기시 노이즈발생