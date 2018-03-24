#include <inttypes.h>
#include "fixed-point-numbers.h"


#define f 16384
/*
converts the integer/real numbers to fixed point numbers
*/
int
convert_to_fixed_point(int n){
	return n * f;
}
/*
converts the fixed point numbers to integer/real numbers
*/
int 
convert_FP_to_integer(int x){
	if(x>=0){
		return (x+f/2)/f;
	}else{
		return (x - f/2)/f;
	}
	
}
/*
add two fixed point numbers
*/
int 
add_FP_numbers(int x,int y){
	return x + y;
}
/*
subtract two fixed point numbers
*/
int 
sub_FP_numbers(int x,int y){
	return x - y;
}
/*
Adds a fixed point number and a integer
*/
int 
add_FP_and_int_numbers(int x,int n){
	return (x + n * f);
}

/*
Subtract a fixed point number and a integer
*/
int 
sub_FP_and_int_numbers(int x,int n){
	return (x - n * f);
}

/*
Multiply 2 fixed point number 
*/
int 
mul_FP_numbers(int x,int y){
	return (((int64_t)x) * y)/f;
}

/*
Multiply a fixed point number and a integer
*/
int 
mul_FP_and_int_numbers(int x,int n){
	return x * n;
}

/*
Divides 2 fixed point number 
*/
int 
div_FP_numbers(int x,int y){
	return ((int64_t)x) * f/y;
}

/*
Divides a fixed point number and a integer
*/

int 
div_FP_and_int_numbers(int x,int n){
	return x / n;
}





