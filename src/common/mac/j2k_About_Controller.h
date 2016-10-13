//
//  j2k_About_Controller.h
//
//  Created by Brendan Bolles on 9/28/11.
//  Copyright 2011 fnord. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface j2k_About_Controller : NSObject {
	IBOutlet NSWindow *theWindow;
	IBOutlet NSTextField *versionString;
}

- (id)init:(const char *)verion_string;

- (IBAction)clickedOK:(id)sender;

- (NSWindow *)getWindow;

@end
