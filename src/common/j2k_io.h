
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



#ifndef J2K_IO_H
#define J2K_IO_H


namespace j2k
{


class InputFile
{
  public:
	enum {
		J2K_READ_SEEKABLE = (1L << 0)
	};
	
	typedef unsigned int ReadFlags;
	

  public:
	InputFile() {}
	virtual ~InputFile() {}
	
	virtual ReadFlags Flags() const = 0;
	
	virtual size_t FileSize() = 0;
	virtual size_t Read(void *buf, size_t num_bytes) = 0;
	virtual bool Seek(size_t position) = 0;
	virtual size_t Tell() = 0;
};


class OutputFile
{
  public:
	enum {
		J2K_WRITE_SEEKABLE = (1L << 0),
		J2K_WRITE_READABLE = (1L << 1)
	};
	
	typedef unsigned int WriteFlags;
	

  public:
	OutputFile() {}
	virtual ~OutputFile() {}
	
	virtual WriteFlags Flags() const = 0;
	
	virtual size_t Read(void *buf, size_t num_bytes) = 0;
	virtual size_t Write(const void *buf, size_t num_bytes) = 0;
	virtual bool Seek(size_t position) = 0;
	virtual size_t Tell() = 0;
};


}; // namespace j2k


#endif // J2K_IO_H
