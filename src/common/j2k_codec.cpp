
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

#include "j2k_codec.h"

#include "j2k_exception.h"

#include "j2k_grok_codec.h"
#include "j2k_openjpeg_codec.h"

#ifdef J2K_USE_KAKADU
	#include "j2k_kakadu_codec.h"
#endif

#include "lcms2.h"

#ifdef __APPLE__
	#include <mach/mach.h>
#endif

#ifdef WIN32
	#include <Windows.h>
#endif

#include <algorithm>
#include <limits>

#include <assert.h>

namespace j2k
{


bool
Codec::Verify(InputFile &file)
{
	return (GetFileFormat(file) != UNKNOWN_FORMAT);
}


static inline unsigned int
PlatformSwap(const unsigned int &v)
{
#if defined(__ppc__)
	return v;
#else
	return ((v >> 24) | ((v >> 8) & 0xff00) | ((v << 8) & 0xff0000) | (v << 24));
#endif
}

static inline bool
FourCharCodeCompare(const char *buf, const char *str)
{
	return (buf[0] == str[0] &&
			buf[1] == str[1] &&
			buf[2] == str[2] &&
			buf[3] == str[3]);
}

static inline bool
IntCompare(const unsigned int &v, const char *str)
{
	unsigned int i = v;
	
	const char *s = (const char *)&i;
	
	return FourCharCodeCompare(s, str);
}

static inline bool
IntCompare(const unsigned int &v, const unsigned int &y)
{
	return (v == PlatformSwap(y));
}


Format
Codec::GetFileFormat(InputFile &file)
{
	Format format = UNKNOWN_FORMAT;
	

	file.Seek(0);
	
	unsigned char buf[24];
	unsigned int *i_buf = (unsigned int *)buf;
	
	const size_t buf_read = file.Read(buf, 24);
	
	assert(buf_read == 24);
	
	if(IntCompare(i_buf[0], 0x0000000c) &&	// length of jp2 superbox signature: 12
		IntCompare(i_buf[1], "jP  ") &&		// JP2 4cc
		IntCompare(i_buf[2], 0x0d0a870a) && // JP2 signature
		//IntCompare(i_buf[3], 0x00000014) &&	// next box length: 20
		IntCompare(i_buf[4], "ftyp"))		// JP2 file type
	{
		// JP2 family
		if(IntCompare(i_buf[5], "jp2 ") ||
			IntCompare(i_buf[5], "J2P0") ||
			IntCompare(i_buf[5], "J2P1"))
		{
			format = JP2;
		}
		else if(IntCompare(i_buf[5], "jpx ") ||
				IntCompare(i_buf[5], "jpxb"))
		{
			format = JPX;
		}
	}
	else if(buf[0] == 0xff &&
			buf[1] == 0x4f)
	{
		format = J2C;
	}
	
	
	return format;
}

static inline cmsUInt16Number
SwapEndian(cmsUInt16Number in)
{
#ifdef __ppc__
	return in;
#else
	return ( (in >> 8) | (in << 8) );
#endif
}

void *
Codec::CreateProfile(ColorSpace colorSpace, size_t &profileSize)
{
	void *profile = NULL;

	assert(colorSpace == sRGB);
	
	cmsHPROFILE iccH = cmsCreate_sRGBProfile();
	
	if(iccH)
	{
		cmsUInt32Number icc_profile_len;
		
		if( cmsSaveProfileToMem(iccH, NULL, &icc_profile_len) )
		{
			profile = malloc(icc_profile_len);
			
			if(profile)
			{
				cmsSaveProfileToMem(iccH, profile, &icc_profile_len);

				profileSize = icc_profile_len;
				
				// The profile written to memory is given a timestamp, which will make
				// profiles that should be identical slightly different, which will
				// mess with AE.  We're going to hard code our own time instead.
				
				cmsICCHeader *icc_header = (cmsICCHeader *)profile;
				
				icc_header->date.year	= SwapEndian(2012);
				icc_header->date.month	= SwapEndian(12);
				icc_header->date.day	= SwapEndian(12);
				icc_header->date.hours	= SwapEndian(12);
				icc_header->date.minutes= SwapEndian(12);
				icc_header->date.seconds= SwapEndian(12);
			}
		}
		
		cmsCloseProfile(iccH);
	}
	
	return profile;
}


bool
Codec::IssRGBProfile(const void *iccProfile, size_t profileSize)
{
	bool result = FALSE;
	
	cmsHPROFILE iccH = cmsOpenProfileFromMem(iccProfile, profileSize);
	
	if(iccH)
	{
		char name[256];
		cmsUInt32Number namelen = cmsGetProfileInfoASCII(iccH, cmsInfoDescription,
														"en", cmsNoCountry, name, 255);
		if(namelen)
		{
			result = ( !strcmp(name, "sRGB IEC61966-2.1") ||
						!strcmp(name, "sRGB built-in"));
		}
	
		cmsCloseProfile(iccH);
	}
	
	return result;
}


template <typename DESTTYPE, typename SRCTYPE>
static void
CopyChannel(const Channel &dest, const Channel &src)
{
	const int height = dest.height;
	const int width = dest.width;
	
	assert(dest.subsampling.x == 1 && dest.subsampling.y == 1); // not handling destination subsampling right now
	
	const int destStep = (dest.colbytes / sizeof(DESTTYPE));
	const int srcStep = (src.colbytes / sizeof(SRCTYPE));

	assert(dest.depth <= (sizeof(DESTTYPE) * 8)); // make sure the specified depth
	assert(src.depth <= (sizeof(SRCTYPE) * 8));   // fits in the specified type

	assert(!dest.sgnd || std::numeric_limits<DESTTYPE>::is_signed); // in other words, a signed channel
	assert(!src.sgnd || std::numeric_limits<SRCTYPE>::is_signed);   // should have a signed type
	
	const int bitShift = ((int)dest.depth - (int)src.depth);
	
	
	unsigned char *destRow = dest.buf;
	unsigned char *srcRow = src.buf;
	
	for(int y=1; y <= height; y++)
	{
		DESTTYPE *d = (DESTTYPE *)destRow;
		SRCTYPE *s = (SRCTYPE *)srcRow;
		
		if(dest.sgnd == src.sgnd)
		{
			if(bitShift == 0)
			{
				// no shift
				for(int x=1; x <= width; x++)
				{
					*d = *s;
					
					d += destStep;
					
					if(x % src.subsampling.x == 0)
						s += srcStep;
				}
			}
			else if(bitShift > 0)
			{
				// upshift
				if(src.depth >= 8)
				{
					assert(bitShift <= 24);
					
					if(bitShift <= src.depth)
					{
						// so we just have to repeat some bits in the gap
						const int fillDownshift =  (src.depth - bitShift);
					
						for(int x=1; x <= width; x++)
						{
							*d = ( ((DESTTYPE)*s << bitShift) | (*s >> fillDownshift) );
							
							d += destStep;
							
							if(x % src.subsampling.x == 0)
								s += srcStep;
						}
					}
					else
					{
						// have to do two fills
						// the first one doubles the bit depth
						// making the second one like the one above
						const int firstShift = src.depth;
						const int secondShift = (bitShift - firstShift);
						const int fillDownshift = ((src.depth * 2) - secondShift);
						
						for(int x=1; x <= width; x++)
						{
							const DESTTYPE t = (((DESTTYPE)*s << firstShift) | *s);
						
							*d = ( (t << secondShift) | (t >> fillDownshift) );
							
							d += destStep;
							
							if(x % src.subsampling.x == 0)
								s += srcStep;
						}
					}
				}
				else
					assert(false); // TODO: write upshift for < 8 bit
			}
			else
			{
				// downshift
				const int downShift = -bitShift;
				
				for(int x=1; x <= width; x++)
				{
					*d = (*s >> downShift);
					
					d += destStep;
					
					if(x % src.subsampling.x == 0)
						s += srcStep;
				}
			}
		}
		else
			assert(false); // TODO: write singed-unsigned conversions
		
		destRow += dest.rowbytes;
		
		if(y % src.subsampling.y == 0)
			srcRow += src.rowbytes;
	}
}

template <typename DESTTYPE>
static void
CopyChannel(const Channel &dest, const Channel &src)
{
	if(src.sampleType == UCHAR)
	{
		CopyChannel<DESTTYPE, unsigned char>(dest, src);
	}
	else if(src.sampleType == USHORT)
	{
		CopyChannel<DESTTYPE, unsigned short>(dest, src);
	}
	else if(src.sampleType == UINT)
	{
		CopyChannel<DESTTYPE, unsigned int>(dest, src);
	}
	else if(src.sampleType == INT)
	{
		CopyChannel<DESTTYPE, int>(dest, src);
	}
}

void
Codec::CopyBuffer(const Buffer &destination, const Buffer &source)
{
	for(int i=0; i < destination.channels && i < source.channels; i++)
	{
		const Channel &dest = destination.channel[i];
		const Channel &src = source.channel[i];
		
		if(dest.sampleType == UCHAR)
		{
			CopyChannel<unsigned char>(dest, src);
		}
		else if(dest.sampleType == USHORT)
		{
			CopyChannel<unsigned short>(dest, src);
		}
		else if(dest.sampleType == UINT)
		{
			CopyChannel<unsigned int>(dest, src);
		}
		else if(dest.sampleType == INT)
		{
			CopyChannel<int>(dest, src);
		}
	}
}


unsigned int
Codec::NumberOfCPUs()
{
	static unsigned int cpus = 0;
	
	if(cpus == 0)
	{
	#ifdef __APPLE__
		// get number of CPUs using Mach calls
		host_basic_info_data_t hostInfo;
		mach_msg_type_number_t infoCount;
		
		infoCount = HOST_BASIC_INFO_COUNT;
		host_info(mach_host_self(), HOST_BASIC_INFO, 
				  (host_info_t)&hostInfo, &infoCount);
		
		cpus = hostInfo.avail_cpus;
	#else // WIN_ENV
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);

		cpus = systemInfo.dwNumberOfProcessors;
	#endif
	}
	
	return cpus;
}


size_t
SizeOfSample(SampleType type)
{
	switch(type)
	{
		case UCHAR:		return sizeof(unsigned char);
		case USHORT:	return sizeof(unsigned short);
		case UINT:		return sizeof(unsigned int);
		case INT:		return sizeof(int);
		
		default:
			throw Exception("invalid type!");
	}
}


unsigned int
SubsampledSize(unsigned int size, int subsampling)
{
	assert(subsampling > 0);

	if(subsampling == 1)
		return size;
		
	return ceil((double)size / (double)subsampling);
}


static bool CodecCompare(const Codec *first, const Codec *second)
{
	const std::string s1 = first->Name();
	const std::string s2 = second->Name();
	
	return (s1 < s2);
}


class CodecContainer
{
  public:
	CodecContainer();
	~CodecContainer();
	
	const CodecList & GetCodecList() const { return _codecList; }
	
  private:
	CodecList _codecList;
};

CodecContainer::CodecContainer()
{
	// un-comment the line below to start testing the Grok codec!
	//_codecList.push_back(new GrokCodec);
	_codecList.push_back(new OpenJPEGCodec);
	
#ifdef J2K_USE_KAKADU
	_codecList.push_back(new KakaduCodec);
#endif
	
	_codecList.sort(CodecCompare);
}

CodecContainer::~CodecContainer()
{
	for(CodecList::iterator i = _codecList.begin() ; i != _codecList.end(); ++i)
	{
		Codec *codec = *i;
	
		delete codec;
	}
}

static CodecContainer g_CodecContainer;


const CodecList & GetCodecList()
{
	return g_CodecContainer.GetCodecList();
}


Codec * GetDefaultCodec()
{
	const CodecList &codecList = GetCodecList();
	
	if(codecList.size() == 0)
		throw Exception("No codecs!");
	
	return codecList.front();
}

}; // namespace j2k
