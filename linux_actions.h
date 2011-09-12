#ifndef ACTIONS_H
#define ACTIONS_H

#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>

#include "server.h"

Display* d;

void sendKey(keyboard_t);
void clickMouse(int);
void moveMouse(int, int);
void sendTo(char*, keyboard_t);

#endif

