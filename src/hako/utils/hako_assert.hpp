#ifndef _HAKO_ASSERT_HPP_
#define _HAKO_ASSERT_HPP_

#include "types/hako_types.hpp"
#include "utils/hako_logger.hpp"


#if !defined(NDEBUG)
#define HAKO_ASSERT(expr)	\
do {	\
	if (!(expr))	{	\
		printf("ASSERTION FAILED:%s:%s:%d:%s", __FILE__, __FUNCTION__, __LINE__, #expr);	\
		assert(!(expr));	\
	}	\
} while (0)
#else
#define HAKO_ASSERT(ignore) ((void)0)
#endif


#endif /* _HAKO_ASSERT_HPP_ */