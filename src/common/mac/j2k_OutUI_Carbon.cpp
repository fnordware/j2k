// j2k
//
// Carbon dialog


#include "j2k_OutUI.h"

#include <Carbon/Carbon.h>

UInt32	g_item_clicked = 0;

#ifndef MIN
#define MIN(A, B)	( (A) < (B) ? (A) : (B) )
#endif

#ifndef MAX
#define MAX(A, B)	( (A) > (B) ? (A) : (B) )
#endif

static void SetControlVal(WindowRef window, OSType sig, SInt32 id, SInt32 val)
{
	ControlID cid = {sig, id};
	ControlRef ref;
	
	OSStatus result = GetControlByID(window, &cid, &ref);
	SetControl32BitValue(ref, val);
	//SetControlValue(ref, val);
}

static SInt32 GetControlVal(WindowRef window, OSType sig, SInt32 id)
{
	ControlID cid = {sig, id};
	ControlRef ref;
	
	OSStatus result = GetControlByID(window, &cid, &ref);
	
	return GetControl32BitValue(ref);
}

static void SetTextControlVal(WindowRef window, OSType sig, SInt32 id, SInt32 val)
{
	ControlID cid = {sig, id};
	ControlRef ref;
	
	OSStatus result = GetControlByID(window, &cid, &ref);
	
	CFStringRef txt = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"), val);
	
	SetControlData(ref, 0, kControlEditTextCFStringTag, sizeof(CFStringRef), &txt);
	
	Draw1Control(ref);
	
	CFRelease(txt);
}

static SInt32 GetTextControlVal(WindowRef window, OSType sig, SInt32 id)
{
	ControlID cid = {sig, id};
	ControlRef ref;
	
	OSStatus result = GetControlByID(window, &cid, &ref);
	
	CFStringRef txt;
	
	result = GetControlData(ref, 0, kControlEditTextCFStringTag, sizeof(CFStringRef), &txt, NULL);
	
	SInt32 out = CFStringGetIntValue(txt);
	
	CFRelease(txt);
	
	return out;
}

static void EnableControl(WindowRef window, OSType sig, SInt32 id, Boolean enabled)
{
	ControlID cid = {sig, id};
	ControlRef ref;
	
	OSStatus result = GetControlByID(window, &cid, &ref);
	
	if(enabled)
		EnableControl(ref);
	else
		DisableControl(ref);
}

#pragma mark-

static void TrackMethod(WindowRef window, DialogMethod method)
{
	Boolean enabled = (method != DIALOG_METHOD_CINEMA);
	
	EnableControl(window, 'Form', 0, enabled);
	EnableControl(window, 'Form', 1, enabled);
	EnableControl(window, 'Cust', 0, enabled);
	EnableControl(window, 'Tick', 0, enabled);
	EnableControl(window, 'Ycc ', 0, enabled);
	EnableControl(window, 'Flot', 0, enabled);
	EnableControl(window, 'Tile', 0, enabled);
	EnableControl(window, 'Tile', 1, enabled);
	EnableControl(window, 'Ordr', 0, enabled);
	EnableControl(window, 'Ordr', 1, enabled);
	EnableControl(window, 'Prof', 0, enabled);
	EnableControl(window, 'Prof', 1, enabled);
	
	EnableControl(window, 'DCIp', 0, !enabled);
	EnableControl(window, 'DCIp', 1, !enabled);
	EnableControl(window, 'DCIs', 0, !enabled);
}

static void SetMethod(WindowRef window, DialogMethod method)
{
	// manage radio buttons
	SetControlVal(window, 'Meth', 0, (method == DIALOG_METHOD_LOSSLESS) );
	SetControlVal(window, 'Meth', 1, (method == DIALOG_METHOD_SIZE) );
	SetControlVal(window, 'Meth', 2, (method == DIALOG_METHOD_QUALITY) );
	SetControlVal(window, 'Meth', 3, (method == DIALOG_METHOD_CINEMA) );
	
	TrackMethod(window, method);
}

static DialogMethod GetMethod(WindowRef window)
{
	return	GetControlVal(window, 'Meth', 1) ? DIALOG_METHOD_SIZE :
			GetControlVal(window, 'Meth', 2) ? DIALOG_METHOD_QUALITY :
			GetControlVal(window, 'Meth', 3) ? DIALOG_METHOD_CINEMA :
			DIALOG_METHOD_LOSSLESS;
}

static void TrackFloatEncoding(WindowRef window)
{
	ControlID	lossless_id = {'Meth', 0};
	ControlRef lossless_ref;
	
	OSStatus result = GetControlByID(window, &lossless_id, &lossless_ref);
	
	if( GetControlVal(window, 'Advn', 0) && GetControlVal(window, 'Flot', 0) )
		result = SetControlTitleWithCFString(lossless_ref, CFSTR("Maximum"));
	else
		result = SetControlTitleWithCFString(lossless_ref, CFSTR("Lossless"));
}

static void SetFloatEncoding(WindowRef window, bool float_encoding)
{
	SetControlVal(window, 'Flot', 0, float_encoding);
	
	if(float_encoding)
		TrackFloatEncoding(window);
}

static void TrackAdvanced(WindowRef window)
{
	OSStatus result = noErr;
	
	ControlID	ok_id = {'Ok  ', 0},
				cancel_id = {'Canc', 0};
	
	ControlRef ok_ref, cancel_ref;
	
	result = GetControlByID(window, &ok_id, &ok_ref);
	result = GetControlByID(window, &cancel_id, &cancel_ref);

	if( GetControlVal(window, 'Advn', 0) )
	{
		SizeWindow(window, 600, 315, TRUE);

		MoveControl(ok_ref, 495, 275);
		MoveControl(cancel_ref, 398, 275);
	}
	else
	{
		MoveControl(ok_ref, 495-300, 275);
		MoveControl(cancel_ref, 398-300, 275);
		
		SizeWindow(window, 300, 315, TRUE);
	}
	
	// also need to do this
	TrackFloatEncoding(window);
}

static void SetAdvanced(WindowRef window, bool advanced)
{
	SetControlVal(window, 'Advn', 0, advanced);
	
	if(!advanced)
		TrackAdvanced(window);
}

static void SetTileSize(WindowRef window, SInt32 size)
{
	SInt32 sizes[] = {2048, 1024, 512, 256, 128, 64, 0};
	
	int val=0, i;
	
	for(i=1; i<7; i++)
		if(size < sizes[i-1])
			val = i;
	
	SetControlVal(window, 'Tile', 0, val+1);
}

static SInt32 GetTileSize(WindowRef window)
{
	SInt32 sizes[] = {2048, 1024, 512, 256, 128, 64, 0};
	
	return sizes[ GetControlVal(window, 'Tile', 0) - 1 ];
}

static void SetColorProfile(WindowRef window, DialogProfile color)
{
	SInt32 val =	(color == DIALOG_PROFILE_ICC) ? 2 : 1;
	
	SetControlVal(window, 'Prof', 0, val);
}

static void SetProfileName(WindowRef window, const char *profile_name)
{
	ControlID cid = {'Prof', 0};
	ControlRef ref;
	
	OSStatus result = GetControlByID(window, &cid, &ref);
	
	CFStringRef prof_name = CFStringCreateWithCString(kCFAllocatorDefault, profile_name, kCFStringEncodingMacRoman);
	
	MenuRef menu;
	result = GetControlData(ref, 0, kControlPopupButtonMenuRefTag, sizeof(MenuRef), &menu, NULL);
	
	result = SetMenuItemTextWithCFString(menu, 2, prof_name);
	
	EnableMenuItem(menu, 2);
	
	CFRelease(prof_name);
}

static void SetGenericName(WindowRef window, const char *profile_name)
{
	ControlID cid = {'Prof', 0};
	ControlRef ref;
	
	OSStatus result = GetControlByID(window, &cid, &ref);
	
	CFStringRef prof_name = CFStringCreateWithCString(kCFAllocatorDefault, profile_name, kCFStringEncodingMacRoman);
	
	MenuRef menu;
	result = GetControlData(ref, 0, kControlPopupButtonMenuRefTag, sizeof(MenuRef), &menu, NULL);
	
	result = SetMenuItemTextWithCFString(menu, 1, prof_name);
	
	CFRelease(prof_name);
}


static DialogProfile GetColorProfile(WindowRef window)
{
	return (GetControlVal(window, 'Prof', 0) == 1) ? DIALOG_PROFILE_GENERIC : DIALOG_PROFILE_ICC;
}

static void SetDCIProfile(WindowRef window, DialogDCIProfile val)
{
	SetControlVal(window, 'DCIp', 0, val + 1);
}

static DialogDCIProfile GetDCIProfile(WindowRef window)
{
	return (DialogDCIProfile)(GetControlVal(window, 'DCIp', 0) - 1);
}

static void RedrawSliderLabels(WindowRef window)
{
	// redraw text guys
	ControlID small_id = {'Smal', 0}, high_id = {'High', 0};
	ControlRef small_ref, high_ref;
	
	OSStatus result = GetControlByID(window, &small_id, &small_ref);
	result = GetControlByID(window, &high_id, &high_ref);
	
	Draw1Control(small_ref);
	Draw1Control(high_ref);
}

static void SliderControlAction(ControlRef theControl, ControlPartCode partCode)
{
	WindowRef window = GetControlOwner(theControl);
	//SInt32 value = GetControl32BitValue(theControl);
	
	SetTextControlVal(window, 'Qual', 0, GetControl32BitValue(theControl) );
	
	// redraw text guys
	//RedrawSliderLabels(window);
}

static void TrackQuality(WindowRef window)
{
	SInt32 val = GetTextControlVal(window, 'Qual', 0);
	
	if(val < 1)
		SetTextControlVal(window, 'Qual', 0, 1);
	else if(val > 100)
		SetTextControlVal(window, 'Qual', 0, 100);
		
	SetControlVal(window, 'Slid', 0, GetTextControlVal(window, 'Qual', 0) );
	
	RedrawSliderLabels(window);
	
	SetMethod(window, DIALOG_METHOD_QUALITY);
}

static void TrackSize(WindowRef window)
{
	SInt32 val = GetTextControlVal(window, 'Size', 0);
	
	if(val < 1)
		SetTextControlVal(window, 'Size', 0, 1);
	else if(val > 99999)
		SetTextControlVal(window, 'Size', 0, 99999);
	
	SetMethod(window, DIALOG_METHOD_SIZE);
}

static void TickControlAction(ControlRef theControl, ControlPartCode partCode)
{
	WindowRef window = GetControlOwner(theControl);
	SInt32 value = GetControl32BitValue(theControl);

	switch (partCode)
	{
		case kControlUpButtonPart:		SetControl32BitValue(theControl, ++value);	break;
		case kControlDownButtonPart:	SetControl32BitValue(theControl, --value);	break;
	}
	
	SetTextControlVal(window, 'Bits', 0, GetControl32BitValue(theControl) );
}


#pragma mark-

static pascal OSStatus
WindowEventHandler( EventHandlerCallRef inCaller, EventRef inEvent, void* inRefcon )
{
#pragma unused( inCaller )

	OSStatus  result = eventNotHandledErr;

	WindowRef *window = (WindowRef *)inRefcon;
	//WindowRef other = ActiveNonFloatingWindow();
  
	switch ( GetEventClass( inEvent ) )
	{
		case kEventClassCommand:
		{
			HICommand  cmd;

			GetEventParameter( inEvent, kEventParamDirectObject, typeHICommand, NULL, sizeof( cmd ), NULL, &cmd );
			
			switch ( GetEventKind( inEvent ) )
			{
				case kEventCommandProcess:
					switch ( cmd.commandID )
					{
						case kHICommandOK:
							QuitAppModalLoopForWindow( *window );
							g_item_clicked = cmd.commandID;
							result = noErr;
						break;

						case kHICommandCancel:
							QuitAppModalLoopForWindow( *window );
							g_item_clicked = cmd.commandID;
							result = noErr;
						break;
						
						case 'Loss':	SetMethod(*window, DIALOG_METHOD_LOSSLESS);	break;
						case 'Size':	SetMethod(*window, DIALOG_METHOD_SIZE);		break;
						case 'Qual':	SetMethod(*window, DIALOG_METHOD_QUALITY);		break;
						case 'Dcin':	SetMethod(*window, DIALOG_METHOD_CINEMA);		break;
						
						case 'Advn':	TrackAdvanced(*window); break;
						
						case 'Quat':	TrackQuality(*window); break;
						case 'Sizt':	TrackSize(*window); break;
						case 'Flot':	TrackFloatEncoding(*window); break;
						
						case 'Slid':	RedrawSliderLabels(*window);
										SetMethod(*window, DIALOG_METHOD_QUALITY);
										break;

						default:
						break;
					}
				break;

				default:
				break;
			}
			break;
		}

		default:
		break;
	}
  
  return result;
}


bool
j2k_OutUI(
	j2k_OutUI_Data	*params,
	const char		*generic_profile,
	const char		*color_profile,
	bool			show_subsample,
	const void		*plugHndl,
	const void		*mwnd)
{
	OSStatus		result = noErr;
	
	bool hit_ok = false;
	
	CFStringRef bundle_id = CFStringCreateWithCString(kCFAllocatorDefault, (const char *)plugHndl, kCFStringEncodingMacRoman);

	// get the window from the nib from the bundle from the path
	CFBundleRef bundle_ref = CFBundleGetBundleWithIdentifier(bundle_id);
	
	if(bundle_ref)
	{
		CFRetain(bundle_ref);
		
		IBNibRef nib_ref = NULL;
		result = CreateNibReferenceWithCFBundle(bundle_ref, CFSTR("Out_Dialog"), &nib_ref);
		
		if(nib_ref)
		{
			WindowRef window = NULL;
			result = CreateWindowFromNib(nib_ref, CFSTR("OutDialog"), &window);
			
			if(window)
			{
				// but image in HIImageView
				CFURLRef png_url = CFBundleCopyResourceURL(bundle_ref, CFSTR("j2k_banner.png"), NULL, NULL);
				
				HIViewRef banner_view = NULL;
				HIViewID  hiViewID = {'Banr', 0};
				
				result = HIViewFindByID(HIViewGetRoot(window), hiViewID, &banner_view);
				
				if(png_url && banner_view)
				{
					CGDataProviderRef png_provider = CGDataProviderCreateWithURL(png_url);
					
					CGImageRef png_image = CGImageCreateWithPNGDataProvider(png_provider, NULL, FALSE, kCGRenderingIntentDefault);
					
					result = HIImageViewSetImage(banner_view, png_image);
					result = HIViewSetVisible(banner_view, true);
					result = HIViewSetNeedsDisplay(banner_view, true);
					
					CGImageRelease(png_image);
					CFRelease(png_url);
				}
				
				// make 'Tick' do what it should
				ControlID tick_id = {'Tick', 0};
				ControlRef tick_ref;
				result = GetControlByID(window, &tick_id, &tick_ref);
				SetControlAction(tick_ref, TickControlAction);
				
				// make slider do its thing
				ControlID slide_id = {'Slid', 0};
				ControlRef slide_ref;
				result = GetControlByID(window, &slide_id, &slide_ref);
				SetControlAction(slide_ref, SliderControlAction);
				
				// set control values
				SetAdvanced(window, params->advanced);
				SetControlVal(window, 'Form', 0, params->format);
				SetControlVal(window, 'Cust', 0, params->customDepth);
				SetControlVal(window, 'Tick', 0, params->bitDepth);
				SetFloatEncoding(window, !(params->reversible) );
				SetMethod(window, params->method);
				SetTextControlVal(window, 'Qual', 0, params->quality);
				SetControlVal(window, 'Slid', 0, params->quality);
				SetTextControlVal(window, 'Size', 0, params->size);
				SetControlVal(window, 'Ycc ', 0, params->ycc);
				SetControlVal(window, 'Ordr', 0, params->order);
				SetTileSize(window, params->tileSize);
				SetColorProfile(window, params->icc_profile);
				SetDCIProfile(window, params->dci_profile);
				SetControlVal(window, 'DCIs', 0, params->dci_safe);
				
				SetTextControlVal(window, 'Bits', 0, GetControlVal(window, 'Tick', 0) );
				
				// profile names
				if(color_profile)
					SetProfileName(window, color_profile);
				
				if(generic_profile)
					SetGenericName(window, generic_profile);
				
				// set up events
				EventTypeSpec  kWindowEvents[] =  {  { kEventClassCommand, kEventCommandProcess} };
				EventHandlerUPP windowUPP = NewEventHandlerUPP( WindowEventHandler );
				
				EventTypeSpec eventList[] = {{kEventClassCommand, kEventCommandProcess}};
				//InstallStandardEventHandler( GetWindowEventTarget(dialog_win) );
				InstallWindowEventHandler(window, windowUPP, GetEventTypeCount( eventList ), eventList, (void *)&window, NULL );
				
				ShowWindow(window);
				//ShowControl(root_control);
				
				// event loop
				//RunApplicationEventLoop();
				RunAppModalLoopForWindow(window);
				
				
				if(g_item_clicked == kHICommandOK)
				{
					// set options
					params->format			= (DialogFormat)GetControlVal(window, 'Form', 0);
					params->customDepth		= GetControlVal(window, 'Cust', 0);
					params->bitDepth		= GetControlVal(window, 'Tick', 0);
					params->reversible		= !GetControlVal(window, 'Flot', 0);
					params->method			= GetMethod(window);
					params->quality			= MIN(100, MAX(1, GetTextControlVal(window, 'Qual', 0) ) );
					params->size			= GetTextControlVal(window, 'Size', 0);
					params->advanced		= GetControlVal(window, 'Advn', 0);
					params->ycc				= GetControlVal(window, 'Ycc ', 0);
					params->order			= (DialogOrder)GetControlVal(window, 'Ordr', 0);
					params->tileSize		= GetTileSize(window);
					params->icc_profile		= GetColorProfile(window);
					params->dci_profile		= GetDCIProfile(window);
					params->dci_safe		= GetControlVal(window, 'DCIs', 0);
					
					hit_ok = true;
				}
				else
					hit_ok = false;
				

				DisposeWindow(window);
				DisposeEventHandlerUPP(windowUPP);
			}
			
			DisposeNibReference(nib_ref);
		}
		
		CFRelease(bundle_ref);
		CFRelease(bundle_id);
	}
	
	return hit_ok;
}


void
j2k_About(
	const void		*plugHndl,
	const void		*mwnd)
{
	// AE doesn't do about and this is only being used in AE
}

