
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


//
//	fnord_SuiteHandler
//		by Brendan Bolles <brendan@fnordware.com>
//
//	A multi-version SuiteHandler.  Ripped off the Adobe SDK verison.
//
//	Part of the fnord OpenEXR tools
//		http://www.fnordware.com/OpenEXR/
//
//

#include "fnord_SuiteHandler.h"

void AEGP_SuiteHandler::MissingSuiteError() const
{
	//	Yes, we've read Scott Meyers, and know throwing
	//	a stack-based object can cause problems. Since
	//	the err is just a long, and since we aren't de-
	//	referencing it in any way, risk is mimimal.

	//	As always, we expect those of you who use
	//	exception-based code to do a little less rudi-
	//	mentary job of it than we are here. 
	
	//	Also, excuse the Madagascar-inspired monkey 
	//	joke; couldn't resist. 
	//								-bbb 10/10/05
	
	PF_Err poop = PF_Err_BAD_CALLBACK_PARAM;

	throw poop;
}

