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


#include "system.h"


#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "server.h"

#if USE_X11
#include "linux_actions.h"
#elif USE_MAC
#include "mac_actions.h"
#endif


#define BUFFER_SIZE 256
#ifndef MIN
#define MIN(x,y) x < y ? x : y
#endif

/********************************************************
  Client send information as follows:
  Relative mouse movement by (x,y):  x,y.
  Toggle left mouse button: !!.
  Character c: UnicodeInt.		e.g. 77.
  10 is Return
  -1987 is backspace
 *********************************************************/

#if USE_MAC
#define XK_Return '\n'
#define XK_BackSpace '\b'
#endif

static int sock;
static int acc;




/**
 * @brief Checks if the character needs the shift key
 * @param c Character to be checked
 * @return Returns 1 for shiftkey, 0 otherwise
 */
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

char* stristr(char* big, char* small){
		if(small == NULL)
				return big;
		if(big == NULL && strlen(big) < strlen(small))
				return 0;
		for(; *big; big++){
				if(toupper(*big) == toupper(*small)){
						char *b, *s;
						for(b = big, s = small; *b && *s; b++, s++){
								if(toupper(*b) != toupper(*s))
										break;
						}
						if(!*s)
								return big;
				}
		}
		return 0;
}

int setKey(keyboard_t* key, char* text){
		if(shift((char)key->unicode))
				key->shift = 1;
		else
				key->shift = 0;
		key->ctrl = 0;
		key->alt = 0;
		if( strchr(text,'^') != NULL ){
				for(; *text != '^'; text++){
						if(*text == 'C')
								key->ctrl = 1;
						else if(*text == 'A')
								key->alt = 1;
				}
				text++;
		}
		key->unicode = atoi(text);
		if(key->unicode == 10) //Enter key
				return 1;
		else if(key->unicode == 8) //Backspace
				return 1;
		//else if( key->unicode > 31 && key->unicode < 127)//Valid character ranges because otherwise my keyboard doesn't have them!
				return 1;
		//return 0;
}

// Main loop detecting the protocol
void parser(char* buffer, int len){
		int dx;
		int lm_down = 0;
		char *comma, *tok;
		tok = strtok(buffer,".");
		while( tok != NULL ){
				if(strchr(tok,',') != NULL){
						comma = strtok(tok, ",");
						if(comma != NULL){
								dx = atoi(comma);
								if( (comma = strtok(NULL,",")) != NULL)
										moveMouse(dx,atoi(comma));
						}
				}
				else if(!strcmp(tok,"!!")){
						lm_down = (lm_down+1)% 2;
						clickMouse(lm_down);
				}
				else if(strchr(tok, '>') != NULL){
						keyboard_t key;
						comma = strtok(tok, ">");
						if(setKey(&key, comma)){
								comma = strtok(NULL, ">");
								char* title = comma;
								sendTo(title, key);
						}
						else
								printf("Character no: %i looks dangerous, better skip it.\n",key.unicode);

				}
				else if(strchr(tok, '\n') == NULL && strchr(tok,'\r')== NULL){
						keyboard_t key;
						if(setKey(&key, tok))
								sendKey(key);
						else
								printf("Character no: %i looks dangerous, better skip it.\n",key.unicode);

				}
				tok = strtok(NULL, ".");
		}
}

void clean(){
		close(sock);
		close(acc);
#if USE_X11
		XFree(d);
#endif
}

int main(int argc, char* argv[]){
		signal(SIGTERM, clean);
#if USE_X11
		d = XOpenDisplay(NULL);
		if( d == NULL ){
				perror("Unable to open display");
				return -1;
		}
#endif
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
						parser(buffer, MIN(n-1, BUFFER_SIZE));
						bzero(buffer, BUFFER_SIZE);
				}
				printf("Bye to %s\n", inet_ntoa(cliadd.sin_addr));
		}
		clean();
		return 0;
}
