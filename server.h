#ifndef SERVER_H
#define SERVER_H


typedef struct keyboard{
	int unicode;
	int shift;
	int alt;
	int ctrl;
} keyboard_t;

int shift(char);

#endif
