#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#ifndef __TERMINABLE_BASE_HPP__
#define __TERMINABLE_BASE_HPP__ 1

namespace xw { namespace cpp { 
	//just dummy type so we can do type check in exit routine of singleton
	struct terminable_base_t{};
}}
#endif 
