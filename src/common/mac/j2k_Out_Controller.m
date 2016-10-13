#import "j2k_Out_Controller.h"

@implementation j2k_Out_Controller
- (id)init:(DialogMethod)method
	size:(long)the_size
	quality:(int)the_quality
	format:(DialogFormat)the_format
	customDepth:(BOOL)custom_depth
	bitDepth:(int)bit_depth
	reversible:(BOOL)use_reversible
	advanced:(BOOL)use_advanced
	ycc:(BOOL)use_ycc
	subsampling:(DialogSubsample)sub
	order:(DialogOrder)the_order
	tileSize:(int)tile_size
	iccProfile:(DialogProfile)icc_profile
	dciProfile:(DialogDCIProfile)dci_profile
	dciDataRate:(int)dci_data_rate
	dciPerFrame:(DialogDCIPerFrame)dci_per_frame
	dciFrameRate:(int)dci_frame_rate
	dciStereo:(BOOL)dci_stereo
	genericName:(const char *)generic_name
	profileName:(const char *)prof_name
	show_subsample:(BOOL)show_sub
{
	self = [super init];
	
	if(!([NSBundle loadNibNamed:@"Out_Dialog" owner:self]))
		return nil;
	
	[theWindow center];
	
	theResult = DIALOG_RESULT_CONTINUE;
	
	[self setMethod:method];
	[fileSizeField setIntValue:the_size];
	[qualitySlider setIntValue:the_quality];
	[qualityField setIntValue:the_quality];
	[advancedCheck setState:(use_advanced ? NSOnState : NSOffState)];
	[self setFormat:the_format];
	[bitDepthCheck setState:(custom_depth ? NSOnState : NSOffState)];
	[bitDepthField setIntValue:bit_depth];
	[bitDepthStepper setIntValue:bit_depth];
	[floatCheck setState:((!use_reversible) ? NSOnState : NSOffState)];
	[yccCheck setState:(use_ycc ? NSOnState : NSOffState)];
	[self setSubsample:sub];
	[self setOrder:the_order];
	[self setTileSize:tile_size];
	[self setProfile:icc_profile];
	[self setDCIProfile:dci_profile];
	[self setDCIPerFrame:dci_per_frame];
	[self setDCIFrameRate:dci_frame_rate];
	[dciStereoCheck setState:(dci_stereo ? NSOnState : NSOffState)];
	
	if(dci_per_frame == DIALOG_DCI_PER_FRAME)
	{
		[self setDCIRate:dci_data_rate];
	}
	else
	{
		[self setDCIRate:(dci_data_rate * 8 / 1024)];
	}


	if(generic_name)
	{
		[[profileMenu itemAtIndex:0] setTitle:[NSString stringWithUTF8String:generic_name]];
	}
	
	[profileMenu setAutoenablesItems:FALSE];
	
	if(prof_name)
	{
		[[profileMenu itemAtIndex:1] setTitle:[NSString stringWithUTF8String:prof_name]];
		[[profileMenu itemAtIndex:1] setEnabled:TRUE];
	}
	else
	{
		[profileMenu selectItemAtIndex:0];
	}
	
	if(show_sub)
	{
		[dciProfileLabel setHidden:TRUE];
		[dciProfileMenu setHidden:TRUE];
		[digitalCinemaRadio setEnabled:FALSE];
	}
	else
	{
		[subsampleLabel setHidden:TRUE];
		[subsampleMenu setHidden:TRUE];
	}


	
	[self trackAdvanced:nil];
	[self trackFloat:nil];
	
	return self;
}

- (IBAction)clickedCancel:(id)sender {
    theResult = DIALOG_RESULT_CANCEL;
}

- (IBAction)clickedOK:(id)sender {
    theResult = DIALOG_RESULT_OK;
}

- (IBAction)trackAdvanced:(id)sender {
    BOOL changed = FALSE;
	BOOL advanced = ([advancedCheck state] == NSOnState);
	
	NSRect window_frame = [theWindow frame];
	NSRect ok_frame = [okButton frame];
	NSRect cancel_frame = [cancelButton frame];
	
	if(advanced)
	{
		if(window_frame.size.width == 300)
		{
			window_frame.size.width = 600;
			
			ok_frame.origin.x += 300;
			cancel_frame.origin.x += 300;
			
			changed = TRUE;
		}
	}
	else
	{
		if(window_frame.size.width == 600)
		{
			window_frame.size.width = 300;
			
			ok_frame.origin.x -= 300;
			cancel_frame.origin.x -= 300;
			
			changed = TRUE;
		}
	}
	
	if(changed)
	{
		[okButton setFrame:ok_frame];
		[cancelButton setFrame:cancel_frame];
		[theWindow setFrame:window_frame display:TRUE animate:FALSE];
		[self trackFloat:sender];
	}
}

- (IBAction)trackCustomBitDepth:(id)sender {
    
}

- (IBAction)trackFloat:(id)sender {
    BOOL float_encoding = ([floatCheck state] == NSOnState);
	BOOL advanced = ([advancedCheck state] == NSOnState);
	
	if(advanced && float_encoding)
	{
		[losslessRadio setTitle:@"Maximum"];
	}
	else
	{
		[losslessRadio setTitle:@"Lossless"];
	}
}

- (IBAction)trackMethod:(id)sender {
    // manage the other radio buttons
	if(sender != losslessRadio)
		[losslessRadio setState:NSOffState];

	if(sender != digitalCinemaRadio)
		[digitalCinemaRadio setState:NSOffState];

	if(sender != fileSizeRadio)
	{
		[fileSizeRadio setState:NSOffState];
		[fileSizeField setEnabled:FALSE];
	}
	else
	{
		[fileSizeField setEnabled:TRUE];
	}

	if(sender != qualityRadio)
	{
		[qualityRadio setState:NSOffState];
		[qualityField setEnabled:FALSE];
		[qualitySlider setEnabled:FALSE];
	}
	else
	{
		[qualityField setEnabled:TRUE];
		[qualitySlider setEnabled:TRUE];
	}
	
			
	BOOL enable_controls = ([digitalCinemaRadio state] == NSOffState);
	NSColor *label_color = (enable_controls ? [NSColor textColor] : [NSColor disabledControlTextColor]);
	NSColor *not_label_color = (!enable_controls ? [NSColor textColor] : [NSColor disabledControlTextColor]);
	
	[bitDepthCheck setEnabled:enable_controls];
	[bitDepthField setEnabled:enable_controls];
	[bitDepthStepper setEnabled:enable_controls];
	[yccCheck setEnabled:enable_controls];
	[floatCheck setEnabled:enable_controls];
	[formatMenu setEnabled:enable_controls];
	[orderMenu setHidden:!enable_controls];
	[orderLabel setHidden:!enable_controls];
	[profileMenu setHidden:!enable_controls];
	[profileLabel setHidden:!enable_controls];
	[tileSizeMenu setHidden:!enable_controls];
	[tileSizeLabel setHidden:!enable_controls];
	[dciProfileMenu setEnabled:!enable_controls];
	[dciRateSlider setHidden:enable_controls];
	[dciRateField setHidden:enable_controls];
	[dciRateLabel setHidden:enable_controls];
	[dciPerFrameMenu setHidden:enable_controls];
	[dciFPSmenu setHidden:enable_controls];
	[dciFPSlabel setHidden:enable_controls];
	[dciStereoCheck setHidden:enable_controls];

	[formatLabel setTextColor:label_color];
	[dciProfileLabel setTextColor:not_label_color];
	
	//[label_color release];
	//[not_label_color release];
}

#define DCI_MAX		(1302083 / 1024)

- (IBAction)trackPerFrame:(id)sender {
	int minVal, maxVal;
	BOOL enable_fps;
	
	int old_rate = [dciRateField intValue];
	int new_rate;
	
	int stereo_mult = ([dciStereoCheck state] == NSOnState ? 2 : 1);
		
	if([[dciPerFrameMenu selectedItem] tag] == DIALOG_DCI_PER_FRAME)
	{
		maxVal = DCI_MAX;
		minVal = DCI_MAX / 100;
		
		enable_fps = FALSE;
		
		new_rate = ((double)(old_rate * 1024 / 8) / (double)([self getDCIFrameRate] * stereo_mult)) + 0.5;
	}
	else
	{
		maxVal = DCI_MAX * 24 * 8 / 1024;
		minVal = DCI_MAX * 24 * 8 / (100 * 1024);
		
		enable_fps = TRUE;
		
		new_rate = ((double)old_rate * 8.0 / 1024.0) * (double)([self getDCIFrameRate] * stereo_mult) + 0.5;
	}

	[dciRateSlider setMinValue:minVal];
	[dciRateSlider setMaxValue:maxVal];
	
	[dciFPSmenu setEnabled:enable_fps];
	[dciStereoCheck setEnabled:enable_fps];
	
	[self setDCIRate:new_rate];
	
	
	NSColor *label_color = (enable_fps ? [NSColor textColor] : [NSColor disabledControlTextColor]);
	
	[dciFPSlabel setTextColor:label_color];
}

- (NSWindow *)getWindow {
	return theWindow;
}

- (DialogResult)getResult {
	return theResult;
}

- (void)setMethod:(DialogMethod)method {
	NSButton *radio = NULL;

	switch(method)
	{
		case DIALOG_METHOD_SIZE:		radio = fileSizeRadio;			break;
		case DIALOG_METHOD_QUALITY:		radio = qualityRadio;			break;
		case DIALOG_METHOD_CINEMA:		radio = digitalCinemaRadio;	break;
		
		case DIALOG_METHOD_LOSSLESS:
		default:
			radio = losslessRadio;	break;
	}
	
	[radio setState:NSOnState];
	[self trackMethod:radio];
}

- (void)setFormat:(DialogFormat)the_format {
	[formatMenu selectItem:[formatMenu itemAtIndex:(the_format - 1)]];
}

- (void)setSubsample:(DialogSubsample)sub {
	NSInteger index =	(sub == DIALOG_SUBSAMPLE_NONE) ? 8 :
						(sub == DIALOG_SUBSAMPLE_422) ? 0 :
						(sub == DIALOG_SUBSAMPLE_411) ? 1 :
						(sub == DIALOG_SUBSAMPLE_420) ? 2 :
						(sub == DIALOG_SUBSAMPLE_311) ? 3 :
						(sub == DIALOG_SUBSAMPLE_2x2) ? 5 :
						(sub == DIALOG_SUBSAMPLE_3x3) ? 6 :
						(sub == DIALOG_SUBSAMPLE_4x4) ? 7 :
						8;
	
	[subsampleMenu selectItemAtIndex:index];
}

- (void)setOrder:(DialogOrder)the_order {
	[orderMenu selectItem:[orderMenu itemAtIndex:the_order]];
}

- (void)setTileSize:(int)tile_size {
	int menu_index =	(tile_size >= 2048) ? 0 :
						(tile_size >= 1024) ? 1 :
						(tile_size >= 512)  ? 2 :
						(tile_size >= 256)  ? 3 :
						(tile_size >= 128)  ? 4 :
						(tile_size >= 64)   ? 5 :
						6;

	[tileSizeMenu selectItem:[tileSizeMenu itemAtIndex:menu_index]];
}

- (void)setProfile:(DialogProfile)icc_profile {
	int menu_index = (icc_profile == DIALOG_PROFILE_ICC ? 1 : 0);
	
	[profileMenu selectItem:[profileMenu itemAtIndex:menu_index]];
}

- (void)setDCIProfile:(DialogDCIProfile)dci_profile {
	[dciProfileMenu selectItem:[dciProfileMenu itemAtIndex:dci_profile]];
}

- (void)setDCIRate:(int)dci_data_rate {
	[dciRateField setIntValue:dci_data_rate];
	[dciRateSlider setIntValue:dci_data_rate];
}

- (void)setDCIPerFrame:(DialogDCIPerFrame)dci_per_frame {
	[dciPerFrameMenu selectItemAtIndex:(dci_per_frame == DIALOG_DCI_PER_FRAME ? 0 : 1)];
	
	[self trackPerFrame:nil];
}

- (void)setDCIFrameRate:(int)dci_frame_rate {
	int menu_index =	(dci_frame_rate <= 24) ? 0 :
						(dci_frame_rate <= 25) ? 1 :
						(dci_frame_rate <= 30) ? 2 :
						(dci_frame_rate <= 48) ? 3 :
						(dci_frame_rate <= 50) ? 4 :
						(dci_frame_rate <= 60) ? 5 :
						0;
						
	[dciFPSmenu selectItemAtIndex:menu_index];
}

- (DialogMethod)getMethod {
	if([fileSizeRadio state] == NSOnState)
		return DIALOG_METHOD_SIZE;
	else if([qualityRadio state] == NSOnState)
		return DIALOG_METHOD_QUALITY;
	else if([digitalCinemaRadio state] == NSOnState)
		return DIALOG_METHOD_CINEMA;
	else
		return DIALOG_METHOD_LOSSLESS;
}

- (long)getSize {
	return [fileSizeField intValue];
}

- (int)getQuality {
	return [qualityField intValue];
}

- (DialogFormat)getFormat {
	return ([formatMenu indexOfSelectedItem] + 1);
}

- (BOOL)getCustomDepth {
	return ([bitDepthCheck state] == NSOnState);
}

- (int)getBitDepth {
	return [bitDepthField intValue];
}

- (BOOL)getReversible {
	return ([floatCheck state] == NSOffState);
}

- (BOOL)getAdvanced {
	return ([advancedCheck state] == NSOnState);
}

- (BOOL)getYcc {
	return ([yccCheck state] == NSOnState);
}

- (DialogSubsample)getSubsample {
	NSInteger index = [subsampleMenu indexOfSelectedItem];
	
	return	(index == 0) ? DIALOG_SUBSAMPLE_422 :
			(index == 1) ? DIALOG_SUBSAMPLE_411 :
			(index == 2) ? DIALOG_SUBSAMPLE_420 :
			(index == 3) ? DIALOG_SUBSAMPLE_311 :
			(index == 5) ? DIALOG_SUBSAMPLE_2x2 :
			(index == 6) ? DIALOG_SUBSAMPLE_3x3 :
			(index == 7) ? DIALOG_SUBSAMPLE_4x4 :
			(index == 8) ? DIALOG_SUBSAMPLE_NONE :
			DIALOG_SUBSAMPLE_NONE;
}

- (DialogOrder)getOrder {
	return [orderMenu indexOfSelectedItem];
}

- (int)getTileSize {
	int menu_index = [tileSizeMenu indexOfSelectedItem];
	
	switch(menu_index)
	{
		case 0: return 2048;
		case 1: return 1024;
		case 2: return 512;
		case 3: return 256;
		case 4: return 128;
		case 5: return 64;
		case 6:
		default: return 0;
	}
}

- (DialogProfile)getProfile {
	if([profileMenu indexOfSelectedItem] >= 1)
		return DIALOG_PROFILE_ICC;
	else
		return DIALOG_PROFILE_GENERIC;
}

- (DialogDCIProfile)getDCIProfile {
	return [[dciProfileMenu selectedItem] tag];
}

- (int)getDCIDataRate {
	if([self getDCIPerFrame] == DIALOG_DCI_PER_FRAME)
		return [dciRateField intValue];
	else
		return [dciRateField intValue] * 1024 / 8;
}

- (DialogDCIPerFrame)getDCIPerFrame {
	return [[dciPerFrameMenu selectedItem] tag];
}

- (int)getDCIFrameRate {
	return [[dciFPSmenu selectedItem] tag];
}

- (BOOL)getDCIStereo {
	return ([dciStereoCheck state] == NSOnState);
}

@end
