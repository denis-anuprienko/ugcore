
#ifndef __H__UG__MAP_ALGEBRA__SPARSEMATRIX_PRINT__
#define  __H__UG__MAP_ALGEBRA__SPARSEMATRIX_PRINT__

#include "mapsparsematrix.h"
#include "common/common.h"

namespace ug {

//!
//! print to console whole SparseMatrix
template<typename T>
void MapSparseMatrix<T>::print(const char * const text) const
{
	UG_LOG("================= MapSparseMatrix " << num_rows() << "x" << num_cols() << " =================\n");
	for(size_t i=0; i < num_rows(); i++)
		printrow(i);
}


//!
//! print the row row to the console
template<typename T>
void MapSparseMatrix<T>::printrow(size_t row) const
{
	UG_LOG("row " << row << ": ");
	for(const_row_iterator it=begin_row(row); it != end_row(row); ++it)
	{
		if(it.value() == 0.0) continue;
		UG_LOG(" ");
		UG_LOG("(" << it.index() << " -> " << it.value() << ")");
	}

	UG_LOG("\n");
}

template<typename T>
void MapSparseMatrix<T>::printtype() const
{
	std::cout << *this;
}

}
#endif // __H__UG__MAP_ALGEBRA__SPARSEMATRIX_PRINT__
