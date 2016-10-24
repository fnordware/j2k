
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
	
	
	const int idxStep = static_cast<const int>(idxChan.colbytes / sizeof(unsigned char));
	const int rStep = static_cast<const int>(channels[0]->colbytes / sizeof(PIXTYPE));
	const int gStep = static_cast<const int>(channels[1]->colbytes / sizeof(PIXTYPE));
	const int bStep = static_cast<const int>(channels[2]->colbytes / sizeof(PIXTYPE));
	
	
	for(unsigned int y=0; y < idxChan.height; y++)
	{
		unsigned char *idx = (idxChan.buf + (y * idxChan.rowbytes));
		PIXTYPE *r = (PIXTYPE *)(channels[0]->buf + (y * channels[0]->rowbytes));
		PIXTYPE *g = (PIXTYPE *)(channels[1]->buf + (y * channels[1]->rowbytes));
		PIXTYPE *b = (PIXTYPE *)(channels[2]->buf + (y * channels[2]->rowbytes));
		
		for(unsigned int x=0; x < idxChan.width; x++)
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


typedef struct
{
	Channel y;
	Channel cb;
	Channel cr;
	
} YCCbuffer;

enum YccChannelName
{
	Y = RED,
	CB = GREEN,
	CR = BLUE
};

template <typename PIXTYPE>
static PIXTYPE Clamp(const int &val, const int &max)
{
	return std::min<int>(std::max<int>(val, 0), max);
}

template <typename PIXTYPE>
static void
FullsYCCtoRGBType(const RGBAbuffer &rgbBuffer, const YCCbuffer &yccBuffer, bool reversible)
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
	
	const int maxVal = static_cast<const int>(sgnd ? pow(2, depth - 1) - 1 : pow(2, depth) - 1);
	const int signedDiff = static_cast<const int>(sgnd ? 0 : pow(2, depth - 1));
	
	const Channel &yChan = yccBuffer.y;
	const Channel &cbChan = yccBuffer.cb;
	const Channel &crChan = yccBuffer.cr;
	
	const Channel &rChan = rgbBuffer.r;
	const Channel &gChan = rgbBuffer.g;
	const Channel &bChan = rgbBuffer.b;
	
	const int yStep = static_cast<const int>(yChan.colbytes / sizeof(PIXTYPE));
	const int cbStep = static_cast<const int>(cbChan.colbytes / sizeof(PIXTYPE));
	const int crStep = static_cast<const int>(crChan.colbytes / sizeof(PIXTYPE));
	
	const int rStep = static_cast<const int>(rChan.colbytes / sizeof(PIXTYPE));
	const int gStep = static_cast<const int>(gChan.colbytes / sizeof(PIXTYPE));
	const int bStep = static_cast<const int>(bChan.colbytes / sizeof(PIXTYPE));
	
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
				
				*r = Clamp<PIXTYPE>(static_cast<const int>(sR + signedDiff + 0.5f), maxVal);
				*g = Clamp<PIXTYPE>(static_cast<const int>(sG + signedDiff + 0.5f), maxVal);
				*b = Clamp<PIXTYPE>(static_cast<const int>(sB + signedDiff + 0.5f), maxVal);
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
FullsYCCtoRGB(const RGBAbuffer &rgbBuffer, const YCCbuffer &yccBuffer, bool reversible)
{
	const SampleType sampleType = rgbBuffer.r.sampleType;
	
	assert(yccBuffer.y.sampleType == sampleType);
	
	assert(rgbBuffer.r.depth == yccBuffer.y.depth);
	
	assert(yccBuffer.cb.subsampling.x == 1 && yccBuffer.cb.subsampling.y == 1);
	assert(yccBuffer.cr.subsampling.x == 1 && yccBuffer.cr.subsampling.y == 1);

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
CopyBuffer(const YCCbuffer &yccDest, const YCCbuffer &yccSrc)
{
	Buffer dst;
	
	dst.channels = 3;
	dst.channel[0] = yccDest.y;
	dst.channel[1] = yccDest.cb;
	dst.channel[2] = yccDest.cr;
	
	
	Buffer src;
	
	src.channels = 3;
	src.channel[0] = yccSrc.y;
	src.channel[1] = yccSrc.cb;
	src.channel[2] = yccSrc.cr;
	
	
	Codec::CopyBuffer(dst, src);
}

static void
sYCCtoRGB(const RGBAbuffer &rgbBuffer, const YCCbuffer &yccBuffer, bool reversible)
{
	const SampleType sampleType = rgbBuffer.r.sampleType;
	const unsigned char depth = rgbBuffer.r.depth;
	const bool sgnd = rgbBuffer.r.sgnd;
	
	bool reuseBuffer = true;
	
	const Channel *ycc_channels[3] = { &yccBuffer.y,
										&yccBuffer.cb,
										&yccBuffer.cr };
	
	for(int i=0; i < 3; i++)
	{
		const Channel &yccChan = *ycc_channels[i];
		
		if(yccChan.subsampling.x != 1 ||
			yccChan.subsampling.y != 1 ||
			yccChan.sampleType != sampleType ||
			yccChan.depth != depth ||
			yccChan.sgnd != sgnd)
		{
			reuseBuffer = false;
		}
	}
	
	
	YCCbuffer tempBuffer;

	const YCCbuffer &fullYccBuffer = (reuseBuffer ? yccBuffer : tempBuffer);
	
	if(!reuseBuffer)
	{
		// need to make a non-subsampled YCC buffer with the same bit depth as the output
		Channel *temp_channels[3] = { &tempBuffer.y,
										&tempBuffer.cb,
										&tempBuffer.cr };
		
		for(int i=0; i < 3; i++)
		{
			const Channel &subsampledChannel = *ycc_channels[i];
			
			Channel &tempChannel = *temp_channels[i];
			
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
		
		CopyBuffer(tempBuffer, yccBuffer);
	}
	
	
	FullsYCCtoRGB(rgbBuffer, fullYccBuffer, false); // for sYCC I think we always do irreversible
	
	
	// free any temp buffers
	if(tempBuffer.y.buf != NULL)
		free(tempBuffer.y.buf);

	if(tempBuffer.cb.buf != NULL)
		free(tempBuffer.cb.buf);

	if(tempBuffer.cr.buf != NULL)
		free(tempBuffer.cr.buf);	
}


template <typename PIXTYPE>
static void
FillChannelType(Channel &channel, bool fillWhite)
{
	if(channel.buf == NULL)
		return;

	const int colstep = static_cast<const int>(channel.colbytes / sizeof(PIXTYPE));
	
	const PIXTYPE val = static_cast<const uint8_t>(fillWhite ? (pow(2, channel.depth) - 1) : 0);
	
	unsigned char *row = channel.buf;
	
	for(unsigned int y=0; y < channel.height; y++)
	{
		PIXTYPE *pix = (PIXTYPE *)(row);
		
		for(unsigned int x=0; x < channel.width; x++)
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
RGBAinputFile::ReadFile(RGBAbuffer &buffer, unsigned int subsample, Progress *progress)
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
						
						assigned[i] = true;
						
						assert(subsample * rgbaChan.width == _fileInfo.width);
						assert(subsample * rgbaChan.height == _fileInfo.height);
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
	
	
	_codec->ReadFile(_file, j2kBuffer, subsample, progress);
		
	
	
#define NOABORT() (progress == NULL ? true : \
					!progress->keepGoing ? false : \
					progress->abortProc == NULL ? true : \
						(progress->keepGoing = progress->abortProc(progress->refCon)))
	
	bool haveAlpha = false;
	
	if(reuseChannels)
	{
		haveAlpha = (effectiveChannels == 4);
	}
	else if( NOABORT() )
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
							
							assigned[i] = true;
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
			
			YCCbuffer yccBuffer;
			
			Channel *ycc_channels[3] = { &yccBuffer.y,
											&yccBuffer.cb,
											&yccBuffer.cr };
											
			YccChannelName ycc_names[3] = { Y, CB, CR };
			
			bool ycc_assigned[3] = { false, false, false };
			
			for(int c=0; c < 3; c++)
			{
				Channel &j2kChan = j2kBuffer.channel[c];
				
				const ChannelName j2kName = _fileInfo.channelMap[c];
				
				for(int i=0; i < 3; i++)
				{
					const YccChannelName yccName = ycc_names[i];
					
					if(j2kName == (ChannelName)yccName)
					{
						if(ycc_assigned[i] == false)
						{
							Channel &yccChan = *ycc_channels[i];
							
							yccChan = j2kChan;
							
							ycc_assigned[i] = true;
						}
						else
							assert(false); // channel appears twice?
					}
				}
			}
			
			assert(ycc_assigned[0] == true && ycc_assigned[1] == true && ycc_assigned[2] == true);
			
			
			sYCCtoRGB(buffer, yccBuffer, _fileInfo.settings.reversible);
		}
		else
			assert(false);
		
		
	}
	
	if(!reuseChannels)
	{
		for(int i=0; i < j2kBuffer.channels; i++)
		{
			Channel &j2kChan = j2kBuffer.channel[i];
			
			if(j2kChan.buf != NULL)
			{
				free(j2kChan.buf);
			}
		}
	}
	
	if(!haveAlpha && NOABORT())
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
