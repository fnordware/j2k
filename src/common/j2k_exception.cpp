/*
 *  j2k_exception.cpp
 *  j2k_AE
 *
 *  Created by Brendan Bolles on 10/9/16.
 *  Copyright 2016 fnord. All rights reserved.
 *
 */

#include "j2k_exception.h"


namespace j2k
{


Exception::Exception(const std::string &s) throw() :
	std::exception(),
	_s(s)
{

}


}; // namespace j2k