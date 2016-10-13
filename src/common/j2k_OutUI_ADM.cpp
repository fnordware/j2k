
#include "j2k_OutUI.h"

#include "AE_GeneralPlug.h"
#include "A.h"
#include "ADM_PaletteUIConstants.h"
#include "ADMTypes.h"
#include "ADMBasic.h"
#include "ADMItem.h"
#include "ADMEntry.h"
#include "ADMList.h"
#include "ADMDialog.h"
#include "ADMDialogGroup.h"
#include "ADMNotifier.h"
#include "SDK_ADM.h"
#include "AE_EffectUI.h"

#include "DlgLayoutUtils.h"
#include "String_Utils.h"
#include "SPSuites.h"
#include "AE_AdvEffectSuites.h"
#include "AE_EffectCBSuites.h"
#include "AE_Macros.h"
#include "AEGP_SuiteHandler.h"

#include <stdio.h>
#include <string.h>

#ifdef macintosh

// ColorSync headers
#include "CMApplication.h"
#include "Displays.h"

#include "FullPath.h"

#else

#include <windows.h>
#include "icm.h"

#endif

#include "ParseRestrictedICCProfile.h"
#include "SwapEndian.h"


#include "jp2_shared.h"
#include "jp2.h"
#include "jpx.h"


// dialog comtrols
enum {
	OUT_noUI = -1,
	OUT_OK = 1,
	OUT_Cancel,
	OUT_Picture,
	OUT_Lossless,
	OUT_Size,
	OUT_Quality,
	OUT_Size_Field,
	OUT_Size_Field_Label,
	OUT_Quality_Field,
	OUT_Quality_Slider,
	OUT_Quality_Slider_Low_Text,
	OUT_Quality_Slider_Hi_Text,
	OUT_Border,
	OUT_Border_Label,
	OUT_Advanced,
	OUT_Format,
	OUT_Format_Label,
	OUT_Custom_Depth,
	OUT_Depth,
	OUT_Reversible,
	OUT_Ycc,
	OUT_Subsampling,
	OUT_Subsampling_Label,
	OUT_Layers,
	OUT_Layers_Label,
	OUT_Tiles,
	OUT_Tiles_Label,
	OUT_Order,
	OUT_Order_Label,
	OUT_Color_Space,
	OUT_Color_Space_Label
};




#define DIALOG_TITLE	"j2k Options"

#define DIALOG_WIDTH	300
#define DIALOG_HEIGHT	275

#define ADVANCED_DIALOG_WIDTH	600
#define ADVANCED_DIALOG_HEIGHT	275


#define BUTTON_WIDTH	80
#define BUTTON_HEIGHT	20

#define MENU_WIDTH		100
#define MENU_HEIGHT		25
#define MENU_LABEL_DOWN	2

#define LABEL_WIDTH		50
#define LABEL_HEIGHT	20
#define LABEL_SPACE		10

#define RADIO_WIDTH		130
#define RADIO_HEIGHT	25
#define RADIO_SPACE		8

#define CHECK_WIDTH		200
#define CHECK_HEIGHT	20

#define BORDER_MARGIN	10

#define BORDER_TEXT_HEIGHT	12
#define BORDER_TEXT_WIDTH	30
#define BORDER_TEXT_OVER	10

#define FIELD_HEIGHT		20
#define FIELD_WIDTH			60
#define FIELD_H_OFFSET		10
#define FIELD_V_OFFSET		0

#define SLIDER_HEIGHT		10

#define DIALOG_MARGIN	20

#define PICTURE_MARGIN	5

// vertical space between items
#define ITEM_SPACE		15


#define OK_TOP(D_HEIGHT)	(D_HEIGHT - (DIALOG_MARGIN + BUTTON_HEIGHT) )
#define OK_BOTTOM(D_HEIGHT)	(OK_TOP(D_HEIGHT) + BUTTON_HEIGHT)
#define OK_LEFT(D_WIDTH)	(D_WIDTH - (DIALOG_MARGIN + BUTTON_WIDTH) )
#define OK_RIGHT(D_WIDTH)	(OK_LEFT(D_WIDTH) + BUTTON_WIDTH)
#define OK_TEXT				"OK"

#define CANCEL_SEPERATION	20

#define CANCEL_TOP(D_HEIGHT)	OK_TOP(D_HEIGHT)
#define CANCEL_BOTTOM(D_HEIGHT)	OK_BOTTOM(D_HEIGHT)
#define CANCEL_LEFT(D_WIDTH)	(OK_LEFT(D_WIDTH) - (BUTTON_WIDTH + CANCEL_SEPERATION) )
#define CANCEL_RIGHT(D_WIDTH)	(CANCEL_LEFT(D_WIDTH) + BUTTON_WIDTH)
#define CANCEL_TEXT		"Cancel"

#define PICTURE_WIDTH	250
#define PICTURE_HEIGHT	50

#define PICTURE_TOP		PICTURE_MARGIN
#define PICTURE_BOTTOM	(PICTURE_TOP + PICTURE_HEIGHT + 1)
#define PICTURE_LEFT	DIALOG_MARGIN
#define PICTURE_RIGHT	(PICTURE_LEFT + PICTURE_WIDTH + 1)


#define LOSSLESS_TOP	(PICTURE_BOTTOM + PICTURE_MARGIN)
#define LOSSLESS_BOTTOM	(LOSSLESS_TOP + RADIO_HEIGHT)
#define LOSSLESS_LEFT	(DIALOG_MARGIN + BORDER_MARGIN)
#define LOSSLESS_RIGHT	(LOSSLESS_LEFT + RADIO_WIDTH)
#define LOSSLESS_TEXT	"Lossless"
#define LOSSLESS_IRREVERSIBLE_TEXT	"Maximum"


#define SIZE_TOP	(LOSSLESS_BOTTOM + RADIO_SPACE + RADIO_SPACE)
#define SIZE_BOTTOM	(SIZE_TOP + RADIO_HEIGHT)
#define SIZE_LEFT	(LOSSLESS_LEFT + BORDER_MARGIN)
#define SIZE_RIGHT	(SIZE_LEFT + RADIO_WIDTH)
#define SIZE_TEXT	"File Size"

#define SIZE_FIELD_TOP		(SIZE_TOP - FIELD_V_OFFSET)
#define SIZE_FIELD_BOTTOM	(SIZE_FIELD_TOP + FIELD_HEIGHT)
#define SIZE_FIELD_LEFT		(SIZE_RIGHT + FIELD_H_OFFSET)
#define SIZE_FIELD_RIGHT	(SIZE_FIELD_LEFT + FIELD_WIDTH)

#define SIZE_FIELD_LABEL_TOP	(SIZE_TOP + 3)
#define SIZE_FIELD_LABEL_BOTTOM	SIZE_BOTTOM
#define SIZE_FIELD_LABEL_LEFT	(SIZE_FIELD_RIGHT + 3)
#define SIZE_FIELD_LABEL_RIGHT	(SIZE_FIELD_LABEL_LEFT + 10)
#define SIZE_FIELD_LABEL_TEXT	"k"

#define QUALITY_TOP		(SIZE_BOTTOM + RADIO_SPACE)
#define QUALITY_BOTTOM	(QUALITY_TOP + RADIO_HEIGHT)
#define QUALITY_LEFT	SIZE_LEFT
#define QUALITY_RIGHT	SIZE_RIGHT
#define QUALITY_TEXT	"Quality"

#define QUALITY_FIELD_TOP	(QUALITY_TOP - FIELD_V_OFFSET)
#define QUALITY_FIELD_BOTTOM (QUALITY_FIELD_TOP + FIELD_HEIGHT)
#define QUALITY_FIELD_LEFT	SIZE_FIELD_LEFT
#define QUALITY_FIELD_RIGHT	SIZE_FIELD_RIGHT

#define QUALITY_SLIDER_TOP		(QUALITY_BOTTOM + RADIO_SPACE + RADIO_SPACE)
#define QUALITY_SLIDER_BOTTOM	(QUALITY_SLIDER_TOP + SLIDER_HEIGHT)
#define QUALITY_SLIDER_LEFT		QUALITY_LEFT
#define QUALITY_SLIDER_RIGHT	QUALITY_FIELD_RIGHT

#define SLIDER_LOW_TEXT_TOP		(QUALITY_SLIDER_TOP - BORDER_TEXT_HEIGHT)
#define SLIDER_LOW_TEXT_BOTTOM	(SLIDER_LOW_TEXT_TOP + BORDER_TEXT_HEIGHT)
#define SLIDER_LOW_TEXT_LEFT	QUALITY_SLIDER_LEFT
#define SLIDER_LOW_TEXT_RIGHT	(SLIDER_LOW_TEXT_LEFT + FIELD_WIDTH)
#define SLIDER_LOW_TEXT_TEXT	"Small File"

#define SLIDER_HI_TEXT_TOP		SLIDER_LOW_TEXT_TOP
#define SLIDER_HI_TEXT_BOTTOM	SLIDER_LOW_TEXT_BOTTOM
#define SLIDER_HI_TEXT_RIGHT	QUALITY_SLIDER_RIGHT
#define SLIDER_HI_TEXT_LEFT		(SLIDER_HI_TEXT_RIGHT - FIELD_WIDTH)
#define SLIDER_HI_TEXT_TEXT		"High Quality"

#define BORDER_TOP		(SIZE_TOP - BORDER_MARGIN)
#define BORDER_BOTTOM	(QUALITY_SLIDER_BOTTOM + BORDER_MARGIN)
#define BORDER_LEFT		(SIZE_LEFT - BORDER_MARGIN)
#define BORDER_RIGHT	(SIZE_FIELD_RIGHT + BORDER_MARGIN + BORDER_MARGIN)


#define BORDER_TEXT_TOP		(BORDER_TOP - BORDER_TEXT_HEIGHT/2)
#define BORDER_TEXT_BOTTOM	(BORDER_TEXT_TOP + BORDER_TEXT_HEIGHT)
#define BORDER_TEXT_LEFT	(BORDER_LEFT + BORDER_TEXT_OVER)
#define BORDER_TEXT_RIGHT	(BORDER_TEXT_LEFT + BORDER_TEXT_WIDTH)
#define BORDER_TEXT_TEXT	"Lossy"

#define ADVANCED_CHECK_TOP	(BORDER_BOTTOM + ITEM_SPACE)
#define ADVANCED_CHECK_BOTTOM (ADVANCED_CHECK_TOP + FIELD_HEIGHT)
#define ADVANCED_CHECK_LEFT	BORDER_LEFT
#define ADVANCED_CHECK_RIGHT (BORDER_LEFT + FIELD_WIDTH + FIELD_WIDTH)
#define ADVANCED_CHECK_TEXT	"advanced"

// Second column
#define FORMAT_LABEL_TOP		DIALOG_MARGIN
#define FORMAT_LABEL_BOTTOM		(FORMAT_LABEL_TOP + MENU_HEIGHT)
#define FORMAT_LABEL_LEFT		(BORDER_RIGHT + DIALOG_MARGIN + DIALOG_MARGIN)
#define FORMAT_LABEL_RIGHT		(FORMAT_LABEL_LEFT + LABEL_WIDTH )
#define FORMAT_LABEL_TEXT		"Format"

#define FORMAT_MENU_TOP			(FORMAT_LABEL_TOP - MENU_LABEL_DOWN)
#define FORMAT_MENU_BOTTOM		(FORMAT_MENU_TOP + MENU_HEIGHT)
#define FORMAT_MENU_LEFT		FORMAT_LABEL_RIGHT
#define FORMAT_MENU_RIGHT		(FORMAT_MENU_LEFT + MENU_WIDTH)

#define DEPTH_CUSTOM_TOP		(FORMAT_MENU_BOTTOM + ITEM_SPACE)
#define DEPTH_CUSTOM_BOTTOM		(DEPTH_CUSTOM_TOP + FIELD_HEIGHT)
#define DEPTH_CUSTOM_LEFT		FORMAT_LABEL_LEFT
#define DEPTH_CUSTOM_RIGHT		(DEPTH_CUSTOM_LEFT + MENU_WIDTH + MENU_WIDTH/3)
#define DEPTH_CUSTOM_TEXT		"custom bit depth:"

#define DEPTH_FIELD_TOP			DEPTH_CUSTOM_TOP
#define DEPTH_FIELD_BOTTOM		DEPTH_CUSTOM_BOTTOM
#define DEPTH_FIELD_LEFT		DEPTH_CUSTOM_RIGHT
#define DEPTH_FIELD_RIGHT		(DEPTH_FIELD_LEFT + FIELD_WIDTH)

#define YCC_CHECK_TOP			(DEPTH_CUSTOM_BOTTOM + ITEM_SPACE)
#define YCC_CHECK_BOTTOM		(YCC_CHECK_TOP + FIELD_HEIGHT)
#define YCC_CHECK_LEFT			DEPTH_CUSTOM_LEFT
#define YCC_CHECK_RIGHT			(YCC_CHECK_LEFT + FIELD_WIDTH)
#define YCC_CHECK_TEXT			"ycc"

#define REVERSIBLE_CHECK_TOP	YCC_CHECK_TOP
#define REVERSIBLE_CHECK_BOTTOM	YCC_CHECK_BOTTOM
#define REVERSIBLE_CHECK_LEFT	DEPTH_FIELD_LEFT
#define REVERSIBLE_CHECK_RIGHT	(REVERSIBLE_CHECK_LEFT + FIELD_WIDTH + FIELD_WIDTH)
#define REVERSIBLE_CHECK_TEXT	"float encoding"

#define SUBSAMPLING_LABEL_TOP		(YCC_CHECK_TOP + MENU_LABEL_DOWN)
#define SUBSAMPLING_LABEL_BOTTOM	YCC_CHECK_BOTTOM
#define SUBSAMPLING_LABEL_LEFT		(YCC_CHECK_RIGHT + ITEM_SPACE)
#define SUBSAMPLING_LABEL_RIGHT		(SUBSAMPLING_LABEL_LEFT + LABEL_WIDTH + 30)
#define SUBSAMPLING_LABEL_TEXT		"Subsampling"

#define SUBSAMPLING_MENU_TOP		(SUBSAMPLING_LABEL_TOP - MENU_LABEL_DOWN)
#define SUBSAMPLING_MENU_BOTTOM		(SUBSAMPLING_MENU_TOP + MENU_HEIGHT)
#define SUBSAMPLING_MENU_LEFT		SUBSAMPLING_LABEL_RIGHT
#define SUBSAMPLING_MENU_RIGHT		(SUBSAMPLING_MENU_LEFT + MENU_WIDTH)

#define LAYERS_LABEL_TOP		(YCC_CHECK_BOTTOM + ITEM_SPACE)
#define LAYERS_LABEL_BOTTOM		(LAYERS_LABEL_TOP + FIELD_HEIGHT)
#define LAYERS_LABEL_LEFT		FORMAT_LABEL_LEFT
#define LAYERS_LABEL_RIGHT		FORMAT_LABEL_RIGHT
#define LAYERS_LABEL_TEXT		"Layers"

#define LAYERS_FIELD_TOP		LAYERS_LABEL_TOP
#define LAYERS_FIELD_BOTTOM		LAYERS_LABEL_BOTTOM
#define LAYERS_FIELD_LEFT		FORMAT_MENU_LEFT
#define LAYERS_FIELD_RIGHT		(LAYERS_FIELD_LEFT + FIELD_WIDTH)

#define TILES_LABEL_TOP			(YCC_CHECK_BOTTOM + ITEM_SPACE + ITEM_SPACE)
#define TILES_LABEL_BOTTOM		(TILES_LABEL_TOP + FIELD_HEIGHT)
#define TILES_LABEL_LEFT		FORMAT_LABEL_LEFT
#define TILES_LABEL_RIGHT		FORMAT_LABEL_RIGHT
#define TILES_LABEL_TEXT		"Tile size"

#define TILES_MENU_TOP			(TILES_LABEL_TOP - MENU_LABEL_DOWN)
#define TILES_MENU_BOTTOM		(TILES_MENU_TOP + MENU_HEIGHT)
#define TILES_MENU_LEFT			TILES_LABEL_RIGHT
#define TILES_MENU_RIGHT		(TILES_MENU_LEFT + MENU_WIDTH)

#define ORDER_LABEL_TOP			(TILES_LABEL_BOTTOM + ITEM_SPACE - 5)
#define ORDER_LABEL_BOTTOM		(ORDER_LABEL_TOP + LABEL_HEIGHT)
#define ORDER_LABEL_LEFT		FORMAT_LABEL_LEFT
#define ORDER_LABEL_RIGHT		FORMAT_LABEL_RIGHT
#define ORDER_LABEL_TEXT		"Order"

#define ORDER_MENU_TOP			(ORDER_LABEL_TOP - MENU_LABEL_DOWN)
#define ORDER_MENU_BOTTOM		(ORDER_MENU_TOP + MENU_HEIGHT)
#define ORDER_MENU_LEFT			FORMAT_MENU_LEFT
#define ORDER_MENU_RIGHT		(FORMAT_MENU_RIGHT + FIELD_WIDTH)

#define COLOR_LABEL_TOP			(ORDER_LABEL_BOTTOM + ITEM_SPACE - 5)
#define COLOR_LABEL_BOTTOM		(COLOR_LABEL_TOP + MENU_HEIGHT)
#define COLOR_LABEL_LEFT		FORMAT_LABEL_LEFT
#define COLOR_LABEL_RIGHT		FORMAT_LABEL_RIGHT
#define COLOR_LABEL_TEXT		"Profile"

#define COLOR_MENU_TOP			(COLOR_LABEL_TOP - MENU_LABEL_DOWN)
#define COLOR_MENU_BOTTOM		(COLOR_MENU_TOP + MENU_HEIGHT)
#define COLOR_MENU_LEFT			FORMAT_MENU_LEFT
#define COLOR_MENU_RIGHT		(FORMAT_MENU_RIGHT + FIELD_WIDTH)


#ifdef macintosh
#define INIT_RECT(TOP, LEFT, BOTTOM, RIGHT)		{ (TOP), (LEFT), (BOTTOM), (RIGHT) }
#else
#define INIT_RECT(TOP, LEFT, BOTTOM, RIGHT)		{ (LEFT), (TOP), (RIGHT), (BOTTOM) }
#endif


extern AEGP_PluginID			S_mem_id;


static SPBasicSuite			*sP							=	NULL;


static JPEG_Method			g_method;
static A_u_char				g_quality;
static A_long				g_size;
static A_Boolean			g_advanced;
static JPEG_Format			g_format;
static A_Boolean			g_custom_depth;
static A_u_char				g_depth;
static A_Boolean			g_reversible;
static A_Boolean			g_ycc;
static JPEG_Subsampling		g_sub;
static A_u_char				g_layers;
static A_u_char				g_order;
static A_u_short			g_tile_size;
static JPEG_Color			g_color_space;
static void					*g_profile;

static ADMImageRef			g_image = NULL;


#define FILE_PROFILE		123
#define FILE_DIALOG			124
static AEIO_Handle			g_file_profileH = NULL;



static ADMEntryRef g_sRGB_entry;
static ADMEntryRef g_sYCC_entry;
static ADMEntryRef g_display_entry;

static ADMEntryRef g_last_color_entry;



static A_Boolean
LoadFileProfile(void)
{
	AEGP_SuiteHandler	suites(sP);
	
	A_Boolean found = FALSE;

	SPPlatformFileSpecification file_spec;
	
#ifdef macintosh
	ADMPlatformFileTypesSpecification3 in_filter;

	in_filter.types = NULL;
	in_filter.numTypes = 0;
	
	strcpy(in_filter.filter, "*.icc,*.jp2,*.jpx,*.jpf,*.j2k,*.j2c");
#else
	ADMPlatformFileTypesSpecification3 in_filter = {
		"JPEG 2000 Files (*.jpx, *.jp2, *.jpf, *.j2k, *.j2c)\0"
			"*.jpx;*.jp2;*.jpf;*.j2k;*.j2c\0"
		"ICC Profiles (*.icc)\0"
			"*.icc\0"
		"All Files\0"
			"*.*\0"
		"\0"
	};
#endif			

	if(suites.ADMBasicSuite()->StandardGetFileDialog("Load Profile", &in_filter, NULL, NULL, &file_spec) )
	{
#ifdef macintosh
			// getting the full path from the FSSpec
			char path[kMaxPathLength];
			Handle path_handle;
			short path_len;
			
			FSpGetFullPath((FSSpec *)&file_spec, &path_len, &path_handle);
			
			HLock(path_handle);
			
			strncpy(path, (char *)(*path_handle), path_len);
			
			path[path_len] = '\0';
			
			DisposeHandle(path_handle);
#else
			// on Windows we already get the path
			char *path;
			
			path = file_spec.path;
#endif

		// delete any existing file profile handle
		if(g_file_profileH)
		{
			suites.MemorySuite()->AEGP_FreeMemHandle(g_file_profileH);
			g_file_profileH = NULL;
			
			// select sRGB and reset display just in case
			suites.ADMEntrySuite()->Select(g_sRGB_entry, true);
			suites.ADMEntrySuite()->SetText(g_display_entry, "");
			suites.ADMEntrySuite()->SetUserData(g_display_entry, (ADMUserData)JP2_COLOR_sRGB);
			suites.ADMEntrySuite()->Enable(g_display_entry, false);
			g_last_color_entry = g_sRGB_entry;
		}

		
		JPEG_Format format = GetFileType(path);
		
		if(format == JP2_TYPE_J2C)
			return FALSE; // won't get anything from a j2c
		else if(format == JP2_TYPE_UNKNOWN)
		{
			// a raw ICC profile perhaps?
			
			FILE *fp = fopen(path, "r");
			
			if(fp)
			{
				uint32_t prof_len;
				
				fread((void *)&prof_len, sizeof(uint32_t), 1, fp);
				
				prof_len = SwapEndianMisaligned(prof_len);
			
				// profile starts out with a length
				if(prof_len)
				{
					fseek(fp, 0, SEEK_SET); // back to beginning
					
					suites.MemorySuite()->AEGP_NewMemHandle( S_mem_id, "File Profile",
													prof_len,
													AEGP_MemFlag_CLEAR, &g_file_profileH);
					
					char *prof_buf = NULL;
					
					suites.MemorySuite()->AEGP_LockMemHandle(g_file_profileH, (void**)&prof_buf);

				#ifdef macintosh
					if( fread((void *)prof_buf, prof_len, 1, fp) ) // we read the whole thing?
				#else
					fread((void *)prof_buf, prof_len, 1, fp); // this return value is confusing me

					if( true )
				#endif
					{
						// check the profile
						ICCProfile *icc = (ICCProfile *)prof_buf;
						
						if(	icc->head.dwProfileSignature == SwapEndianMisaligned(kdwProfileSignature) &&
							icc->head.dwColorSpaceType == SwapEndianMisaligned(kdwRGBData) &&
							(icc->head.dwProfileClass == SwapEndianMisaligned(kdwInputProfile) || 
							 icc->head.dwProfileClass == SwapEndianMisaligned(kdwDisplayProfile) ) )
						{
							suites.MemorySuite()->AEGP_UnlockMemHandle(g_file_profileH);
						
							// set up our place in the menu
							suites.ADMEntrySuite()->Select(g_display_entry, true);
							suites.ADMEntrySuite()->SetText(g_display_entry, GetProfileDescription(icc) );
							suites.ADMEntrySuite()->SetUserData(g_display_entry, (ADMUserData)FILE_PROFILE);
							suites.ADMEntrySuite()->Enable(g_display_entry, true);
							found = TRUE;
						}
						else
						{
							suites.ADMBasicSuite()->Beep();
							suites.ADMBasicSuite()->ErrorAlert("Not a valid RGB profile.");
						}
					}
					else
						suites.ADMBasicSuite()->ErrorAlert("Didn't read file!");
					
					suites.MemorySuite()->AEGP_UnlockMemHandle(g_file_profileH);
					
					if(!found)
					{
						suites.MemorySuite()->AEGP_FreeMemHandle(g_file_profileH);
						g_file_profileH = NULL;
					}
				}
				
				fclose(fp);
			}
		}
		else // JP2 or JPX
		{
			jp2_family_src	fam_src;
			jp2_source jp2_in;
			jpx_source jpx_in;
			
			jp2_colour colr;
			
			
			fam_src.open(path);
			
			if(format == JP2_TYPE_JPX)
			{
				jpx_in.open(&fam_src, FALSE);
				
				jpx_layer_source jpx_layer = jpx_in.access_layer(0);
				
				colr = jpx_layer.access_colour(0);
			}
			else // JP2
			{
				jp2_in.open(&fam_src);
				
				jp2_in.read_header();
				
				colr = jp2_in.access_colour();
			}
			
			// color info
			jp2_colour_space colorSpace = (jp2_colour_space)-1; // unknown right now
			
			if( colr.exists() )
			{
				colorSpace = colr.get_space();

				if(colorSpace == JP2_sRGB_SPACE || colorSpace == JP2_sLUM_SPACE)
				{
					// we're sRGB
					suites.ADMEntrySuite()->Select(g_sRGB_entry, true);
					found = TRUE;
				}
				//else if(colorSpace == JP2_sYCC_SPACE)
				//{
				//	// YCC
				//	suites.ADMEntrySuite()->Select(g_sYCC_entry, true);
				//	found = TRUE;
				//}
				else if(colorSpace == JP2_iccRGB_SPACE || colorSpace == JP2_iccLUM_SPACE ||
						colorSpace == JP2_iccANY_SPACE )
				{
					// got a profile - better be RGB
					ICCProfile *icc = (ICCProfile *)colr.get_icc_profile();
					
					if(kdwRGBData == SwapEndianMisaligned(icc->head.dwColorSpaceType) ||
						kdwGrayData == SwapEndianMisaligned(icc->head.dwColorSpaceType) )
					{
						uint32_t prof_len = SwapEndianMisaligned(icc->head.dwProfileSize);
						
						suites.MemorySuite()->AEGP_NewMemHandle( S_mem_id, "File Profile",
														prof_len,
														AEGP_MemFlag_CLEAR, &g_file_profileH);
						
						char *prof_buf = NULL;
						
						suites.MemorySuite()->AEGP_LockMemHandle(g_file_profileH, (void**)&prof_buf);

						// copy profile to buffer
						memcpy((char*)prof_buf, (char*)icc, prof_len);

						suites.MemorySuite()->AEGP_UnlockMemHandle(g_file_profileH);
						

						// set up our place in the menu
						suites.ADMEntrySuite()->Select(g_display_entry, true);
						suites.ADMEntrySuite()->SetText(g_display_entry, GetProfileDescription(icc) );
						suites.ADMEntrySuite()->SetUserData(g_display_entry, (ADMUserData)FILE_PROFILE);
						suites.ADMEntrySuite()->Enable(g_display_entry, true);
						found = TRUE;
					}
				}
			}
			
			// files should close by themselves (thanks, destructors)
		}
	}
	
	return found;
}


#define MAX_PROFILES	255

#define PROF_TO_USER(NUM)	(NUM + 1000)
#define USER_TO_PROF(NUM)	(NUM - 1000)
#define IS_OS_PROFILE(NUM)	(NUM >= 1000)


#ifdef macintosh

static CMProfileLocation g_profs[ MAX_PROFILES ];
static int g_prof_count = 0;

static CMProfileLocation g_disp_prof;



static OSErr profIterateProc(CMProfileIterateData* data, void* refcon)
{
	OSErr err = noErr;
	
	AEGP_SuiteHandler	suites(sP);
	
	ADMListRef Color_list = (ADMListRef)refcon;
	
	if(	data->header.dataColorSpace == cmRGBData &&
		(data->header.profileClass == cmInputClass || data->header.profileClass == cmDisplayClass) )
	{
		if(g_prof_count < MAX_PROFILES)
		{
			g_profs[ g_prof_count ] = data->location;
			
			// ADM part
			ADMEntryRef entry = suites.ADMListSuite()->InsertEntry(Color_list, -1);
			suites.ADMEntrySuite()->SetText(entry, (char *)data->asciiName);
			suites.ADMEntrySuite()->SetUserData(entry, (ADMUserData)PROF_TO_USER(g_prof_count) );
			
		
			// check for display profile
			if(data->location.u.fileLoc.spec.vRefNum == g_disp_prof.u.fileLoc.spec.vRefNum &&
				!strncmp((char *)data->location.u.fileLoc.spec.name,
					(char *)g_disp_prof.u.fileLoc.spec.name,
					 data->location.u.fileLoc.spec.name[0]) )
			{
				char name[255];
				
				sprintf(name, "Display Profile (%s)", data->asciiName);
			
				// fill in the display entry thing
				suites.ADMEntrySuite()->SetText(g_display_entry, (char *)name);
				suites.ADMEntrySuite()->SetUserData(g_display_entry, (ADMUserData)PROF_TO_USER(g_prof_count) );
				suites.ADMEntrySuite()->Enable(g_display_entry, true);
			}
			
			g_prof_count++;
		}
	}
	
	return err;
}


static void GetDisplayProfile(CMProfileLocation *profLoc)
{
	CMProfileRef prof;
	
	CMError     theErr;
	AVIDType    theAVID;
	GDHandle    theDevice;
	
	UInt32 locationSize = cmCurrentProfileLocationSize;

	// Get the main GDevice.
	theDevice = GetMainDevice();

	// Get the AVID for that device.
	theErr = DMGetDisplayIDByGDevice(theDevice, &theAVID, true);

	// Get the profile for that AVID.
	theErr = CMGetProfileByAVID(theAVID, &prof);
	
	// Get location (FSRef) for that profile
	theErr = NCMGetProfileLocation(prof, profLoc, &locationSize);
}


static void FillInProfiles(ADMListRef Color_list)
{
	UInt32 seed = 0;
	UInt32 count;

	// store display profile
	GetDisplayProfile(&g_disp_prof);

	//Get profile list
	CMProfileIterateUPP iterateUPP;
	iterateUPP = NewCMProfileIterateUPP((CMProfileIterateProcPtr)&profIterateProc);

	CMIterateColorSyncFolder(iterateUPP, &seed, &count, (void *)Color_list);
		
	DisposeCMProfileIterateUPP(iterateUPP);
}


static long GetProfileSize(JPEG_Color prof_index)
{
	OSErr err = noErr;

	uint32_t profSize = 0;
	
	CMProfileLocation profLoc = g_profs[ USER_TO_PROF( prof_index ) ];

	if(profLoc.locType == cmFileBasedProfile)
	{
		SInt16 refNum;
		SInt32 theCount;
		
		err = FSpOpenDF(&profLoc.u.fileLoc.spec, fsCurPerm, &refNum);
		
		if(!err)
		{
			theCount = sizeof(uint32_t);
			
			err = FSRead(refNum, &theCount, &profSize);
			
			profSize = SwapEndianMisaligned(profSize);
			
			err = FSClose(refNum);
		}
	}
	
	return profSize;
}


static void CopyProfile(JPEG_Color prof_index, void *buf)
{
	OSErr err = noErr;

	uint32_t profSize;
	
	CMProfileLocation profLoc = g_profs[ USER_TO_PROF( prof_index ) ];

	if(profLoc.locType == cmFileBasedProfile)
	{
		SInt16 refNum;
		SInt32 theCount;
		
		err = FSpOpenDF(&profLoc.u.fileLoc.spec, fsCurPerm, &refNum);
		
		if(!err)
		{
			theCount = sizeof(UInt32);
			
			err = FSRead(refNum, &theCount, &profSize);
			
			profSize = SwapEndianMisaligned(profSize);
			
			SetFPos (refNum, fsFromStart, 0);
			
			theCount = profSize;
			
			err = FSRead(refNum, &theCount, buf);
			
			err = FSClose(refNum);
		}
	}
}

#else

static void FillInProfiles(ADMListRef Color_list)
{
	DWORD err = ERROR_SUCCESS;
	
	DWORD buf_size;
	DWORD profiles;
	BYTE buf[1024];
	char *prof_name = (char *)buf;
	
	// the fields that we are specifying (color space)
	ENUMTYPE head;
	head.dwFields = ET_DATACOLORSPACE;
	head.dwDataColorSpace = SPACE_RGB;
	head.dwDataColorSpace = ' BGR'; // try this
	//head.dwClass = CLASS_SCANNER;

	AEGP_SuiteHandler suites(sP);


	if( EnumColorProfiles(NULL, &head, buf, &buf_size, &profiles) )
	{
		for(int i=0; i<profiles; i++)
		{
			ADMEntryRef entry = suites.ADMListSuite()->InsertEntry(Color_list, -1);
			suites.ADMEntrySuite()->SetText(entry, prof_name);
			//suites.ADMEntrySuite()->SetUserData(entry, (ADMUserData)PROF_TO_USER(g_prof_count) );

			prof_name += (strlen(prof_name) + 1);
		}
	}
	else
		err = GetLastError();
}


static long GetProfileSize(JPEG_Color prof_index)
{
	// not doing profiles on Windows now
	return 0;
}


static void CopyProfile(JPEG_Color prof_index, void *buf)
{
	// not actually copying these on Win now
}


#endif // macintosh

#pragma mark-


static void ASAPI
OKNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	AEGP_SuiteHandler suites(sP);

	suites.ADMItemSuite()->DefaultNotify(item, notifier);
		
	if (suites.ADMNotifierSuite()->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		// do this when the user clicks OK
		ADMDialogRef dialog = suites.ADMItemSuite()->GetDialog(item);
		
		// get the format type
		// we could move these to their own pocs if we wanted
		ADMItemRef format_menu = suites.ADMDialogSuite()->GetItem(dialog, OUT_Format);

		ADMListRef menu_list = suites.ADMItemSuite()->GetList(format_menu);
		
		ADMEntryRef sel_ent = suites.ADMListSuite()->GetActiveEntry(menu_list);
		
		g_format = (A_long)suites.ADMEntrySuite()->GetUserData(sel_ent);


		// get the order
		ADMItemRef order_menu = suites.ADMDialogSuite()->GetItem(dialog, OUT_Order);

		ADMListRef order_list = suites.ADMItemSuite()->GetList(order_menu);
		
		ADMEntryRef selected = suites.ADMListSuite()->GetActiveEntry(order_list);
		
		g_order = (A_long)suites.ADMEntrySuite()->GetUserData(selected);
		
		// get the tile size
		ADMItemRef tiles_menu = suites.ADMDialogSuite()->GetItem(dialog, OUT_Tiles);

		ADMListRef tiles_list = suites.ADMItemSuite()->GetList(tiles_menu);
		
		ADMEntryRef sel_tiles = suites.ADMListSuite()->GetActiveEntry(tiles_list);
		
		g_tile_size = (A_long)suites.ADMEntrySuite()->GetUserData(sel_tiles);

		// get the subsampling
/*		ADMItemRef sub_menu = suites.ADMDialogSuite()->GetItem(dialog, OUT_Subsampling);

		ADMListRef sub_list = suites.ADMItemSuite()->GetList(sub_menu);
		
		ADMEntryRef sel_sub = suites.ADMListSuite()->GetActiveEntry(sub_list);
		
		g_sub = (A_long)suites.ADMEntrySuite()->GetUserData(sel_sub);
*/
		// get the color space
/* -- now using a notify proc
		ADMItemRef color_menu = suites.ADMDialogSuite()->GetItem(dialog, OUT_Color_Space);

		ADMListRef color_list = suites.ADMItemSuite()->GetList(color_menu);
		
		ADMEntryRef sel_color = suites.ADMListSuite()->GetActiveEntry(color_list);
		
		g_color_space = (A_long)suites.ADMEntrySuite()->GetUserData(sel_color);
*/
	}
}


static void ASAPI
RadioNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	AEGP_SuiteHandler suites(sP);

	suites.ADMItemSuite()->DefaultNotify(item, notifier);
		
	if (suites.ADMNotifierSuite()->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		g_method = (A_long)suites.ADMItemSuite()->GetUserData(item);
	}
}


static void ASAPI
SizeFieldNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	AEGP_SuiteHandler suites(sP);

	suites.ADMItemSuite()->DefaultNotify(item, notifier);
		
	if (suites.ADMNotifierSuite()->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		g_size = suites.ADMItemSuite()->GetIntValue(item);
	}
}


static void ASAPI
QualityFieldNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	AEGP_SuiteHandler suites(sP);

	suites.ADMItemSuite()->DefaultNotify(item, notifier);
		
	if (suites.ADMNotifierSuite()->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		g_quality = (A_long)suites.ADMItemSuite()->GetIntValue(item);
		
		// update the slider
		ADMDialogRef dialog = suites.ADMItemSuite()->GetDialog(item);

		ADMItemRef quality_slider = suites.ADMDialogSuite()->GetItem(dialog, OUT_Quality_Slider);
		
		suites.ADMItemSuite()->SetIntValue(quality_slider, g_quality);
		
		// do we need to redraw?
	}
}


static ASBoolean ASAPI
QualitySliderTrackProc(ADMItemRef item, ADMTrackerRef inTracker)
{
	AEGP_SuiteHandler suites(sP);

	ADMDialogRef dialog = suites.ADMItemSuite()->GetDialog(item);
	
	ADMItemRef quality_textbox, lossless_radio, size_radio, quality_radio;
	
	suites.ADMItemSuite()->DefaultTrack(item, inTracker);
	
	//ADMAction action = sADMTrack->GetAction(inTracker);

	if(suites.ADMTrackerSuite()->TestAction(inTracker, kADMButtonUpAction) )
	{
		quality_textbox = suites.ADMDialogSuite()->GetItem(dialog, OUT_Quality_Field);

		lossless_radio = suites.ADMDialogSuite()->GetItem(dialog, OUT_Lossless);
		size_radio     = suites.ADMDialogSuite()->GetItem(dialog, OUT_Size);
		quality_radio  = suites.ADMDialogSuite()->GetItem(dialog, OUT_Quality);

		suites.ADMItemSuite()->SetBooleanValue(lossless_radio, false);
		suites.ADMItemSuite()->SetBooleanValue(size_radio,     false);
		suites.ADMItemSuite()->SetBooleanValue(quality_radio,  true );
		
		suites.ADMItemSuite()->Activate(quality_textbox, true);
		suites.ADMItemSuite()->SelectAll(quality_textbox);
				
		g_quality = suites.ADMItemSuite()->GetIntValue(quality_textbox);
		g_method = JP2_METHOD_QUALITY;
	}
	else
	{
		quality_textbox = suites.ADMDialogSuite()->GetItem(dialog, OUT_Quality_Field);
		
		suites.ADMItemSuite()->SetIntValue(quality_textbox,
												suites.ADMItemSuite()->GetIntValue(item) );
	}		
	
	return suites.ADMItemSuite()->DefaultTrack(item, inTracker);
}


static void ASAPI
AdvancedNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	AEGP_SuiteHandler suites(sP);

	suites.ADMItemSuite()->DefaultNotify(item, notifier);
		
	if (suites.ADMNotifierSuite()->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		g_advanced = suites.ADMItemSuite()->GetBooleanValue(item);
		
		// move them buttons
		ADMDialogRef dialog = suites.ADMItemSuite()->GetDialog(item);

		ADMItemRef OK_button = suites.ADMDialogSuite()->GetItem(dialog, OUT_OK);
		ADMItemRef Cancel_button = suites.ADMDialogSuite()->GetItem(dialog, OUT_Cancel);
		

		int d_width, d_height;
		
		if(g_advanced)
		{
			d_width = ADVANCED_DIALOG_WIDTH;
			d_height = ADVANCED_DIALOG_HEIGHT;
		}
		else
		{
			d_width = DIALOG_WIDTH;
			d_height = DIALOG_HEIGHT;
		}
		
		ASRect OK_rect = INIT_RECT(OK_TOP(d_height), OK_LEFT(d_width), OK_BOTTOM(d_height), OK_RIGHT(d_width) );
		ASRect Cancel_rect = INIT_RECT(CANCEL_TOP(d_height), CANCEL_LEFT(d_width), CANCEL_BOTTOM(d_height), CANCEL_RIGHT(d_width) );
		
		
		// move!
		suites.ADMDialogSuite()->Size(dialog, d_width, d_height);

		suites.ADMItemSuite()->SetBoundsRect(OK_button, &OK_rect);
		suites.ADMItemSuite()->SetBoundsRect(Cancel_button, &Cancel_rect);

		// irreversible is never truly lossless
		ADMItemRef Lossless_radio = suites.ADMDialogSuite()->GetItem(dialog, OUT_Lossless);
		
		if(g_advanced && !g_reversible)
			suites.ADMItemSuite()->SetText(Lossless_radio, LOSSLESS_IRREVERSIBLE_TEXT);
		else
			suites.ADMItemSuite()->SetText(Lossless_radio, LOSSLESS_TEXT);
	}
}


static void ASAPI
CustomDepthNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	AEGP_SuiteHandler suites(sP);

	suites.ADMItemSuite()->DefaultNotify(item, notifier);
		
	if (suites.ADMNotifierSuite()->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		g_custom_depth = suites.ADMItemSuite()->GetBooleanValue(item);

		// depth field is active when this box is checked
		ADMDialogRef dialog = suites.ADMItemSuite()->GetDialog(item);

		ADMItemRef Depth_field = suites.ADMDialogSuite()->GetItem(dialog, OUT_Depth);

		suites.ADMItemSuite()->Enable(Depth_field, g_custom_depth);
	}
}


static void ASAPI
DepthFieldNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	AEGP_SuiteHandler suites(sP);

	suites.ADMItemSuite()->DefaultNotify(item, notifier);
		
	if (suites.ADMNotifierSuite()->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		g_depth = suites.ADMItemSuite()->GetIntValue(item);
	}
}


/*
static void ASAPI
LayersFieldNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	AEGP_SuiteHandler suites(sP);

	suites.ADMItemSuite()->DefaultNotify(item, notifier);
		
	if (suites.ADMNotifierSuite()->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		g_layers = suites.ADMItemSuite()->GetIntValue(item);
	}
}
*/

static void ASAPI
ReversibleNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	AEGP_SuiteHandler suites(sP);

	suites.ADMItemSuite()->DefaultNotify(item, notifier);
		
	if (suites.ADMNotifierSuite()->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		// this box checked now means not reversible
		g_reversible = !(suites.ADMItemSuite()->GetBooleanValue(item));

		// irreversible is never truly lossless
		ADMDialogRef dialog = suites.ADMItemSuite()->GetDialog(item);

		ADMItemRef Lossless_radio = suites.ADMDialogSuite()->GetItem(dialog, OUT_Lossless);
		
		if(g_advanced && !g_reversible)
			suites.ADMItemSuite()->SetText(Lossless_radio, LOSSLESS_IRREVERSIBLE_TEXT);
		else
			suites.ADMItemSuite()->SetText(Lossless_radio, LOSSLESS_TEXT);
	}
}


static void ASAPI
YccNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	AEGP_SuiteHandler suites(sP);

	suites.ADMItemSuite()->DefaultNotify(item, notifier);
		
	if (suites.ADMNotifierSuite()->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		g_ycc = suites.ADMItemSuite()->GetBooleanValue(item);
	}
}


static void ASAPI
DrawPictureProc(ADMItemRef item, ADMDrawerRef drawer)
{
	AEGP_SuiteHandler suites(sP);
	
	extern ADMImageRef g_image;
	ASRect pos;
	
	if( g_image ) //sADMItem->IsVisible(item)
	{
		suites.ADMDrawerSuite()->GetBoundsRect(drawer, &pos);

		suites.ADMImageSuite()->BeginADMDrawer(g_image);
		suites.ADMDrawerSuite()->DrawADMImageCentered(drawer, g_image, &pos);
		suites.ADMImageSuite()->EndADMDrawer(g_image);
	}
	else
		suites.ADMDrawerSuite()->Clear(drawer);
}

static void ASAPI
ColorNotifyProc(ADMItemRef item, ADMNotifierRef notifier)
{
	ASErr err = kSPNoError;
	AEGP_SuiteHandler suites(sP);

	suites.ADMItemSuite()->DefaultNotify(item, notifier);
		
	if (suites.ADMNotifierSuite()->IsNotifierType(notifier, kADMUserChangedNotifier))	
	{
		ADMDialogRef dialog = suites.ADMItemSuite()->GetDialog(item);

		ADMItemRef color_menu = suites.ADMDialogSuite()->GetItem(dialog, OUT_Color_Space);

		ADMListRef color_list = suites.ADMItemSuite()->GetList(color_menu);
		
		ADMEntryRef sel_color = suites.ADMListSuite()->GetActiveEntry(color_list);
		
		
		A_long prof = (A_long)suites.ADMEntrySuite()->GetUserData(sel_color);
		

		if(prof == FILE_DIALOG)
		{
			// load file
			A_Boolean loaded = LoadFileProfile();
			
			if(!loaded)
				suites.ADMEntrySuite()->Select(g_last_color_entry, true); // bounce back to last one

			// this may be different now if the user loaded a file
			sel_color = suites.ADMListSuite()->GetActiveEntry(color_list);
			prof = (A_long)suites.ADMEntrySuite()->GetUserData(sel_color);
		}

		g_color_space = prof;

		// store this
		g_last_color_entry = sel_color;
	}
}

#pragma mark-


static ASErr ASAPI StandardInit(ADMDialogRef dialog)
{
	ASErr err = kSPNoError;

	AEGP_SuiteHandler	suites(sP);
	
	
	// picture
	ASRect Picure_rect = INIT_RECT(PICTURE_TOP, PICTURE_LEFT, PICTURE_BOTTOM, PICTURE_RIGHT);
	
	ADMItemRef Picture_item = suites.ADMItemSuite()->Create(dialog, OUT_Picture,
								kADMPictureStaticType, &Picure_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetDrawProc(Picture_item, DrawPictureProc);
	
	
	// create border text
	ASRect Border_text_rect = INIT_RECT(BORDER_TEXT_TOP, BORDER_TEXT_LEFT, BORDER_TEXT_BOTTOM, BORDER_TEXT_RIGHT);

	ADMItemRef Border_text = suites.ADMItemSuite()->Create(dialog, OUT_Border_Label,
								kADMTextStaticType, &Border_text_rect, NULL, NULL, NULL);

	suites.ADMItemSuite()->SetText(Border_text, BORDER_TEXT_TEXT);
	suites.ADMItemSuite()->SetJustify(Border_text, kADMCenterJustify);
	suites.ADMItemSuite()->SetFont(Border_text, kADMPaletteFont);


#ifdef macintosh
	// create border
	ASRect Border_rect = INIT_RECT(BORDER_TOP, BORDER_LEFT, BORDER_BOTTOM, BORDER_RIGHT);

	ADMItemRef Border = suites.ADMItemSuite()->Create(dialog, OUT_Border,
								kADMFrameType, &Border_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetItemStyle(Border, kADMEtchedFrameStyle);
#endif

	// create the radio buttons
	ASRect Lossless_rect = INIT_RECT(LOSSLESS_TOP, LOSSLESS_LEFT, LOSSLESS_BOTTOM, LOSSLESS_RIGHT);
	ASRect Size_rect = INIT_RECT(SIZE_TOP, SIZE_LEFT, SIZE_BOTTOM, SIZE_RIGHT);
	ASRect Quality_rect = INIT_RECT(QUALITY_TOP, QUALITY_LEFT, QUALITY_BOTTOM, QUALITY_RIGHT);
	
	ADMItemRef Lossless_radio = suites.ADMItemSuite()->Create(dialog, OUT_Lossless,
								kADMTextRadioButtonType, &Lossless_rect, NULL, NULL, NULL);

	ADMItemRef Size_radio = suites.ADMItemSuite()->Create(dialog, OUT_Size,
								kADMTextRadioButtonType, &Size_rect, NULL, NULL, NULL);

	ADMItemRef Quality_radio = suites.ADMItemSuite()->Create(dialog, OUT_Quality,
								kADMTextRadioButtonType, &Quality_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Lossless_radio, LOSSLESS_TEXT);
	suites.ADMItemSuite()->SetText(Size_radio, SIZE_TEXT);
	suites.ADMItemSuite()->SetText(Quality_radio, QUALITY_TEXT);
	
	suites.ADMItemSuite()->SetUserData(Lossless_radio, (ADMUserData)JP2_METHOD_LOSSLESS);
	suites.ADMItemSuite()->SetUserData(Size_radio, (ADMUserData)JP2_METHOD_SIZE);
	suites.ADMItemSuite()->SetUserData(Quality_radio, (ADMUserData)JP2_METHOD_QUALITY);
	
	suites.ADMItemSuite()->SetNotifyProc(Lossless_radio, RadioNotifyProc);
	suites.ADMItemSuite()->SetNotifyProc(Size_radio, RadioNotifyProc);
	suites.ADMItemSuite()->SetNotifyProc(Quality_radio, RadioNotifyProc);

	
	// irreversible is never truly lossless
	if(g_advanced && !g_reversible)
		suites.ADMItemSuite()->SetText(Lossless_radio, LOSSLESS_IRREVERSIBLE_TEXT);
	

	if(g_method == JP2_METHOD_LOSSLESS)
		suites.ADMItemSuite()->SetBooleanValue(Lossless_radio, TRUE);
	else if(g_method == JP2_METHOD_SIZE)
		suites.ADMItemSuite()->SetBooleanValue(Size_radio, TRUE);
	else
		suites.ADMItemSuite()->SetBooleanValue(Quality_radio, TRUE);
		

	// make fields
	ASRect Size_field_rect = INIT_RECT(SIZE_FIELD_TOP, SIZE_FIELD_LEFT, SIZE_FIELD_BOTTOM, SIZE_FIELD_RIGHT);
	ASRect Quality_field_rect = INIT_RECT(QUALITY_FIELD_TOP, QUALITY_FIELD_LEFT, QUALITY_FIELD_BOTTOM, QUALITY_FIELD_RIGHT);

	ADMItemRef Size_field = suites.ADMItemSuite()->Create(dialog, OUT_Size_Field,
								kADMTextEditType, &Size_field_rect, NULL, NULL, NULL);

	ADMItemRef Quality_field = suites.ADMItemSuite()->Create(dialog, OUT_Quality_Field,
								kADMTextEditType, &Quality_field_rect, NULL, NULL, NULL);


	suites.ADMItemSuite()->SetIntValue(Size_field, g_size);
	suites.ADMItemSuite()->SetIntValue(Quality_field, g_quality);

	
	suites.ADMItemSuite()->SetMinIntValue(Size_field, 1);
	suites.ADMItemSuite()->SetMaxIntValue(Size_field, 99999);
	
	suites.ADMItemSuite()->SetMinIntValue(Quality_field, 1);
	suites.ADMItemSuite()->SetMaxIntValue(Quality_field, 100);


	suites.ADMItemSuite()->SetUnits(Size_field, kADMNoUnits);
	suites.ADMItemSuite()->SetUnits(Quality_field, kADMNoUnits);
	
	//suites.ADMItemSuite()->ShowUnits(Size_field, FALSE);
	//suites.ADMItemSuite()->ShowUnits(Quality_field, FALSE);
	
	suites.ADMItemSuite()->SetJustify(Size_field, kADMRightJustify);
	suites.ADMItemSuite()->SetJustify(Quality_field, kADMRightJustify);
	
	
	suites.ADMItemSuite()->SetNotifyProc(Size_field, SizeFieldNotifyProc);
	suites.ADMItemSuite()->SetNotifyProc(Quality_field, QualityFieldNotifyProc);


	// make the slider
	ASRect Quality_slider_rect = INIT_RECT(QUALITY_SLIDER_TOP, QUALITY_SLIDER_LEFT, QUALITY_SLIDER_BOTTOM, QUALITY_SLIDER_RIGHT);

	ADMItemRef Quality_slider = suites.ADMItemSuite()->Create(dialog, OUT_Quality_Slider,
								kADMSliderType, &Quality_slider_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetMinIntValue(Quality_slider, 1);
	suites.ADMItemSuite()->SetMaxIntValue(Quality_slider, 100);

	suites.ADMItemSuite()->SetIntValue(Quality_slider, g_quality);

	// we need to track the slider
	suites.ADMItemSuite()->SetTrackProc(Quality_slider, QualitySliderTrackProc);


	// make the slider labels
	ASRect Quality_slider_lo_rect = INIT_RECT(SLIDER_LOW_TEXT_TOP, SLIDER_LOW_TEXT_LEFT, SLIDER_LOW_TEXT_BOTTOM, SLIDER_LOW_TEXT_RIGHT);
	ASRect Quality_slider_hi_rect = INIT_RECT(SLIDER_HI_TEXT_TOP, SLIDER_HI_TEXT_LEFT, SLIDER_HI_TEXT_BOTTOM, SLIDER_HI_TEXT_RIGHT);
	
	ADMItemRef Quality_slider_lo = suites.ADMItemSuite()->Create(dialog, OUT_Quality_Slider_Low_Text,
								kADMTextStaticType, &Quality_slider_lo_rect, NULL, NULL, NULL);

	ADMItemRef Quality_slider_hi = suites.ADMItemSuite()->Create(dialog, OUT_Quality_Slider_Hi_Text,
								kADMTextStaticType, &Quality_slider_hi_rect, NULL, NULL, NULL);
					
	suites.ADMItemSuite()->SetText(Quality_slider_lo, SLIDER_LOW_TEXT_TEXT);
	suites.ADMItemSuite()->SetJustify(Quality_slider_lo, kADMLeftJustify);
	suites.ADMItemSuite()->SetFont(Quality_slider_lo, kADMPaletteFont);
	suites.ADMItemSuite()->Enable(Quality_slider_lo, FALSE);

	suites.ADMItemSuite()->SetText(Quality_slider_hi, SLIDER_HI_TEXT_TEXT);
	suites.ADMItemSuite()->SetJustify(Quality_slider_hi, kADMRightJustify);
	suites.ADMItemSuite()->SetFont(Quality_slider_hi, kADMPaletteFont);
	suites.ADMItemSuite()->Enable(Quality_slider_hi, FALSE);

	
	// make the k for kilobytes
	ASRect Size_label_rect = INIT_RECT(SIZE_FIELD_LABEL_TOP, SIZE_FIELD_LABEL_LEFT, SIZE_FIELD_LABEL_BOTTOM, SIZE_FIELD_LABEL_RIGHT);
	
	ADMItemRef Size_label = suites.ADMItemSuite()->Create(dialog, OUT_Size_Field_Label,
								kADMTextStaticType, &Size_label_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Size_label, SIZE_FIELD_LABEL_TEXT);


#ifndef macintosh
	// create border
	ASRect Border_rect = INIT_RECT(BORDER_TOP, BORDER_LEFT, BORDER_BOTTOM, BORDER_RIGHT);

	ADMItemRef Border = suites.ADMItemSuite()->Create(dialog, OUT_Border,
								kADMFrameType, &Border_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetItemStyle(Border, kADMEtchedFrameStyle);
#endif

	
	// create the advanced checkbox
	ASRect Advanced_rect = INIT_RECT(ADVANCED_CHECK_TOP, ADVANCED_CHECK_LEFT, ADVANCED_CHECK_BOTTOM, ADVANCED_CHECK_RIGHT);
	
	ADMItemRef Advanced_check = suites.ADMItemSuite()->Create(dialog, OUT_Advanced,
								kADMTextCheckBoxType, &Advanced_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetBooleanValue(Advanced_check, g_advanced);
	
	suites.ADMItemSuite()->SetText(Advanced_check, ADVANCED_CHECK_TEXT);
	
	suites.ADMItemSuite()->SetFont(Advanced_check, kADMPaletteFont);

	suites.ADMItemSuite()->SetNotifyProc(Advanced_check, AdvancedNotifyProc);


	return err;
}

static ASErr ASAPI AdvancedInit(ADMDialogRef dialog)
{
	ASErr err = kSPNoError;

	AEGP_SuiteHandler	suites(sP);


	// create the format menu label
	ASRect Format_label_rect = INIT_RECT(FORMAT_LABEL_TOP, FORMAT_LABEL_LEFT, FORMAT_LABEL_BOTTOM, FORMAT_LABEL_RIGHT);
	
	ADMItemRef Format_label = suites.ADMItemSuite()->Create(dialog, OUT_Format_Label,
								kADMTextStaticType, &Format_label_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Format_label, FORMAT_LABEL_TEXT);
	//suites.ADMItemSuite()->SetJustify(Format_label, kADMRightJustify);

	
	
	// create the format menu
	ASRect Format_menu_rect = INIT_RECT(FORMAT_MENU_TOP, FORMAT_MENU_LEFT, FORMAT_MENU_BOTTOM, FORMAT_MENU_RIGHT);
	
	ADMItemRef Compression_menu = suites.ADMItemSuite()->Create(dialog, OUT_Format,
								kADMPopupListType, &Format_menu_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Compression_menu, "blah");

	ADMListRef Format_list = suites.ADMItemSuite()->GetList(Compression_menu);


	// add the menu items
	if(Format_list)
	{
		ADMEntryRef entry;
	
#define ADD_FORMAT_MENU_ITEM( NAME, CODE ) \
		entry =  suites.ADMListSuite()->InsertEntry(Format_list, -1); \
		suites.ADMEntrySuite()->SetText(entry, NAME); \
		suites.ADMEntrySuite()->SetUserData(entry, (ADMUserData)CODE); \
		if(g_format == CODE) suites.ADMEntrySuite()->Select(entry, true);


		ADD_FORMAT_MENU_ITEM( "j2c", JP2_TYPE_J2C);
		ADD_FORMAT_MENU_ITEM( "JP2", JP2_TYPE_JP2);
		ADD_FORMAT_MENU_ITEM( "JPX", JP2_TYPE_JPX);
	}

	
	// custom depth checkbox
	ASRect Custom_depth_rect = INIT_RECT(DEPTH_CUSTOM_TOP, DEPTH_CUSTOM_LEFT, DEPTH_CUSTOM_BOTTOM, DEPTH_CUSTOM_RIGHT);
	
	ADMItemRef Custom_depth_check = suites.ADMItemSuite()->Create(dialog, OUT_Custom_Depth,
								kADMTextCheckBoxType, &Custom_depth_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetBooleanValue(Custom_depth_check, g_custom_depth);
	
	suites.ADMItemSuite()->SetText(Custom_depth_check, DEPTH_CUSTOM_TEXT);

	suites.ADMItemSuite()->SetNotifyProc(Custom_depth_check, CustomDepthNotifyProc);
	
	
	// depth field
	ASRect Depth_field_rect = INIT_RECT(DEPTH_FIELD_TOP, DEPTH_FIELD_LEFT, DEPTH_FIELD_BOTTOM, DEPTH_FIELD_RIGHT);

	ADMItemRef Depth_field = suites.ADMItemSuite()->Create(dialog, OUT_Depth,
								kADMSpinEditType, &Depth_field_rect, NULL, NULL, NULL);


	suites.ADMItemSuite()->SetIntValue(Depth_field, g_depth);

	suites.ADMItemSuite()->SetMinIntValue(Depth_field, 1);
	suites.ADMItemSuite()->SetMaxIntValue(Depth_field, 32);
	
	suites.ADMItemSuite()->SetUnits(Depth_field, kADMNoUnits);
	suites.ADMItemSuite()->SetJustify(Depth_field, kADMRightJustify);
	
	suites.ADMItemSuite()->SetNotifyProc(Depth_field, DepthFieldNotifyProc);
	
	if(!g_custom_depth) // only enabled when custom depth is checked
		suites.ADMItemSuite()->Enable(Depth_field, FALSE);


	// reversible checkbox
	// now means the opposite: encode float
	ASRect Reversible_rect = INIT_RECT(REVERSIBLE_CHECK_TOP, REVERSIBLE_CHECK_LEFT, REVERSIBLE_CHECK_BOTTOM, REVERSIBLE_CHECK_RIGHT);
	
	ADMItemRef Reversible_check = suites.ADMItemSuite()->Create(dialog, OUT_Reversible,
								kADMTextCheckBoxType, &Reversible_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetBooleanValue(Reversible_check, !g_reversible);
	
	suites.ADMItemSuite()->SetText(Reversible_check, REVERSIBLE_CHECK_TEXT);

	suites.ADMItemSuite()->SetNotifyProc(Reversible_check, ReversibleNotifyProc);


	// create the YCC checkbox
	ASRect YCC_rect = INIT_RECT(YCC_CHECK_TOP, YCC_CHECK_LEFT, YCC_CHECK_BOTTOM, YCC_CHECK_RIGHT);
	
	ADMItemRef YCC_check = suites.ADMItemSuite()->Create(dialog, OUT_Ycc,
								kADMTextCheckBoxType, &YCC_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetBooleanValue(YCC_check, g_ycc);
	
	suites.ADMItemSuite()->SetText(YCC_check, YCC_CHECK_TEXT);

	suites.ADMItemSuite()->SetNotifyProc(YCC_check, YccNotifyProc);
	
	
	// subsampling menu label
/*
	ASRect Sub_label_rect = INIT_RECT(SUBSAMPLING_LABEL_TOP, SUBSAMPLING_LABEL_LEFT, SUBSAMPLING_LABEL_BOTTOM, SUBSAMPLING_LABEL_RIGHT);
	
	ADMItemRef Sub_label = suites.ADMItemSuite()->Create(dialog, OUT_Subsampling_Label,
								kADMTextStaticType, &Sub_label_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Sub_label, SUBSAMPLING_LABEL_TEXT);
	
	
	
	// subsampling menu

	ASRect Sub_menu_rect = INIT_RECT(SUBSAMPLING_MENU_TOP, SUBSAMPLING_MENU_LEFT, SUBSAMPLING_MENU_BOTTOM, SUBSAMPLING_MENU_RIGHT);
	
	ADMItemRef Sub_menu = suites.ADMItemSuite()->Create(dialog, OUT_Subsampling,
								kADMPopupListType, &Sub_menu_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Sub_menu, "blah");

	ADMListRef Sub_list = suites.ADMItemSuite()->GetList(Sub_menu);

	// add the subsampling menu items
	if(Sub_list)
	{
		ADMEntryRef entry;
	
#define ADD_SUBSAMPLING_MENU_ITEM( NAME, VAL ) \
		entry =  suites.ADMListSuite()->InsertEntry(Sub_list, -1); \
		suites.ADMEntrySuite()->SetText(entry, NAME); \
		suites.ADMEntrySuite()->SetUserData(entry, (ADMUserData)VAL); \
		if(g_sub == VAL) suites.ADMEntrySuite()->Select(entry, true);

		ADD_SUBSAMPLING_MENU_ITEM( "4:2:2"	, JP2_SUBSAMPLE_422 );
		ADD_SUBSAMPLING_MENU_ITEM( "4:1:1"	, JP2_SUBSAMPLE_411 );
		ADD_SUBSAMPLING_MENU_ITEM( "4:2:0"	, JP2_SUBSAMPLE_420 );
		ADD_SUBSAMPLING_MENU_ITEM( "3:1:1"	, JP2_SUBSAMPLE_311 );
		ADD_SUBSAMPLING_MENU_ITEM( "-"		, -1 ); // divider!
		ADD_SUBSAMPLING_MENU_ITEM( "2x2"	, JP2_SUBSAMPLE_2x2 );
		ADD_SUBSAMPLING_MENU_ITEM( "3x3"	, JP2_SUBSAMPLE_3x3 );
		ADD_SUBSAMPLING_MENU_ITEM( "4x4"	, JP2_SUBSAMPLE_4x4 );
		ADD_SUBSAMPLING_MENU_ITEM( "None"	, JP2_SUBSAMPLE_NONE );
	}

	// disable menu if YCC not checked
	if(!g_ycc)
		suites.ADMItemSuite()->Enable(Sub_menu, FALSE);
*/

/* -- not anymore	
	// layers label
	ASRect Layers_label_rect = INIT_RECT(LAYERS_LABEL_TOP, LAYERS_LABEL_LEFT, LAYERS_LABEL_BOTTOM, LAYERS_LABEL_RIGHT);
	
	ADMItemRef Layers_label = suites.ADMItemSuite()->Create(dialog, OUT_Layers_Label,
								kADMTextStaticType, &Layers_label_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Layers_label, LAYERS_LABEL_TEXT);
	
	suites.ADMItemSuite()->SetFont(Layers_label, kADMPaletteFont);
	
	// layers field
	ASRect Layers_field_rect = INIT_RECT(LAYERS_FIELD_TOP, LAYERS_FIELD_LEFT, LAYERS_FIELD_BOTTOM, LAYERS_FIELD_RIGHT);

	ADMItemRef Layers_field = suites.ADMItemSuite()->Create(dialog, OUT_Layers,
								kADMSpinEditType, &Layers_field_rect, NULL, NULL, NULL);


	suites.ADMItemSuite()->SetIntValue(Layers_field, g_layers);

	suites.ADMItemSuite()->SetMinIntValue(Layers_field, 1);
	suites.ADMItemSuite()->SetMaxIntValue(Layers_field, 50);
	
	suites.ADMItemSuite()->SetUnits(Layers_field, kADMNoUnits);
	suites.ADMItemSuite()->SetJustify(Layers_field, kADMRightJustify);
	
	suites.ADMItemSuite()->SetNotifyProc(Layers_field, LayersFieldNotifyProc);
	
	suites.ADMItemSuite()->SetFont(Layers_field, kADMPaletteFont);
*/	

	// tiles menu label
	ASRect Tiles_label_rect = INIT_RECT(TILES_LABEL_TOP, TILES_LABEL_LEFT, TILES_LABEL_BOTTOM, TILES_LABEL_RIGHT);
	
	ADMItemRef Tiles_label = suites.ADMItemSuite()->Create(dialog, OUT_Tiles_Label,
								kADMTextStaticType, &Tiles_label_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Tiles_label, TILES_LABEL_TEXT);
	
	suites.ADMItemSuite()->SetFont(Tiles_label, kADMPaletteFont);
	
	
	// tiles menu
	ASRect Tiles_menu_rect = INIT_RECT(TILES_MENU_TOP, TILES_MENU_LEFT, TILES_MENU_BOTTOM, TILES_MENU_RIGHT);
	
	ADMItemRef Tiles_menu = suites.ADMItemSuite()->Create(dialog, OUT_Tiles,
								kADMPopupListType, &Tiles_menu_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Tiles_menu, "blah");

	suites.ADMItemSuite()->SetFont(Tiles_menu, kADMPaletteFont);

	ADMListRef Tiles_list = suites.ADMItemSuite()->GetList(Tiles_menu);
	

	// add the tile menu items
	if(Tiles_list)
	{
		ADMEntryRef entry;
	
#define ADD_TILE_MENU_ITEM( NAME, VAL ) \
		entry =  suites.ADMListSuite()->InsertEntry(Tiles_list, -1); \
		suites.ADMEntrySuite()->SetText(entry, NAME); \
		suites.ADMEntrySuite()->SetUserData(entry, (ADMUserData)VAL); \
		if(g_tile_size == VAL) suites.ADMEntrySuite()->Select(entry, true);


		ADD_TILE_MENU_ITEM( "2048",		2048);
		ADD_TILE_MENU_ITEM( "1024", 	1024);
		ADD_TILE_MENU_ITEM( "512", 		512);
		ADD_TILE_MENU_ITEM( "256", 		256);
		ADD_TILE_MENU_ITEM( "128", 		128);
		ADD_TILE_MENU_ITEM( "64", 		64);
		ADD_TILE_MENU_ITEM( "No Tiles",	0);
	}


	// Order menu label
	ASRect Order_label_rect = INIT_RECT(ORDER_LABEL_TOP, ORDER_LABEL_LEFT, ORDER_LABEL_BOTTOM, ORDER_LABEL_RIGHT);
	
	ADMItemRef Order_label = suites.ADMItemSuite()->Create(dialog, OUT_Order_Label,
								kADMTextStaticType, &Order_label_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Order_label, ORDER_LABEL_TEXT);

	suites.ADMItemSuite()->SetFont(Order_label, kADMPaletteFont);	

	
	// Order menu
	ASRect Order_menu_rect = INIT_RECT(ORDER_MENU_TOP, ORDER_MENU_LEFT, ORDER_MENU_BOTTOM, ORDER_MENU_RIGHT);
	
	ADMItemRef Order_menu = suites.ADMItemSuite()->Create(dialog, OUT_Order,
								kADMPopupListType, &Order_menu_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Order_menu, "blah");

	ADMListRef Order_list = suites.ADMItemSuite()->GetList(Order_menu);
	
	suites.ADMItemSuite()->SetFont(Order_menu, kADMPaletteFont);


	// add the menu items
	if(Order_list)
	{
		ADMEntryRef entry;
	
#define ADD_ORDER_MENU_ITEM( NAME, CODE ) \
		entry =  suites.ADMListSuite()->InsertEntry(Order_list, -1); \
		suites.ADMEntrySuite()->SetText(entry, NAME); \
		suites.ADMEntrySuite()->SetUserData(entry, (ADMUserData)CODE); \
		if(g_order == CODE) suites.ADMEntrySuite()->Select(entry, true);

		// L=layer; R=resolution; C=component; P=position
		ADD_ORDER_MENU_ITEM( "Layer", JP2_ORDER_LRCP);
		ADD_ORDER_MENU_ITEM( "Resolution, Layer", JP2_ORDER_RLCP);
		ADD_ORDER_MENU_ITEM( "Resolution, Position", JP2_ORDER_RPCL);
		ADD_ORDER_MENU_ITEM( "Position", JP2_ORDER_PCRL);
		ADD_ORDER_MENU_ITEM( "Component", JP2_ORDER_CPRL);
	}


	// color menu label
	ASRect Color_label_rect = INIT_RECT(COLOR_LABEL_TOP, COLOR_LABEL_LEFT, COLOR_LABEL_BOTTOM, COLOR_LABEL_RIGHT);
	
	ADMItemRef Color_label = suites.ADMItemSuite()->Create(dialog, OUT_Color_Space_Label,
								kADMTextStaticType, &Color_label_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Color_label, COLOR_LABEL_TEXT);
	
	suites.ADMItemSuite()->SetFont(Color_label, kADMPaletteFont);
	
	
	// color menu
	ASRect Color_menu_rect = INIT_RECT(COLOR_MENU_TOP, COLOR_MENU_LEFT, COLOR_MENU_BOTTOM, COLOR_MENU_RIGHT);
	
	ADMItemRef Color_menu = suites.ADMItemSuite()->Create(dialog, OUT_Color_Space,
								kADMPopupListType, &Color_menu_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Color_menu, "blah");

	suites.ADMItemSuite()->SetFont(Color_menu, kADMPaletteFont);

	ADMListRef Color_list = suites.ADMItemSuite()->GetList(Color_menu);


	// add the menu items
	if(Color_list)
	{
		ADMEntryRef entry;
	
#define ADD_COLOR_MENU_ITEM( NAME, CODE ) \
		entry =  suites.ADMListSuite()->InsertEntry(Color_list, -1); \
		suites.ADMEntrySuite()->SetText(entry, NAME); \
		suites.ADMEntrySuite()->SetUserData(entry, (ADMUserData)CODE); \
		if(g_color_space == CODE) suites.ADMEntrySuite()->Select(entry, true);

#define ADD_SEPARATOR() \
		ADD_COLOR_MENU_ITEM( "-", 0 ); \
		suites.ADMEntrySuite()->MakeSeparator(entry, true);
		

		ADD_COLOR_MENU_ITEM( "sRGB", JP2_COLOR_sRGB );	g_sRGB_entry = entry;
		//ADD_COLOR_MENU_ITEM( "sYCC", JP2_COLOR_sYCC );	g_sYCC_entry = entry;
		
		// add display profile (and disable it until we've got one)
		entry =  suites.ADMListSuite()->InsertEntry(Color_list, -1);
		g_display_entry = entry; // store this
		suites.ADMEntrySuite()->SetUserData(g_display_entry, (ADMUserData)JP2_COLOR_sRGB);
		suites.ADMEntrySuite()->SetText(g_display_entry, "");
		suites.ADMEntrySuite()->Enable(g_display_entry, false);
		
		// divider
		ADD_SEPARATOR();
		
		// add current profile (if there is one)
		if(g_color_space == JP2_COLOR_ICC)
		{
			ICCProfile *icc = (ICCProfile *)g_profile;
		
			ADD_COLOR_MENU_ITEM( GetProfileDescription(icc), JP2_COLOR_ICC );
			ADD_SEPARATOR();
		}
		
		// Add the OS profiles
		FillInProfiles(Color_list);
#ifdef macintosh
		ADD_SEPARATOR();
#endif

		// Add a file option
		ADD_COLOR_MENU_ITEM( "File...", FILE_DIALOG );
		
		
		// remember what was selected
		g_last_color_entry = suites.ADMListSuite()->GetActiveEntry(Color_list);
	}

	suites.ADMItemSuite()->SetNotifyProc(Color_menu, ColorNotifyProc);


	return err;
}


static ASErr ASAPI DialogInit(ADMDialogRef dialog)
{
	ASErr err = kSPNoError;

	AEGP_SuiteHandler	suites(sP);
	
	
	int d_width, d_height;
	
	if(g_advanced)
	{
		d_width = ADVANCED_DIALOG_WIDTH;
		d_height = ADVANCED_DIALOG_HEIGHT;
	}
	else
	{
		d_width = DIALOG_WIDTH;
		d_height = DIALOG_HEIGHT;
	}
	
	
	// do the moving targets - OK and Cancel buttons
	
	// set the dialog's size and name
	suites.ADMDialogSuite()->Size(dialog, d_width, d_height);
	suites.ADMDialogSuite()->SetText(dialog, DIALOG_TITLE);
	
	
	// create the OK button
	ASRect OK_rect = INIT_RECT(OK_TOP(d_height), OK_LEFT(d_width), OK_BOTTOM(d_height), OK_RIGHT(d_width) );
	
	ADMItemRef OK_button = suites.ADMItemSuite()->Create(dialog, OUT_OK,
								kADMTextPushButtonType, &OK_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(OK_button, OK_TEXT);
	
	suites.ADMItemSuite()->SetNotifyProc(OK_button, OKNotifyProc);


	// create the Cancel button
	ASRect Cancel_rect = INIT_RECT(CANCEL_TOP(d_height), CANCEL_LEFT(d_width), CANCEL_BOTTOM(d_height), CANCEL_RIGHT(d_width) );
	
	ADMItemRef Cancel_button = suites.ADMItemSuite()->Create(dialog, OUT_Cancel,
								kADMTextPushButtonType, &Cancel_rect, NULL, NULL, NULL);
	
	suites.ADMItemSuite()->SetText(Cancel_button, CANCEL_TEXT);
	
	
	// now to the rest
	
	err = StandardInit(dialog);
	
	
	// are we advanced?
	//if(g_advanced) // always make all items, just resize dialog
		err = AdvancedInit(dialog);
	
	return err;
}

static A_u_char GammaCorrect(A_u_char input)
{
#ifdef macintosh

	return input; // banner authored on a Mac, we must corrrect for Win

#else

#define MAX(A,B)			( (A) > (B) ? (A) : (B))
#define AE8_RANGE(NUM)		(A_u_short)MIN( MAX( (NUM), 0 ), PF_MAX_CHAN8 )

#define AE8_TO_FLOAT(NUM)		( (float)(NUM) / (float)PF_MAX_CHAN8 )
#define FLOAT_TO_AE8(NUM)		AE8_RANGE( ( (NUM) * (float)PF_MAX_CHAN8 ) + 0.5)

#define CORRECT_GAMMA (1.8 / 2.2)
#define GAMMA_CORRECT(NUM)	FLOAT_TO_AE8( pow( AE8_TO_FLOAT( NUM ), CORRECT_GAMMA ) )

	return GAMMA_CORRECT(input);
	
#endif
}


static void CreateImage(void)
{
	AEGP_SuiteHandler	suites(sP);
	
	char pixel_buf[] =
#include "j2k_banner.h"
	
	g_image = suites.ADMImageSuite()->Create(PICTURE_WIDTH, PICTURE_HEIGHT);
	ASBytePtr image_data =  suites.ADMImageSuite()->BeginBaseAddressAccess(g_image);
	ASInt32 rowbytes = suites.ADMImageSuite()->GetByteWidth(g_image);
	
	A_u_char *row = image_data;
	A_u_char *buf = (A_u_char *)pixel_buf;
	
	PF_Pixel temp;
	
	for(int y=0; y<PICTURE_HEIGHT; y++)
	{
		A_u_char *pix = row;
		
		for(int x=0; x<PICTURE_WIDTH; x++)
		{
			temp.alpha = *buf++; // alpha (ignored)
			temp.red   = GammaCorrect( *buf++ ); // red
			temp.green = GammaCorrect( *buf++ ); // green
			temp.blue  = GammaCorrect( *buf++ ); // blue

		#ifdef macintosh
			*pix++ = temp.alpha;
			*pix++ = temp.red;
			*pix++ = temp.green;
			*pix++ = temp.blue;
		#else
			*pix++ = temp.blue;
			*pix++ = temp.green;
			*pix++ = temp.red;
			*pix++ = temp.alpha;
		#endif
		}
		
		row += rowbytes;
	}
	
	suites.ADMImageSuite()->EndBaseAddressAccess(g_image);
}

A_Err	
j2k_OutDialog(
	AEIO_BasicData		*basic_dataP,
	AEIO_Handle			optionsH,
	A_Boolean			*user_interactedPB0)
{
	A_Err			err 		= A_Err_NONE;
	

	// set the basic suite
	sP = (SPBasicSuite *)basic_dataP->pica_basicP;

	AEGP_SuiteHandler	suites(sP);

	j2k_outData *options  = NULL;


	// lock the options data
	if(optionsH)
		err = suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
	else
		return A_Err_GENERIC;
	
	
	g_method = options->method;
	g_quality = options->quality;
	g_size = options->size;
	g_advanced = options->advanced;
	g_format = options->format;
	g_custom_depth = options->custom_depth;
	g_depth = options->bit_depth;
	g_reversible = options->reversible;
	g_ycc = options->ycc;
	g_sub = options->sub;
	g_layers = options->layers;
	g_order = options->order;
	g_tile_size = options->tile_size;
	g_color_space = options->color_space;
	g_profile = (void *)options->profile;
	

	// create banner image
	CreateImage();
	
	
	ASInt32 item = suites.ADMDialogSuite()->Modal(NULL, DIALOG_TITLE, NULL,
							kADMModalDialogStyle, DialogInit, NULL, NULL);
	

	// kill image
	if(g_image)
	{
		suites.ADMImageSuite()->Destroy(g_image);
		g_image = NULL;
	}

	if(item == OUT_OK)
	{
		options->method = g_method;
		options->quality = g_quality;
		options->size = g_size;
		options->advanced = g_advanced;
		options->format = g_format;
		options->custom_depth = g_custom_depth;
		options->bit_depth = g_depth;
		options->reversible = g_reversible;
		options->ycc = g_ycc;
		options->sub = g_sub;
		options->layers = g_layers;
		options->order = g_order;
		options->tile_size = g_tile_size;
		
		// copy color profile
		if( IS_OS_PROFILE( g_color_space ) )
		{
			// we have to embed the profile in our options
			long prof_size = GetProfileSize( g_color_space );

			// expand handle to hold embedded profile
			suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
			
			suites.MemorySuite()->AEGP_ResizeMemHandle("Embed Profile", sizeof(j2k_outData) + prof_size, optionsH);
			
			suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
			
			CopyProfile( g_color_space, options->profile );
			
			options->color_space = JP2_COLOR_ICC;
		}
		else if(FILE_PROFILE == g_color_space) // file handle
		{
			ICCProfile *icc;
			
			assert(g_file_profileH != NULL);
			
			suites.MemorySuite()->AEGP_LockMemHandle(g_file_profileH, (void**)&icc);
			
			uint32_t prof_size = SwapEndianMisaligned(icc->head.dwProfileSize);

			// expand handle to hold file profile
			suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
			
			suites.MemorySuite()->AEGP_ResizeMemHandle("Embed Profile", sizeof(j2k_outData) + prof_size, optionsH);
			
			suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
			
			// copy profile to buffer
			memcpy((char*)&options->profile, (char*)icc, prof_size);
			
			options->color_space = JP2_COLOR_ICC;
			
			// done with handle
			suites.MemorySuite()->AEGP_UnlockMemHandle(g_file_profileH);
		}
		else
		{
			if(options->color_space == JP2_COLOR_ICC && g_color_space != JP2_COLOR_ICC) // we had an embedded proile before but not now
			{
				// shrink the profile to original size
				suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
				
				suites.MemorySuite()->AEGP_ResizeMemHandle("Remove Profile", sizeof(j2k_outData), optionsH );
				
				suites.MemorySuite()->AEGP_LockMemHandle(optionsH, (void**)&options);
			}
			
			options->color_space = g_color_space;
		}
				
		*user_interactedPB0 = TRUE;
	}
	else
		*user_interactedPB0 = FALSE;
		
	
	// should always free this handle
	if(g_file_profileH)
	{
		suites.MemorySuite()->AEGP_FreeMemHandle(g_file_profileH);
		g_file_profileH = NULL;
	}	
	
	
	// unlock the handle	
	suites.MemorySuite()->AEGP_UnlockMemHandle(optionsH);
	
		
	return err;
}
