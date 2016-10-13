//
// j2k
//
// Compression settings dialog
//

#include "j2k_OutUI.h"

#include "j2k_version.h"

#include <Windows.h>
#include <commctrl.h>

#ifndef MIN
#define MIN(A, B)	( (A) < (B) ? (A) : (B) )
#endif

#ifndef MAX
#define MAX(A, B)	( (A) > (B) ? (A) : (B) )
#endif

enum {
	OUT_noUI = -1,
	OUT_OK = IDOK,
	OUT_Cancel = IDCANCEL,
	OUT_Lossless = 3,
	OUT_Cinema,
	OUT_Size,
	OUT_Quality,
	OUT_Size_Field,
	OUT_Quality_Field,
	OUT_Quality_Slider,
	OUT_Advanced,
	OUT_Format,
	OUT_Format_Label,
	OUT_Custom_Depth,
	OUT_Depth,
	OUT_Depth_Spinner,
	OUT_Ycc,
	OUT_Float,
	OUT_Tiles,
	OUT_Tiles_Label,
	OUT_Order,
	OUT_Order_Label,
	OUT_Profile,
	OUT_Profile_Label,
	OUT_DCI_Profile,
	OUT_DCI_Profile_Label,
	OUT_Subsample,
	OUT_Subsample_Label,
	OUT_DCI_Data_Rate_Slider,
	OUT_DCI_Data_Rate,
	OUT_DCI_Data_Rate_Label,
	OUT_DCI_Per_Frame,
	OUT_DCI_Frame_Rate,
	OUT_DCI_Frame_Rate_Label,
	OUT_DCI_Stereo
};

static const char			*g_generic_profile = NULL;
static const char			*g_color_profile = NULL;
static bool					g_show_subsample = false;

static DialogMethod			g_method;
static long					g_quality;
static int					g_size;
static bool					g_advanced;
static DialogFormat			g_format;
static bool					g_custom_depth;
static int					g_depth;
static bool					g_reversible;
static bool					g_ycc;
static DialogSubsample		g_sub;
static DialogOrder			g_order;
static int					g_tile_size;
static DialogProfile		g_profile;
static DialogDCIProfile		g_dci_profile;
static int					g_dci_data_rate;
static DialogDCIPerFrame	g_dci_per_frame;
static int					g_dci_frame_rate;
static bool					g_dci_stereo;



static WORD	g_item_clicked = 0;


// sensible Win macros
#define GET_ITEM(ITEM)	GetDlgItem(hwndDlg, (ITEM))

#define SET_CHECK(ITEM, VAL)	SendMessage(GET_ITEM(ITEM), BM_SETCHECK, (WPARAM)(VAL), (LPARAM)0)
#define GET_CHECK(ITEM)			SendMessage(GET_ITEM(ITEM), BM_GETCHECK, (WPARAM)0, (LPARAM)0)

#define SET_FIELD(ITEM, VAL)	SetDlgItemInt(hwndDlg, (ITEM), (VAL), FALSE)
#define GET_FIELD(ITEM)			GetDlgItemInt(hwndDlg, (ITEM), NULL, FALSE)

#define SET_SLIDER(ITEM, VAL)	SendMessage(GET_ITEM(ITEM),(UINT)TBM_SETPOS, (WPARAM)(BOOL)TRUE, (LPARAM)(VAL));
#define GET_SLIDER(ITEM)		SendMessage(GET_ITEM(ITEM), TBM_GETPOS, (WPARAM)0, (LPARAM)0 )

#define ADD_MENU_ITEM(MENU, INDEX, STRING, VALUE, SELECTED) \
				SendMessage(GET_ITEM(MENU),( UINT)CB_ADDSTRING, (WPARAM)wParam, (LPARAM)(LPCTSTR)STRING ); \
				SendMessage(GET_ITEM(MENU),(UINT)CB_SETITEMDATA, (WPARAM)INDEX, (LPARAM)(DWORD)VALUE); \
				if(SELECTED) \
					SendMessage(GET_ITEM(MENU), CB_SETCURSEL, (WPARAM)INDEX, (LPARAM)0);

#define GET_MENU_VALUE(MENU)		SendMessage(GET_ITEM(MENU), (UINT)CB_GETITEMDATA, (WPARAM)SendMessage(GET_ITEM(MENU),(UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0), (LPARAM)0)

#define ENABLE_ITEM(ITEM, ENABLE)	EnableWindow(GetDlgItem(hwndDlg, (ITEM)), (ENABLE));

#define SHOW_ITEM(ITEM, SHOW)				ShowWindow(GetDlgItem(hwndDlg, (ITEM)), (SHOW) ? SW_SHOW : SW_HIDE)


static void ResizeWindow(HWND hwndDlg, BOOL shrink)
{
	WINDOWPLACEMENT winPlace, okPlace, cancelPlace;
	winPlace.length = okPlace.flags = cancelPlace.length = sizeof(WINDOWPLACEMENT);

	GetWindowPlacement(hwndDlg, &winPlace);
	GetWindowPlacement(GET_ITEM(OUT_OK), &okPlace);
	GetWindowPlacement(GET_ITEM(OUT_Cancel), &cancelPlace);


	int resize = 300;

	if(shrink)
		resize *= -1;


	winPlace.rcNormalPosition.right += resize;
	okPlace.rcNormalPosition.left += resize;
	okPlace.rcNormalPosition.right += resize;
	cancelPlace.rcNormalPosition.left += resize;
	cancelPlace.rcNormalPosition.right += resize;


	SetWindowPlacement(GET_ITEM(OUT_Cancel), &cancelPlace);
	SetWindowPlacement(GET_ITEM(OUT_OK), &okPlace);
	SetWindowPlacement(hwndDlg, &winPlace);
}

#define DCI_MAX		(1302083 / 1024)

static int g_dci_slider_val = 50;

static void SetDCIslider(HWND hwndDlg)
{
	DialogDCIPerFrame per_frame = (DialogDCIPerFrame)GET_MENU_VALUE(OUT_DCI_Per_Frame);

	int current_value = GET_FIELD(OUT_DCI_Data_Rate);

	int val = 50; // 1-100 scale

	if(per_frame == DIALOG_DCI_PER_FRAME)
	{
		val = (((double)current_value * 100.0) / (double)DCI_MAX) + 0.5;
	}
	else
	{
		val = ((double)(current_value * 100 * 1024 / 8) / (double)(24 * DCI_MAX)) + 0.5;
	}

	val = MAX(1, MIN(val, 100));

	g_dci_slider_val = val;

	SET_SLIDER(OUT_DCI_Data_Rate_Slider, val);
}


static void TrackDCIslider(HWND hwndDlg)
{
	DialogDCIPerFrame per_frame = (DialogDCIPerFrame)GET_MENU_VALUE(OUT_DCI_Per_Frame);

	int val = GET_SLIDER(OUT_DCI_Data_Rate_Slider); // 1-100 scale

	if(val != g_dci_slider_val)
	{
		int new_value = DCI_MAX * 8 / 1024;

		if(per_frame == DIALOG_DCI_PER_FRAME)
		{
			new_value = ((double)DCI_MAX * (double)val / 100.0) + 0.5;
		}
		else
		{
			int frame_rate = GET_MENU_VALUE(OUT_DCI_Frame_Rate);
			int stereo_mult = ( GET_CHECK(OUT_DCI_Stereo) ? 2 : 1 );

			new_value = ((double)(24 * DCI_MAX * 8 / 1024) * (double)val / 100.0) + 0.5 ;
		}

		SET_FIELD(OUT_DCI_Data_Rate, new_value);
	}
}

static void RecalcDataRate(HWND hwndDlg)
{
	DialogDCIPerFrame per_frame = (DialogDCIPerFrame)GET_MENU_VALUE(OUT_DCI_Per_Frame);

	int current_value = GET_FIELD(OUT_DCI_Data_Rate);

	int val = current_value;

	if(per_frame != g_dci_per_frame)
	{
		int frame_rate = (DialogDCIPerFrame)GET_MENU_VALUE(OUT_DCI_Frame_Rate);

		int stereo_mult = (GET_CHECK(OUT_DCI_Stereo) ? 2 : 1);

		if(per_frame == DIALOG_DCI_PER_SECOND)
		{
			val = (((double)current_value * 8.0 / 1024.0) * (double)frame_rate * (double)stereo_mult) + 0.5;
		}
		else
		{
			val = ((double)(current_value * 1024 / 8) / (double)(frame_rate * stereo_mult)) + 0.5;
		}

		g_dci_per_frame = per_frame;
	}

	if(val != current_value)
		SET_FIELD(OUT_DCI_Data_Rate, val);

	SetDCIslider(hwndDlg);
}

static void TrackDCIPerFrame(HWND hwndDlg)
{
	BOOL per_sec = (DIALOG_DCI_PER_SECOND == (DialogDCIPerFrame)GET_MENU_VALUE(OUT_DCI_Per_Frame));

	ENABLE_ITEM(OUT_DCI_Frame_Rate, per_sec);
	ENABLE_ITEM(OUT_DCI_Frame_Rate_Label, per_sec);
	ENABLE_ITEM(OUT_DCI_Stereo, per_sec);
}

static void TrackMethod(HWND hwndDlg)
{
	ENABLE_ITEM(OUT_Size_Field, GET_CHECK(OUT_Size));

	ENABLE_ITEM(OUT_Quality_Field, GET_CHECK(OUT_Quality));
	ENABLE_ITEM(OUT_Quality_Slider, GET_CHECK(OUT_Quality));

	
	BOOL enable_controls = !GET_CHECK(OUT_Cinema);

	for(int i=OUT_Format; i <= OUT_Profile_Label; i++)
	{
		ENABLE_ITEM(i, enable_controls);
	}

	ENABLE_ITEM(OUT_DCI_Profile, !enable_controls);
	ENABLE_ITEM(OUT_DCI_Profile_Label, !enable_controls);
	
	SHOW_ITEM(OUT_Tiles, enable_controls);
	SHOW_ITEM(OUT_Tiles_Label, enable_controls);
	SHOW_ITEM(OUT_Order, enable_controls);
	SHOW_ITEM(OUT_Order_Label, enable_controls);
	SHOW_ITEM(OUT_Profile, enable_controls);
	SHOW_ITEM(OUT_Profile_Label, enable_controls);

	SHOW_ITEM(OUT_DCI_Data_Rate_Slider, !enable_controls);
	SHOW_ITEM(OUT_DCI_Data_Rate, !enable_controls);
	SHOW_ITEM(OUT_DCI_Data_Rate_Label, !enable_controls);
	SHOW_ITEM(OUT_DCI_Per_Frame, !enable_controls);
	SHOW_ITEM(OUT_DCI_Frame_Rate, !enable_controls);
	SHOW_ITEM(OUT_DCI_Frame_Rate_Label, !enable_controls);
	SHOW_ITEM(OUT_DCI_Stereo, !enable_controls);

	TrackDCIPerFrame(hwndDlg);
}

static void TrackLosslessLabel(HWND hwndDlg)
{
	SetDlgItemText(hwndDlg, OUT_Lossless, (GET_CHECK(OUT_Advanced) && GET_CHECK(OUT_Float)) ? "Maximum" : "Lossless");
}

BOOL CALLBACK DialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
    BOOL fError; 
 
    switch (message) 
    { 
		case WM_INITDIALOG:
			do{
				// set method radio button
				SET_CHECK( (g_method == DIALOG_METHOD_CINEMA ? OUT_Cinema :
							g_method == DIALOG_METHOD_SIZE ? OUT_Size :
							g_method == DIALOG_METHOD_QUALITY ? OUT_Quality :
							OUT_Lossless), TRUE);

				// text fields
				SET_FIELD(OUT_Size_Field, g_size);
				SET_FIELD(OUT_Quality_Field, g_quality);
				SET_FIELD(OUT_Depth, g_depth);

				if(g_dci_per_frame == DIALOG_DCI_PER_FRAME)
					SET_FIELD(OUT_DCI_Data_Rate, g_dci_data_rate);
				else
					SET_FIELD(OUT_DCI_Data_Rate, (g_dci_data_rate * 8 / 1024));

				// check boxes
				SET_CHECK(OUT_Advanced, g_advanced);
				SET_CHECK(OUT_Custom_Depth, g_custom_depth);
				SET_CHECK(OUT_Ycc, g_ycc);
				SET_CHECK(OUT_Float, !g_reversible);
				SET_CHECK(OUT_DCI_Stereo, g_dci_stereo);


				// spinner (up-down control)
				SendMessage(GET_ITEM(OUT_Depth_Spinner), (UINT)UDM_SETRANGE, (WPARAM)0, (LPARAM)MAKELONG(32,1) );


				// menus (oh, I mean combo boxes)
				ADD_MENU_ITEM(OUT_Format, 0, "j2c", DIALOG_TYPE_J2C, g_format == DIALOG_TYPE_J2C);
				ADD_MENU_ITEM(OUT_Format, 1, "JP2", DIALOG_TYPE_JP2, g_format == DIALOG_TYPE_JP2);
				ADD_MENU_ITEM(OUT_Format, 2, "JPX", DIALOG_TYPE_JPX, g_format == DIALOG_TYPE_JPX);

				ADD_MENU_ITEM(OUT_DCI_Profile, 0, "Cinema 2K", DIALOG_DCI_2K, g_dci_profile == DIALOG_DCI_2K);
				ADD_MENU_ITEM(OUT_DCI_Profile, 1, "Cinema 4K", DIALOG_DCI_4K, g_dci_profile == DIALOG_DCI_4K);

				ADD_MENU_ITEM(OUT_DCI_Per_Frame, 0, "KB / frame", DIALOG_DCI_PER_FRAME, g_dci_per_frame == DIALOG_DCI_PER_FRAME);
				ADD_MENU_ITEM(OUT_DCI_Per_Frame, 1, "Mb / second", DIALOG_DCI_PER_SECOND, g_dci_per_frame == DIALOG_DCI_PER_SECOND);

				ADD_MENU_ITEM(OUT_DCI_Frame_Rate, 0, "24", 24, g_dci_frame_rate == 24);
				ADD_MENU_ITEM(OUT_DCI_Frame_Rate, 1, "25", 25, g_dci_frame_rate == 25);
				ADD_MENU_ITEM(OUT_DCI_Frame_Rate, 2, "30", 30, g_dci_frame_rate == 30);
				ADD_MENU_ITEM(OUT_DCI_Frame_Rate, 3, "48", 48, g_dci_frame_rate == 48);
				ADD_MENU_ITEM(OUT_DCI_Frame_Rate, 4, "50", 50, g_dci_frame_rate == 50);
				ADD_MENU_ITEM(OUT_DCI_Frame_Rate, 5, "60", 60, g_dci_frame_rate == 60);

				ADD_MENU_ITEM(OUT_Subsample, 0, "4:2:2", DIALOG_SUBSAMPLE_422, g_sub == DIALOG_SUBSAMPLE_422);
				ADD_MENU_ITEM(OUT_Subsample, 1, "4:1:1", DIALOG_SUBSAMPLE_411, g_sub == DIALOG_SUBSAMPLE_411);
				ADD_MENU_ITEM(OUT_Subsample, 2, "4:2:0", DIALOG_SUBSAMPLE_420, g_sub == DIALOG_SUBSAMPLE_420);
				ADD_MENU_ITEM(OUT_Subsample, 3, "3:1:1", DIALOG_SUBSAMPLE_311, g_sub == DIALOG_SUBSAMPLE_311);
				ADD_MENU_ITEM(OUT_Subsample, 4, "2x2", DIALOG_SUBSAMPLE_2x2, g_sub == DIALOG_SUBSAMPLE_2x2);
				ADD_MENU_ITEM(OUT_Subsample, 5, "3x3", DIALOG_SUBSAMPLE_3x3, g_sub == DIALOG_SUBSAMPLE_3x3);
				ADD_MENU_ITEM(OUT_Subsample, 6, "4x4", DIALOG_SUBSAMPLE_4x4, g_sub == DIALOG_SUBSAMPLE_4x4);
				ADD_MENU_ITEM(OUT_Subsample, 7, "None", DIALOG_SUBSAMPLE_NONE, g_sub == DIALOG_SUBSAMPLE_NONE);

				ADD_MENU_ITEM(OUT_Tiles, 0, "2048", 2048, g_tile_size == 2048);
				ADD_MENU_ITEM(OUT_Tiles, 1, "1024", 1024, g_tile_size == 1024);
				ADD_MENU_ITEM(OUT_Tiles, 2, "512", 512, g_tile_size == 512);
				ADD_MENU_ITEM(OUT_Tiles, 3, "256", 256, g_tile_size == 256);
				ADD_MENU_ITEM(OUT_Tiles, 4, "128", 128, g_tile_size == 128);
				ADD_MENU_ITEM(OUT_Tiles, 5, "64", 64, g_tile_size == 64);
				ADD_MENU_ITEM(OUT_Tiles, 6, "No Tiles", 0, g_tile_size == 0);

				ADD_MENU_ITEM(OUT_Order, 0, "Layer", DIALOG_ORDER_LRCP, g_order == DIALOG_ORDER_LRCP);
				ADD_MENU_ITEM(OUT_Order, 1, "Resolution, Layer", DIALOG_ORDER_RLCP, g_order == DIALOG_ORDER_RLCP);
				ADD_MENU_ITEM(OUT_Order, 2, "Resolution, Position", DIALOG_ORDER_RPCL, g_order == DIALOG_ORDER_RPCL);
				ADD_MENU_ITEM(OUT_Order, 3, "Position", DIALOG_ORDER_PCRL, g_order == DIALOG_ORDER_PCRL);
				ADD_MENU_ITEM(OUT_Order, 4, "Component", DIALOG_ORDER_CPRL, g_order == DIALOG_ORDER_CPRL);

				ADD_MENU_ITEM(OUT_Profile, 0, g_generic_profile, DIALOG_PROFILE_GENERIC, (g_profile == DIALOG_PROFILE_GENERIC) );
				if(g_color_profile)
				{	ADD_MENU_ITEM(OUT_Profile, 1, g_color_profile, DIALOG_PROFILE_ICC, g_profile == DIALOG_PROFILE_ICC); }
				else
				{	ADD_MENU_ITEM(OUT_Profile, 1, "(No Profile Provided)", DIALOG_PROFILE_ICC, g_profile == DIALOG_PROFILE_ICC ); }
				

				// sliders
				HWND slider = GetDlgItem(hwndDlg, OUT_Quality_Slider);

				if(slider)
				{
					SendMessage(slider,(UINT)TBM_SETRANGEMIN, (WPARAM)(BOOL)FALSE, (LPARAM)1);
					SendMessage(slider,(UINT)TBM_SETRANGEMAX, (WPARAM)(BOOL)FALSE, (LPARAM)100);
					SendMessage(slider,(UINT)TBM_SETPOS, (WPARAM)(BOOL)TRUE, (LPARAM)g_quality);
				}
				
				HWND dci_slider = GetDlgItem(hwndDlg, OUT_DCI_Data_Rate_Slider);

				if(dci_slider)
				{
					SendMessage(dci_slider,(UINT)TBM_SETRANGEMIN, (WPARAM)(BOOL)FALSE, (LPARAM)1);
					SendMessage(dci_slider,(UINT)TBM_SETRANGEMAX, (WPARAM)(BOOL)FALSE, (LPARAM)100);
					SetDCIslider(hwndDlg);
				}


				if(g_show_subsample)
				{
					SHOW_ITEM(OUT_DCI_Profile, FALSE);
					SHOW_ITEM(OUT_DCI_Profile_Label, FALSE);
					ENABLE_ITEM(OUT_Cinema, FALSE);
					ENABLE_ITEM(OUT_Ycc, FALSE);
				}
				else
				{
					SHOW_ITEM(OUT_Subsample, FALSE);
					SHOW_ITEM(OUT_Subsample_Label, FALSE);
				}

				TrackLosslessLabel(hwndDlg);
				TrackMethod(hwndDlg);

				if(!GET_CHECK(OUT_Advanced))
					ResizeWindow(hwndDlg, TRUE);

			}while(0);
		return TRUE;
 
		case WM_NOTIFY:
			switch(LOWORD(wParam))
			{
				case OUT_Quality_Slider:
					SET_FIELD(OUT_Quality_Field, GET_SLIDER(OUT_Quality_Slider) );
				return TRUE;

				case OUT_Depth_Spinner:
					//if(HIWORD(wParam) == UDN_DELTAPOS)
					SET_FIELD(OUT_Depth, MAX( 1, MIN( 32, GET_FIELD(OUT_Depth) + ((LPNMUPDOWN)lParam)->iDelta) ) );
				return TRUE;

				case OUT_DCI_Data_Rate_Slider:
					TrackDCIslider(hwndDlg);
				return TRUE;
			}
		return FALSE;

        case WM_COMMAND: 
			g_item_clicked = LOWORD(wParam);

            switch (LOWORD(wParam)) 
            { 
                case OUT_OK: 
				case OUT_Cancel:  // do the same thing, but g_item_clicked will be different

					// set globals based on UI
					g_method =	GET_CHECK(OUT_Size) ?  DIALOG_METHOD_SIZE :
								GET_CHECK(OUT_Quality) ?  DIALOG_METHOD_QUALITY :
								GET_CHECK(OUT_Cinema) ? DIALOG_METHOD_CINEMA :
								DIALOG_METHOD_LOSSLESS;

					// text fields
					g_size = GET_FIELD(OUT_Size_Field);
					g_quality = GET_FIELD(OUT_Quality_Field);
					g_depth = GET_FIELD(OUT_Depth);

					// check boxes
					g_advanced = GET_CHECK(OUT_Advanced);
					g_custom_depth = GET_CHECK(OUT_Custom_Depth);
					g_ycc = GET_CHECK(OUT_Ycc);
					g_reversible = !GET_CHECK(OUT_Float);
					g_dci_stereo = GET_CHECK(OUT_DCI_Stereo);

					// menus
					g_format = (DialogFormat)GET_MENU_VALUE(OUT_Format);
					g_tile_size = GET_MENU_VALUE(OUT_Tiles);
					g_order = (DialogOrder)GET_MENU_VALUE(OUT_Order);
					g_profile = (DialogProfile)GET_MENU_VALUE(OUT_Profile);
					g_dci_profile = (DialogDCIProfile)GET_MENU_VALUE(OUT_DCI_Profile);
					g_sub = (DialogSubsample)GET_MENU_VALUE(OUT_Subsample);
					g_dci_per_frame = (DialogDCIPerFrame)GET_MENU_VALUE(OUT_DCI_Per_Frame);
					g_dci_frame_rate = GET_MENU_VALUE(OUT_DCI_Frame_Rate);

					if(g_dci_per_frame == DIALOG_DCI_PER_FRAME)
						g_dci_data_rate = GET_FIELD(OUT_DCI_Data_Rate);
					else
						g_dci_data_rate = GET_FIELD(OUT_DCI_Data_Rate) * 1024 / 8;


					// quit dialog
					//PostMessage((HWND)hwndDlg, WM_QUIT, (WPARAM)WA_ACTIVE, lParam);
					EndDialog(hwndDlg, 0);
                    //DestroyWindow(hwndDlg); 
				return TRUE;


				case OUT_Lossless:
				case OUT_Cinema:
				case OUT_Size:
				case OUT_Quality:
					TrackMethod(hwndDlg);
					return TRUE;

				case OUT_DCI_Per_Frame:
					TrackDCIPerFrame(hwndDlg);
				case OUT_DCI_Frame_Rate:
					if(HIWORD(wParam) == CBN_CLOSEUP)
						RecalcDataRate(hwndDlg);
					return TRUE;

				case OUT_DCI_Stereo:
					RecalcDataRate(hwndDlg);
				return TRUE;

				case OUT_Size_Field:
					switch(HIWORD(wParam))
					{
						case EN_KILLFOCUS:
							// set slider
							if( GET_FIELD(OUT_Size_Field) > 99999 || GET_FIELD(OUT_Size_Field) < 1)
							{	SET_FIELD(OUT_Size_Field, g_size); }
						return TRUE;
					}
				return FALSE;

				case OUT_Quality_Field:
					switch(HIWORD(wParam))
					{
						case EN_KILLFOCUS:
							// set slider
							if( GET_FIELD(OUT_Quality_Field) > 100 || GET_FIELD(OUT_Quality_Field) < 1)
							{	SET_FIELD(OUT_Quality_Field, g_quality); }

							SET_SLIDER(OUT_Quality_Slider, GET_FIELD(OUT_Quality_Field) );
						return TRUE;
					}
				return FALSE;

				case OUT_Depth:
					switch(HIWORD(wParam))
					{
						case EN_KILLFOCUS:
							// set slider
							if( GET_FIELD(OUT_Depth) > 32 || GET_FIELD(OUT_Depth) < 1)
							{	SET_FIELD(OUT_Depth, g_depth); }
						return TRUE;
					}
				return FALSE;

				case OUT_DCI_Data_Rate:
					switch(HIWORD(wParam))
					{
						case EN_KILLFOCUS:
							// set slider
							if( GET_FIELD(OUT_DCI_Data_Rate) > 9999999999 || GET_FIELD(OUT_DCI_Data_Rate) < 1)
							{
								SET_FIELD(OUT_DCI_Data_Rate, g_dci_data_rate * 8 / 1024);
							}
							else
							{
								g_dci_data_rate = GET_FIELD(OUT_DCI_Data_Rate) * 1024 / 8;
							}

							SetDCIslider(hwndDlg);
						return TRUE;
					}
				return FALSE;

				case OUT_Advanced:
					ResizeWindow(hwndDlg, !GET_CHECK(OUT_Advanced));
				case OUT_Float:
					TrackLosslessLabel(hwndDlg);
				return FALSE;
            } 
    } 
    return FALSE; 
} 
/*
// thanks, Photoshop 6 SDK!
void CenterDialog(HWND hDlg)
{
	int  nHeight;
    int  nWidth;
    int  nTitleBits;
    RECT rcDialog;
    RECT rcParent;
    int  xOrigin;
    int  yOrigin;
    int  xScreen;
    int  yScreen;
    HWND hParent = GetParent(hDlg);

    if  (hParent == NULL)
        hParent = GetDesktopWindow();

    GetClientRect(hParent, &rcParent);
    ClientToScreen(hParent, (LPPOINT)&rcParent.left);  // point(left,  top)
    ClientToScreen(hParent, (LPPOINT)&rcParent.right); // point(right, bottom)

    // Center on Title: title bar has system menu, minimize,  maximize bitmaps
    // Width of title bar bitmaps - assumes 3 of them and dialog has a sysmenu
    nTitleBits = GetSystemMetrics(SM_CXSIZE);

    // If dialog has no sys menu compensate for odd# bitmaps by sub 1 bitwidth
    if  ( ! (GetWindowLong(hDlg, GWL_STYLE) & WS_SYSMENU))
        nTitleBits -= nTitleBits / 3;

    GetWindowRect(hDlg, &rcDialog);
    nWidth  = rcDialog.right  - rcDialog.left;
    nHeight = rcDialog.bottom - rcDialog.top;

    xOrigin = max(rcParent.right - rcParent.left - nWidth, 0) / 2
            + rcParent.left - nTitleBits;
    xScreen = GetSystemMetrics(SM_CXSCREEN);
    if  (xOrigin + nWidth > xScreen)
        xOrigin = max (0, xScreen - nWidth);

	yOrigin = max(rcParent.bottom - rcParent.top - nHeight, 0) / 3
            + rcParent.top;
    yScreen = GetSystemMetrics(SM_CYSCREEN);
    if  (yOrigin + nHeight > yScreen)
        yOrigin = max(0 , yScreen - nHeight);

    SetWindowPos(hDlg, NULL, xOrigin, yOrigin, nWidth, nHeight, SWP_NOZORDER);
}
*/

bool
j2k_OutUI(
	j2k_OutUI_Data	*params,
	const char		*generic_profile,
	const char		*color_profile,
	bool			show_subsample,
	const void		*plugHndl,
	const void		*mwnd)
{
	// set globals
	g_generic_profile	= generic_profile;
	g_color_profile		= color_profile;
	g_show_subsample	= show_subsample;
	
	
	g_method			= params->method;
	g_quality			= params->quality;
	g_size				= params->size;
	g_advanced			= params->advanced;
	g_format			= params->format;
	g_custom_depth		= params->customDepth;
	g_depth				= params->bitDepth;
	g_reversible		= params->reversible;
	g_ycc				= params->ycc;
	g_sub				= params->sub;
	g_order				= params->order;
	g_tile_size			= params->tileSize;
	g_profile			= params->icc_profile;
	g_dci_profile		= params->dci_profile;
	g_dci_data_rate		= params->dci_data_rate;
	g_dci_per_frame		= params->dci_per_frame;
	g_dci_frame_rate	= params->dci_frame_rate;
	g_dci_stereo		= params->dci_stereo;


	int status = DialogBox((HINSTANCE)plugHndl, (LPSTR)"OUT_DIALOG", (HWND)mwnd, (DLGPROC)DialogProc);
	

	if(g_item_clicked == OUT_OK)
	{
		params->method			= g_method;
		params->quality			= g_quality;
		params->size			= g_size;
		params->advanced		= g_advanced;
		params->format			= g_format;
		params->customDepth		= g_custom_depth;
		params->bitDepth		= g_depth;
		params->reversible		= g_reversible;
		params->ycc				= g_ycc;
		params->sub				= g_sub;
		params->order			= g_order;
		params->tileSize		= g_tile_size;
		params->icc_profile		= g_profile;
		params->dci_profile		= g_dci_profile;
		params->dci_data_rate	= g_dci_data_rate;
		params->dci_per_frame	= g_dci_per_frame;
		params->dci_frame_rate	= g_dci_frame_rate;
		params->dci_stereo		= g_dci_stereo;
		
		return true;
	}
	else
		return false;
}

static BOOL CALLBACK AboutDialogProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam) 
{ 
	if(message == WM_INITDIALOG)
	{
		SetDlgItemText(hwndDlg, 7, "v" j2k_Version_String " - " j2k_Build_Date);

		return TRUE;
	}
	else if(message == WM_COMMAND)
	{
		if(LOWORD(wParam) == OUT_OK || LOWORD(wParam) == OUT_Cancel)
		{
			EndDialog(hwndDlg, 0);

			return TRUE;
		}
	}

	return FALSE;
} 


void
j2k_About(
	const void		*plugHndl,
	const void		*mwnd)
{
	int status = DialogBox((HINSTANCE)plugHndl, (LPSTR)"ABOUT_DIALOG", (HWND)mwnd, (DLGPROC)AboutDialogProc);
}

