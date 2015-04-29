#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#ifndef __meme_core_h__
#define __meme_core_h__

#if defined(HW_UNIX)
#define DLL_EXPORT
#else
#define DLL_EXPORT __declspec(dllexport)
#endif

DLL_EXPORT void* laco_alloc(size_t size);
DLL_EXPORT int laco_free(void* ptr, size_t size);
DLL_EXPORT void* laco_realloc(void* ptr, size_t oldsize, size_t size);

#endif