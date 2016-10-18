
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


//
//	fnord_SuiteHandler
//		by Brendan Bolles <brendan@fnordware.com>
//
//	A multi-version SuiteHandler.  Ripped off the Adobe SDK verison.
//
//
//

#include <fnord_SuiteHandler.h>
#include <AE_Macros.h>

AEGP_SuiteHandler::AEGP_SuiteHandler(const SPBasicSuite *pica_basicP) :
	i_pica_basicP(pica_basicP)
{
	AEFX_CLR_STRUCT(i_suites);

	if (!i_pica_basicP) {						//can't construct w/out basic suite, everything else is demand loaded
		MissingSuiteError();
	}
}

AEGP_SuiteHandler::~AEGP_SuiteHandler()
{
	ReleaseAllSuites();
}

// Had to go to the header file to be inlined to please CW mach-o target
/*void *AEGP_SuiteHandler::pLoadSuite(A_char *nameZ, A_long versionL) const
{
	const void *suiteP;
	A_long err = i_pica_basicP->AcquireSuite(nameZ, versionL, &suiteP);

	if (err || !suiteP) {
		MissingSuiteError();
	}

	return (void*)suiteP;
}*/

// Free a particular suite. Ignore errors, because, what is there to be done if release fails?
void AEGP_SuiteHandler::ReleaseSuite(A_char *nameZ, A_long versionL)
{
	i_pica_basicP->ReleaseSuite(nameZ, versionL);
}

