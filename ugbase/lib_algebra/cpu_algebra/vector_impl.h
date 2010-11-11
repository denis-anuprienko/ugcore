/*
 *  Vector.hpp
 *  flexamg
 *
 *  Created by Martin Rupp on 04.11.09.
 *  Copyright 2009 G-CSC, University of Frankfurt. All rights reserved.
 *
 */

#ifndef __H__UG__CPU_ALGEBRA__VECTOR_IMPL__
#define __H__UG__CPU_ALGEBRA__VECTOR_IMPL__

#include <fstream>
#include "algebra_misc.h"
#include "common/math/ugmath.h" // for urand

#define prefetchReadWrite(a)

namespace ug{
template<typename value_type>
inline value_type &Vector<value_type>::operator [] (size_t i)
{
	UG_ASSERT(i >= 0 && i < length, *this << ": tried to access element " << i);
	return values[i];
}

template<typename value_type>
inline const value_type &Vector<value_type>::operator [] (size_t i) const
{
	UG_ASSERT(i >= 0 && i < length, *this << ": tried to access element " << i);
	return values[i];
}


// energynorm2 = x*(A*x)
/*inline double Vector<value_type>::energynorm2(const SparseMatrix &A) const
{
	double sum=0;
	for(size_t i=0; i<length; i++)	sum += (A[i] * (*this)) * values[i];
	//FOR_UNROLL_FWD(i, 0, length, UNROLL, sum += A[i] * (*this) * values[i]);
	return sum;
}*/

// dotprod
template<typename value_type>
inline double Vector<value_type>::dotprod(const Vector &w) //const
{
	UG_ASSERT(length == w.length,  *this << " has not same length as " << w);

	double sum=0;
	for(size_t i=0; i<length; i++)	sum += VecProd(values[i], w[i]);
	return sum;
}

// assign double to whole Vector
template<typename value_type>
inline double Vector<value_type>::operator = (double d)
{
	for(size_t i=0; i<length; i++)
		values[i] = d;
	return d;
}

template<typename value_type>
inline bool Vector<value_type>::set_random(double from, double to)
{
	for(size_t i=0; i<size(); i++)
		for(size_t j=0; j<GetSize(values[i]); j++)
			BlockRef(values[i], j) = urand(from, to);
	return true;
}


// für Function Expression, sh. TemplateExpression.h
template<typename value_type>
template<class Function> inline void Vector<value_type>::operator = (Function &ex)
{
	ex.applyto(*this);
}

template<typename value_type>
inline void Vector<value_type>::operator = (const Vector &v)
{
	v.applyto(*this);
}
template<typename value_type>
inline void Vector<value_type>::applyto(Vector &v) const
{
	UG_ASSERT(v.length == length, *this << " has not same length as " << v);

	for(size_t i=0; i<length; i++)
		v.values[i] = values[i];
}


template<typename value_type>
template<typename Type> inline void Vector<value_type>::operator = (const Type &t)
{
	//IF_PRINTLEVEL(5) cout << *this << " = " << t << " (unspecialized) " << endl;
	/*UG_ASSERT(t.size() == length, *this << " has not same length as " << t);
	t.preventForbiddenDestination(this);

	for(size_t i=0; i < length; i++)
	{
		prefetchReadWrite(values+i+512);
		t.assign(values[i], i);
	}*/
	VectorAssign(*this, t);
}

// v += exp
template<typename value_type>
template<typename Type> inline void Vector<value_type>::operator += (const Type &t)
{
	/*UG_ASSERT(t.size() == length, *this << " has not same length as " << t);

	for(size_t i=0; i < length; i++)
	{
		prefetchReadWrite(values+i+512);
		t.addTo(values[i], i);
	}*/
	VectorAdd(*this, t);
}

// v -= exp
template<typename value_type>
template<typename Type> inline void Vector<value_type>::operator -= (const Type &t)
{
	/*UG_DLOG(LIB_ALG_VECTOR, 5, *this << " -= " << t << " (unspecialized) ");
	UG_ASSERT(t.size() == length, *this << " has not same length as " << t);
	//t.preventForbiddenDestination(this);

	for(size_t i=0; i < length; i++)
		t.substractFrom(values[i], i);
		*/
	VectorSub(*this, t);
}



////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


template<typename value_type>
Vector<value_type>::Vector ()
{
	FORCE_CREATION { p(); } // force creation of this rountines for gdb.

	length = 0; values = NULL;
}

template<typename value_type>
Vector<value_type>::Vector(size_t _length)
{
	FORCE_CREATION { p(); } // force creation of this rountines for gdb.

	length = 0;
	create(_length);
}

template<typename value_type>
Vector<value_type>::~Vector()
{
	destroy();
}

template<typename value_type>
bool Vector<value_type>::destroy()
{
	if(values)
	{
		delete [] values;
		values = NULL;
	}
	length = 0;
	return true;
}


template<typename value_type>
bool Vector<value_type>::create(size_t _length)
{
	UG_ASSERT(length == 0, *this << " already created");
	length = _length;
	values = new value_type[length];

	return true;
}


template<typename value_type>
bool Vector<value_type>::create(const Vector &v)
{
	UG_ASSERT(length == 0, *this << " already created");
	length = v.length;
	values = new value_type[length];

	// we cannot use memcpy here bcs of variable blocks.
	for(size_t i=0; i<length; i++)
		values[i] = v.values[i];

	return true;
}


// print
template<typename value_type>
void Vector<value_type>::print(const char * const text) const
{

	if(text) cout << " == " << text;
	cout << " length: " << length << " =================" << endl;
	for(size_t i=0; i<length; i++)
		//cout << values[i] << " ";
		cout << i << ": " << values[i] << endl;
	cout << endl;
}

template<typename value_type>
void Vector<value_type>::printtype() const
{
	cout << *this;
}


template<typename value_type>
template<typename V>
bool Vector<value_type>::add(const V& u)
{
	for(size_t i=0; i < u.size(); i++)
		values[u.index(i)] += u[i];
	return true;
}

template<typename value_type>
template<typename V>
bool Vector<value_type>::set(const V& u)
{
	for(size_t i=0; i < u.size(); i++)
		values[u.index(i)] = u[i];
	return true;
}

template<typename value_type>
template<typename V>
bool Vector<value_type>::get(V& u) const
{
	for(size_t i=0; i < u.size(); i++)
		u[i] = values[u.index(i)];
	return true;
}




template<typename value_type>
bool Vector<value_type>::add(const value_type *u, const size_t *indices, int nr)
{
	for(size_t i=0; i < nr; i++)
		values[indices[i]] += u[i];
	return true;
}

template<typename value_type>
bool Vector<value_type>::set(const value_type *u, const size_t *indices, int nr)
{
	for(size_t i=0; i < nr; i++)
		values[indices[i]] = u[i];
	return true;
}

template<typename value_type>
bool Vector<value_type>::get(value_type *u, const size_t *indices, int nr) const
{
	for(size_t i=0; i < nr; i++)
		u[i] = values[indices[i]] ;
	return true;
}



template<typename value_type>
void Vector<value_type>::add(const value_type &d, size_t i)
{
	values[i] += d;
}
template<typename value_type>
void Vector<value_type>::set(const value_type &d, size_t i)
{
	values[i] = d;
}
template<typename value_type>
void Vector<value_type>::get(value_type &d, size_t i) const
{
	d = values[i];
}


template<typename value_type>
double operator *(const TRANSPOSED<Vector<value_type> > &x, const Vector<value_type> &y)
{
	return x.T().dotprod(y);
}

template<typename value_type>
inline double Vector<value_type>::norm()
{
	double d=0;
	for(size_t i=0; i<size(); ++i)
		d+=BlockNorm2(values[i]);
	return sqrt(d);
}

}//namespace ug

#endif
