#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#ifndef __meme_core_hpp__
#define __meme_core_hpp__

#include <iostream>
#include <vector>
#include <string.h>

#include <boost/cstdint.hpp>

#if (defined(__linux__) || defined(__APPLE__)) && ! defined(HW_UNIX)
#define HW_UNIX
#endif

//#define __xw__logign__

#if defined(__xw__logign__)
	#include <xwee/iglobals.hpp>
	// now we force class loader form root xwee.exe
	#define XW_GET_MASTER_CLASS_LOADER() xw::getGlobals().getClassLoader()
	// now if we handle root class loader macro override we can start 
	#include <xw-incl/log/logging.hpp>
#else
	#define XW_INIT_LOGGING()
	#define XW_LOG(__logger1, __kind1, __msg1)
	#define XW_LOG_TRACE(__logger1, __msg1)
	#define XW_LOG_DEBUG(__logger1, __msg1)
	#define XW_LOG_INFO(__logger1, __msg1)
	#define XW_LOG_WARN(__logger1, __msg1)
	#define XW_LOG_ERROR(__logger1, __msg1)
	#define XW_LOG_FATAL(__logger1, __msg1)
	#define XW_SCP_LOG_INIT(__logger_name)
	#define XW_SCP_LOG(__kind1, __msg1)
	#define XW_SCP_LOG_TRACE(__msg1)
	#define XW_SCP_LOG_DEBUG(__msg1)
	#define XW_SCP_LOG_INFO(__msg1)
	#define XW_SCP_LOG_WARN(__msg1)
	#define XW_SCP_LOG_ERROR(__msg1)
	#define XW_SCP_LOG_FATAL(__msg1)
	#define XW_SHORT_LOG(__logger_name, __kind1, __msg1)
#endif



#include <incl/os/os_mem_functions.hpp>
#include <incl/cntrs/bit-array.hpp>
#include <incl/cpp/phoenix.hpp>


#if defined(__LIMIT_TO_31_bit__)
    //2GB
    #define MEMCORE_MAX_HEAP_SIZE 0x80000000ull
#else
    //3.5GB
    #define MEMCORE_MAX_HEAP_SIZE 0xE0000000ull
#endif



namespace xw { namespace md {
/*

    int ram_dmp() {

        size_t free = 0;
        FILE* pF = fopen("/proc/self/maps", "r");

        if (pF) {
                    // Linux /proc/self/maps -> parse
                    // Format:
                    // lower    upper    prot     stuff                 path
                    // 08048000-0804c000 r-xp 00000000 03:03 1010107    /bin/cat
                    unsigned long rlower, rupper;
                    char r, w, x;
                    while (fscanf(pF, "%lx-%lx %c%c%c", &rlower, &rupper, &r, &w, &x) != EOF) {
                        if(rlower - free > 0 ){
                            printf("\nF %15lx:%15lx\t%15lx %s%s%s\t%15ld:%15ld\t%15ld", free, rlower, rlower - free, "n","n","n", free, rlower, (rlower - free) / (1024 * 1024));
                        }
                        printf("\nN %15lx:%15lx\t%15lx %c%c%c\t%15ld:%15ld\t%15ld", rlower, rupper, rupper - rlower, r , w, x , rlower, rupper, (rupper - rlower) / (1024 * 1024) );
                        //next free
                        free = rupper;

                        // Read to end of line
                        int c;
                        while ((c = fgetc(pF)) != '\n') {
                            if (c == EOF){
                                break;
                            }
                            printf("%c", c);
                        }
                        if (c == EOF){
                            break;
                        }
                    }
                    fclose(pF);
                    return false;
                }

        return 0;
    }

    */


    typedef xw::cntrs::bit_array_vm_alloc_t bit_set_t;

    struct __mem_block{
        void*       _ptr;       // region start pointer
        void*       _endPtr;    // region end pointer
        size_t      _size;      // region size
        size_t      _pages;     // region pages
        size_t      _pageSize;  // page size
        bit_set_t   _bm;        // used pages bit map
        
        __mem_block(void* ptr, size_t pages, size_t pageSize) : 
            _ptr(ptr),
            _endPtr((void*)((size_t)ptr + pages * pageSize)), 
            _size(pages * pageSize), 
            _pages(pages), 
            _pageSize(pageSize), 
            _bm(pages){
        }

        ~__mem_block(){
            xw::os::vm_release(_ptr, _size);
        }

        //no move will be handled for now!!!!
        // return values:
        //  -1 error
        //   0 OK done
        //   1 OK but not mine
        //   2 OK but ram was not resized, no more space 
        boost::int32_t realloc(void* ptr, size_t oSize, size_t nSize, void** result){
            XW_SCP_LOG_INIT("LuaJitVMM");
            //check if ptr is mine
            if(_ptr <= ptr && _endPtr > ptr){

                size_t dif = (size_t)ptr - (size_t)_ptr;
                size_t startIndex = dif / _pageSize;
                size_t needPages = oSize / _pageSize + ( oSize % _pageSize ? 1 : 0);

                //now calculate new pages need
                size_t newNeedPages = nSize / _pageSize + ( nSize % _pageSize ? 1 : 0);

                if(needPages == newNeedPages){
                    //no change will be done just return
                    return (*result = ptr, 0);
                }else if(needPages > newNeedPages){
                    //shrink original, freeing some pages from the end
                    void* unCommitFrom = (void*)((size_t)ptr + (newNeedPages * _pageSize));
                    
                    xw::os::vm_uncommit(unCommitFrom, (needPages - newNeedPages) * _pageSize);

                    _bm.unset_region(startIndex + newNeedPages, needPages - newNeedPages);

                    XW_SCP_LOG_TRACE("R-\t" << (size_t)ptr << "\t-" << needPages - newNeedPages << "\t(" << needPages << "->" << newNeedPages);
                    return (*result = ptr, 0); //OK

                }else{
                    // increase space
                    void* commitFrom = (void*)((size_t)ptr + (needPages * _pageSize)); // go to the end of current
                    size_t pagesForCommit = newNeedPages - needPages; //new pages
                    // first check if we have them free
                    // find free region
                    size_t pos = _bm.get_next_free_region(startIndex + needPages - 1, newNeedPages - needPages);
                    bool ok = pos == startIndex + needPages;
                    if(ok){

                        void* ret = xw::os::vm_commit(commitFrom, pagesForCommit * _pageSize);

                        if(ret != NULL){
                            _bm.set_region(startIndex + needPages, newNeedPages - needPages);
                        }else{
                            XW_SCP_LOG_ERROR("R+\t" << (size_t)ptr << "\tCANT COMMIT");
                            return (*result = NULL, -1); //can't commit
                        }                        

                        XW_SCP_LOG_TRACE("R+\t" << (size_t)ptr << "\t" << newNeedPages - needPages << "\t(" << needPages << "->" << newNeedPages);
                        return (*result = ptr, 0); //OK

                    }else{
                        XW_SCP_LOG_WARN("R+\t" << (size_t)ptr << "\tSPACE");
                        return (*result = NULL, 2); // no more space
                    }
                }                
            }

            return (*result = ptr, 1); //not mine
        }

        boost::int32_t free(void* ptr, size_t size){
            XW_SCP_LOG_INIT("LuaJitVMM");
            //check if ptr is mine
            if(_ptr <= ptr && _endPtr > ptr){
                size_t dif = (size_t)ptr - (size_t)_ptr;
                size_t startIndex = dif / _pageSize;
                //size_t needPages = it->second;
                size_t needPages = size / _pageSize + ( size % _pageSize ? 1 : 0);

                //free pages and mark them as free
                xw::os::vm_uncommit(ptr, needPages * _pageSize);

                _bm.unset_region(startIndex, needPages);

                XW_SCP_LOG_TRACE("F\t" << (size_t)ptr << "\t-"<< needPages);
                return static_cast<boost::int32_t>(needPages * _pageSize);
            }
            XW_SCP_LOG_TRACE("?\t" << (size_t)ptr << "(" << (size_t)_ptr << "," << (size_t)_endPtr << ")");
            return 0; //not mine
        }

        void* alloc(size_t size){
            XW_SCP_LOG_INIT("LuaJitVMM");

            //look if we have consecutive block of free pages
            size_t needPages = size / _pageSize + ( size % _pageSize ? 1 : 0);
       
            size_t startIndex = _bm.get_next_free_region(needPages);

            if(startIndex != NO_POS){
                //we have region so mark it
                void* wantedPtr =  (void*)((size_t)_ptr + startIndex * _pageSize);

                void* ret = os::vm_commit(wantedPtr, size);

                if(ret != INVALID_PTR){
                    XW_SCP_LOG_TRACE("A\t" << (size_t)ret << "\t"<< needPages);
                   
                    _bm.set_region(startIndex, needPages);
                    return ret;
                }
            }
            return NULL;
        }

        void dump(const char* fname){
            _bm.dump_to_err(fname);
        }
    };

    typedef __mem_block* __mem_blk_ptr_t;
    typedef std::vector<__mem_blk_ptr_t> __mem_blk_ptr_vector_t;

    class SMemCore{
        boost::uint32_t _pageSize;
        void*           _lowestAddress;
        size_t          _vas_alloc_unit;
        size_t          _reservedSize;
        __mem_blk_ptr_vector_t _blocks;
        size_t          _allocated_space;

    protected:
        SMemCore() : _pageSize(0), _lowestAddress(0), _vas_alloc_unit(0x10000), _reservedSize(0), _allocated_space(0){

            //ram_dmp();

            //init logging
            XW_INIT_LOGGING();
            
            //init scoped logging
            XW_SCP_LOG_INIT("LuaJitVMM_MAIN");
            XW_SCP_LOG_TRACE("SMemCore starting ...");

            //detect page size
#ifndef HW_UNIX
            SYSTEM_INFO sSysInfo;  
            GetSystemInfo(&sSysInfo);
            _pageSize = sSysInfo.dwPageSize;
            _lowestAddress = sSysInfo.lpMinimumApplicationAddress;
            _vas_alloc_unit = sSysInfo.dwAllocationGranularity;
#else
            _pageSize = getpagesize();
            _lowestAddress = 0;
            _vas_alloc_unit = _pageSize;
#endif

            //reserve blocks
            size_t minBlkSize = 64 * 4096; //64 pages
            size_t reservedSize = 0;
            size_t addressHint = 0x1000000ull; //experimental starting point

            // use reserving from os_mem_functions
            // doing atempts for simpler blocks if not found biggers
            for (size_t size = MEMCORE_MAX_HEAP_SIZE; size >= minBlkSize && reservedSize < MEMCORE_MAX_HEAP_SIZE; size /= 2){

                addressHint = 0x1000000ull;

                while(reservedSize < MEMCORE_MAX_HEAP_SIZE){
                    XW_SCP_LOG_TRACE("Try to reserve block of -> " << size << " bytes from: " << addressHint);

                    size_t lastReserved = 0;
                    void* p = xw::os::vm_reserve_limited((void*)addressHint, size, lastReserved, MEMCORE_MAX_HEAP_SIZE - reservedSize);

                    if (p == INVALID_PTR){
                        XW_SCP_LOG_TRACE("Reserving block of " << size << " form " << addressHint << " failed!");
                        break; // no more free blocks so change size
                    }else{
                        reservedSize += lastReserved;
                        XW_SCP_LOG_TRACE("Save reserved block -> " << lastReserved << " at " << (size_t)p );
                        _blocks.push_back(new __mem_block(p, lastReserved / _pageSize, _pageSize));

                        addressHint += lastReserved;
                    }
                }
            }

            //total reserved ram
            XW_SCP_LOG_TRACE("Reserved ram -> " << reservedSize << " in " << (size_t)_blocks.size() << " blocks.");
			_reservedSize = reservedSize;
            //ram_dmp();
        }

    public:
        ~SMemCore(){
            //XW_SCP_LOG_INIT("LuaJitVMM");
            //XW_SCP_LOG_TRACE("SMemCore -> ending ...");
            __mem_blk_ptr_vector_t::iterator it = _blocks.begin();
            for(; it != _blocks.end(); ++it){
                delete (*it);
            }
            //XW_SCP_LOG_TRACE("SMemCore -> ended ...");
        }

        void* alloc_ram(size_t size){
            XW_SCP_LOG_INIT("LuaJitVMM_MAIN");
            XW_SCP_LOG_TRACE("Alloc request for -> " << size);
            //try to take ram using all possible blocks til one give me it
            void* ret = NULL;
            boost::int32_t cnt = 0;
            __mem_blk_ptr_vector_t::iterator it = _blocks.begin();
            for(; it != _blocks.end(); ++it){
                ret = (*it)->alloc(size);
                //check if we had luck
                if(ret){ 
                    XW_SCP_LOG_TRACE("Alloc request success -> " << (size_t)ret << " from block:" << cnt);
                    _allocated_space += size;
                    XW_SHORT_LOG("LuaJitVMM", xw::log::kindTrace, "S\t" << _allocated_space << "\tFree:" << (_reservedSize - _allocated_space));
                    return ret;
                }
                ++cnt;
            }
            XW_SCP_LOG_ERROR("Alloc request failed!");
            /*
            it = _blocks.begin();
            cnt = 0;
            for(; it != _blocks.end(); ++it){
                FILE* f = fopen("/tmp/logs/mm.dmp.log", "a+b");
                if(f){
                    fprintf(f, "Failed alloc size : %ulx in Block %d\n", size, cnt++);
                    fclose(f);
                }

                (*it)->dump("/tmp/logs/mm.dmp.log");
            }
            */
            return NULL; //not found space
        }

        void* realloc_ram(void* ptr, size_t oldsize, size_t size){
            XW_SCP_LOG_INIT("LuaJitVMM_MAIN");
            XW_SCP_LOG_TRACE("Realloc request for -> " << size);
            //try to take ram using all possible blocks til one give me it
            void* ret = NULL;
            boost::int32_t state = -1;
            boost::int32_t cnt = 0;
            __mem_blk_ptr_vector_t::iterator it = _blocks.begin();
            for(; it != _blocks.end(); ++it){
                state = (*it)->realloc(ptr, oldsize, size, &ret);
                //check if we had luck
                switch(state){
                case 0:
                    _allocated_space += (size - oldsize);
                    XW_SHORT_LOG("LuaJitVMM", xw::log::kindTrace, "S\t" << _allocated_space << "\tFree:" << (_reservedSize - _allocated_space));
                    XW_SCP_LOG_TRACE("Realloc request success -> " << (size_t)ret << " from block:" << cnt);
                    return ret;
                case 1:
                     ++cnt;
                     continue;
                case 2:{
                            //block was mine, but no more space
                            XW_SCP_LOG_TRACE("Realloc attempt using move!");
                            //try to alloc new, move and free old
                            void* newPtr = alloc_ram(size);
                            if(newPtr != NULL){
                                memcpy(newPtr, ptr, oldsize);
                                free_ram(ptr, oldsize);

                                _allocated_space += (size - oldsize);
                                XW_SHORT_LOG("LuaJitVMM", xw::log::kindTrace, "S\t" << _allocated_space << "\tFree:" << (_reservedSize - _allocated_space));

                                XW_SCP_LOG_TRACE("Realloc request success doing move to -> " << (size_t)newPtr);
                                return newPtr;
                            }   
                       }
                    break;
                case -1:
                    break;
                }               
            }
            XW_SCP_LOG_ERROR("Realloc request failed!");
            return NULL; //not found space
        }

        bool free_ram(void* ptr, size_t size){
            XW_SCP_LOG_INIT("LuaJitVMM_MAIN");
            XW_SCP_LOG_TRACE("Free request for -> " << (size_t)ptr);
            //try free it in some block
            boost::int32_t cnt = 0;
            __mem_blk_ptr_vector_t::iterator it = _blocks.begin();
            for(; it != _blocks.end(); ++it){
                boost::int32_t ret = (*it)->free(ptr, size); 
                if( ret > 0 ){
                    _allocated_space -= ret;
                    XW_SHORT_LOG("LuaJitVMM", xw::log::kindTrace, "S\t" << _allocated_space << "\tFree:" << (_reservedSize - _allocated_space));

                    XW_SCP_LOG_TRACE("Free request success -> " << (size_t)ptr << " from block:" << cnt);
                    return true;
                }
                ++cnt;
            }
            XW_SCP_LOG_ERROR("Free request failed!");
            return false;
        }
    };


    class SMemCore_factory {
    public:

        SMemCore & ref()
        {
            return xw::cpp::phoenix< SMemCore, SMemCore_factory >::instance() ;
        }
    } ;

}} // namespace xw::md

#endif
