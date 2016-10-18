
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

#ifndef J2K_GROK_CODEC_H
#define J2K_GROK_CODEC_H

#include "j2k_codec.h"


namespace j2k
{


class GrokCodec : public Codec
{
  public:
	GrokCodec() {}
	virtual ~GrokCodec() {}
	
	virtual const char * Name() const { return "Grok"; }
	virtual const char * FourCharCode() const { return "grok"; }
	
	virtual ReadFlags GetReadFlags() { return (J2K_CAN_READ | J2K_CAN_SUBSAMPLE); }
	virtual WriteFlags GetWriteFlags() { return (J2K_CAN_WRITE); }
	
	virtual bool Verify(InputFile &file);
	virtual void GetFileInfo(InputFile &file, FileInfo &info);
	virtual void ReadFile(InputFile &file, const Buffer &buffer, unsigned int subsample = 0);
	
	virtual void WriteFile(OutputFile &file, const FileInfo &info, const Buffer &buffer);
};


}; // namespace j2k

#endif // J2K_GROK_CODEC_H