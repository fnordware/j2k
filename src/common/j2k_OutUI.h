
/* ---------------------------------------------------------------------
// 
// j2k - JPEG 2000 plug-ins for Adobe programs
// Copyright (c) 2002-2016,  Brendan Bolles, http://www.fnordware.com
// Copyright (c) 2016,       Aaron Boxer,    http://grokimagecompression.github.io/grok
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
// -------------------------------------------------------------------*/


#ifndef j2k_OutUI_H
#define j2k_OutUI_H

#pragma once

typedef enum{
	DIALOG_METHOD_LOSSLESS = 1,
	DIALOG_METHOD_SIZE,
	DIALOG_METHOD_QUALITY,
	DIALOG_METHOD_CINEMA
} DialogMethod;

typedef enum{
	DIALOG_SUBSAMPLE_NONE = 0,
	DIALOG_SUBSAMPLE_422,
	DIALOG_SUBSAMPLE_411,
	DIALOG_SUBSAMPLE_420,
	DIALOG_SUBSAMPLE_311,
	DIALOG_SUBSAMPLE_2x2,
	DIALOG_SUBSAMPLE_3x3,
	DIALOG_SUBSAMPLE_4x4
} DialogSubsample;

typedef enum {
	DIALOG_TYPE_UNKNOWN = 0,
	DIALOG_TYPE_J2C,
	DIALOG_TYPE_JP2,
	DIALOG_TYPE_JPX
} DialogFormat;

typedef enum{
	DIALOG_ORDER_LRCP = 0,
	DIALOG_ORDER_RLCP,
	DIALOG_ORDER_RPCL,
	DIALOG_ORDER_PCRL,
	DIALOG_ORDER_CPRL
} DialogOrder;

typedef enum {
	DIALOG_PROFILE_GENERIC = 0,
	DIALOG_PROFILE_ICC
} DialogProfile;

typedef enum {
	DIALOG_DCI_2K = 0,
	DIALOG_DCI_4K
} DialogDCIProfile;

typedef enum {
	DIALOG_DCI_PER_FRAME = 0,
	DIALOG_DCI_PER_SECOND
} DialogDCIPerFrame;

typedef struct {
	DialogMethod		method;
	long				size;
	int					quality;
	bool				advanced;
	DialogFormat		format;
	bool				customDepth;
	int					bitDepth;
	bool				reversible;
	bool				ycc;
	DialogSubsample		sub;
	DialogOrder			order;
	int					tileSize;
	DialogProfile		icc_profile;
	DialogDCIProfile	dci_profile;
	int					dci_data_rate; // always in kilobytes
	DialogDCIPerFrame	dci_per_frame;
	int					dci_frame_rate;
	bool				dci_stereo;				
} j2k_OutUI_Data, *j2k_OutUI_Ptr, **j2k_OutUI_Hndl;


// j2k_OutUI
//
// return true if user hit OK
// if user hit OK, params block will have been modified
//
// send in block of parameters, names for profile menu, and weather to show subsample menu
// plugHndl is bundle identifier string on Mac, hInstance on win
// mwnd is the main window, Windows only (NULL on Mac)

bool
j2k_OutUI(
	j2k_OutUI_Data	*params,
	const char		*generic_profile,
	const char		*color_profile,
	bool			show_subsample,
	const void		*plugHndl,
	const void		*mwnd);

void
j2k_About(
	const void		*plugHndl,
	const void		*mwnd);
	
#endif // j2k_OutUI_H