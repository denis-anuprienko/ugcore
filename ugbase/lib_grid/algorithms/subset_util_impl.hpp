//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y09 m07 d21

#ifndef __H__LIB_GRID__SUBSET_UTIL_IMPL__
#define __H__LIB_GRID__SUBSET_UTIL_IMPL__

#include "subset_util.h"

namespace ug
{
////////////////////////////////////////////////////////////////////////
//	FindFirstFreeSubset
template <class TElem>
int GetMaxSubsetIndex(SubsetHandler& sh)
{
//	go from back to front
	for(int i = (int)sh.num_subsets() - 1; i >= 0; --i)
	{
		if(sh.num_elements<TElem>(i) > 0)
		{
		//	this is the highest subset that contains elements of type TElem
			return i;
		}
	}

//	no subset contains elements of type TElem.
	return -1;
}

////////////////////////////////////////////////////////////////////////
//	MakeSubsetsConsecutive
template <class TElem>
void MakeSubsetsConsecutive(SubsetHandler& sh)
{
//	TODO: this algo could be slightly improved regarding runtime.

//	iterate through all subsets.
	for(int i = 0; i < sh.num_subsets(); ++i)
	{
	//	check whether the subset is empty
		if(sh.num_elements<TElem>(i) == 0)
		{
		//	it is. find the next filled one.
			for(int j = i + 1; j < sh.num_subsets(); ++j)
			{
				if(sh.num_elements<TElem>(j) > 0)
				{
				//	this subset is filled. move it to position i.
					sh.move_subset(j, i);
					break;
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
//	AssignAssociatedVerticesToSubset
template <class TIterator>
void AssignAssociatedVerticesToSubset(ISubsetHandler& sh, TIterator elemsBegin,
										TIterator elemsEnd, int subsetIndex)
{
//	iterate through the elements
	for(;elemsBegin != elemsEnd; elemsBegin++)
	{
		typename TIterator::value_type elem = *elemsBegin;
		uint numVrts = elem->num_vertices();
	//	iterate through the vertices of elem and assign them
		for(uint i = 0; i < numVrts; ++i)
			sh.assign_subset(elem->vertex(i), subsetIndex);
	}
}

////////////////////////////////////////////////////////////////////////
template <class TElem, class TSubsetHandler>
void AssignAssociatedVerticesToSubsets(TSubsetHandler& sh,
									const ISubsetHandler& srcIndHandler)
{
	typedef typename geometry_traits<TElem>::const_iterator iterator;
	for(size_t l  = 0; l < sh.num_levels(); ++l){
		for(int si = 0; si < sh.num_subsets(); ++si){
			for(iterator iter = sh.template begin<TElem>(si, l);
				iter != sh.template end<TElem>(si, l); ++iter)
			{
				TElem* e = *iter;
				for(size_t i = 0; i < e->num_vertices(); ++i)
				{
					VertexBase* vrt = e->vertex(i);
					sh.assign_subset(vrt, srcIndHandler.get_subset_index(vrt));
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
template <class TElem, class TSubsetHandler>
void AssignAssociatedEdgesToSubsets(TSubsetHandler& sh,
									const ISubsetHandler& srcIndHandler)
{
	typedef typename geometry_traits<TElem>::const_iterator iterator;
	std::vector<EdgeBase*> vEdges;

	for(size_t l  = 0; l < sh.num_levels(); ++l){
		for(int si = 0; si < sh.num_subsets(); ++si){
			for(iterator iter = sh.template begin<TElem>(si, l);
				iter != sh.template end<TElem>(si, l); ++iter)
			{
				TElem* e = *iter;
				CollectEdges(vEdges, *sh.get_assigned_grid(), e);

				for(size_t i = 0; i < vEdges.size(); ++i)
				{
					EdgeBase* edge = vEdges[i];
					sh.assign_subset(edge, srcIndHandler.get_subset_index(edge));
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
template <class TElem, class TSubsetHandler>
void AssignAssociatedFacesToSubsets(TSubsetHandler& sh,
									const ISubsetHandler& srcIndHandler)
{
	typedef typename geometry_traits<TElem>::const_iterator iterator;
	std::vector<Face*> vFaces;

	for(size_t l  = 0; l < sh.num_levels(); ++l){
		for(int si = 0; si < sh.num_subsets(); ++si){
			for(iterator iter = sh.template begin<TElem>(si, l);
				iter != sh.template end<TElem>(si, l); ++iter)
			{
				TElem* e = *iter;
				CollectFaces(vFaces, *sh.get_assigned_grid(), e);

				for(size_t i = 0; i < vFaces.size(); ++i)
				{
					Face* f = vFaces[i];
					sh.assign_subset(f, srcIndHandler.get_subset_index(f));
				}
			}
		}
	}
}

///	helper with with dummy-param for compile-time function selection.
template <class TElem, class TSubsetHandlerDest, class TSubsetHandlerSrc>
void AssignAssociatedLowerDimElemsToSubsets(TSubsetHandlerDest& sh,
									const TSubsetHandlerSrc& srcIndHandler,
									const Volume&)
{
//	we have to find all associated elements of lower dimension.
	if(srcIndHandler.template num<Face>() > 0)
		AssignAssociatedFacesToSubsets<TElem>(sh, srcIndHandler);
	if(srcIndHandler.template num<EdgeBase>() > 0)
		AssignAssociatedEdgesToSubsets<TElem>(sh, srcIndHandler);
	if(srcIndHandler.template num<VertexBase>() > 0)
		AssignAssociatedVerticesToSubsets<TElem>(sh, srcIndHandler);
}

///	helper with with dummy-param for compile-time function selection.
template <class TElem, class TSubsetHandlerDest, class TSubsetHandlerSrc>
void AssignAssociatedLowerDimElemsToSubsets(TSubsetHandlerDest& sh,
									const TSubsetHandlerSrc& srcIndHandler,
									const Face&)
{
//	we have to find all associated elements of lower dimension.
	if(srcIndHandler.template num<EdgeBase>() > 0)
		AssignAssociatedEdgesToSubsets<TElem>(sh, srcIndHandler);
	if(srcIndHandler.template num<VertexBase>() > 0)
		AssignAssociatedVerticesToSubsets<TElem>(sh, srcIndHandler);
}

///	helper with with dummy-param for compile-time function selection.
template <class TElem, class TSubsetHandlerDest, class TSubsetHandlerSrc>
void AssignAssociatedLowerDimElemsToSubsets(TSubsetHandlerDest& sh,
									const TSubsetHandlerSrc& srcIndHandler,
									const EdgeBase&)
{
//	we have to find all associated elements of lower dimension.
	if(srcIndHandler.template num<VertexBase>() > 0)
		AssignAssociatedVerticesToSubsets<TElem>(sh, srcIndHandler);
}

////////////////////////////////////////////////////////////////////////
template <class TElem, class TSubsetHandlerDest, class TSubsetHandlerSrc>
void AssignAssociatedLowerDimElemsToSubsets(TSubsetHandlerDest& sh,
									const TSubsetHandlerSrc& srcIndHandler)
{
	AssignAssociatedLowerDimElemsToSubsets<TElem>(sh,
											srcIndHandler, TElem());
}

////////////////////////////////////////////////////////////////////////
//	CreateSurfaceView
template <class TIterator>
void CreateSurfaceView(SubsetHandler& shSurfaceViewOut, MultiGrid& mg,
						ISubsetHandler& sh, TIterator iterBegin,
						TIterator iterEnd)
{
	while(iterBegin != iterEnd)
	{
		if(!mg.has_children(*iterBegin)){
			shSurfaceViewOut.assign_subset(*iterBegin,
									sh.get_subset_index(*iterBegin));
		}
		++iterBegin;
	}
}

template <class TElem>
void SeparateSubsetsByLowerDimSubsets(Grid& grid, SubsetHandler& sh)
{
	SeparateSubsetsByLowerDimSeparators<TElem>(grid, sh, IsNotInSubset(sh, -1));
}

template <class TElem>
void SeparateSubsetsByLowerDimSelection(Grid& grid, SubsetHandler& sh,
										Selector& sel)
{
	SeparateSubsetsByLowerDimSeparators<TElem>(grid, sh, IsSelected(sel));
}

template <class TElem>
void SeparateSubsetsByLowerDimSeparators(Grid& grid, SubsetHandler& sh,
					boost::function<bool (typename TElem::lower_dim_base_object*)>
						cbIsSeparator)

{
	using namespace std;

//	the element type of separating elements
	typedef typename TElem::lower_dim_base_object	TSide;

//	assign all elements to subset -1
	sh.assign_subset(grid.begin<TElem>(), grid.end<TElem>(), -1);

//	we'll keep all unassigned volumes in a selector.
	Selector sel(grid);
	sel.select(grid.begin<TElem>(), grid.end<TElem>());

//	those vectors will be used to gather element neighbours.
	vector<TSide*> vSides;
	vector<TElem*> vElems;

//	this stack contains all volumes that we still have to check for neighbours.
	stack<TElem*> stkElems;

//	now - while there are unassigned elements.
	int subsetIndex = 0;
	while(!sel.empty())
	{
	//	choose the element with which we want to start
	//	TODO: if material-points are supplied, this should be the
	//		the element that contains the i-th material point.
		stkElems.push(*sel.begin<TElem>());
		while(!stkElems.empty())
		{
			TElem* elem = stkElems.top();
			stkElems.pop();
		//	if the volume is unselected it has already been processed.
			if(!sel.is_selected(elem))
				continue;
			sel.deselect(elem);

		//	assign elem to its new subset
			sh.assign_subset(elem, subsetIndex);

		//	check neighbour-elements, whether they belong to the same subset.
		//	iterate through the sides of the element
			for(uint i = 0; i < elem->num_sides(); ++i)
			{
			//	get the i-th side
				TSide* side = grid.get_side(elem, i);

			//	check whether the side is regarded as a separator.
			//	If not, we'll add all associated elements.
				if(!cbIsSeparator(side))
				{
					CollectAssociated(vElems, grid, side);

				//	add all elements that are still selected (elem is not selected anymore).
					for(uint j = 0; j < vElems.size(); ++j)
					{
						if(sel.is_selected(vElems[j]))
							stkElems.push(vElems[j]);
					}
				}
			}
		}
	//	the stack is empty. increase subset index.
		subsetIndex++;
	}
}

}//	end of namespace

#endif
