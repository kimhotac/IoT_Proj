// Header File
#include <stdio.h>          	// 입출력 관련 
#include <stdlib.h>         	// 문자열 변환, 메모리 관련 
//#include <unistd.h>       	    // POSIX 운영체제 API에 대한 액세스 제공 
#include <fcntl.h>			    // 타겟시스템 입출력 장치 관련 
#include <sys/types.h>    	    // 시스템에서 사용하는 자료형 정보 
//#include <sys/ioctl.h>    	    // 하드웨어의 제어와 상태 정보 
#include <sys/stat.h>     	    // 파일의 상태에 대한 정보 
#include <string.h>       	    // 문자열 처리 
#include <time.h>         	    // 시간 관련 
#include <stdbool.h>

// Target System
#define fnd "/dev/fnd"			// 7-Segment FND : 7-Segment 4EA 
#define dot "/dev/dot"			// Dot Matrix : 8x8 LED Dot Matrix
#define tact "/dev/tactsw"    	// Tact Switch : Tact Switch 12EA
#define led "/dev/led"			// LED : Chip LED 8EA
#define dip "/dev/dipsw"		// Dip Switch : Switch DIP Switch 8EA
#define clcd "/dev/clcd"		// Character LCD : 2x16 Character LCD

int turn(int);
int roll_calc_score(int*);

// 전역 변수
// 입출력장치
int dipsw;
int tactsw;

int leds;
int dot_mtx;
int clcds;
int fnds;
char lcd_output[18][17] = {
    " P1 000  P2 000 ",
    "      Ones      ",
    "      Twos      ",
    "     Threes     ",
    "      Fours     ",
    "      Fives     ",
    "      Sixes     ",
    "    Full House  ",
    " Four of a Kind ",
    "Little Straight ",
    "  Big Straight  ",
    "     Choice     ",
    "      Yacht     ",
    "  P1 turn Roll  ",
    "  P2 turn Roll  ",
    "    P1 win!!    ",
    "    P2 win!!    ",
    "      draw      "
};
unsigned char dot_output[8] = {
    {0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000,
    0b00000000
    }
};
unsigned char fnd_output[14] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
    0b00111110, // U
    0b01101101, // S
    0b01111001, // E
    0b01011110  // d
};

// FND LED 관련 전역 변수 설정
unsigned char fnd_num[4] = {0,};


int main() {
    srand(time(NULL));

    // 합산 스코어
    int score[2] = {0,0};

    //사용 족보 바이너리로 바꾸어서 플래그로 사용(0~4095) 사용시 1
    int score_category[2] = {0,0};

    //12라운드
    for(int r=0;r<12;r++) {
        //0 : p1턴/ 1 : p2턴
        for(int t=0;t<2;t++) {
            set_lcd(lcd_output[0],lcd_output[13+t]);
            score[t] += turn(score_category[t]);
            //LCD_output[0] 갱신
            char temp_score = score[t];
            snprintf(temp_score, 4, "%03d", score[t]);
            memcpy(lcd_output[0] + 3 + t * 8, temp_score, 3); // 종결 문자는 복사하지 않음
        }
    }

    //p1 win
    if(score[0] > score[1]) {
        set_lcd(lcd_output[0], lcd_output[15]);
    }
    //p2 win
    else if(score[1] > score[0]) {
        set_lcd(lcd_output[0], lcd_output[16]);
    }
    //draw
    else {
        set_lcd(lcd_output[0],lcd_output[17]);
    }
    return 0;
}


int turn(int category) {
    int score[12] = {0};
    int roll_count = 0;
    int dip_input = 0;
    int tact_input = 0;
    int before_input = 0;

    dipsw = open(dip, O_RDONLY);
    tactsw = open(tact, O_RDONLY);

    while(1) {
        //입력 및 주사위 굴리기
        while(1) {
            usleep(10000); //0.01 초 쉬기
            read(tactsw, &tact_input, sizeof(tact_input));
            //tactsw 입력 없고 3번 이하로 굴렸을 때
            if(!tact_input && roll_count < 3) {
                read(dipsw, &dip_input, sizeof(dip_input));
                // 딥스위치 맨 오른쪽 올렸을 때
                if(dip_input & 1) {
                    roll_calc_score(score);
                    //여기에 주사위 돌아가는 애니메이션 dip 내려갈때 까지 표시
                    roll_count++;
                    tact_input = 1;
                    break;
                }
            }
            //아직 한번도 주사위 안굴렸으면 점수출력X
            else if(roll_count != 0) {
                break;
            }
        }
        //점수출력
        //LCD에 족보이름
        set_lcd(lcd_output[0],lcd_output[tact_input]);
        //fnd에 점수
        //category에 사용한 족보 넣기
        
        if(tact_input == before_input) {
            category |= 1 << (tact_input-1);
            return score;
        }

        if((1 << (tact_input - 1)) & category) {
            //사용한 족보는 다시 사용 못하도록
            printf("[7seg]used");
            set_fnd()
            continue;
        }
        else{
            printf("[7seg]%d",score);
            before_input = tact_input;
        }
    }
    close(dipsw);
    close(tactsw);
}

int roll_calc_score(int*score){
    int dice[5] = {0,0,0,0,0};
    int hold = get_dipsw();
    for(int i=0;i<5;i++){
        if(!(hold & (1<<(i+3)))){
            dice[i] = rand()%6+1;
        }
    }

    // 1부터 6까지의 주사위 눈의 빈도를 저장하기 위한 배열, 0번 인덱스는 사용하지 않음
    int counts[7] = {0}; 
    for(int i=1;i<7;i++){
        int cnt = 0;
        for(int j=0;j<5;j++){
            if(dice[j]==i){
                cnt++;
            }
            counts[dice[i]]++; // 주사위 빈도 계산
        }
        score[i]=cnt*i;//ones ~ sixs
    }

    // Full House: 3개가 같고 2개가 같을 때
    bool three_of_a_kind = false, pair = false;
    for (int i = 1; i <= 6; i++) {
        if (counts[i] == 3) three_of_a_kind = true;
        if (counts[i] == 2) pair = true;
    }
    if (three_of_a_kind && pair) {
        score[7] = 25; // Full House 점수는 25점으로 가정
    } else {
        score[7] = 0;
    }

    // Four of a Kind: 같은 눈 4개
    for (int i = 1; i <= 6; i++) {
        if (counts[i] >= 4) {
            score[8] = counts[i] * i; // Four of a Kind 점수는 해당 눈의 값 * 4
        }
    }

    // Little Straight: 1-2-3-4-5
    if (counts[1] == 1 && counts[2] == 1 && counts[3] == 1 && counts[4] == 1 && counts[5] == 1) {
        score[9] = 30; // Little Straight 점수는 30점
    } else {
        score[9] = 0;
    }

    // Big Straight: 2-3-4-5-6
    if (counts[2] == 1 && counts[3] == 1 && counts[4] == 1 && counts[5] == 1 && counts[6] == 1) {
        score[10] = 30; // Big Straight 점수는 30점
    } else {
        score[10] = 0;
    }

    // Choice: 모든 주사위의 합
    int sum = 0;
    for (int i = 1; i <= 6; i++) {
        sum += counts[i] * i;
    }
    score[11] = sum;

    // Yacht: 모든 주사위가 같을 때
    for (int i = 1; i <= 6; i++) {
        if (counts[i] == 5) {
            score[12] = 50; // Yacht 점수는 50점
        }
    }

    //dipsw 내려갈때 까지 주사위 굴리는 애니메이션
    int fake_dice[5] = {0,0,0,0,0};
    while(1){
        read(dipsw, &dip_input, sizeof(dip_input));
        for(int i=0;i<5;i++){
            if(!(hold & (1<<(i+3)))){
                dice[i] = rand()%6+1;
            }
        }
        // 딥스위치 맨 오른쪽 내렸을때
        if(!(dip_input & 1)) {
            break;
        }
        set_dot_dice(fake_dice);
        usleep(100000); //0.1 초 쉬기
    }

}

void set_lcd(int score_line, int game_line) {
    int clcds = open("/dev/clcd", O_RDWR);
    if (clcds < 0) {
        printf("lcd open error");
        exit(0);
    }

    char buffer[32];  // 32글자를 위한 버퍼
    snprintf(buffer, sizeof(buffer), "%s%s", lcd_output[score_line], lcd_output[game_line]);
    write(clcds, buffer, 32);  // 32글자 전송
    close(clcds);
}

void set_dot_dice(int* dice) {
    for(int i = 0;i<5;i++){
        //세로줄
        for(int j=2;j<8;j++){
            if(8-j<=dice[i]){
                dot_output[j] |= (128 >> i);
            }
            else{
                dot_output[j] &= ~(128 >> i);
    
            }
        }
    }
}

void set_fnd(int[4] fnd_num){
    //4글자 받아서 fnd에 출력
    if((fnds = open(fnd,O_RDWR)) <0){
        printf("fnd open error");
        return 0;
    }
    write(fnds, &fnd_num, sizeof(fnd_num));
    close(fnds);
}

void set_led(int ){
  unsigned char data;

  leds = open(led, O_RDWR);
  if (leds < 0) {
    printf("Can't open LED.\n");
    exit(0);
  }

  data = led_array[user_score];
  
  write(leds, &data, sizeof(unsigned char));

  close(leds);
}
