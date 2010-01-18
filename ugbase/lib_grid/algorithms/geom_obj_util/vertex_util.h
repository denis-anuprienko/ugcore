//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y09 m02 d02

#ifndef __H__LIB_GRID__VERTEX_UTIL__
#define __H__LIB_GRID__VERTEX_UTIL__

#include <vector>
#include "lib_grid/lg_base.h"
#include "common/math/ugmath.h"

namespace ug
{
/** \defgroup vertexUtil Vertex Util
 * @{
 */

////////////////////////////////////////////////////////////////////////
//	GetVertexIndex
///	returns the index at which vertex v is found in the given edge
/**
 * returns -1 if the vertex was not found.
 */
int GetVertexIndex(EdgeBase* e, VertexBase* v);

////////////////////////////////////////////////////////////////////////
//	GetVertexIndex
///	returns the index at which vertex v is found in the given face
/**
 * returns -1 if the vertex was not found.
 */
int GetVertexIndex(Face* f, VertexBase* v);

////////////////////////////////////////////////////////////////////////
//	GetVertexIndex
///	returns the index at which vertex v is found in the given volume
/**
 * returns -1 if the vertex was not found.
 */
int GetVertexIndex(Volume* vol, VertexBase* v);

////////////////////////////////////////////////////////////////////////
//	GetConnectedVertex
///	returns the vertex that is connected to v via e.
/**
 * returns NULL if v is not contained in e.
 */
VertexBase* GetConnectedVertex(EdgeBase* e, VertexBase* v);

////////////////////////////////////////////////////////////////////////
//	GetConnectedVertexIndex
///	returns the index of the first vertex that is contained in the specified face and is not contained in the given edge.
int GetConnectedVertexIndex(Face* f, const EdgeDescriptor& ed);

////////////////////////////////////////////////////////////////////////
//	CollectNeighbours
///	fills an array with all neighbour-vertices of v.
/**
 * v will not be contained in vNeighboursOut.
 * requires grid-option GRIDOPT_STANDARD_INTERCONNECTION.
 * This method is fast if grid-options FACEOPT_AUTOGENERATE_EDGES
 * and VOLOPT_AUTOGENERATE_EDGES are enabled - if there are any
 * faces and volumes.
 * It works without these options, too. The method however will
 * require more time in this case.
 */
void CollectNeighbours(std::vector<VertexBase*>& vNeighborsOut, Grid& grid, VertexBase* v);

////////////////////////////////////////////////////////////////////////
//	FindVertexByCoordiante
///	returns the vertex that is the closest to the given coordinate
/**
 * returns NULL if no vertex was found (if iterBegin == iterEnd).
 */
VertexBase* FindVertexByCoordiante(vector3& coord, VertexBaseIterator iterBegin, VertexIterator iterEnd,
									Grid::VertexAttachmentAccessor<APosition>& aaPos);

////////////////////////////////////////////////////////////////////////
//	CalculateBoundingBox
/// calculates the BoundingBox
void CalculateBoundingBox(vector3& vMinOut, vector3& vMaxOut, VertexBaseIterator vrtsBegin,
						  VertexBaseIterator vrtsEnd, Grid::VertexAttachmentAccessor<AVector3>& aaPos);

////////////////////////////////////////////////////////////////////////
//	CalculateBarycenter - mstepnie
/// calculates the barycenter of a set of vertices
vector3 CalculateBarycenter(VertexBaseIterator vrtsBegin, VertexBaseIterator vrtsEnd,
							Grid::VertexAttachmentAccessor<AVector3>& aaPos);

////////////////////////////////////////////////////////////////////////
//	MergeVertices
///	merges two vertices and restructures the adjacent elements.
/**
 * Since vertex v2 has to be removed in the process, the associated elements
 * of this vertex have to be replaced by new ones. Values attached to
 * old elements are passed on to the new ones using grid::pass_on_values.
 */
void MergeVertices(Grid& grid, VertexBase* v1, VertexBase* v2);

////////////////////////////////////////////////////////////////////////
//	RemoveDoubles
///	merges all vertices that are closer to each other than the specified threshold.
void RemoveDoubles(Grid& grid, const VertexBaseIterator& iterBegin,
					const VertexBaseIterator& iterEnd, AVector3& aPos,
					number threshold);

////////////////////////////////////////////////////////////////////////
///	returns whether a vertex lies on the boundary of a 2D grid.
/** A vertex is regarded as a 2d boundary vertex if it lies on a
 * 2d boundary edge.
 * if EDGEOPT_STORE_ASSOCIATED_FACES and VRTOPT_STORE_ASSOCIATED_EDGES
 * are enabled, the algorithm will be faster.
 */
bool IsBoundaryVertex2D(Grid& grid, VertexBase* v);

/**@}*/ // end of doxygen defgroup command

}//	end of namespace

#endif
