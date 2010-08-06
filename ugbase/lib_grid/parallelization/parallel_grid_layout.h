//	created by Sebastian Reiter
//	s.b.reiter@googlemail.com
//	y09 m12 d08

#ifndef __H__LIB_GRID__PARALLEL_GRID_LAYOUT__
#define __H__LIB_GRID__PARALLEL_GRID_LAYOUT__

#include <vector>
#include <list>
#include <map>
#include "pcl/pcl.h"
#include "lib_grid/lg_base.h"

//	specialize pcl::type_traits for VertexBase, EdgeBase, Face and Volume
namespace pcl
{
///	VertexBase interfaces and layouts store elements of type VertexBase*
template <>
struct type_traits<ug::VertexBase>
{
	typedef ug::VertexBase* Elem;
};

///	EdgeBase interfaces and layouts store elements of type VertexBase*
template <>
struct type_traits<ug::EdgeBase>
{
	typedef ug::EdgeBase* Elem;
};

///	Face interfaces and layouts store elements of type VertexBase*
template <>
struct type_traits<ug::Face>
{
	typedef ug::Face* Elem;
};

///	Volume interfaces and layouts store elements of type VertexBase*
template <>
struct type_traits<ug::Volume>
{
	typedef ug::Volume* Elem;
};

}//	end of namespace pcl

namespace ug
{

/// \addtogroup lib_grid_parallelization
/// @{

////////////////////////////////////////////////////////////////////////
///	The types of interface-entries.
/**	INT_MASTER and INT_SLAVE describe (horizontal) connections between
 *	nodes on one level in a grid-hierarchy. They are used to communicate
 *	data between neighbours.
 *
 *	INT_VERTICAL_MASTER and INT_VERTICAL_SLAVE describe connections
 *	between nodes on different levels of a grid. They are used to
 *	communicate data between parents and children. They are only used
 *	for multigrids.
 */
enum InterfaceNodeTypes
{
	INT_UNKNOWN =	0,
	INT_MASTER,	///< horizontal master
	INT_SLAVE,	///< horizontal slave
	INT_VIRTUAL_MASTER,	///< virtual horizontal master. Required to build master-interfaces in special cases.
	INT_VIRTUAL_SLAVE,	///< virtual horizontal slave. Required to build slave-interfaces in special cases.
	INT_VERTICAL_MASTER,
	INT_VERTICAL_SLAVE
};

//	declare vertex-, edge-, face- and volume-layouts
//	we're using std::list as interface-element container, since we
//	require interface-element-iterators that stay valid even if the
//	interface is altered.
//	Make sure that those layouts match the ones in GridLayoutMap.
typedef pcl::MultiLevelLayout<
		pcl::OrderedInterface<VertexBase, std::list> >	VertexLayout;
typedef pcl::MultiLevelLayout<
		pcl::OrderedInterface<EdgeBase, std::list> >	EdgeLayout;
typedef pcl::MultiLevelLayout<
		pcl::OrderedInterface<Face, std::list> >		FaceLayout;
typedef pcl::MultiLevelLayout<
		pcl::OrderedInterface<Volume, std::list> >		VolumeLayout;


////////////////////////////////////////////////////////////////////////
//	GridLayoutMap
///	lets you access layouts by type and key
/**
 * The GridLayoutMap helps you to organize your layouts
 * (e.g. master- and slave-layouts).
 *
 * You may query layouts for VertexBase, EdgeBase, Face and Volume.
 *
 * You may use a LayoutMap as follows:
 *
 * \code
 * GridLayoutMap layoutMap;
 * assert(!layoutMap.has_layout<VertexBase>(0));
 * VertexLayout& layout = layoutMap.get_layout<VertexBase>(0);
 * assert(layoutMap.has_layout<VertexBase>(0));
 * \endcode
 *
 * To get associated types you may use the GridLayoutMap::Types array:
 * \code
 * GridLayoutMap::Types<VertexBase>::Layout l = layoutMap.get_layout<VertexBase>(0);
 * \endcode
 *
 * The Types struct is very useful when it comes to using a LayoutMap in
 * template code, too.
 */
class GridLayoutMap
{
	public:
		typedef int	Key;

	///	defines the types that are used by a LayoutMap for a given TType.
		template <class TType>
		struct Types
		{
			typedef pcl::OrderedInterface<TType, std::list> 	Interface;
			typedef typename pcl::MultiLevelLayout<Interface>	Layout;
			typedef typename Interface::Element					Element;
			typedef std::map<Key, Layout>						Map;
		};

	public:
	///	checks whether the layout associated with the given key exists for the given type.
		template <class TType>
		bool
		has_layout(const Key& key);

	///	creates the required layout if it doesn't exist already.
		template <class TType>
		typename Types<TType>::Layout&
		get_layout(const Key& key);

	///	begin-iterator to the layout-map for the given type.
	/**	iter.first will return the key, iter.second the layout
	 *	(of type LayoutMap::Types<TType>::Layout).*/
		template <class TType>
		typename Types<TType>::Map::iterator
		layouts_begin();

	///	end-iterator to the layout-map for the given type.
	/**	iter.first will return the key, iter.second the layout
	 *	(of type LayoutMap::Types<TType>::Layout).*/
		template <class TType>
		typename Types<TType>::Map::iterator
		layouts_end();

	///	erases the specified layout
	/**	returns an iterator to the next layout.*/
		template <class TType>
		typename Types<TType>::Map::iterator
		erase_layout(typename Types<TType>::Map::iterator iter);
								
	///	erases the specified layout if it exists
		template <class TType>
		void erase_layout(const Key& key);

	private:
		template <class TType>
		inline typename Types<TType>::Map&
		get_layout_map();
		
	///	the argument is only a dummy to allow to choose the right method at compile time
	// \{
		inline Types<VertexBase>::Map&
		get_layout_map(VertexBase*);

		inline Types<EdgeBase>::Map&
		get_layout_map(EdgeBase*);

		inline Types<Face>::Map&
		get_layout_map(Face*);

		inline Types<Volume>::Map&
		get_layout_map(Volume*);
	// \}
	
	private:
		Types<VertexBase>::Map	m_vertexLayoutMap;
		Types<EdgeBase>::Map	m_edgeLayoutMap;
		Types<Face>::Map		m_faceLayoutMap;
		Types<Volume>::Map		m_volumeLayoutMap;
};

/// @}
}//	end of namespace

////////////////////////////////
//	include implementation
#include "parallel_grid_layout_impl.hpp"

#endif
