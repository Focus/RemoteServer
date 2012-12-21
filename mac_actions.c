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

#include "mac_actions.h"
#include <Cocoa/Cocoa.h>

void sendKey(keyboard_t key){
	CGEventSourceRef eventSource = CGEventSourceCreate(kCGEventSourceStateHIDSystemState);
	CGEventRef keyEventDown = CGEventCreateKeyboardEvent(eventSource, 0, true);
	UniChar buffer = (char) key.unicode;
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

void windowListApp(const void *dict, void *names){
		NSDictionary *entry = (NSDictionary*)dict;
		//NSLog(@"%i", [(NSNumber*)[entry objectForKey:@"kCGWindowLayer"] intValue] );
		if( [(NSNumber*)[entry objectForKey:@"kCGWindowLayer"] intValue] < 1)
				return;
		NSMutableArray *list = (NSMutableArray*) names;
		//NSString *sname = [entry objectForKey:(id)kCGWindowOwnerName];
		[list addObject:entry];
		//NSLog(@"%@",sname);
}

void sendTo(char* title, keyboard_t key){
		if(title == NULL)
				return;
		NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
		NSString *search = [NSString stringWithCString:title encoding:NSUTF8StringEncoding];
		CFArrayRef windowList =  CGWindowListCopyWindowInfo(kCGWindowListExcludeDesktopElements, kCGNullWindowID);
		NSMutableArray *list = [NSMutableArray array];
    	CFArrayApplyFunction(windowList, CFRangeMake(0, CFArrayGetCount(windowList)), &windowListApp, list);
    	CFRelease(windowList);
		NSUInteger i = [list indexOfObjectPassingTest:
				^ (id obj, NSUInteger idx, BOOL *stop){
						NSDictionary *entry = (NSDictionary*) obj;
						NSString *windowName = [entry objectForKey:@"kCGWindowOwnerName"];
						if( [windowName rangeOfString:search options:NSCaseInsensitiveSearch].location != NSNotFound )
								return YES;
						else
								return NO;
				}];
		if(i == NSNotFound)
				return;
		NSString *winName = [(NSDictionary*)[list objectAtIndex:i] objectForKey:@"kCGWindowOwnerName"];
		NSString *command = [NSString stringWithFormat:@"%@%@%@%@%@%c%@",@"tell application \"",winName,@"\" to activate\n\
				tell application \"System Events\"\n\
				tell process \"",winName,@"\"\n\
				keystroke \"",(char)key.unicode,@"\" using command down\n\
				end tell\n\
				end tell"];
		NSAppleScript *script = [[NSAppleScript alloc] initWithSource:command];
		[script executeAndReturnError:nil];
		//NSLog(@"%@: \n%@\n\n",winName,command);
		[pool release];
}
