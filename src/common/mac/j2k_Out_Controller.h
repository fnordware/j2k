#import <Cocoa/Cocoa.h>

#include "j2k_OutUI.h"

#if !NSINTEGER_DEFINED
typedef int NSInteger;
#endif

typedef enum {
	DIALOG_RESULT_CONTINUE = 0,
	DIALOG_RESULT_OK,
	DIALOG_RESULT_CANCEL
} DialogResult;

@interface j2k_Out_Controller : NSObject {
    IBOutlet NSButton *losslessRadio;
    IBOutlet NSButton *digitalCinemaRadio;
    IBOutlet NSButton *fileSizeRadio;
    IBOutlet NSTextField *fileSizeField;
    IBOutlet NSButton *qualityRadio;
    IBOutlet NSTextField *qualityField;
    IBOutlet NSSlider *qualitySlider;
    IBOutlet NSButton *advancedCheck;
    IBOutlet NSPopUpButton *formatMenu;
    IBOutlet NSTextField *formatLabel;
    IBOutlet NSButton *bitDepthCheck;
    IBOutlet NSTextField *bitDepthField;
    IBOutlet NSStepper *bitDepthStepper;
    IBOutlet NSButton *yccCheck;
    IBOutlet NSButton *floatCheck;
    IBOutlet NSTextField *orderLabel;
    IBOutlet NSPopUpButton *orderMenu;
    IBOutlet NSTextField *tileSizeLabel;
    IBOutlet NSPopUpButton *tileSizeMenu;
    IBOutlet NSTextField *profileLabel;
    IBOutlet NSPopUpButton *profileMenu;
	IBOutlet NSTextField *subsampleLabel;
	IBOutlet NSPopUpButton *subsampleMenu;
	IBOutlet NSTextField *dciProfileLabel;
	IBOutlet NSPopUpButton *dciProfileMenu;
	IBOutlet NSSlider *dciRateSlider;
	IBOutlet NSTextField *dciRateField;
	IBOutlet NSTextField *dciRateLabel;
	IBOutlet NSPopUpButton *dciPerFrameMenu;
	IBOutlet NSPopUpButton *dciFPSmenu;
	IBOutlet NSTextField *dciFPSlabel;
	IBOutlet NSButton *dciStereoCheck;
	
    IBOutlet NSWindow *theWindow;
	IBOutlet NSButton *okButton;
	IBOutlet NSButton *cancelButton;
	
	DialogResult theResult;
}
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
	show_subsample:(BOOL)show_sub;
	

- (IBAction)clickedCancel:(id)sender;
- (IBAction)clickedOK:(id)sender;
- (IBAction)trackAdvanced:(id)sender;
- (IBAction)trackCustomBitDepth:(id)sender;
- (IBAction)trackFloat:(id)sender;
- (IBAction)trackMethod:(id)sender;
- (IBAction)trackPerFrame:(id)sender;

- (NSWindow *)getWindow;
- (DialogResult)getResult;

- (void)setMethod:(DialogMethod)method;
- (void)setFormat:(DialogFormat)the_format;
- (void)setSubsample:(DialogSubsample)sub;
- (void)setOrder:(DialogOrder)the_order;
- (void)setTileSize:(int)tile_size;
- (void)setProfile:(DialogProfile)icc_profile;
- (void)setDCIProfile:(DialogDCIProfile)dci_profile;
- (void)setDCIRate:(int)dci_data_rate;
- (void)setDCIPerFrame:(DialogDCIPerFrame)dci_per_frame;
- (void)setDCIFrameRate:(int)dci_frame_rate;


- (DialogMethod)getMethod;
- (long)getSize;
- (int)getQuality;
- (DialogFormat)getFormat;
- (BOOL)getCustomDepth;
- (int)getBitDepth;
- (BOOL)getReversible;
- (BOOL)getAdvanced;
- (BOOL)getYcc;
- (DialogSubsample)getSubsample;
- (DialogOrder)getOrder;
- (int)getTileSize;
- (DialogProfile)getProfile;
- (DialogDCIProfile)getDCIProfile;
- (int)getDCIDataRate;
- (DialogDCIPerFrame)getDCIPerFrame;
- (int)getDCIFrameRate;
- (BOOL)getDCIStereo;

@end
