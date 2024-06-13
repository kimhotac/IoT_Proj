#include <stdio.h>          	// 입출력 관련 
#include <stdlib.h>         	// 문자열 변환, 메모리 관련 
#include <string.h>       	    // 문자열 처리 
#include <time.h>         	    // 시간 관련
#include<unistd.h>       		// POSIX 운영체제 API에 대한 액세스 제공 
#include<fcntl.h>			    // 타겟시스템 입출력 장치 관련 
#include<sys/types.h>    		// 시스템에서 사용하는 자료형 정보 
#include<sys/ioctl.h>    		// 하드웨어의 제어와 상태 정보 
#include<sys/stat.h>     		// 파일의 상태에 대한 정보 
#include <errno.h>              // 에러 정보
#include<stdbool.h>

// Target System
#define fnd "/dev/fnd"			// 7-Segment FND 
#define dot "/dev/dot"			// Dot Matrix
#define tact "/dev/tactsw"    	// Tact Switch
#define led "/dev/led"			// LED 
#define dip "/dev/dipsw"		// Dip Switch
#define clcd "/dev/clcd"		// Character LCD

void set_dice(int*);
// void set_roll_cnt(char);
void set_lcd_bot(int);
void set_turn_score(int);
void cleanup_resources();

int tactsw = 0;
int dipsw = 0;
int leds = 0;
int dot_mtx = 0;
int clcds = 0;
int fnds = 0;

unsigned char fnd_output[15] = {
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
unsigned char dot_buffer[8] = {
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
};
char clcd_top[17] = {
    " P1 000  P2 000 "
};
char clcd_bot[18][17] = {
    "      Ones      ",
    "      Twos      ",
    "     Threes     ",
    "      Fours     ",
    "      Fives     ",
    "      Sixes     ",
    "   Full House   ",
    " Four of a Kind ",
    "Little Straight ",
    "  Big Straight  ",
    "     Choice     ",
    "      Yacht     ",
    "  P1 turn Roll  ",
    "  P2 turn Roll  ",
    "    P1 win!!    ",
    "    P2 win!!    ",
    "      draw      ",
    "  pick or roll  "
};

int main() {
    srand(time(NULL));
    atexit(cleanup_resources);

    if((tactsw = open( tact, O_RDWR )) < 0){
        printf("tact open error\n");
        exit(1);
    }

    if((dipsw = open( dip, O_RDWR )) < 0){
        printf("dip open error\n");
        exit(1);
    }

    if((clcds = open(clcd, O_RDWR)) < 0 ) {
        printf("clcd open error\n");
        exit(1);
    }
    
    if((dot_mtx = open(dot, O_RDWR)) < 0 ){
        printf("dot open error\n");
        exit(1);
    }

    if((fnds = open(fnd, O_RDWR)) < 0 ) {
        printf("fnd open error\n");
        exit(1);
    }
    set_lcd_bot(0);
    return 0;
}

void set_dice(int* dice) {
    int i, j;

    // 열
    for(i = 0; i < 5; i++) { 
        // 행
        for(j = 7; j > 1 ; j--) {
            if(8 - j <= dice[i]) {
                dot_buffer[j] |= (128 >> i);  // 해당 비트를 켭니다.
            } 
            else {
                dot_buffer[j] &= ~(128 >> i); // 해당 비트를 끕니다.
            }
        }
    }

    printf("[dot] %d %d %d %d %d\n",dice[0],dice[1],dice[2],dice[3],dice[4]);
    unsigned char bytes_written = write(dot_mtx, &dot_buffer, sizeof(dot_buffer));
    if (bytes_written == -1) {
        fprintf(stderr, "Failed to write to the dot matrix: %s\n", strerror(errno));
    }
    usleep(10000); // 0.01초

    return;
}

void set_lcd_bot(int line) {

    char buffer[32];  // 32글자를 위한 버퍼
    snprintf(buffer, 32, "%s%s", clcd_top, clcd_bot[line]);

    printf("[CLCD] %s\n",buffer);
    unsigned char bytes_written = write(clcds, buffer, 32);
    if (bytes_written == -1) {
        fprintf(stderr, "Failed to write to the CLCD: %s\n", strerror(errno));
    }
    usleep(100000); // 0.01초

    return;
}

void set_turn_score(int score) {
    unsigned char buffer[4];

    //턴당 점수 50 이상 불가능 USEd에 사용
    if(score>50) {
        buffer[0] = fnd_output[10];
        buffer[1] = fnd_output[11];
        buffer[2] = fnd_output[12];
        buffer[3] = fnd_output[13];
    }
    else {
        int tens = (score / 10) % 10;
        int ones = score % 10;
        buffer[0] = fnd_output[14];
        buffer[1] = fnd_output[tens];
        buffer[2] = fnd_output[ones];
        buffer[3] = fnd_output[14];
    }

    printf("[fnd] %d\n",score);
    unsigned char bytes_written = write(fnds, &buffer, sizeof(buffer));
    if (bytes_written == -1) {
        fprintf(stderr, "Failed to write to the fnd: %s\n", strerror(errno));
    }
    sleep(2);
    // usleep(100000); // 0.01초

    return;
}

void cleanup_resources() {
    if (tactsw > 0) {
        close(tactsw);
        printf("Tact switch file descriptor closed.\n");
    }
    if (dipsw > 0) {
        close(dipsw);
        printf("Dip switch file descriptor closed.\n");
    }
    if (leds > 0) {
        close(leds);
        printf("LED file descriptor closed.\n");
    }
    if (dot_mtx > 0) {
        close(dot_mtx);
        printf("Dot matrix file descriptor closed.\n");
    }
    if (clcds > 0) {
        close(clcds);
        printf("Character LCD file descriptor closed.\n");
    }
    if (fnds > 0) {
        close(fnds);
        printf("FND file descriptor closed.\n");
    }
}