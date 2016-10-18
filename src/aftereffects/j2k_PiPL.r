
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

#include "AEConfig.h"

#ifndef AE_OS_WIN
	#include "AE_General.r"
#endif

resource 'PiPL' (16000) {
	{	/* array properties: 7 elements */
		/* [1] */
		Kind {
			AEGP
		},
		/* [2] */
		Name {
			"j2k"
		},
		/* [3] */
		Category {
			"General Plugin"
		},
		/* [4] */
		Version {
			65536
		},
		/* [5] */
#ifdef AE_OS_WIN
	#ifdef AE_PROC_INTELx64
		CodeWin64X86 {"GPMain_IO"},
	#else
		CodeWin32X86 { "GPMain_IO" },
	#endif
#else	
	#ifdef AE_PROC_INTELx64
		CodeMacIntel64 { "GPMain_IO" },
	#else
		CodeMachOPowerPC { "GPMain_IO" },
		CodeMacIntel32 { "GPMain_IO" },
	#endif
#endif
	}
};



#ifdef AE_OS_MAC

#include "MacTypes.r"

#define NAME				"j2k"
#define VERSION_STRING		"2.0"
resource 'vers' (1, NAME " Version", purgeable)
{
	5, 0x50, final, 0, verUs,
	VERSION_STRING,
	VERSION_STRING
	"\n�2007 fnord"
};

resource 'vers' (2, NAME " Version", purgeable)
{
	5, 0x50, final, 0, verUs,
	VERSION_STRING,
	"by Brendan Bolles"
};

#endif
