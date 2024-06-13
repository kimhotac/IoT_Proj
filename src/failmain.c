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
char clcd_top[17] = { " P1 000  P2 000 " };
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

    //open 및 오류처리
    if((tactsw = open( tact, O_RDWR )) < 0){printf("tact open error\n"); exit(1);}
    if((dipsw = open( dip, O_RDWR )) < 0){printf("dip open error\n"); exit(1);}

    pthread_t dot_thread;
    pthread_create(&dot_thread, NULL, dot_fnd_thread, NULL);

    // 합산 스코어
    int score[2] = {0, 0};
    //사용 족보 바이너리로 바꾸어서 플래그로 사용(0~4095) 사용시 1
    int score_category[2] = {0, 0};
    int r; // round : 12라운드
    int t; // 0: p1턴/ 1 : p2턴

    for(r = 0; r < 12; r++) {
        for(t = 0; t < 2; t++) {
            memset(dot_buffer, 0x00, sizeof(dot_buffer)); // 도트 버퍼 초기화
            memset(fnd_buffer, 0xff, sizeof(fnd_buffer)); // fnd 버퍼 초기화

            // P0 turn Roll
            set_lcd_bot(12 + t);
            score[t] += turn(&score_category[t]);
            
            // clcd 윗줄 수정
            clcd_top[4 + t * 8] = (score[t] / 100) + '0';
            clcd_top[5 + t * 8] = (score[t] / 10 % 10) + '0';
            clcd_top[6 + t * 8] = (score[t] % 10) + '0';
        }
    }

    //p1 win
    if(score[0] > score[1]) {
        set_lcd_bot(14);
    }
    //p2 win
    else if(score[1] > score[0]) {
        set_lcd_bot(15);
    }
    //draw
    else {
        set_lcd_bot(16);
    }

    // 스레드 종료
    pthread_cancel(dot_thread);
    pthread_join(dot_thread, NULL);
    // 뮤텍스 제거
    pthread_mutex_destroy(&dot_mutex);
    pthread_mutex_destroy(&fnd_mutex);
    
    close(tactsw);
    close(dipsw);
    return 0;
}

void* dot_fnd_thread(void* arg) {
    while (1) {
        dot_mtx = open(dot, O_RDWR);
        pthread_mutex_lock(&dot_mutex);
        write(dot_mtx, dot_buffer, sizeof(dot_buffer));
        pthread_mutex_unlock(&dot_mutex);
        usleep(300000); // 0.3 초
        close(dot_mtx);

        fnds = open(fnd, O_RDWR);
        pthread_mutex_lock(&fnd_mutex);
        write(fnds, &fnd_buffer, sizeof(fnd_buffer));
        pthread_mutex_unlock(&fnd_mutex);
        usleep(300000); // 0.3 초
        close(fnds); 
    }
    return NULL;
}

int turn(int* category) {

    int score[12] = {0,};
    int roll_count = 0;
    unsigned char dip_input = 0;
    unsigned char tact_input = 0;
    unsigned char before_input = -1;
    int dice[5] = {0,};

    while(1) {
        if(roll_count < 3){
            usleep(10000);
            read(dipsw, &dip_input, sizeof(dip_input));
            if(dip_input & 128) {
                roll_dice(dice);
                calc_score(score, dice);
                set_lcd_bot(17);
                roll_count++;
                dip_input = 0;
                continue;
            }
        }
        
        // 주사위 안굴렸으면 dip만 읽어야함
        if(roll_count == 0) continue;
        usleep(10000);
        
        read(tactsw, &tact_input, sizeof(tact_input));
        // tact 누름
        while(0 < tact_input && tact_input < 13){
            unsigned char temp = 0;
            usleep(1000); // 0.001 초 쉬기
            read(tactsw, &temp, sizeof(temp));
            // tact 뗌
            if(!temp){
                //tact 눌렀을 때 이벤트 처리
                // 점수 출력 부분
                if(tact_input != before_input) {
                    set_lcd_bot(tact_input-1);
                    printf("[dot] %d %d %d %d %d\n",dice[0],dice[1],dice[2],dice[3],dice[4]);
                    
                    if((1 << (tact_input - 1)) & *category) {
                        set_turn_score(100);
                    }
                    else {
                        set_turn_score(score[tact_input-1]);
                        before_input = tact_input;
                    }
                }
                // 점수 확정 부분
                else {
                    //TODO category에 사용한 족보 넣기
                    *category |= 1 << (tact_input - 1);
                    return score[tact_input];
                }
                break;
            }
        }
    }
}

void roll_dice(int* dice) {
    unsigned char hold;
    int i, j;

    while(1) {
        usleep(100000); // 0.1 초 쉬기
        read(dipsw, &hold, sizeof(hold));
        // 딥스위치 맨 오른쪽 내렸을때
        if(!(hold & 128)) {
            break;
        }
    }
    
    for(i = 0; i < 5; i++) {
        if(!(hold & (1 << i))){
            dice[i] = rand() % 6 + 1;
        }
    }
    pthread_mutex_lock(&dot_mutex);
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
    pthread_mutex_unlock(&dot_mutex);
}

// void set_roll_cnt(char roll_cnt) {
//     dot_buffer[0] &= 0x00;
//     switch (roll_cnt) {
//         case 3:
//             dot_buffer[0] |= 0x07; // 0000 0111
//             break;
//         case 2:
//             dot_buffer[0] |= 0x03; // 0000 0011
//             break;
//         case 1:
//             dot_buffer[0] |= 0x01; // 0000 0001
//             break;
//     }

//     write(dot_mtx,&dot_buffer, sizeof(dot_buffer));
//     return;
// }

void set_lcd_bot(int line) {
    char buffer[32];  // 32글자를 위한 버퍼
    snprintf(buffer, 32, "%s%s", clcd_top, clcd_bot[line]);

    printf("[CLCD] %s\n",buffer);

    if((clcds = open(clcd, O_RDWR)) < 0 ) {printf("clcd open error\n"); exit(1);}
    write(clcds, buffer, 32);
    close(clcds);

    return;
}

void set_turn_score(int score) {
    //턴당 점수 50 이상 불가능 USEd에 사용
    pthread_mutex_lock(&fnd_mutex);
    if(score>50) {
        fnd_buffer[0] = fnd_map[10];
        fnd_buffer[1] = fnd_map[11];
        fnd_buffer[2] = fnd_map[12];
        fnd_buffer[3] = fnd_map[13];
    }
    else {
        int tens = (score / 10) % 10;
        int ones = score % 10;
        fnd_buffer[0] = fnd_map[14];
        fnd_buffer[1] = fnd_map[tens];
        fnd_buffer[2] = fnd_map[ones];
        fnd_buffer[3] = fnd_map[14];
    }
    pthread_mutex_unlock(&fnd_mutex);

    printf("[fnd] %d\n",score);
    return;
}

void calc_score(int* score, int* dice) {
    int counts[7] = {0};
    bool isLittleStraight = true, isBigStraight = true;
    int i;

    // 주사위 눈 빈도 계산
    for (i = 0; i < 5; i++) {
        counts[dice[i]]++;
    }

    // 점수 계산 로직
    for (i = 1; i <= 6; i++) {
        score[i-1] = counts[i] * i;  // "Ones" to "Sixes"
    }

    // Full House, Four of a Kind, Yacht
    bool three = false, pair = false;
    int four = 0;
    for (i = 1; i <= 6; i++) {
        if (counts[i] == 2) pair = true;
        if (counts[i] == 3) three = true;
        if (counts[i] >= 4) four = counts[i] * i;
        if (counts[i] == 5) score[11] = 50; // Yacht
        if (i < 6) isLittleStraight &= counts[i] == 1;
        if (i > 1) isBigStraight &= counts[i] == 1;
    }
    score[6] = (three && pair) ? 25 : 0; // Full House
    score[7] = four; // Four of a Kind
    score[8] = isLittleStraight ? 30 : 0; // Little Straight
    score[9] = isBigStraight ? 30 : 0; // Big Straight

    // Choice
    int total = 0;
    for (i = 1; i <= 6; i++) {
        total += counts[i] * i;
    }
    score[10] = total; // Choice
    
    return;
}