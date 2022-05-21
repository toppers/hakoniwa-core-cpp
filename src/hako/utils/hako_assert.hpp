#ifndef _HAKO_ASSERT_HPP_
#define _HAKO_ASSERT_HPP_

#include "utils/hako_logger.hpp"

#include <assert.h>

#if !defined(NDEBUG)
#define HAKO_ASSERT(expr)	\
do {	\
	if (!(expr))	{	\
		hako::utils::logger::get("core")->critical("ASSERTION FAILED:{0}:{1}:{2}:{3}", __FILE__, __FUNCTION__, __LINE__, #expr);	\
		assert(!(expr));	\
	}	\
} while (0)
#else
#define HAKO_ASSERT(ignore) ((void)0)
#endif


#endif /* _HAKO_ASSERT_HPP_ */