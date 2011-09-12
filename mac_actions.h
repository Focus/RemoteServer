#ifndef ACTIONS_H
#define ACTIONS_H

#include <ApplicationServices/ApplicationServices.h>
#include <AppKit/NSEvent.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Carbon/Carbon.h>

#include "server.h"

void sendKey(keyboard_t);
void clickMouse(int);
void moveMouse(int, int);
void sendTo(char*, keyboard_t);

#endif

