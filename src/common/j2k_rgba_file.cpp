
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

#include "j2k_rgba_file.h"

#include "j2k_exception.h"

#include <assert.h>

namespace j2k
{

RGBAinputFile::RGBAinputFile(InputFile &file, Codec *codec) :
	_file(file),
	_codec(codec)
{
	if(_codec == NULL)
	{
		_codec = GetDefaultCodec();
	}
	
	if(_codec == NULL)
		throw Exception("No codec!!!");
	
	_codec->GetFileInfo(file, _fileInfo);
}

RGBAinputFile::~RGBAinputFile()
{
	if(_fileInfo.iccProfile != NULL)
		free(_fileInfo.iccProfile);
}

template <typename PIXTYPE>
static inline PIXTYPE whiteVal();

template <>
static inline unsigned char whiteVal<unsigned char>()
{
	return UCHAR_MAX;
}

template <>
static inline unsigned short whiteVal<unsigned short>()
{
	return USHRT_MAX;
}

template <typename PIXTYPE>
static void
FillChannelType(Channel &channel, bool fillWhite)
{
	const int colstep = (channel.colbytes / sizeof(PIXTYPE));
	
	const PIXTYPE val = (fillWhite ? whiteVal<PIXTYPE>() : 0);
	
	unsigned char *row = channel.buf;
	
	for(int y=0; y < channel.height; y++)
	{
		PIXTYPE *pix = (PIXTYPE *)(row);
		
		for(int x=0; x < channel.width; x++)
		{
			*pix = val;
			
			pix += colstep;
		}
		
		row += channel.rowbytes;
	}
}

static void
FillChannel(Channel &channel, bool fillWhite)
{
	if(channel.sampleType == USHORT)
	{
		FillChannelType<unsigned short>(channel, fillWhite);
	}
	else
	{
		assert(channel.sampleType == UCHAR);
		
		FillChannelType<unsigned char>(channel, fillWhite);
	}
}

void
RGBAinputFile::ReadFile(RGBAbuffer &buffer, unsigned int subsample)
{
	Channel *channels[4] = { &buffer.r,
								&buffer.g,
								&buffer.b,
								&buffer.a };
									
	ChannelName names[4] = { RED, GREEN, BLUE, ALPHA };
	
	bool assigned[4] = { false, false, false, false };
	
	
	assert(_fileInfo.channels <= J2K_CODEC_MAX_CHANNELS);
	
	Buffer j2kBuffer;
	
	j2kBuffer.channels = _fileInfo.channels;
	
	
	for(int c=0; c < _fileInfo.channels; c++)
	{
		Channel &j2kChan = j2kBuffer.channel[c];
		
		const ChannelName j2kName = _fileInfo.channelMap[c];
		
		for(int i=0; i < 4; i++)
		{
			const ChannelName rgbaName = names[i];
			
			if(j2kName == rgbaName)
			{
				if(assigned[i] == false)
				{
					Channel &rgbaChan = *channels[i];
					
					j2kChan = rgbaChan;
					
					assigned[i] = true;
				}
				else
					assert(false); // channel appears twice?
			}
		}
	}
	
	
	_codec->ReadFile(_file, j2kBuffer, subsample);
		
	
	for(int i=0; i < 4; i++)
	{
		if(assigned[i] == false)
		{
			const bool fillWhite = (names[i] == ALPHA);
			
			FillChannel(*channels[i], fillWhite);
		}
	}
}


RGBAoutputFile::RGBAoutputFile(OutputFile &file, const FileInfo &info, Codec *codec) :
	_file(file),
	_fileInfo(info),
	_codec(codec)
{
	if(_codec == NULL)
	{
		_codec = GetDefaultCodec();
	}
	
	if(_codec == NULL)
		throw Exception("No codec!!!");
		
	if(info.channels == 3)
	{
		assert(info.alpha == NO_ALPHA);
	}
	else
	{
		assert(info.channels == 4);
		assert(info.alpha != NO_ALPHA);
	}
}


void
RGBAoutputFile::WriteFile(RGBAbuffer &buffer)
{
	Channel *channels[4] = { &buffer.r,
								&buffer.g,
								&buffer.b,
								&buffer.a };
									
	ChannelName names[4] = { RED, GREEN, BLUE, ALPHA };
	
	bool assigned[4] = { false, false, false, false };
	
	
	assert(_fileInfo.channels <= J2K_CODEC_MAX_CHANNELS);
	
	Buffer j2kBuffer;
	
	j2kBuffer.channels = _fileInfo.channels;
	
	
	for(int c=0; c < _fileInfo.channels; c++)
	{
		Channel &j2kChan = j2kBuffer.channel[c];
		
		const ChannelName j2kName = _fileInfo.channelMap[c];
		
		for(int i=0; i < 4; i++)
		{
			const ChannelName rgbaName = names[i];
			
			if(j2kName == rgbaName)
			{
				if(assigned[i] == false)
				{
					Channel &rgbaChan = *channels[i];
					
					j2kChan = rgbaChan;
					
					assigned[i] = true;
				}
				else
					assert(false); // channel appears twice?
			}
		}
	}
	
	
	_codec->WriteFile(_file, _fileInfo, j2kBuffer);
}


}; // namespace j2k
