#include "mem_core.hpp"


#if defined(HW_UNIX)
#define DLL_EXPORT
#else
#define DLL_EXPORT __declspec(dllexport)
#endif

extern "C"{
	DLL_EXPORT void laco_init(){
		xw::md::SMemCore_factory().ref();
	}
    DLL_EXPORT void* laco_alloc(size_t size){
        return xw::md::SMemCore_factory().ref().alloc_ram(size);
    }

    DLL_EXPORT int laco_free(void* ptr, size_t size){
        return xw::md::SMemCore_factory().ref().free_ram(ptr, size) ? 0 : -1;
    }

    DLL_EXPORT void* laco_realloc(void* ptr, size_t oldsize, size_t size){
        return xw::md::SMemCore_factory().ref().realloc_ram(ptr, oldsize, size);
    }
};


