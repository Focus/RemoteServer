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

#include "linux_actions.h"
#include <stdio.h>

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

void sendKey(keyboard_t key){
		KeySym ks = (char) key.unicode;
		if(key.shift)
				XTestFakeKeyEvent( d, XKeysymToKeycode(d, XK_Shift_L), True, CurrentTime );
		XTestFakeKeyEvent( d, XKeysymToKeycode(d,ks), True, CurrentTime );
		XTestFakeKeyEvent( d, XKeysymToKeycode(d,ks), False, CurrentTime );
		if(key.shift)
				XTestFakeKeyEvent( d, XKeysymToKeycode(d, XK_Shift_L), False, CurrentTime );
		XSync(d,0);
}

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

		void sendTo(char* title, keyboard_t key){
				Window win;
				if( getWindow(title, &win, RootWindow(d, DefaultScreen(d))) == 0 ){
						printf("Window with title %s not found.\n", title);
						return;
				}
				Window back;
				int rev;
				XGetInputFocus(d, &back, &rev);
				XSetInputFocus(d, win, RevertToParent, CurrentTime);
				XSync(d, 0);
				sendKey(key);
				XSetInputFocus(d, back, rev, CurrentTime);
				XSync(d, 0);
		}