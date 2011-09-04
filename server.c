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
#define USE_X11 1
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

#if USE_X11
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#elif USE_MAC
#include <ApplicationServices/ApplicationServices.h>
#include <AppKit/NSEvent.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>
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

#if USE_X11
static Display* d;
#else
#define XK_Return '\n'
#define XK_BackSpace '\b'
#endif

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

//Key senders
#if USE_X11
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


int getWindow(char* name, Window* ret, Window win){
	int k, i, j;
	Window temp, *children;
	int nchildren;
	if(!XQueryTree(d, win, &temp, &temp, &children, &nchildren)){
		perror("XQeuryTree failed");
	}
	for(i = 0; i < nchildren; i++){
		XTextProperty tp;
		XGetWMName(d, children[i], &tp);
		char** list;
		int nlist;
		XWindowAttributes att;
		XGetWindowAttributes(d, children[i], &att);
		//if(att.map_state == IsViewable){
		Xutf8TextPropertyToTextList(d, &tp, &list, &nlist);
		for(j = 0; j < nlist; j++){
			if(stristr(list[j], name)!=NULL && att.map_state != 0){
				XFreeStringList(list);
				XFree(tp.value);
				*ret = children[i];
				XFree(children);
				return 1;
			}
		}
		XFreeStringList(list);
		XFree(tp.value);
		if(getWindow(name, ret, children[i])){
			XFree(children);
			return 1;
		}
	}
	return 0;
}
//TODO: This doesn't actually send anything :(
void sendTo(char* title, char input){
	Window win;
	if( getWindow(title, &win, RootWindow(d, DefaultScreen(d))) == 0 ){
		printf("Window with title %s not found.\n", title);
		return;
	}

	XWindowAttributes wa;
	if(XGetWindowAttributes(d, win, &wa) == BadWindow){
		perror("Unable to get attributes");
		return;
	}
	XKeyEvent event;
	event.display = d;
	event.keycode = XKeysymToKeycode(d, input);
	event.root = wa.root;
	event.window = win;
	event.x = 1;
	event.y = 1;
	event.x_root = 1;
	event.y_root = 1;
	event.type = KeyPress;
	event.time = CurrentTime;
	event.same_screen = True;
	event.subwindow = None;
	printf("Sending '%c' to %s\n", input, title);
	XSetInputFocus(d, event.window, RevertToParent, CurrentTime);
	XSendEvent(event.display, event.window, True, KeyPressMask, (XEvent *)&event);

	event.type = KeyRelease;

	XSendEvent(event.display, event.window, True, KeyReleaseMask, (XEvent *)&event);

}

#elif USE_MAC
void sendKey(char c, int shift){
	CGEventSourceRef eventSource = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
	CGEventRef keyEventDown = CGEventCreateKeyboardEvent(eventSource, 0, true);
	UniChar buffer = c;
	keyEventDown = CGEventCreateKeyboardEvent(eventSource, 1, true);
	CGEventKeyboardSetUnicodeString(keyEventDown, 1, &buffer);
	CGEventPost(kCGHIDEventTap, keyEventDown);
	CFRelease(keyEventDown);
	CFRelease(eventSource);
}

void clickMouse(int lm_down){
	CGEventRef event = CGEventCreate(NULL);
	NSPoint point = CGEventGetLocation(event);
	CFRelease(event);
	CGEventRef click;
	if(lm_down)
		click = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseDown, point, kCGMouseButtonLeft);
	else
		click = CGEventCreateMouseEvent(NULL, kCGEventLeftMouseUp, point, kCGMouseButtonLeft);
	CGEventPost(kCGHIDEventTap, click);
	CFRelease(click);
}

void moveMouse(int dx, int dy){
	CGEventRef event = CGEventCreate(NULL);
	NSPoint point = CGEventGetLocation(event);
	CFRelease(event);
	point.x += (CGFloat) dx;
	point.y += (CGFloat) dy;
	CGEventRef move = CGEventCreateMouseEvent(NULL, kCGEventMouseMoved, point, kCGMouseButtonLeft);
	CGEventPost(kCGHIDEventTap, move);
	CFRelease(move);
}
#endif

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
		else if(strchr(tok, '>') != 0){
					comma = strtok(tok, ">");
					char input = *comma;
					comma = strtok(NULL, ">");
					char* title = comma;
					sendTo(title, input);
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
