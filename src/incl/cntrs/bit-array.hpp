/*
 bit_array functions
 special thanks to : Isaac Turner
 here we used fragments of its original code url: https://github.com/noporpoise/BitArray/
*/
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#ifndef XW_INCL_CNTR_BIT_ARRAY_HPP_
#define XW_INCL_CNTR_BIT_ARRAY_HPP_

#include <incl/os/os_mem_functions.hpp>
#include <boost/cstdint.hpp>
#include <exception>
#include <iostream>
#include <ios>
#include <string.h>

#ifndef HW_UNIX
    #include <intrin.h>
#endif

#ifdef strRev
#	undef strRev
#endif
#define strRev "$Revision: 1.3 $"


#define NO_POS ~(size_t)0
//
// Bit array (bitset)
//
#define bitsetX_wrd(wrdbits,pos) ((pos) / (wrdbits))
#define bitsetX_idx(wrdbits,pos) ((pos) % (wrdbits))
#define bitset64_wrd(pos) ((pos) >> 6)
#define bitset64_idx(pos) ((pos) & 63)

// need to check for length == 0, undefined behavior if uint64_t >> 64 etc
#define bitmask(nbits,type) ((nbits) ? ~(type)0 >> (sizeof(type)*8-(nbits)): (type)0)
#define bitmask64(nbits) bitmask(nbits, boost::uint64_t)


// trailing_zeros is number of least significant zeros
// leading_zeros is number of most significant zeros
#if defined(_WIN32)
#define trailing_zeros(_r, x) ((x) ? ( _BitScanForward64(&_r, x), _r) : (64))
#define leading_zeros(_r, x)  ((x) ? ( _BitScanReverse64(&_r, x), (63 - _r)) : (64))
#else
#define trailing_zeros(_r, x) ((x) ? (__typeof(x))__builtin_ctzll(x) : (__typeof(x))sizeof(x)*8)
#define leading_zeros(_r, x) ((x) ? (__typeof(x))__builtin_clzll(x) : (__typeof(x))sizeof(x)*8)
#endif

#if !defined(XW_DEFINE_FAST_EXCEPTION)
	#define XW_DEFINE_FAST_EXCEPTION(name) class name : public std::exception{}
#endif
#define XW_THROW_FAST_IF_NOT_CHECK(_fc, type) {if (!(_fc)) {throw type();}}

namespace xw { namespace cntrs {
    
    XW_DEFINE_FAST_EXCEPTION( ex_cntrs_out_of_bounds );

    namespace dtl{

        class auto_unchecked_ptr{
            size_t  _size;
            void*   _ptr;

        public:
            auto_unchecked_ptr(size_t size) : _size(size), _ptr(malloc(size)){
            }

            ~auto_unchecked_ptr(){
                if(_ptr)
                    free(_ptr);
            }

            template<typename _T>
            _T * const get(){
                return (_T*)_ptr;
            }
        };
    }
    
    //template<typename slot_t>
    typedef boost::uint64_t slot_t;
    


    template<typename _AllocatorT>
    class bit_array{
        //typedef xw::os::vm_auto_unchecked_ptr slot_scparr_t;
    public:
        // FillAction is fill with 0 or 1 or toggle
        typedef enum {ZERO_REGION, FILL_REGION, SWAP_REGION} FillAction;
        const size_t          _slot_size_bytes;
        const size_t          _slot_size_bits;
   
    private:
        size_t                          _slots_size;
        size_t                          _size;        
        xw::os::vm_auto_unchecked_ptr   _slots_array;
        slot_t * const                  _slots;


    //
    // Fill a region (internal use only)
    //
    inline void _set_region(size_t start, size_t length, FillAction action)
    {
        if(length == 0) return;
        //adjust length
        length = length > _size - start ? _size - start : length;

        size_t first_word = bitset64_wrd(start);
        size_t last_word =  bitset64_wrd(start+length-1);
        size_t foffset =    bitset64_idx(start);
        size_t loffset =    bitset64_idx(start+length-1);

        if(first_word == last_word)
        {
            slot_t mask = bitmask64(length) << foffset;

            switch(action)
            {
            case ZERO_REGION: _slots[first_word] &= ~mask; break;
            case FILL_REGION: _slots[first_word] |=  mask; break;
            case SWAP_REGION: _slots[first_word] ^=  mask; break;
            }
        }
        else
        {
            // Set first word
            switch(action)
            {
            case ZERO_REGION: _slots[first_word] &=  bitmask64(foffset); break;
            case FILL_REGION: _slots[first_word] |= (~bitmask64(foffset)); break;
            case SWAP_REGION: _slots[first_word] ^= (~bitmask64(foffset)); break;
            }

            size_t i;

            // Set whole words
            switch(action)
            {
            case ZERO_REGION:
                for(i = first_word + 1; i < last_word; i++)
                    _slots[i] = (slot_t)0;
                break;
            case FILL_REGION:
                for(i = first_word + 1; i < last_word; i++)
                    _slots[i] = (~(slot_t)0);
                break;
            case SWAP_REGION:
                for(i = first_word + 1; i < last_word; i++)
                    _slots[i] ^= (~(slot_t)0);
                break;
            }

            // Set last word
            switch(action)
            {
            case ZERO_REGION: _slots[last_word] &= (~bitmask64(loffset+1)); break;
            case FILL_REGION: _slots[last_word] |=  bitmask64(loffset+1); break;
            case SWAP_REGION: _slots[last_word] ^=  bitmask64(loffset+1); break;
            }
        }
    }   

    public:
        bit_array(size_t size, bool create_set = false) :
            _slot_size_bytes(sizeof(slot_t)),
            _slot_size_bits(_slot_size_bytes * 8),
            _slots_size(roundup_bits2slots(size)), 
            _size(size), 
            _slots_array(_slots_size * _slot_size_bytes), 
            _slots(_slots_array.get<slot_t>()){
                memset(_slots, create_set ? 0xff : 0, _slot_size_bytes * _slots_size);
        }

        inline slot_t roundup_bits2slots(size_t bits) const {
            return ((bits + _slot_size_bits - 1) / _slot_size_bits);
        }
        
        inline bool test(size_t pos){
            XW_THROW_FAST_IF_NOT_CHECK(pos < _size, ex_cntrs_out_of_bounds);
            return ((_slots[bitset64_wrd(pos)] >> bitset64_idx(pos)) & 0x1) == 0x1 ;
        }
        inline void set(size_t pos){
            XW_THROW_FAST_IF_NOT_CHECK(pos < _size, ex_cntrs_out_of_bounds);
            _slots[bitset64_wrd(pos)] |= (0x1ull << bitset64_idx(pos));
        }
        inline void unset(size_t pos){
            XW_THROW_FAST_IF_NOT_CHECK(pos < _size, ex_cntrs_out_of_bounds);
            _slots[bitset64_wrd(pos)] &= (~(0x1ull << bitset64_idx(pos)));
        }
        inline void toggle(size_t pos){
            XW_THROW_FAST_IF_NOT_CHECK(pos < _size, ex_cntrs_out_of_bounds);
            _slots[bitset64_wrd(pos)] ^= (0x1ull << bitset64_idx(pos));
        }        
        void set_region(size_t from, size_t len){
            _set_region(from, len, FILL_REGION);
        }
        void unset_region(size_t from, size_t len){
            _set_region(from, len, ZERO_REGION);
        }
        void toggle_region(size_t from, size_t len){
            _set_region(from, len, SWAP_REGION);
        }
        void set_all(){
            memset(_slots, 0xff, _slot_size_bytes * _slots_size);
        }
        void unset_all(){
            memset(_slots, 0, _slot_size_bytes * _slots_size);
        }
        void toggle_all(){
            _set_region(0, _size, SWAP_REGION);
        }


        // Find the index of the next bit that is set, at or after `offset`
        // Returns pos if such a bit is found, otherwise NO_POS
        // This call can be limited in length of scan, it will stop 
        // if it will not found set bit within wanted scan len,
        // this can be handy for regions state check
        size_t get_next_set_bit(size_t offset = 0, size_t max_scan_len = NO_POS){
            if(offset >= _size) { return NO_POS; }
            /* Find first word that is greater than zero */ 
            size_t i = bitset64_wrd(offset); 
            slot_t w = _slots[i] & ~bitmask64(bitset64_idx(offset));
            size_t tmp = roundup_bits2slots(max_scan_len == NO_POS ? _size : max_scan_len) + i; //we have scan len so we have to shift it;
            size_t last_slot_for_scan = tmp < _slots_size ? tmp : _slots_size;

            
            while(1) { 
                if(w > 0) {
                    unsigned long tul = 0;
                    size_t pos = i * _slot_size_bits + trailing_zeros(tul, w); 
                    if(pos < _size){ 
                        return pos; 
                    } 
                    return NO_POS;  
                } 
                ++i; 
                //here we will test also eventual stp of can
                if(i >= last_slot_for_scan) break;
                w = _slots[i];
            }        
            return NO_POS;
        }

        // Find the index of the next bit that is NOT set, at or after `offset`
        // Returns pos if such a bit is found, otherwise NO_POS
        // This call can be limited in length of scan, it will stop 
        // if it will not found set bit within wanted scan len,
        // this can be handy for regions state check
        size_t get_next_clear_bit(size_t offset = 0, size_t max_scan_len = NO_POS){
            if(offset >= _size) { return NO_POS; }
            /* Find first word that is greater than zero */ 
            size_t i = bitset64_wrd(offset); 
            slot_t w = ~(_slots[i]) & ~bitmask64(bitset64_idx(offset));
            size_t tmp = roundup_bits2slots(max_scan_len == NO_POS ? _size : max_scan_len) + i; //we have scan len so we have to shift it;
            size_t last_slot_for_scan = tmp < _slots_size ? tmp : _slots_size;

            while(1) { 
                if(w > 0) {
                    unsigned long tul = 0;
                    size_t pos = i * _slot_size_bits + trailing_zeros(tul, w); 
                    if(pos < _size){ 
                        return pos; 
                    } 
                    return NO_POS;  
                } 
                ++i; 
                //here we will test also eventual stp of can
                if(i >= last_slot_for_scan) break;
                w = ~(_slots[i]);
            }        
            return NO_POS;
        }

        // Find the index of the previous bit that is set, before `offset`.
        // Returns pos if such a bit is found, otherwise NO_POS
        // This call can be limited in length of scan, it will stop
        // if it will not found set bit within wanted scan len,
        // this can be handy for regions state check
        size_t get_prev_set_bit(size_t offset = NO_POS, size_t max_scan_len = NO_POS){ 
            if(offset == NO_POS || offset > _size) offset = _size;
            if(offset == 0) { return NO_POS; }
            /* Find prev word that is greater than zero */ 
            size_t i = bitset64_wrd(offset-1); 
            size_t w = _slots[i] & bitmask64(bitset64_idx(offset-1)+1);
            size_t tmp = roundup_bits2slots(max_scan_len == NO_POS ? _size : max_scan_len);
            // if current offset is smaller then max scan size, 
            // just use bottom as limit, if not
            // we can establish new bottom line
            size_t last_slot_for_scan = tmp > i ? NO_POS : i - tmp;

            
            if(w > 0) { 
                unsigned long tul = 0;
                return (i+1) * _slot_size_bits - leading_zeros(tul, w) - 1;
            }
            /* i is unsigned so have to use break when i == 0 */ 
            for(--i; i != last_slot_for_scan; i--) { 
                w = _slots[i]; 
                if(w > 0) { 
                    unsigned long tul = 0;
                    return (i+1) * _slot_size_bits - leading_zeros(tul, w) - 1;
                } 
            } 

            return NO_POS;
        }

        // Find the index of the previous bit that is NOT set, before `offset`.
        // Returns pos if such a bit is found, otherwise NO_POS
        // This call can be limited in length of scan, it will stop
        // if it will not found set bit within wanted scan len,
        // this can be handy for regions state check
        size_t get_prev_clear_bit(size_t offset = NO_POS, size_t max_scan_len = NO_POS){ 
            if(offset == NO_POS || offset > _size) offset = _size;
            if(offset == 0) { return NO_POS; }
            /* Find prev word that is greater than zero */ 
            size_t i = bitset64_wrd(offset-1); 
            size_t w = ~(_slots[i]) & bitmask64(bitset64_idx(offset-1)+1);
            size_t tmp = roundup_bits2slots(max_scan_len == NO_POS ? _size : max_scan_len);
            // if current offset is smaller then max scan size, 
            // just use bottom as limit, if not
            // we can establish new bottom line
            size_t last_slot_for_scan = tmp > i ? NO_POS : i - tmp;
            if(w > 0) { 
                unsigned long tul = 0;
                return (i+1) * _slot_size_bits - leading_zeros(tul, w) - 1;
            }
            /* i is unsigned so have to use break when i == 0 */ 
            for(--i; i != last_slot_for_scan; i--) { 
                w = ~(_slots[i]); 
                if(w > 0) { 
                    unsigned long tul = 0;
                    return (i+1) * _slot_size_bits - leading_zeros(tul, w) - 1;
                } 
            } 

            return NO_POS;
        }

        // GET region form START
        // run in and search first non set bit, then 
        // try search first set bit which must be far enough :)
        // if not exists return NO_POS and position otherwise
        size_t get_next_free_region(size_t len, size_t offset = 0){
            size_t pos = offset;
            while(1){
                // potential first bit 
                size_t next_clear = get_next_clear_bit(pos);
                if(next_clear == NO_POS) break;
                // now check free space, starting from position just after our free found
                // running at least len bits + some gap, so we not run to much uselessly, 
                // if not found we are OK we have free region
                pos = get_next_set_bit(next_clear, len + _slot_size_bits);
                if(pos == NO_POS || ( (pos - next_clear) >= len )){
                    //not found any used in limited area, or found and there is enough room
                    return next_clear; //we have enough space
                }
            }
            return NO_POS;
        }

        // GET region from END of space
        // run in and search previous non set bit, then 
        // try search previous set bit which must be far enough :)
        // if not exists return NO_POS and position of
        // last not set bit of region, otherwise
        size_t get_prev_free_region(size_t len, size_t offset = NO_POS){
            size_t pos = offset;
            while(1){
                //potential last bit
                size_t prev_clear = get_prev_clear_bit(pos);
                if(prev_clear == NO_POS) break;
                // now check free space, starting from our free found
                // running at least len bits + some gap back, so we not run to much uselessly, 
                // if not found we are OK we have free region
                // so we return first free - wanted len
                pos = get_prev_set_bit(prev_clear, len + _slot_size_bits);
                if(pos == NO_POS || ( (prev_clear - len) >= pos )){
                    //not found any used in limited area, or found and there is enough room
                    return (prev_clear - len + 1); //we have enough space last is contained so + 1
                }
            }
            return NO_POS;
        }

        private :

            template <typename T, T m, int k>
            static inline T swapbits(T p) {
                T q = ((p>>k)^p)&m;
                return p^q^(q<<k);
            }

            // Knuth�s 64-bit reverse
            uint64_t kbitreverse (boost::uint64_t n)
            {
                static const boost::uint64_t m0 = 0x5555555555555555ull;
                static const boost::uint64_t m1 = 0x0300c0303030c303ull;
                static const boost::uint64_t m2 = 0x00c0300c03f0003full;
                static const boost::uint64_t m3 = 0x00000ffc00003fffull;
                n = ((n>>1)&m0) | (n&m0)<<1;
                n = swapbits<boost::uint64_t, m1, 4>(n);
                n = swapbits<boost::uint64_t, m2, 8>(n);
                n = swapbits<boost::uint64_t, m3, 20>(n);
                n = (n >> 34) | (n << 30);
                return n;
            }

        public :

        /*
        std::string num32bit_2_bin(boost::uint32_t val){
            
            std::string num("00000000000000000000000000000000");
            char tmp[33];
            num.append(itoa(val, tmp, 2));
            return num.substr(num.size() - 32, 32);
        }*/

        std::string num32bit_2_bin(boost::uint32_t n)
        {
            const size_t size = sizeof(n) * 8;
            std::string num("00000000000000000000000000000000");
            char result[size];

            unsigned index = size;
            do {
                result[--index] = '0' + (n & 1);
            } while (n >>= 1);
            num.append(result + index, result + size);
            return num.substr(num.size() - 32, 32);
        }

        //dump bits in stream
        std::ostream& dump_to(std::ostream& dest){
            //seek
            dest.seekp(0, std::ios_base::end);
            std::streamoff pos = dest.tellp();
            std::string sep = pos != 0 ? "," : "";
            for(int i = 0; i < _slots_size; ++i){
                //low
                boost::uint64_t val = kbitreverse(_slots[i]);
                dest << sep << num32bit_2_bin(static_cast<boost::uint32_t>(val >> 32)) << "," << num32bit_2_bin(static_cast<boost::uint32_t>(val));
                sep = ",";
            } 

            return dest;
        }

        void dump_to_err(const char* fname){
            FILE* f = fopen(fname, "a+b");
            if(f){
                for(int i = 0; i < _slots_size; ++i){
                    //low
                    boost::uint64_t val = kbitreverse(_slots[i]);
                    std::string up = num32bit_2_bin(static_cast<boost::uint32_t>(val >> 32));
                    std::string down = num32bit_2_bin(static_cast<boost::uint32_t>(val));
                    fprintf(f, "%s,%s\n" , up.c_str() ,  down.c_str());
                }
                fclose(f);
            }
        }
        
    };

    //types
    typedef bit_array<dtl::auto_unchecked_ptr>          bit_array_def_alloc_t;
    typedef bit_array<xw::os::vm_auto_unchecked_ptr>    bit_array_vm_alloc_t;

}} // xw::cntrs

#endif /* XW_INCL_CNTR_BIT_ARRAY_HPP_ */
