//
//  j2k_About_Controller.m
//
//  Created by Brendan Bolles on 9/28/11.
//  Copyright 2011 fnord. All rights reserved.
//

#import "j2k_About_Controller.h"

@implementation j2k_About_Controller

- (id)init:(const char *)version_string {
	self = [super init];
	
	if(!([NSBundle loadNibNamed:@"About_Dialog" owner:self]))
		return nil;
	
	[theWindow center];

	[versionString setStringValue:[NSString stringWithUTF8String:version_string]];
	
	return self;
}

- (IBAction)clickedOK:(id)sender {
    [NSApp stopModal];
}

- (NSWindow *)getWindow {
	return theWindow;
}

@end
