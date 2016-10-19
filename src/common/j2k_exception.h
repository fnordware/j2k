
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

#ifndef J2K_EXCEPTION_H
#define J2K_EXCEPTION_H

#include <exception>
#include <string>

namespace j2k
{


class Exception : public std::exception
{
  public:
	Exception(const std::string &s) throw();
	virtual ~Exception() throw() {}
	
	virtual const char* what() const throw() { return _s.c_str(); }

  private:
	std::string _s;
};


};

#endif // J2K_EXCEPTION_H