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

// Target System
#define fnd "/dev/fnd"			// 7-Segment FND 
#define dot "/dev/dot"			// Dot Matrix
#define tact "/dev/tactsw"    	// Tact Switch
#define dip "/dev/dipsw"		// Dip Switch
#define clcd "/dev/clcd"		// Character LCD

int turn(int*);
void roll_dice(int*);
void set_dot(void);
void set_lcd(int, int);
void set_turn_score(int);
void calc_score(int*, int*);

// 파일 디스크립터
int tactsw = 0;
int dipsw = 0;
int dot_mtx = 0;
int clcds = 0;
int fnds = 0;

unsigned char dot_buffer[8] = {0x00,};
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
char clcd_map[27][17] = {
    "      Ones      ", // 0
    "      Twos      ", // 1
    "     Threes     ", // 2
    "      Fours     ", // 3
    "      Fives     ", // 4
    "      Sixes     ", // 5
    "   Full House   ", // 6
    " Four of a Kind ", // 7
    "Little Straight ", // 8
    "  Big Straight  ", // 9
    "     Choice     ", // 10
    "      Yacht     ", // 11
    "  P1 turn Roll  ", // 12
    "  P2 turn Roll  ", // 13
    "    P1 win!!    ", // 14
    "    P2 win!!    ", // 15
    "      draw      ", // 16
    "  pick or roll  ", // 17
    " P1 000  P2 000 ", // 18
    "     Waiting    ", // 19
    "    Roll Dice   ", // 20
    "Player Turn End ", // 21
    "                ", // 22
    "  Real Choose?  ", // 23
    "   0 0 0 0 0    ", // 24
    "Dip Switch Clear", // 25
    "      pick      ", // 26
    };

int main() {
    srand(time(NULL));

    //open 및 오류처리
    if((tactsw = open( tact, O_RDWR )) < 0){printf("tact open error\n"); exit(1);}
    if((dipsw = open( dip, O_RDWR )) < 0){printf("dip open error\n"); exit(1);}

    // 합산 스코어
    int score[2] = {0, 0};
    //사용 족보 바이너리로 바꾸어서 플래그로 사용(0~4095) 사용시 1
    int score_category[2] = {0, 0};
    int r; // round : 12라운드
    int t; // 0: p1턴/ 1 : p2턴

    for(r = 0; r < 12; r++) {
        for(t = 0; t < 2; t++) {
            // P0 turn Roll -> ok
            set_lcd(18, 12 + t);
            score[t] += turn(&score_category[t]);
            
            // clcd 점수 수정
            clcd_map[18][4 + t * 8] = (score[t] / 100) + '0';
            clcd_map[18][5 + t * 8] = (score[t] / 10 % 10) + '0';
            clcd_map[18][6 + t * 8] = (score[t] % 10) + '0';

            // 턴 종료 -> ok
            set_lcd(18, 21);
            printf("IS_FOR");
            sleep(4);
        }
    }

        //1 주사위 홀드 되는거, 주사위 3번 다굴리는거
        //2 tact 족보 보여주고 결정하기
        //3 dip 내리라고 경고
        //4 Used 보여주기
        //5 이김 

    close(tactsw);
    close(dipsw);

    //p1 win
    if(score[0] > score[1]) {
        set_lcd(22, 14);
    }
    //p2 win
    else if(score[1] > score[0]) {
        set_lcd(22, 15);
    }
    //draw
    else {
        set_lcd(22, 16);
    }
    return 0;
}

int turn(int* category) {
    int dice[5] = {0,};
    int score[12] = {0,};
    int roll_count = 0;
    unsigned char before_input = 13;
    unsigned char dip_input = 0;
    unsigned char tact_input = 0;

    while(1) {
        if(roll_count < 3){       
            usleep(10000);

            read(dipsw, &dip_input, sizeof(dip_input));
            if(dip_input & 128) {
                roll_dice(dice);
                set_dot();
                calc_score(score, dice);
                if(roll_count == 2) set_lcd(24, 26);
                else set_lcd(24, 17);
                printf("TACT_128.");
                roll_count++;
                continue;
            }
            else if(roll_count == 0 && dip_input & 63) {
                // 첫 roll에서 hold가 올라가있으면 경고
                set_lcd(22, 25);
                memset(dot_buffer, 0xFF, sizeof(dot_buffer));
                if((dot_mtx = open(dot, O_RDWR)) < 0) {printf("dot open error\n"); exit(1);}
                write(dot_mtx, dot_buffer, sizeof(dot_buffer));
                sleep(1);
                close(dot_mtx);
                memset(dot_buffer, 0x00, sizeof(dot_buffer));
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
                    set_lcd(23, tact_input - 1);
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
                    return score[tact_input-1];
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
    //4,6,8,10,12
    for(i = 0; i < 5; i++) {
        if(!(hold & (1 << i))){
            dice[i] = rand() % 6 + 1;
            clcd_map[24][3 + 2 * i] = dice[i] + '0';
        }
    }
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
    return;
}

void set_dot() {
    set_lcd(24, 19);
    printf("ROLL_DICE_METHOD");
    if((dot_mtx = open(dot, O_RDWR)) < 0 ) {printf("dot open error\n"); exit(1);}
    write(dot_mtx, dot_buffer, sizeof(dot_buffer));
    // dip
    sleep(4);
    close(dot_mtx);
    return;
}

void set_turn_score(int score) {
    //턴당 점수 50 이상 불가능 USEd에 사용
    unsigned char fnd_buffer[4];

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

    printf("[fnd] %d\n",score);

    if((fnds = open(fnd, O_RDWR)) < 0 ) {printf("fnd open error\n"); exit(1);}

    write(fnds, &fnd_buffer, sizeof(fnd_buffer));
    
    // tact
    sleep(2);

    close(fnds);
    return;
}

void set_lcd(int up, int down) {
    char buffer[33];  // 32글자를 위한 버퍼
    snprintf(buffer, 33, "%s%s", clcd_map[up], clcd_map[down]);

    printf("[CLCD] %s\n",buffer);

    if((clcds = open(clcd, O_RDWR)) < 0 ) {printf("clcd open error\n"); exit(1);}
    write(clcds, buffer, 32);
    close(clcds);

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