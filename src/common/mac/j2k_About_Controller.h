
// ---------------------------------------------------------------------
// 
// j2k - JPEG 2000 plug-ins for Adobe programs
// Copyright (c) 2002-2016 Brendan Bolles and Aaron Boxer
// 
// This file is part of j2k.
//
// j2k is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
// 
// ---------------------------------------------------------------------

#import <Cocoa/Cocoa.h>

@interface j2k_About_Controller : NSObject {
	IBOutlet NSWindow *theWindow;
	IBOutlet NSTextField *versionString;
}

- (id)init:(const char *)verion_string;

- (IBAction)clickedOK:(id)sender;

- (NSWindow *)getWindow;

@end
