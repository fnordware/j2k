/*
 *  j2k_exception.h
 *  j2k_AE
 *
 *  Created by Brendan Bolles on 10/9/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

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