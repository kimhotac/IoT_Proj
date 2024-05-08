#include <stdio.h>
#include <stdlib.h>
#include<time.h>

// Target System
#define fnd "/dev/fnd"			// 7-Segment FND 
#define dot "/dev/dot"			// Dot Matrix
#define tact "/dev/tactsw"    		// Tact Switch
#define led "/dev/led"			// LED 
#define dip "/dev/dipsw"		// Dip Switch
#define clcd "/dev/clcd"		// Character LCD
// Dot Matrix: 8x8 LED Dot Matrix
// LED: Chip LED 8EA
// DIP: Switch DIP Switch 8EA
// Tact Switch: Tact Switch 12EA
// FND: 7-Segment 4EA 
// CLCD: 2x16 Character LCD

int dice_roll(int*, int);
int turn(int);
int num_point(int n,int* arr);

int main() {
    srand(time(NULL));

    //index 0은 p1, 1은 p2
    int point[2] = {0,0};

    //사용 족보 바이너리로 바꾸어서 플래그로 사용(0~4095)
    //사용 한것이 1 사용 안한것이 0
    int point_category[2] = {0,0};
    for(int t=0;t<24;t++){
        //총24턴 12라운드 %2 연산후 0이면 p1
        printf("[LCD]p%d turn\n",t%2+1);
        point[t%2] += turn(point_category[t%2]);
    }
    printf("p%d win!!",point[0]>point[1]?0:1);
    return 0;
}

int turn(int category){
    int dice[5] = {0,0,0,0,0};
    printf("[기본설명]Dip1~5:hold Dip 8:up주사위roll,down확정\ntact1~6숫자대로 족보\n");

    //바이너리로 바꾸어서 플래그로 사용(0~31)
    //홀드 하려면 0 돌리려면 1
    int hold_num = 31;
    char input = 'I';
    char before_input = 'B';
    int point = 0;
    int roll_count = 0;
    while(1){
        //편의상 tact 1~6으로 하고 dip asdfg 하고 roll은 r

        scanf(" %c", &input);
        if((roll_count==0)&&(input!='r')){
            //턴의 첫번째는 무조건 주사위 굴리기
            continue;
        }
        if((roll_count==3)&&(input=='r')){
            //주사위 3번 굴렸을 때 다시 굴리지 못하도록
            continue;
        }
        switch (input)
        {
            case 'r':
                printf("[Dot]");
                dice_roll(dice, hold_num);
                roll_count++;
                continue;
            case 'a':
                hold_num ^=1;
                continue;
            case 's':
                hold_num ^=2;
                continue;
            case 'd':
                hold_num ^=4;
                continue;
            case 'f':
                hold_num ^=8;
                continue;
            case 'g':
                hold_num ^=16;
                continue;
            case '1':
                point = num_point(1,dice);
                printf("[LCD]ones");
                category|=1;
                break;
            case '2':
                point = num_point(2,dice);
                printf("[LCD]twos");
                category|=2;
                break;
            case '3':
                point = num_point(3,dice);
                printf("[LCD]Threes");
                category|=4;
                break;
            case '4':
                point = num_point(4,dice);
                printf("[LCD]Fours");
                category|=8;
                break;
            case '5':
                point = num_point(5,dice);
                printf("[LCD]Fives");
                category|=16;
                break;
            case '6':
                point = num_point(6,dice);
                printf("[LCD]Sixes");
                category|=32;
                break;
            case '7':
                point = 0;
                printf("[LCD]Choice");
                category|=64;
                break;
            case '8':
                point = 0;
                printf("[LCD]Full House");
                category|=128;
                break;
            case '9':
                point = 0;
                printf("[LCD]Little Straight");
                category|=256;
                break;
            case '*':
                point = 0;
                printf("[LCD]Big Straight");
                category|=512;
                break;
            case '0':
                point = 0;
                printf("[LCD]Four of a Kind");
                category|=1024;
                break;
            case '#':
                point = 0;
                printf("[LCD]Yacht");
                category|=2048;
                break;
        }
        if(input == before_input){
            return point;
        }
        if((1 << (input - '1')) & category){
            //사용한 족보는 다시 사용 못하도록
            printf("[7seg]used");
            continue;
        }
        else{
            printf("[7seg]%d",point);
            before_input = input;
        }
        
    }
}

int dice_roll(int* dice, int hold){
    for(int i=0;i<5;i++){
        if(hold & (1 << (i))){
            dice[i] = rand()%6+1;
        }
        printf("%d ",dice[i]);
    }
    return 0;
}

int num_point(int n,int* arr){
    int cnt = 0;
    for(int i=0;i<5;i++){
        if(arr[i]==n){
            cnt++;
        }
    }
    return cnt*n;
}