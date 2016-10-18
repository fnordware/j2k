
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



#ifndef J2K_RGBA_FILE_H
#define J2K_RGBA_FILE_H

#include "j2k_codec.h"

namespace j2k
{

typedef struct
{
	Channel r;
	Channel g;
	Channel b;
	Channel a;
	
} RGBAbuffer;


class RGBAinputFile
{
  public:
	RGBAinputFile(InputFile &file, Codec *codec = NULL);
	~RGBAinputFile();
	
	const FileInfo & GetFileInfo() const { return _fileInfo; }
	
	void ReadFile(RGBAbuffer &buffer, unsigned int subsample = 0);
	
  private:
	InputFile &_file;
	Codec *_codec;
	
	FileInfo _fileInfo;
};


class RGBAoutputFile
{
  public:
	RGBAoutputFile(OutputFile &file, const FileInfo &info, Codec *codec = NULL);
	~RGBAoutputFile() {}
	
	void WriteFile(RGBAbuffer &buffer);
	
  private:
	OutputFile &_file;
	Codec *_codec;
	
	FileInfo _fileInfo;
};


}; // namespace j2k

#endif // J2K_RGBA_FILE_H

