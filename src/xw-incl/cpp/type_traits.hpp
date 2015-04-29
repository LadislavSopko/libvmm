#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#ifndef __TYPE_INFO_HPP__
#define __TYPE_INFO_HPP__

////////////////////////////////////////////////////////////////////////
// type_info.hpp
// A very basic set of typeinfo traits types.
// Author: stephan@s11n.net, based off of much prior art
// Modified: L.Sopko
// License: Public Domain
////////////////////////////////////////////////////////////////////////

namespace xw{ namespace cpp{

	/**
	   A base type for type_traits. No code should use this class
	   directly, except to subclass it.
	*/
	template < typename T, bool IsConst, bool IsPointer, bool IsReference >
	struct type_traits_base
	{
		/** Same as (T). */
		typedef T type;
		/** Same as (T*). **/
		typedef T * pointer;
		/** Same as (T&). **/
		typedef T & reference;
		/** True if T is a const type, else false. */
		static const bool is_const_type = IsConst;
		/** True if T is a pointer type, else false. */
		static const bool is_pointer_type = IsPointer;
		/** True if T is a reference type, else false. */
		static const bool is_reference_type = IsReference;
	};


	/**
	   A simple type traits container. All of its typedefs are
	   documented in the parent type.
	*/
	template < typename T >
	struct type_traits : public type_traits_base<T,false,false,false>
	{
		typedef T type;
		typedef T * pointer;
		typedef T & reference;
	};


	/** Specialization for (const T). */
	template < typename T >
	struct type_traits< const T > : public type_traits_base<T,true,false,false>
	{
		typedef T type;
		typedef T * pointer;
		typedef T & reference;
	};



	/** Specialization for (T &). */
	template < typename T >
	struct type_traits< T & > : public type_traits_base<T,false,false,true>
	{
		typedef T type;
		typedef T * pointer;
		typedef T & reference;
	};

	/** Specialization for (const T &). */
	template < typename T >
	struct type_traits< const T & > : public type_traits_base<T,true,false,true>
	{
		typedef T type;
		typedef T * pointer;
		typedef T & reference;
	};


	/** Specialization for (T *). */
	template < typename T >
	struct type_traits< T * > : public type_traits_base<T,false,true,false>
	{
		typedef T type;
		typedef T * pointer;
		typedef T & reference;
	};


	/** Specialization for (const T *). */
	template < typename T >
	struct type_traits< const T * > : public type_traits_base<T,true,true,false>
	{
		typedef T type;
		typedef T * pointer;
		typedef T & reference;
	};

}} // namespace
#endif 
