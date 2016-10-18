
/* ---------------------------------------------------------------------
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
// -------------------------------------------------------------------*/

#pragma once 


#ifdef MSWindows
#include <windows.h>
#endif

#include "AEConfig.h"
#include "entry.h"
#include "AE_IO.h"
#include "AE_Macros.h"
#include "AE_EffectCBSuites.h"
#include "fnord_SuiteHandler.h"

#ifdef __cplusplus
extern "C" {
#endif


DllExport A_Err
GPMain_IO(
	struct SPBasicSuite		*pica_basicP,			/* >> */
	A_long				 	major_versionL,			/* >> */		
	A_long					minor_versionL,			/* >> */		
#if PF_AE_PLUG_IN_VERSION < PF_AE100_PLUG_IN_VERSION
	const A_char		 	*file_pathZ,			/* >> platform-specific delimiters */
	const A_char		 	*res_pathZ,				/* >> platform-specific delimiters */
#endif
	AEGP_PluginID			aegp_plugin_id,			/* >> */
	void					*global_refconPV);			/* << */


#ifdef __cplusplus
}
#endif

