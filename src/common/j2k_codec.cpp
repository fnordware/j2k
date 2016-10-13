/*
 *  j2k_codec.cpp
 *  j2k_AE
 *
 *  Created by Brendan Bolles on 10/9/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

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
static inline DESTTYPE Convert(const SRCTYPE &s);

template <>
static inline unsigned char
Convert<unsigned char, unsigned char>(const unsigned char &s)
{
	return s;
}

template <>
static inline unsigned char
Convert<unsigned char, unsigned short>(const unsigned short &s)
{
	return (s >> 8);
}

template <>
static inline unsigned char
Convert<unsigned char, unsigned int>(const unsigned int &s)
{
	return (s >> 24);
}

template <>
static inline unsigned short
Convert<unsigned short, unsigned char>(const unsigned char &s)
{
	return ((unsigned short)s << 8 | s);
}

template <>
static inline unsigned short
Convert<unsigned short, unsigned short>(const unsigned short &s)
{
	return s;
}

template <>
static inline unsigned short
Convert<unsigned short, unsigned int>(const unsigned int &s)
{
	return (s >> 16);
}

template <>
static inline unsigned int
Convert<unsigned int, unsigned char>(const unsigned char &s)
{
	return ((unsigned int)s << 24 | (unsigned int)s << 16 | (unsigned int)s << 8 | s);
}

template <>
static inline unsigned int
Convert<unsigned int, unsigned short>(const unsigned short &s)
{
	return ((unsigned int)s << 16 | s);
}

template <>
static inline unsigned int
Convert<unsigned int, unsigned int>(const unsigned int &s)
{
	return s;
}

template <typename DESTTYPE, typename SRCTYPE>
static void
CopyChannel(const Channel &dest, const Channel &src)
{
	const int height = std::min<int>(dest.height, src.height);
	const int width = std::min<int>(dest.width, src.width);
	
	const int destStep = (dest.colbytes / sizeof(DESTTYPE));
	const int srcStep = (src.colbytes / sizeof(SRCTYPE));

	for(int y=0; y < height; y++)
	{
		DESTTYPE *d = (DESTTYPE *)(dest.buf + (y * dest.rowbytes));
		SRCTYPE *s = (SRCTYPE *)(src.buf + (y * src.rowbytes));
		
		for(int x=0; x < width; x++)
		{
			*d = Convert<DESTTYPE, SRCTYPE>(*s);
			
			d += destStep;
			s += srcStep;
		}
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
			if(src.sampleType == UCHAR)
			{
				CopyChannel<unsigned char, unsigned char>(dest, src);
			}
			else if(src.sampleType == USHORT)
			{
				CopyChannel<unsigned char, unsigned short>(dest, src);
			}
			else if(src.sampleType == UINT)
			{
				CopyChannel<unsigned char, unsigned int>(dest, src);
			}
		}
		else if(dest.sampleType == USHORT)
		{
			if(src.sampleType == UCHAR)
			{
				CopyChannel<unsigned short, unsigned char>(dest, src);
			}
			else if(src.sampleType == USHORT)
			{
				CopyChannel<unsigned short, unsigned short>(dest, src);
			}
			else if(src.sampleType == UINT)
			{
				CopyChannel<unsigned short, unsigned int>(dest, src);
			}
		}
		else if(dest.sampleType == UINT)
		{
			if(src.sampleType == UCHAR)
			{
				CopyChannel<unsigned int, unsigned char>(dest, src);
			}
			else if(src.sampleType == USHORT)
			{
				CopyChannel<unsigned int, unsigned short>(dest, src);
			}
			else if(src.sampleType == UINT)
			{
				CopyChannel<unsigned int, unsigned int>(dest, src);
			}
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
