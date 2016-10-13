/*
 *  j2k_io.h
 *  j2k_AE
 *
 *  Created by Brendan Bolles on 10/9/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */


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
