
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

#include "j2k_rgba_file.h"

#include "j2k_exception.h"

#include <assert.h>
#include <algorithm>

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
static inline PIXTYPE ConvertToType(const unsigned char &val);

template <>
static inline unsigned char ConvertToType<unsigned char>(const unsigned char &val)
{
	return val;
}

template <>
static inline unsigned short ConvertToType<unsigned short>(const unsigned char &val)
{
	return (((unsigned short)val << 8) | val);
}

template <typename PIXTYPE>
static void
CopyWithLutType(const RGBAbuffer &buffer, const Channel &idxChan, LUTentry LUT[], unsigned int LUTsize, ChannelName LUTmap[])
{
	const Channel *channels[4] = { &buffer.r,
									&buffer.g,
									&buffer.b,
									&buffer.a };
	
	ChannelName names[4] = { RED, GREEN, BLUE, ALPHA };

	int chanMap[4] = { 0, 1, 2, 3};
	
	bool assigned[4] = { false, false, false, false };
	
	
	for(int c=0; c < 3; c++)
	{
		const ChannelName j2kName = LUTmap[c];
		
		for(int i=0; i < 4; i++)
		{
			const ChannelName rgbaName = names[i];
			
			if(j2kName == rgbaName)
			{
				if(assigned[i] == false)
				{
					chanMap[i] = c;
				}
				else
					assert(false); // channel appears twice?
			}
		}
	}
	
	
	const int idxStep = (idxChan.colbytes / sizeof(unsigned char));
	const int rStep = (channels[0]->colbytes / sizeof(PIXTYPE));
	const int gStep = (channels[1]->colbytes / sizeof(PIXTYPE));
	const int bStep = (channels[2]->colbytes / sizeof(PIXTYPE));
	
	
	for(int y=0; y < idxChan.height; y++)
	{
		unsigned char *idx = (idxChan.buf + (y * idxChan.rowbytes));
		PIXTYPE *r = (PIXTYPE *)(channels[0]->buf + (y * channels[0]->rowbytes));
		PIXTYPE *g = (PIXTYPE *)(channels[1]->buf + (y * channels[1]->rowbytes));
		PIXTYPE *b = (PIXTYPE *)(channels[2]->buf + (y * channels[2]->rowbytes));
		
		for(int x=0; x < idxChan.width; x++)
		{
			const LUTentry &entry = LUT[*idx];
			
			*r = ConvertToType<PIXTYPE>(entry.channel[ chanMap[0] ]);
			*g = ConvertToType<PIXTYPE>(entry.channel[ chanMap[1] ]);
			*b = ConvertToType<PIXTYPE>(entry.channel[ chanMap[2] ]);
			
			idx += idxStep;
			
			r += rStep;
			g += gStep;
			b += bStep;
		}
	}
}

static void
CopyWithLUT(const RGBAbuffer &buffer, const Channel &idxChan, LUTentry LUT[], unsigned int LUTsize, ChannelName LUTmap[])
{
	assert(idxChan.sampleType == UCHAR);
	assert(idxChan.width == buffer.r.width && idxChan.height == buffer.r.height);
	assert(idxChan.width == buffer.g.width && idxChan.height == buffer.g.height);
	assert(idxChan.width == buffer.b.width && idxChan.height == buffer.b.height);
	
	const SampleType sampleType = buffer.r.sampleType;
	
	assert(buffer.g.sampleType == sampleType && buffer.b.sampleType == sampleType);
	
	if(sampleType == USHORT)
	{
		CopyWithLutType<unsigned short>(buffer, idxChan, LUT, LUTsize, LUTmap);
	}
	else
	{
		assert(sampleType == UCHAR);
		
		CopyWithLutType<unsigned char>(buffer, idxChan, LUT, LUTsize, LUTmap);
	}
}


template <typename PIXTYPE>
static PIXTYPE Clamp(const int &val, const int &max)
{
	return std::min<int>(std::max<int>(val, 0), max);
}

template <typename PIXTYPE>
static void
FullsYCCtoRGBType(const RGBAbuffer &rgbBuffer, const Buffer &yccBuffer, bool reversible)
{
#define ALPHA_R 0.299 // These are exact expressions from which the
#define ALPHA_G 0.587 // ICT forward and reverse transform coefficients
#define ALPHA_B 0.114 // may be expressed.

#define CR_FACT_R (2*(1-ALPHA_R))
#define CB_FACT_B (2*(1-ALPHA_B))
#define CR_FACT_G (2*ALPHA_R*(1-ALPHA_R)/ALPHA_G)
#define CB_FACT_G (2*ALPHA_B*(1-ALPHA_B)/ALPHA_G)

#define CR_FACT_R14 ((A_long)(0.5 + CR_FACT_R*(1<<14)))
#define CB_FACT_B14 ((A_long)(0.5 + CB_FACT_B*(1<<14)))
#define CR_FACT_G14 ((A_long)(0.5 + CR_FACT_G*(1<<14)))
#define CB_FACT_G14 ((A_long)(0.5 + CB_FACT_G*(1<<14)))

	const int width = rgbBuffer.r.width;
	const int height = rgbBuffer.r.height;
	
	const int depth = rgbBuffer.r.depth;
	const bool sgnd = rgbBuffer.r.sgnd;
	
	const int maxVal = (sgnd ? pow(2, depth - 1) - 1 : pow(2, depth) - 1);
	const int signedDiff = (sgnd ? 0 : pow(2, depth - 1));
	
	const Channel &yChan = yccBuffer.channel[0];
	const Channel &cbChan = yccBuffer.channel[1];
	const Channel &crChan = yccBuffer.channel[2];
	
	const Channel &rChan = rgbBuffer.r;
	const Channel &gChan = rgbBuffer.g;
	const Channel &bChan = rgbBuffer.b;
	
	const int yStep = (yChan.colbytes / sizeof(PIXTYPE));
	const int cbStep = (cbChan.colbytes / sizeof(PIXTYPE));
	const int crStep = (crChan.colbytes / sizeof(PIXTYPE));
	
	const int rStep = (rChan.colbytes / sizeof(PIXTYPE));
	const int gStep = (gChan.colbytes / sizeof(PIXTYPE));
	const int bStep = (bChan.colbytes / sizeof(PIXTYPE));
	
	for(int y=0; y < height; y++)
	{
		const PIXTYPE *cy= (PIXTYPE *)(yChan.buf + (y * yChan.rowbytes));
		const PIXTYPE *cb = (PIXTYPE *)(cbChan.buf + (y * cbChan.rowbytes));
		const PIXTYPE *cr = (PIXTYPE *)(crChan.buf + (y * crChan.rowbytes));
		
		PIXTYPE *r = (PIXTYPE *)(rChan.buf + (y * rChan.rowbytes));
		PIXTYPE *g = (PIXTYPE *)(gChan.buf + (y * gChan.rowbytes));
		PIXTYPE *b = (PIXTYPE *)(bChan.buf + (y * bChan.rowbytes));
		
		for(int x=0; x < width; x++)
		{
			// make signed
			const int sY = (int)*cy - signedDiff;
			const int sCb = (int)*cb - signedDiff;
			const int sCr = (int)*cr - signedDiff;
			
			if(reversible)
			{
				const int sG = sY - ((sCb + sCr) >> 2);
				const int sR = sG + sCr;
				const int sB = sG + sCb;
				
				*r = Clamp<PIXTYPE>(sR + signedDiff, maxVal);
				*g = Clamp<PIXTYPE>(sG + signedDiff, maxVal);
				*b = Clamp<PIXTYPE>(sB + signedDiff, maxVal);
			}
			else
			{
				// TODO: is there something about R14 to go here?
				
				const float sR = (float)sY + ((float)CR_FACT_R * (float)sCr);
				const float sG = (float)sY - ((float)CR_FACT_G * (float)sCr) - ((float)CB_FACT_G * (float)sCb);
				const float sB = (float)sY + ((float)CB_FACT_B * (float)sCb);
				
				*r = Clamp<PIXTYPE>(sR + signedDiff + 0.5f, maxVal);
				*g = Clamp<PIXTYPE>(sG + signedDiff + 0.5f, maxVal);
				*b = Clamp<PIXTYPE>(sB + signedDiff + 0.5f, maxVal);
			}
			
			
			cy += yStep;
			cb += cbStep;
			cr += crStep;
			
			r += rStep;
			g += gStep;
			b += bStep;
		}
	}
}

static void
FullsYCCtoRGB(const RGBAbuffer &rgbBuffer, const Buffer &yccBuffer, bool reversible)
{
	const SampleType sampleType = rgbBuffer.r.sampleType;
	
	assert(yccBuffer.channel[0].sampleType == sampleType);
	
	assert(rgbBuffer.r.depth == yccBuffer.channel[0].depth);
	
	assert(yccBuffer.channel[1].subsampling.x == 1 && yccBuffer.channel[1].subsampling.y == 1);
	assert(yccBuffer.channel[2].subsampling.x == 1 && yccBuffer.channel[2].subsampling.y == 1);

	if(sampleType == USHORT)
	{
		FullsYCCtoRGBType<unsigned short>(rgbBuffer, yccBuffer, reversible);
	}
	else
	{
		assert(sampleType == UCHAR);
		
		FullsYCCtoRGBType<unsigned char>(rgbBuffer, yccBuffer, reversible);
	}
}

static void
sYCCtoRGB(const RGBAbuffer &rgbBuffer, const Buffer &yccBuffer, bool reversible)
{
	assert(yccBuffer.channels >= 3);
	
	const SampleType sampleType = rgbBuffer.r.sampleType;
	const unsigned char depth = rgbBuffer.r.depth;
	const bool sgnd = rgbBuffer.r.sgnd;
	
	bool reuseBuffer = true;
	
	for(int i=0; i < 3; i++)
	{
		const Channel &yccChan = yccBuffer.channel[i];
		
		if(yccChan.subsampling.x != 1 ||
			yccChan.subsampling.y != 1 ||
			yccChan.sampleType != sampleType ||
			yccChan.depth != depth ||
			yccChan.sgnd != sgnd)
		{
			reuseBuffer = false;
		}
	}
	
	
	Buffer tempBuffer;
	
	const Buffer &fullYccBuffer = (reuseBuffer ? yccBuffer : tempBuffer);
	
	if(!reuseBuffer)
	{
		// need to make a non-subsampled YCC buffer with the same bit depth as the output
		tempBuffer.channels = yccBuffer.channels;
		
		for(int i=0; i < tempBuffer.channels; i++)
		{
			const Channel &subsampledChannel = yccBuffer.channel[i];
			
			Channel &tempChannel = tempBuffer.channel[i];
			
			tempChannel.width = (subsampledChannel.width * subsampledChannel.subsampling.x);
			tempChannel.height = (subsampledChannel.height * subsampledChannel.subsampling.y);
			
			tempChannel.subsampling.x = tempChannel.subsampling.y = 1;
			
			tempChannel.sampleType = sampleType;
			tempChannel.depth = depth;
			tempChannel.sgnd = sgnd;
			
			tempChannel.colbytes = SizeOfSample(tempChannel.sampleType);
			tempChannel.rowbytes = (tempChannel.colbytes * tempChannel.width);
			tempChannel.buf = (unsigned char *)malloc(tempChannel.rowbytes * tempChannel.height);
			
			if(tempChannel.buf == NULL)
				throw Exception("out of memory");
		}
		
		Codec::CopyBuffer(tempBuffer, yccBuffer);
	}
	
	
	FullsYCCtoRGB(rgbBuffer, fullYccBuffer, false); // for sYCC I think we always do irreversible
	
	
	for(int i=0; i < tempBuffer.channels; i++)
	{
		Channel &tempChan = tempBuffer.channel[i];
		
		if(tempChan.buf != NULL)
		{
			free(tempChan.buf);
		}
	}
}


template <typename PIXTYPE>
static void
FillChannelType(Channel &channel, bool fillWhite)
{
	const int colstep = (channel.colbytes / sizeof(PIXTYPE));
	
	const PIXTYPE val = (fillWhite ? (pow(2, channel.depth) - 1) : 0);
	
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
	
	const bool hasPal = (_fileInfo.LUTsize > 0);
	
	const int effectiveChannels = ((hasPal && (_codec->GetReadFlags() & Codec::J2K_APPLIES_LUT)) ? 3 : _fileInfo.channels);
	
	const bool isRGB = (effectiveChannels >= 3) && (_fileInfo.colorSpace == sRGB ||
													_fileInfo.colorSpace == iccRGB ||
													_fileInfo.colorSpace == UNKNOWN_COLOR_SPACE);
	
	bool channelSubsampling = false;
	
	for(int i=0; i < _fileInfo.channels; i++)
	{
		if(_fileInfo.subsampling[i].x != 1 || _fileInfo.subsampling[i].x != 1)
			channelSubsampling = true;
	}
	
	
	const bool reuseChannels = (isRGB && !channelSubsampling);
	
	Buffer j2kBuffer;
	
	if(reuseChannels)
	{
		bool assigned[4] = { false, false, false, false };
		
		// can just point to RGB channels
		assert(_fileInfo.channels <= J2K_CODEC_MAX_CHANNELS);
		
		j2kBuffer.channels = effectiveChannels;
		
		for(int c=0; c < effectiveChannels; c++)
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
						
						assert(rgbaChan.width == _fileInfo.width);
						assert(rgbaChan.height == _fileInfo.height);
					}
					else
						assert(false); // channel appears twice?
				}
			}
		}
	}
	else
	{
		// got to make our own
		assert(effectiveChannels <= J2K_CODEC_MAX_CHANNELS);
		
		const SampleType destType = buffer.r.sampleType;
		const unsigned char destDepth = buffer.r.depth;
		const bool destSgnd = buffer.r.sgnd;
		
		j2kBuffer.channels = std::min<unsigned char>(effectiveChannels, J2K_CODEC_MAX_CHANNELS);
		
		for(int i=0; i < j2kBuffer.channels; i++)
		{
			Channel &j2kChan = j2kBuffer.channel[i];
			
			const Subsampling &sub = _fileInfo.subsampling[i];
			
			j2kChan.width = SubsampledSize(_fileInfo.width, sub.x);
			j2kChan.height = SubsampledSize(_fileInfo.height, sub.y);
			
			j2kChan.subsampling = sub;
			
			j2kChan.sampleType = destType;
			j2kChan.depth = destDepth;
			j2kChan.sgnd = destSgnd;
			
			j2kChan.colbytes = SizeOfSample(j2kChan.sampleType);
			j2kChan.rowbytes = (j2kChan.colbytes * j2kChan.width);
			
			j2kChan.buf = (unsigned char *)malloc(j2kChan.rowbytes * j2kChan.height);
			
			if(j2kChan.buf == NULL)
				throw Exception("out of memory?");
		}
	}
	
	
	_codec->ReadFile(_file, j2kBuffer, subsample);
		
	
	bool haveAlpha = false;
	
	if(reuseChannels)
	{
		haveAlpha = (effectiveChannels == 4);
	}
	else
	{
		assert(effectiveChannels == j2kBuffer.channels);
		
		if(isRGB)
		{
			haveAlpha = (effectiveChannels == 4);
		
			assert(channelSubsampling);
			
			bool assigned[4] = { false, false, false, false };
			
			assert(effectiveChannels <= J2K_CODEC_MAX_CHANNELS);
			
			Buffer j2kRgbBuffer;
			
			assert(j2kBuffer.channels <= 4);
			
			j2kRgbBuffer.channels = j2kBuffer.channels;
			
			for(int c=0; c < j2kBuffer.channels; c++)
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
							Channel &rgbaChan = j2kRgbBuffer.channel[i];
							
							rgbaChan = j2kChan;
						}
						else
							assert(false); // channel appears twice?
					}
				}
			}
			
			
			Buffer destRgbBuffer;
			
			destRgbBuffer.channels = 4;
			
			destRgbBuffer.channel[0] = buffer.r;
			destRgbBuffer.channel[1] = buffer.g;
			destRgbBuffer.channel[2] = buffer.b;
			destRgbBuffer.channel[3] = buffer.a;
			
			
			Codec::CopyBuffer(destRgbBuffer, j2kRgbBuffer);
		}
		else if(effectiveChannels == 1 || effectiveChannels == 2)
		{
			if(hasPal)
			{
				assert(effectiveChannels == 1);
			
				CopyWithLUT(buffer, j2kBuffer.channel[0], _fileInfo.LUT, _fileInfo.LUTsize, _fileInfo.LUTmap);
			}
			else
			{
				assert(_fileInfo.colorSpace == sLUM ||
						_fileInfo.colorSpace == iccLUM ||
						_fileInfo.colorSpace == UNKNOWN_COLOR_SPACE);
			
				Buffer sourceBuffer;
				
				haveAlpha = (effectiveChannels == 2);
				
				sourceBuffer.channels = (haveAlpha ? 4 : 3);
				
				sourceBuffer.channel[0] = sourceBuffer.channel[1] = sourceBuffer.channel[2] = j2kBuffer.channel[0];
				
				if(haveAlpha)
					sourceBuffer.channel[3] = j2kBuffer.channel[1];
				
				
				Buffer destinationBuffer;
				
				destinationBuffer.channels = 4;
				
				destinationBuffer.channel[0] = buffer.r;
				destinationBuffer.channel[1] = buffer.g;
				destinationBuffer.channel[2] = buffer.b;
				destinationBuffer.channel[3] = buffer.a;
					
				
				Codec::CopyBuffer(destinationBuffer, sourceBuffer);
			}
		}
		else if(_fileInfo.colorSpace == sYCC)
		{
			assert(effectiveChannels >= 3);
			
			sYCCtoRGB(buffer, j2kBuffer, _fileInfo.settings.reversible);
		}
		else
			assert(false);
		
		
		for(int i=0; i < j2kBuffer.channels; i++)
		{
			Channel &j2kChan = j2kBuffer.channel[i];
			
			if(j2kChan.buf != NULL)
			{
				free(j2kChan.buf);
			}
		}
	}
	
	
	if(!haveAlpha)
		FillChannel(buffer.a, true);
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
	
	assert(_fileInfo.colorSpace != sYCC); // TODO: RGB to sYCC conversion
	
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
