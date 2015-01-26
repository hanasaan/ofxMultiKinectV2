//
//  timer_uti.cpp
//  example
//
//  Created by Yuya Hanai on 2015/01/26.
//
//

#include "timer_util.h"

#include "ofUtils.h"

namespace libfreenect2
{
	uint64_t getCurrentMillis() {
		return ofGetElapsedTimeMillis();
	}
}
