
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


//#include "AEConfig.h"

#include "j2k_OutUI.h"

#import "j2k_Out_Controller.h"
#import "j2k_About_Controller.h"

#include "j2k_version.h"

// ==========
// Only building this on 64-bit (Cocoa) architectures
// ==========
#if __LP64__ || defined(COCOA_ON_32BIT)

bool
j2k_OutUI(
	j2k_OutUI_Data	*params,
	const char		*generic_profile,
	const char		*color_profile,
	bool			show_subsample,
	const void		*plugHndl,
	const void		*mwnd)
{
	bool result = false;

	NSApplicationLoad();
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSString *bundle_id = [NSString stringWithUTF8String:(const char *)plugHndl];

	Class ui_controller_class = [[NSBundle bundleWithIdentifier:bundle_id]
									classNamed:@"j2k_Out_Controller"];

	if(ui_controller_class)
	{
		j2k_Out_Controller *ui_controller = [[ui_controller_class alloc] init:params->method
												size:params->size
												quality:params->quality
												format:params->format
												customDepth:params->customDepth
												bitDepth:params->bitDepth
												reversible:params->reversible
												advanced:params->advanced
												ycc:params->ycc
												subsampling:params->sub
												order:params->order
												tileSize:params->tileSize
												iccProfile:params->icc_profile
												dciProfile:params->dci_profile
												dciDataRate:params->dci_data_rate
												dciPerFrame:params->dci_per_frame
												dciFrameRate:params->dci_frame_rate
												dciStereo:params->dci_stereo
												genericName:generic_profile
												profileName:color_profile
												show_subsample:show_subsample];
		if(ui_controller)
		{
			NSWindow *my_window = [ui_controller getWindow];
			
			if(my_window)
			{
				NSInteger modal_result;
				DialogResult dialog_result;
				
				// because we're running a modal on top of a modal, we have to do our own							
				// modal loop that we can exit without calling [NSApp endModal], which will also							
				// kill AE's modal dialog.
				NSModalSession modal_session = [NSApp beginModalSessionForWindow:my_window];
				
				do{
					modal_result = [NSApp runModalSession:modal_session];

					dialog_result = [ui_controller getResult];
				}
				while(dialog_result == DIALOG_RESULT_CONTINUE && modal_result == NSRunContinuesResponse);
				
				[NSApp endModalSession:modal_session];
				
				if(dialog_result == DIALOG_RESULT_OK || modal_result == NSRunStoppedResponse)
				{
					params->method = [ui_controller getMethod];
					params->size = [ui_controller getSize];
					params->quality = [ui_controller getQuality];
					params->format = [ui_controller getFormat];
					params->customDepth = [ui_controller getCustomDepth];
					params->bitDepth = [ui_controller getBitDepth];
					params->reversible = [ui_controller getReversible];
					params->advanced = [ui_controller getAdvanced];
					params->ycc = [ui_controller getYcc];
					params->sub = [ui_controller getSubsample];
					params->order = [ui_controller getOrder];
					params->tileSize = [ui_controller getTileSize];
					params->icc_profile = [ui_controller getProfile];
					params->dci_profile = [ui_controller getDCIProfile];
					params->dci_data_rate = [ui_controller getDCIDataRate];
					params->dci_per_frame = [ui_controller getDCIPerFrame];
					params->dci_frame_rate = [ui_controller getDCIFrameRate];
					params->dci_stereo = [ui_controller getDCIStereo];
				
					result = true;
				}
				
				[my_window close];
			}
			
			[ui_controller release];
		}
	}
	
	// don't release when created with stringWithUTF8String
	//[bundle_id release];
	
	if(pool)
		[pool release];

	return result;
}

void
j2k_About(
	const void		*plugHndl,
	const void		*mwnd)
{
	NSApplicationLoad();
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

	NSString *bundle_id = [NSString stringWithUTF8String:(const char *)plugHndl];

	Class about_controller_class = [[NSBundle bundleWithIdentifier:bundle_id]
									classNamed:@"j2k_About_Controller"];
	
	if(about_controller_class)
	{
		j2k_About_Controller *about_controller = [[about_controller_class alloc] init:"v" j2k_Version_String " - " j2k_Build_Date];
		
		if(about_controller)
		{
			NSWindow *the_window = [about_controller getWindow];
			
			if(the_window)
			{
				[NSApp runModalForWindow:the_window];
				
				[the_window close];
			}
			
			[about_controller release];
		}
	}

	if(pool)
		[pool release];
}

#endif // __LP64__
