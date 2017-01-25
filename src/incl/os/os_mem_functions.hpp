#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#ifndef __os_mem_functions_hpp__
#define __os_mem_functions_hpp__

#if (defined(__linux__) || defined(__APPLE__)) && ! defined(HW_UNIX)
#define HW_UNIX
#endif

#ifndef HW_UNIX
    #include <windows.h>
    #define INVALID_PTR NULL
#else
    #include <sys/mman.h>
    #include <unistd.h>
    #include <stdio.h>
    #define INVALID_PTR MAP_FAILED
#endif

#ifndef EOF
# define EOF (-1)
#endif


#if defined(__LIMIT_TO_31_bit__)
    //31bit
    #define POINTER_LIMITING_MASK   0xFFFFFFFF80000000ull
    #define LAST_MAPPABLE_ADDRESS   0x7FFFFFFFull
#else
    //32bit
    #define POINTER_LIMITING_MASK   0xFFFFFFFF00000000ull
    #define LAST_MAPPABLE_ADDRESS   0xFFFFFFFFull
#endif


#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS       MAP_ANON
#endif

namespace xw { namespace os {
    
    namespace dtl{

#if ! defined(HW_UNIX)
        // query ram and find first block which is over 4g
        inline void* windows_get_first_free_block_over_4gb(void* addressHint, size_t& size, size_t maxSize){

            static size_t vas_alloc_unit_size = 0;
            static size_t hi_mask = 0;
            static size_t lo_mask = 0;

            if(vas_alloc_unit_size == 0){
                SYSTEM_INFO sSysInfo;  
                GetSystemInfo(&sSysInfo);
                vas_alloc_unit_size = sSysInfo.dwAllocationGranularity;
                hi_mask = -1 * vas_alloc_unit_size;
                lo_mask = ~hi_mask;
            }

            // take max
            maxSize = maxSize > size ? maxSize : size;

            MEMORY_BASIC_INFORMATION mbi;
            void* current = (void*)0x100000000ull > addressHint ? (void*)0x100000000ull : addressHint; // first available address over 4G or hint
            while(1){
                size_t ret = VirtualQuery(current, &mbi, sizeof(mbi));
                if(ret){
                    // now check if there is MEM_FREE
                    if(mbi.State == MEM_FREE && mbi.RegionSize >= size){
                        // doc msdn mbi.State == MEM_FREE: Indicates free pages not accessible to the calling process and available to be allocated. 
                        // For free pages, the information in the AllocationBase, AllocationProtect, Protect, and Type members is undefined.

                        // so we will take region starting at
                        void* p = mbi.BaseAddress;
                        // align address to next allocation boundary
                        p = (void*)((hi_mask & (size_t)p) + (((size_t)p & lo_mask) != 0 ? vas_alloc_unit_size : 0));
                        //start can be eventually aligned
                        size_t adjust = (size_t)p - (size_t)mbi.BaseAddress;
                        // recheck size, cause start can be moved
                        if(mbi.RegionSize - adjust >= size){
                            size_t maxUsable = (mbi.RegionSize - adjust) > maxSize ? maxSize : (mbi.RegionSize - adjust);
                            // so we will take region starting at
                            return (size = maxUsable & hi_mask,  p); //set back also size of found region 
                        }else{
                            // not enough free space so go for next
                            current = (void*) ((size_t) mbi.BaseAddress + mbi.RegionSize);
                        }
                    }else{
                        // move start address for next query
                        current = (void*) ((size_t) mbi.BaseAddress + mbi.RegionSize);
                    }
                }else{
                    return (size = 0, NULL); //not found due to VirtualQuery return
                }
            }
        }

        // query ram and find first block which is under first 2GB
        // windows round down all VAS starting addresses to allocation unit
        // which is 0x10000 (64kb) so if we found some free region
        // we have to align starting address to the next allocation unit boundary so
        // we will not hit precedent region, but this must be don if we are not already on
        // allocation region boundary
        inline void* windows_get_first_free_block_limited(void* addressHint, size_t& size, size_t maxSize){

            static size_t vas_alloc_unit_size = 0;
            static size_t hi_mask = 0;
            static size_t lo_mask = 0;

            if(vas_alloc_unit_size == 0){
                SYSTEM_INFO sSysInfo;  
                GetSystemInfo(&sSysInfo);
                vas_alloc_unit_size = sSysInfo.dwAllocationGranularity;
                hi_mask = -1 * vas_alloc_unit_size;
                lo_mask = ~hi_mask;
            }

            // take max
            maxSize = maxSize > size ? maxSize : size;

            MEMORY_BASIC_INFORMATION mbi;
            void* current = addressHint!= NULL ? addressHint : (void*)0x00ull; // first available address 0
            while((((size_t)current) & POINTER_LIMITING_MASK) == 0 && (((size_t)current + size) & POINTER_LIMITING_MASK) == 0){
                if(VirtualQuery(current, &mbi, sizeof(mbi))){
                    // now check if there is MEM_FREE
                    if(mbi.State == MEM_FREE && mbi.RegionSize >= size){
                        // doc msdn mbi.State == MEM_FREE: Indicates free pages not accessible to the calling process and available to be allocated. 
                        // For free pages, the information in the AllocationBase, AllocationProtect, Protect, and Type members is undefined.

                        // so we will take region starting at
                        void* p = mbi.BaseAddress;
                        // align address to next allocation boundary
                        p = (void*)((hi_mask & (size_t)p) + (((size_t)p & lo_mask) != 0 ? vas_alloc_unit_size : 0));
                        // start can be eventually aligned
                        size_t adjust = (size_t)p - (size_t)mbi.BaseAddress;
                        // recheck size, cause start can be moved
                        if(mbi.RegionSize - adjust >= size){
                            // paranoid control for to be sure address is max 31 bit wide
                            if((((size_t)p) & POINTER_LIMITING_MASK) == 0 && (((size_t)p + size) & POINTER_LIMITING_MASK) == 0){
                                // check that region size is not over max possible address
                                size_t maxAvailable = LAST_MAPPABLE_ADDRESS - (size_t)p;
                                // consider max wanted
                                size_t maxWanted = maxSize > maxAvailable ? maxAvailable : maxSize; 
                                // consider free space
                                size_t maxUsable = maxWanted > (mbi.RegionSize - adjust) ? (mbi.RegionSize - adjust) : maxWanted;
                                
                                return (size = maxUsable & hi_mask, p); // set also available size
                            }else{
                                //we stop we reach upper max limit
                                return (size = 0, NULL);
                            }
                        }else{
                            // not enough free space so go for next
                            current = (void*) ((size_t) mbi.BaseAddress + mbi.RegionSize);
                        }                        
                    }else {
                        // move start address for next query
                        current = (void*) ((size_t) mbi.BaseAddress + mbi.RegionSize);
                    }
                }else{
                    return (size = 0, NULL); //not found due to VirtualQuery return
                }
            }
            //not found for wanted size
            return (size = 0, NULL);
        }
#else
        //parse "/proc/self/maps" and detect spaces between lines :)
        inline void* linux_get_first_free_block_limited(void* addressHint, size_t& size, size_t maxSize) {

            static size_t vas_alloc_unit_size = 0;
            static size_t hi_mask = 0;
            static size_t lo_mask = 0;
            static size_t last_mappable_address = 0;

            if(vas_alloc_unit_size == 0){
               vas_alloc_unit_size = getpagesize();
                hi_mask = -1 * vas_alloc_unit_size;
                lo_mask = ~hi_mask;
                last_mappable_address = LAST_MAPPABLE_ADDRESS & hi_mask;
            }
            //align sizes up
            size = (hi_mask & size) + ((size & lo_mask) != 0 ? vas_alloc_unit_size : 0);
            maxSize = (hi_mask & maxSize) + ((maxSize & lo_mask) != 0 ? vas_alloc_unit_size : 0);

            //size and max size must be rounded up to whole page
            //adjust sizes
            maxSize = maxSize > size ? maxSize : size;

            //we are out of possible address space
            if((((size_t)addressHint) & POINTER_LIMITING_MASK) != 0 || (((size_t)addressHint + size) & POINTER_LIMITING_MASK) != 0){
                return (size = 0, (void*)NULL);
            }

            size_t current = (size_t)addressHint;

            FILE* pF = fopen("/proc/self/maps", "r");
            if (pF) {
                // Linux /proc/self/maps -> parse
                // Format:
                // lower    upper    prot     stuff                 path
                // 08048000-0804c000 r-xp 00000000 03:03 1010107    /bin/cat
                // we just need calculate differences between lines, cause those are free blocks
                size_t rlower, rupper;
                while (fscanf(pF, "%lx-%lx", &rlower, &rupper) != EOF) {
                    //check if space is enough
                    if(rlower > current && rlower - current >= size ){
                        //we have block but adjust sizes so we don't go out of 31bit address space
                        if((current & POINTER_LIMITING_MASK) == 0 && ((current + size) & POINTER_LIMITING_MASK) == 0){
                            // check that region size is not over max possible address
                            size_t maxAvailable = last_mappable_address - current;
                            // consider max wanted
                            size_t maxWanted = maxSize > maxAvailable ? maxAvailable : maxSize;
                            // consider free space
                            size_t maxUsable = maxWanted > rlower - current ? (rlower - current) : maxWanted;

                            return (size = maxUsable, (void*)current); // set also available size
                        }else{
                            //we stop we reach upper max limit
                            return (size = 0, (void*)NULL);
                        }
                    }
                    current = rupper;
                    // Read to end of line
                    int c;
                    while ((c = fgetc(pF)) != '\n') {
                        if (c == EOF){
                            break;
                        }
                    }
                    if (c == EOF){
                        break;
                    }
                }
                fclose(pF);
                return (size = 0, (void*)NULL);
            }
            return (size = 0, (void*)NULL);
        }

        //parse "/proc/self/maps" and detect spaces between lines :)
        inline void* linux_get_first_free_block_over_4gb(void* addressHint, size_t& size, size_t maxSize) {
            static size_t vas_alloc_unit_size = 0;
            static size_t hi_mask = 0;
            static size_t lo_mask = 0;

            if(vas_alloc_unit_size == 0){
               vas_alloc_unit_size = getpagesize();
                hi_mask = -1 * vas_alloc_unit_size;
                lo_mask = ~hi_mask;
            }
            //align sizes up
            size = (hi_mask & size) + ((size & lo_mask) != 0 ? vas_alloc_unit_size : 0);
            maxSize = (hi_mask & maxSize) + ((maxSize & lo_mask) != 0 ? vas_alloc_unit_size : 0);

            //adjust sizes
            maxSize = maxSize > size ? maxSize : size;
            //move addressHint to the bottom 4GB boundary
            size_t current = 0x100000000ull > (size_t)addressHint ? 0x100000000ull : (size_t)addressHint; // first available address over 4G or hint

            FILE* pF = fopen("/proc/self/maps", "r");
            if (pF) {
                // Linux /proc/self/maps -> parse
                // Format:
                // lower    upper    prot     stuff                 path
                // 08048000-0804c000 r-xp 00000000 03:03 1010107    /bin/cat
                // we just need calculate differences between lines, cause those are free blocks
                size_t rlower, rupper;
                while (fscanf(pF, "%lx-%lx", &rlower, &rupper) != EOF) {
                    //check if space is enough
                    if(rlower > current && rlower - current >= size ){
                        return (size = rlower - current > maxSize ? maxSize : rlower - current, (void*)current);
                    }
                    current = rupper;
                    // Read to end of line
                    int c;
                    while ((c = fgetc(pF)) != '\n') {
                        if (c == EOF){
                            break;
                        }
                    }
                    if (c == EOF){
                        break;
                    }
                }
                fclose(pF);
                return (size = 0, (void*)NULL);
            }
            return (size = 0, (void*)NULL);
        }
#endif


    }

    /*
        Virtual memory functions
        those functions work at page level
        and hide different os handling
        for address space reservation / releasing
        memory committing / uncommitting
    */
    inline void vm_release(void* p, size_t size){
#ifndef HW_UNIX
        VirtualFree(p, 0, MEM_RELEASE);
#else
        msync(p, size, MS_SYNC);
        munmap(p, size);
#endif
    }


    inline void* vm_reserve_limited(void* address_hint, size_t size, size_t& allocated, size_t maxSize = 0){
        void* p = INVALID_PTR;

        // take max
        maxSize = maxSize > size ? maxSize : size;

#ifndef HW_UNIX
        size_t availableSize = size;
        //lets look for possible VAS big at least size bytes
        void* addressHint = dtl::windows_get_first_free_block_limited(address_hint, availableSize, maxSize);
        if(addressHint == NULL){
            //not enough space for wanted size
            return (allocated = 0, INVALID_PTR);
        }

        // manage size take min 
        size = availableSize > maxSize ? maxSize : availableSize;

        DWORD olderr = GetLastError();
        SetLastError(0);
        p = VirtualAlloc(addressHint, size, MEM_RESERVE, PAGE_NOACCESS);
        //DWORD err = GetLastError();
        SetLastError(olderr);


#else
        size_t availableSize = size;
        //lets look for possible VAS big at least size bytes
        void* addressHint = address_hint;
#ifndef __APPLE__
 		// /proc/self/map, is not available on Mac, therefore stick with address_hint in that case
        addressHint = dtl::linux_get_first_free_block_limited(address_hint, availableSize, maxSize);
        if(addressHint == NULL){
            //not enough space for wanted size
            return (allocated = 0, INVALID_PTR);
        }
#endif

        // manage size take min
        size = availableSize > maxSize ? maxSize : availableSize;

        //p = mmap((void*)addressHint, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1 , 0);
        p = mmap((void*)addressHint, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1 , 0);
        msync(p, size, MS_SYNC|MS_INVALIDATE);
#endif
        if(p == INVALID_PTR){
            return (allocated = 0, p);
        }
        //paranoid control for to be sure address is max 31 bit wide
        if((((size_t)p) & POINTER_LIMITING_MASK) != 0 || (((size_t)p + size) & POINTER_LIMITING_MASK) != 0)
        {
            // We don't need this memory, so release it completely.
#ifndef HW_UNIX
            VirtualFree(p, 0, MEM_RELEASE);
#else
            munmap(p, size);
#endif
            return (allocated = 0, INVALID_PTR);
        }

        //all went ok
        return (allocated = size, p);
    }

    // get addresses over 4G
    inline void* vm_reserve_over_4gb(void* address_hint, size_t size, size_t& allocated, size_t maxSize = 0){

        // take max
        maxSize = maxSize > size ? maxSize : size;

#ifndef HW_UNIX
        size_t availableSize = size;
        void* addressHint = dtl::windows_get_first_free_block_over_4gb(addressHint, availableSize, maxSize); //not handle variable blocks
        if(addressHint != NULL){

            // manage size take min 
            size = availableSize > maxSize ? maxSize : availableSize;

            void *p = VirtualAlloc(addressHint, size, MEM_RESERVE, PAGE_NOACCESS);
            return (allocated = p != NULL ? size : 0, p);
        }

        //not enough space for wanted size
        return (allocated = 0, INVALID_PTR);
#else
        size_t availableSize = size;
        void* addressHint = dtl::linux_get_first_free_block_over_4gb(addressHint, availableSize, maxSize); //not handle variable blocks
        if(addressHint != NULL){

            // manage size take min
            size = availableSize > maxSize ? maxSize : availableSize;

            //void* p = mmap((void*)addressHint, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1 , 0);

            void* p = mmap((void*)addressHint, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1 , 0);
            msync(p, size, MS_SYNC|MS_INVALIDATE);

            return (allocated = p != NULL ? size : 0, p);
        }

        //not enough space for wanted size
        return (allocated = 0, INVALID_PTR);
#endif
    }

    // commit ram into reserved space
    inline void* vm_commit(void* ptr, size_t size){
#ifndef HW_UNIX
        return VirtualAlloc(ptr, size, MEM_COMMIT, PAGE_READWRITE);
#else
        /*
        int stat = mprotect(ptr, size, PROT_READ | PROT_WRITE);
        if(stat != 0){
            int i = 0;
        }
        return  stat == 0 ? ptr : INVALID_PTR;*/

        void * tmpPtr = mmap(ptr, size, PROT_READ|PROT_WRITE, MAP_FIXED|MAP_SHARED|MAP_ANONYMOUS, -1, 0);
        msync(ptr, size, MS_SYNC|MS_INVALIDATE);
        return tmpPtr;
#endif
    }

    inline void vm_uncommit(void* ptr, size_t size){
#ifndef HW_UNIX
        VirtualFree(ptr, size, MEM_DECOMMIT);
#else
        /*
        madvise(ptr, size, MADV_DONTNEED);
        mprotect(ptr, size, PROT_NONE);
        */

        // instead of unmapping the address, we're just gonna trick
        // the TLB to mark this as a new mapped area which, due to
        // demand paging, will not be committed until used.

        mmap(ptr, size, PROT_NONE, MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
        msync(ptr, size, MS_SYNC|MS_INVALIDATE);
#endif
    }

    //get exclusively upper ram
    inline void* vm_alloc(size_t size){
#ifndef HW_UNIX
        size_t availableSize = size;
        void* addressHint = dtl::windows_get_first_free_block_over_4gb(NULL, availableSize, size);
        if(addressHint != NULL){
            DWORD olderr = GetLastError();
            SetLastError(0);
            void* r = VirtualAlloc(addressHint, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            //DWORD err = GetLastError();
            SetLastError(olderr);            
            return r;
        }
        //not enough space for wanted size
        return INVALID_PTR;
#else
        return mmap((void*)0x100000000ull, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1 , 0);
#endif        
    }

    // this is no secure holder its just for deleting
    class vm_auto_unchecked_ptr{
        size_t  _size;
        void*   _ptr;
        

    public:
        vm_auto_unchecked_ptr(size_t size) : _size(size), _ptr(vm_alloc(size)){
        }

        ~vm_auto_unchecked_ptr(){
            if(_ptr)
                vm_release(_ptr, _size);
        }

        template<typename _T>
        _T * const get(){
            return (_T*)_ptr;
        }
    };

}} // namespace xw::os

#endif
