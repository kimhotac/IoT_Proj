#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define fnd "/dev/fnd"

unsigned char fnd_data[4] = {
	0b11000001, // U
	0b10010010, // S
	0b10000110, // E
	0b10100001, // d;
};

int show() { 
	int fnd_d; 

 	if((fnd_d = open(fnd,O_RDWR)) < 0) { 
		perror("open"); 
		exit(1); 
	}
	
	write(fnd_d,fnd_data,sizeof(fnd_data));
	

	return 0;
}
int main(){
	show();
    usleep(100000); // 0.01초
}