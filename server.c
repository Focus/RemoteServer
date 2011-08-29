/*------------------------------------------------------------------------------
Copyright 2011 Bati Sengul

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
------------------------------------------------------------------------------*/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>



#define BUFFER_SIZE 256
#define MIN(x,y) x < y ? x : y


/********************************************************
Client send information as follows:
Relative mouse movement by (x,y):  x,y.
Toggle left mouse button: !!.
Character c: UnicodeInt.		e.g. 77.
	10 is Return
	-1987 is backspace
 *********************************************************/


static Display* d;
static int sock;
static int acc;

int shift(char c){

	if(isupper(c))
		return 1;
	switch(c){
	case '~':
	case '!':
	case '@':
	case '#':
	case '$':
	case '%':
	case '^':
	case '&':
	case '*':
	case '(':
	case ')':
	case '_':
	case '+':
	case '|':
	case '{':
	case '}':
	case ':':
	case '"':
	case '<':
	case '>':
	case '?':
		return 1;
	}
	return 0;

}

//Key senders

void sendKey(KeySym ks, int shift){
	if(shift)
		XTestFakeKeyEvent( d, XKeysymToKeycode(d, XK_Shift_L), True, CurrentTime );
	XTestFakeKeyEvent( d, XKeysymToKeycode(d,ks), True, CurrentTime );
	XTestFakeKeyEvent( d, XKeysymToKeycode(d,ks), False, CurrentTime );
	if(shift)
		XTestFakeKeyEvent( d, XKeysymToKeycode(d, XK_Shift_L), False, CurrentTime );
	XSync(d,0);
}

//Mouse stuff

void clickMouse(int lm_down){
	XTestFakeButtonEvent( d, 1, lm_down, CurrentTime );
	XSync(d,0);
}

void moveMouse(int dx, int dy){
	if(XTestFakeRelativeMotionEvent( d, dx, dy, CurrentTime ) == 0)
		perror("Unable to move");
	XSync(d,0);
}


// Main loop detecting the protocol
void input(char* buffer, int len){
	int dx,dy;
	int lm_down = 0;
	char *comma, *tok;
	tok = strtok(buffer,".");
	while( tok != NULL ){
		if(strchr(tok,',') != NULL){ //Either a ,. or 123,123.
			comma = strtok(tok, ",");
			if(comma == NULL) //User sent ,.
				sendKey(XStringToKeysym("comma"), 0);
			else{
				dx = atoi(comma);
				if( (comma = strtok(NULL,",")) != NULL){
					dy = atoi(comma);
					moveMouse(dx, dy);
				}
			}
		}
		else if(!strcmp(tok,"!!")){
			lm_down = (lm_down+1)% 2;
			clickMouse(lm_down);
		}
		else if(strchr(tok, '\n') == NULL && strchr(tok,'\r')== NULL){
			int uni = atoi(tok);
			if(uni == 10) //Enter key
				sendKey(XK_Return, 0);
			else if(uni == -1987) //Backspace
				sendKey(XK_BackSpace, 0);
			else if( uni > 31 && uni < 127){//Valid character ranges because otherwise my keyboard doesn't have them!
				char c = (char)uni;
				sendKey(c, shift(c));
			}
			else
				printf("Character no: %i looks dangerous, better skip it.\n",uni);

		}
		tok = strtok(NULL, ".");
	}
}

void clean(){
	close(sock);
	close(acc);
	XFree(d);
}

int main(int argc, char* argv[]){
	signal(SIGTERM, clean);
	d = XOpenDisplay(NULL);
	if( d == NULL ){
		perror("Unable to open display");
		return -1;
	}
	sock = socket(AF_INET, SOCK_STREAM, 0);
	int port = 1987;
	if(sock < 0){
		perror("Cannot open socket");
		return -1;
	}
	struct sockaddr_in servadd, cliadd;
	memset(&servadd, 0, sizeof(servadd));
	if(argc >= 2)
		port = atoi(argv[1]);
	printf("Using port number: %i\n",port);
	servadd.sin_family = AF_INET;
	servadd.sin_addr.s_addr = INADDR_ANY;
	servadd.sin_port = htons(port);
	if (bind(sock, (struct sockaddr*) &servadd, sizeof(servadd)) < 0){
		perror("Failed to bind");
		return -1;
	}

	signal(SIGTERM, clean);
	while(1){
		printf("Listening for connections..\n");
		listen(sock,5);
		socklen_t clisize = sizeof(cliadd);
		acc = accept(sock, (struct sockaddr*) &cliadd, &clisize);
		if(acc < 0){
			perror("Cannot accept connection");
			return -1;
		}
		printf("%s has connected.\n",inet_ntoa(cliadd.sin_addr));
		int n = 0;
		char buffer[BUFFER_SIZE];
		while( (n = read(acc, buffer, BUFFER_SIZE)) > 0){
			input(buffer, MIN(n-1, BUFFER_SIZE));
			bzero(buffer, BUFFER_SIZE);
		}
		printf("Bye to %s\n", inet_ntoa(cliadd.sin_addr));
	}
	clean();
	return 0;
}
