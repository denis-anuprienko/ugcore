#ifndef FV1IB_GEOM_H_
#define FV1IB_GEOM_H_

// extern libraries
#include <cmath>
#include <map>
#include <vector>

// other ug4 modules
#include "common/common.h"

// library intern includes
#include "lib_grid/tools/subset_handler_interface.h"
#include "lib_disc/reference_element/reference_element.h"
#include "lib_disc/reference_element/reference_element_traits.h"
#include "lib_disc/reference_element/reference_mapping.h"
#include "lib_disc/reference_element/reference_mapping_provider.h"
#include "lib_disc/local_finite_element/local_finite_element_provider.h"
#include "lib_disc/local_finite_element/lagrange/lagrangep1.h"
#include "lib_disc/quadrature/gauss/gauss_quad.h"
#include "fv_util.h"
#include "fv_geom_base.h"

namespace ug{

////////////////////////////////////////////////////////////////////////////////
// FV1 Geometry for Reference Element Type
////////////////////////////////////////////////////////////////////////////////

/// Geometry and shape functions for 1st order Vertex-Centered Finite Volume
/**
 * \tparam	TElem		Element type
 * \tparam	TWorldDim	(physical) world dimension
 */
template <	typename TElem, int TWorldDim>
class FV1IBGeometry : public FVGeometryBase
{
public:
///	type of element
	typedef TElem elem_type;

///	type of reference element
	typedef typename reference_element_traits<TElem>::reference_element_type ref_elem_type;

///	used traits
	typedef fv1_traits<ref_elem_type, TWorldDim> traits;

public:
///	dimension of reference element
	static const int dim = ref_elem_type::dim;

///	dimension of world
	static const int worldDim = TWorldDim;

///	Hanging node flag: this Geometry does not support hanging nodes
	static const bool usesHangingNodes = false;

/// flag indicating if local data may change
	static const bool staticLocalData = true;

public:
///	order
	static const int order = 1;

///	number of SubControlVolumes
	static const size_t numSCV = (ref_elem_type::REFERENCE_OBJECT_ID != ROID_PYRAMID)
								? ref_elem_type::numCorners : 8;

///	type of SubControlVolume
	typedef typename traits::scv_type scv_type;

///	number of SubControlVolumeFaces
	static const size_t numSCVF = (ref_elem_type::REFERENCE_OBJECT_ID != ROID_PYRAMID)
								? ref_elem_type::numEdges : 12;

///	type of Shape function used
	typedef LagrangeP1<ref_elem_type> local_shape_fct_set_type;

///	number of shape functions
	static const size_t nsh = local_shape_fct_set_type::nsh;

/// number of integration points
	static const size_t nip = 1;

public:
///	Sub-Control Volume Face structure
/**
 * Each finite element is cut by several sub-control volume faces. The idea
 * is that the "dual" skeleton formed by the sub control volume faces of
 * all elements again gives rise to a regular mesh with closed
 * (lipschitz-bounded) control volumes. The SCVF are the boundary of the
 * control volume. In computation the flux over each SCVF must be the same
 * in both directions over the face in order to guarantee the conservation
 * property.
 */
	class SCVF
	{
		public:
		///	Number of corners of scvf
			static const size_t numCo =	traits::NumCornersOfSCVF;

		public:
			SCVF() {}

		/// index of SubControlVolume on one side of the scvf
			inline size_t from() const {return From;}

		/// index of SubControlVolume on one side of the scvf
			inline size_t to() const {return To;}

		/// normal on scvf (points direction "from"->"to"). Norm is equal to area
			inline const MathVector<worldDim>& normal() const {return Normal;}

		/// number of integration points on scvf
			inline size_t num_ip() const {return nip;}

		/// local integration point of scvf
			inline const MathVector<dim>& local_ip() const {return localIP;}

		/// global integration point of scvf
			inline const MathVector<worldDim>& global_ip() const {return globalIP;}

		/// Transposed Inverse of Jacobian in integration point
			inline const MathMatrix<worldDim,dim>& JTInv() const {return JtInv;}

		/// Determinant of Jacobian in integration point
			inline number detJ() const {return detj;}

		/// number of shape functions
			inline size_t num_sh() const {return nsh;}

		/// value of shape function i in integration point
			inline number shape(size_t sh) const {return vShape[sh];}

		/// vector of shape functions in ip point
			inline const number* shape_vector() const {return vShape;}

		/// value of local gradient of shape function i in integration point
			inline const MathVector<dim>& local_grad(size_t sh) const
				{UG_ASSERT(sh < num_sh(), "Invalid index"); return vLocalGrad[sh];}

		/// vector of local gradients in ip point
			inline const MathVector<dim>* local_grad_vector() const {return vLocalGrad;}

		/// value of global gradient of shape function i in integration point
			inline const MathVector<worldDim>& global_grad(size_t sh) const
				{UG_ASSERT(sh < num_sh(), "Invalid index"); return vGlobalGrad[sh];}

		/// vector of global gradients in ip point
			inline const MathVector<worldDim>* global_grad_vector() const {return vGlobalGrad;}

		/// number of corners, that bound the scvf
			inline size_t num_corners() const {return numCo;}

		/// return local corner number i
			inline const MathVector<dim>& local_corner(size_t co) const
				{UG_ASSERT(co < num_corners(), "Invalid index."); return vLocPos[co];}

		/// return global corner number i
			inline const MathVector<worldDim>& global_corner(size_t co) const
				{UG_ASSERT(co < num_corners(), "Invalid index."); return vGloPos[co];}

		protected:
		// 	let outer class access private members
			friend class FV1IBGeometry<TElem, TWorldDim>;

		// This scvf separates the scv with the ids given in "from" and "to"
		// The computed normal points in direction from->to
			size_t From, To;

		//	The normal on the SCVF pointing (from -> to)
			MathVector<worldDim> Normal; // normal (incl. area)

		// ordering is:
		// 1D: edgeMidPoint
		// 2D: edgeMidPoint, CenterOfElement
		// 3D: edgeMidPoint, Side one, CenterOfElement, Side two
			MathVector<dim> vLocPos[numCo]; // local corners of scvf
			MathVector<worldDim> vGloPos[numCo]; // global corners of scvf
			MidID vMidID[numCo]; // dimension and id of object, that's midpoint bounds the scvf

		// scvf part
			MathVector<dim> localIP; // local integration point
			MathVector<worldDim> globalIP; // global integration point

		// shapes and derivatives
			number vShape[nsh]; // shapes at ip
			MathVector<dim> vLocalGrad[nsh]; // local grad at ip
			MathVector<worldDim> vGlobalGrad[nsh]; // global grad at ip
			MathMatrix<worldDim,dim> JtInv; // Jacobian transposed at ip
			number detj; // Jacobian det at ip
	};

///	sub control volume structure
	class SCV
	{
		public:
		/// Number of corners of scvf
			static const size_t numCo = traits::NumCornersOfSCV;

		public:
			SCV() {};

		/// volume of scv
			inline number volume() const {return Vol;}

		/// number of corners, that bound the scvf
			inline size_t num_corners() const {return numCo;}

		/// return local corner number i
			inline const MathVector<dim>& local_corner(size_t co) const
				{UG_ASSERT(co < num_corners(), "Invalid index."); return vLocPos[co];}

		/// return global corner number i
			inline const MathVector<worldDim>& global_corner(size_t co) const
				{UG_ASSERT(co < num_corners(), "Invalid index."); return vGloPos[co];}

		/// return local corners
			inline const MathVector<dim>* local_corners() const
				{return &vLocPos[0];}

		/// return global corners
			inline const MathVector<worldDim>* global_corners() const
				{return &vGloPos[0];}

		/// node id that this scv is associated to
			inline size_t node_id() const {return nodeId;}

		/// number of integration points
			inline size_t num_ip() const {return nip;}

		/// local integration point of scv
			inline const MathVector<dim>& local_ip() const {return vLocPos[0];}

		/// global integration point
			inline const MathVector<worldDim>& global_ip() const {return vGloPos[0];}

		/// Transposed Inverse of Jacobian in integration point
			inline const MathMatrix<worldDim,dim>& JTInv() const {return JtInv;}

		/// Determinant of Jacobian in integration point
			inline number detJ() const {return detj;}

		/// number of shape functions
			inline size_t num_sh() const {return nsh;}

		/// value of shape function i in integration point
			inline number shape(size_t sh) const {return vShape[sh];}

		/// vector of shape functions in ip point
			inline const number* shape_vector() const {return vShape;}

		/// value of local gradient of shape function i in integration point
			inline const MathVector<dim>& local_grad(size_t sh) const
				{UG_ASSERT(sh < num_sh(), "Invalid index"); return vLocalGrad[sh];}

		/// vector of local gradients in ip point
			inline const MathVector<dim>* local_grad_vector() const {return vLocalGrad;}

		/// value of global gradient of shape function i in integration point
			inline const MathVector<worldDim>& global_grad(size_t sh) const
				{UG_ASSERT(sh < num_sh(), "Invalid index"); return vGlobalGrad[sh];}

		/// vector of global gradients in ip point
			inline const MathVector<worldDim>* global_grad_vector() const {return vGlobalGrad;}

		private:
		// 	let outer class access private members
			friend class FV1IBGeometry<TElem, TWorldDim>;

		//  node id of associated node
			size_t nodeId;

		//	volume of scv
			number Vol;

		//	local and global positions of this element
			MathVector<dim> vLocPos[numCo]; // local position of node
			MathVector<worldDim> vGloPos[numCo]; // global position of node
			MidID midId[numCo]; // dimension and id of object, that's midpoint bounds the scv

		// shapes and derivatives
			number vShape[nsh]; // shapes at ip
			MathVector<dim> vLocalGrad[nsh]; // local grad at ip
			MathVector<worldDim> vGlobalGrad[nsh]; // global grad at ip
			MathMatrix<worldDim,dim> JtInv; // Jacobian transposed at ip
			number detj; // Jacobian det at ip
	};

///	boundary face
	class BF
	{
		public:
		/// Number of corners of bf
			static const size_t numCo =	traits::NumCornersOfSCVF;

		public:
			BF() {}

		/// index of SubControlVolume of the bf
			inline size_t node_id() const {return nodeId;}

		/// number of integration points on bf
			inline size_t num_ip() const {return nip;}

		/// local integration point of bf
			inline const MathVector<dim>& local_ip() const {return localIP;}

		/// global integration point of bf
			inline const MathVector<worldDim>& global_ip() const {return globalIP;}

		/// outer normal on bf. Norm is equal to area
			inline const MathVector<worldDim>& normal() const {return Normal;} // includes area

		/// volume of bf
			inline number volume() const {return Vol;}

		/// Transposed Inverse of Jacobian in integration point
			inline const MathMatrix<worldDim, dim>& JTInv() const {return JtInv;}

		/// Determinant of Jacobian in integration point
			inline number detJ() const {return detj;}

		/// number of shape functions
			inline size_t num_sh() const {return nsh;}

		/// value of shape function i in integration point
			inline number shape(size_t sh) const
				{UG_ASSERT(sh < num_sh(), "Invalid index"); return vShape[sh];}

		/// vector of local gradients in ip point
			inline const number* shape_vector() const {return vShape;}

		/// value of local gradient of shape function i in integration point
			inline const MathVector<dim>& local_grad(size_t sh) const
				{UG_ASSERT(sh < num_sh(), "Invalid index"); return vLocalGrad[sh];}

		/// vector of local gradients in ip point
			inline const MathVector<dim>* local_grad_vector() const {return vLocalGrad;}

		/// value of global gradient of shape function i in integration point
			inline const MathVector<worldDim>& global_grad(size_t sh) const
				{UG_ASSERT(sh < num_sh(), "Invalid index"); return vGlobalGrad[sh];}

		/// vector of global gradients in ip point
			inline const MathVector<worldDim>* global_grad_vector() const {return vGlobalGrad;}

		/// number of corners, that bound the scvf
			inline size_t num_corners() const {return numCo;}

		/// return local corner number i
			inline const MathVector<dim>& local_corner(size_t co) const
				{UG_ASSERT(co < num_corners(), "Invalid index."); return vLocPos[co];}

		/// return global corner number i
			inline const MathVector<worldDim>& global_corner(size_t co) const
				{UG_ASSERT(co < num_corners(), "Invalid index."); return vGloPos[co];}

		private:
		/// let outer class access private members
			friend class FV1IBGeometry<TElem, TWorldDim>;

		// 	id of scv this bf belongs to
			size_t nodeId;

		// ordering is:
		// 1D: edgeMidPoint
		// 2D: edgeMidPoint, CenterOfElement
		// 3D: edgeMidPoint, Side one, CenterOfElement, Side two
			MathVector<dim> vLocPos[numCo]; // local corners of bf
			MathVector<worldDim> vGloPos[numCo]; // global corners of bf
			MidID vMidID[numCo]; // dimension and id of object, that's midpoint bounds the bf

		// 	scvf part
			MathVector<dim> localIP; // local integration point
			MathVector<worldDim> globalIP; // global integration point
			MathVector<worldDim> Normal; // normal (incl. area)
			number Vol; // volume of bf

		// 	shapes and derivatives
			number vShape[nsh]; // shapes at ip
			MathVector<dim> vLocalGrad[nsh]; // local grad at ip
			MathVector<worldDim> vGlobalGrad[nsh]; // global grad at ip
			MathMatrix<worldDim,dim> JtInv; // Jacobian transposed at ip
			number detj; // Jacobian det at ip
	};


	public:
	/// construct object and initialize local values and sizes
		FV1IBGeometry();

	/// adapt integration points for elements cut by the inner boundary
		void adapt(GridObject* elem, const MathVector<TWorldDim>* vCornerCoords,
					const ISubsetHandler* ish = NULL);

	/// adapt normals for elements cut by the inner boundary
		void adapt_normals(GridObject* elem, const MathVector<TWorldDim>* vCornerCoords,
					const ISubsetHandler* ish = NULL);

	/// adapt integration points for elements cut by the inner boundary
		void adapt_integration_points(GridObject* elem, const MathVector<TWorldDim>* vCornerCoords,
					const ISubsetHandler* ish = NULL);

	///	update local data
		void update_local_data();

	/// update data for given element
		void update(GridObject* elem, const MathVector<worldDim>* vCornerCoords,
					const ISubsetHandler* ish = NULL);

	/// update boundary data for given element
		void update_boundary_faces(GridObject* elem,
								   const MathVector<worldDim>* vCornerCoords,
								   const ISubsetHandler* ish = NULL);

	/// get vector of the global coordinates of corners for current element
		const MathVector<worldDim>* corners() const {return m_vvGloMid[0];}

	/// number of SubControlVolumeFaces
		inline size_t num_scvf() const {return numSCVF;};

	/// const access to SubControlVolumeFace number i
		inline const SCVF& scvf(size_t i) const
			{UG_ASSERT(i < num_scvf(), "Invalid Index."); return m_vSCVF[i];}

	/// number of SubControlVolumes
		// do not use this method to obtain the number of shape functions,
		// since this is NOT the same for pyramids; use num_sh() instead.
		inline size_t num_scv() const {return numSCV;}

	/// const access to SubControlVolume number i
		inline const SCV& scv(size_t i) const
			{UG_ASSERT(i < num_scv(), "Invalid Index."); return m_vSCV[i];}

	/// number of shape functions
		inline size_t num_sh() const {return nsh;};

	public:
	/// returns number of all scvf ips
		size_t num_scvf_ips() const {return numSCVF;}

	/// returns all ips of scvf as they appear in scv loop
		const MathVector<dim>* scvf_local_ips() const {return m_vLocSCVF_IP;}

	/// returns all ips of scvf as they appear in scv loop
		const MathVector<worldDim>* scvf_global_ips() const {return m_vGlobSCVF_IP;}

	/// returns number of all scv ips
		size_t num_scv_ips() const {return numSCV;}

	/// returns all ips of scv as they appear in scv loop
		const MathVector<dim>* scv_local_ips() const {return &(m_vvLocMid[0][0]);}

	/// returns all ips of scv as they appear in scv loop
		const MathVector<worldDim>* scv_global_ips() const {return &(m_vvGloMid[0][0]);}


	protected:
	//	global and local ips on SCVF
		MathVector<worldDim> m_vGlobSCVF_IP[numSCVF];
		MathVector<dim> m_vLocSCVF_IP[numSCVF];

	public:
	/// add subset that is interpreted as boundary subset.
		inline void add_boundary_subset(int subsetIndex) {m_mapVectorBF[subsetIndex];}

	/// removes subset that is interpreted as boundary subset.
		inline void remove_boundary_subset(int subsetIndex) {m_mapVectorBF.erase(subsetIndex);}

	/// reset all boundary subsets
		inline void clear_boundary_subsets() {m_mapVectorBF.clear();}

	/// number of registered boundary subsets
		inline size_t num_boundary_subsets() {return m_mapVectorBF.size();}

	/// number of all boundary faces
		inline size_t num_bf() const
		{
			typename std::map<int, std::vector<BF> >::const_iterator it;
			size_t num = 0;
			for ( it=m_mapVectorBF.begin() ; it != m_mapVectorBF.end(); it++ )
				num += (*it).second.size();
			return num;
		}

	/// number of boundary faces on subset 'subsetIndex'
		inline size_t num_bf(int si) const
		{
			typename std::map<int, std::vector<BF> >::const_iterator it;
			it = m_mapVectorBF.find(si);
			if(it == m_mapVectorBF.end()) return 0;
			else return (*it).second.size();
		}

	/// returns the boundary face i for subsetIndex
		inline const BF& bf(int si, size_t i) const
		{
			UG_ASSERT(i < num_bf(si), "Invalid index.");
			typename std::map<int, std::vector<BF> >::const_iterator it;
			it = m_mapVectorBF.find(si);
			if(it == m_mapVectorBF.end()) UG_THROW("FVGeom: No bnd face for subset"<<si);
			return (*it).second[i];
		}

	/// returns reference to vector of boundary faces for subsetIndex
		inline const std::vector<BF>& bf(int si) const
		{
			typename std::map<int, std::vector<BF> >::const_iterator it;
			it = m_mapVectorBF.find(si);
			if(it == m_mapVectorBF.end()) return m_vEmptyVectorBF;
			return (*it).second;
		}

	protected:
		std::map<int, std::vector<BF> > m_mapVectorBF;
		std::vector<BF> m_vEmptyVectorBF;

	protected:
	///	pointer to current element
		TElem* m_pElem;

	///	max number of geom objects in all dimensions
	// 	(most objects in 1 dim, i.e. number of edges, but +1 for 1D)
		static const int maxMid = numSCVF + 1;

	///	local and global geom object midpoints for each dimension
		MathVector<dim> m_vvLocMid[dim+1][maxMid];
		MathVector<worldDim> m_vvGloMid[dim+1][maxMid];

	///	SubControlVolumeFaces
		SCVF m_vSCVF[numSCVF];

	///	SubControlVolumes
		SCV m_vSCV[numSCV];

	///	Reference Mapping
		ReferenceMapping<ref_elem_type, worldDim> m_mapping;

	///	Reference Element
		const ref_elem_type& m_rRefElem;

	///	Shape function set
		const local_shape_fct_set_type& m_rTrialSpace;

	public:
	/// new members for adaptions
		MathVector<worldDim> vSCVF_PosOnEdge[numSCVF]; 	// global position of scvf on edges
		MathVector<worldDim> m_MidPoint; 				// global position of the midpoint, connecting each entry of PosOnEdges to a scvf


};

////////////////////////////////////////////////////////////////////////////////
// Dim-dependent Finite Volume Geometry
////////////////////////////////////////////////////////////////////////////////

/// Geometry and shape functions for 1st order Vertex-Centered Finite Volume
/**
 * \tparam	TDim		reference element dim
 * \tparam	TWorldDim	(physical) world dimension
 */

template <int TDim, int TWorldDim = TDim>
class DimFV1IBGeometry : public FVGeometryBase
{
	public:
	///	used traits
		typedef fv1_dim_traits<TDim, TWorldDim> traits;

	public:
	///	dimension of reference element
		static const int dim = TDim;

	///	dimension of world
		static const int worldDim = TWorldDim;

	///	Hanging node flag: this Geometry does not support hanging nodes
		static const bool usesHangingNodes = false;

	/// flag indicating if local data may change
		static const bool staticLocalData = false;

	public:
	///	order
		static const int order = 1;

	///	number of SubControlVolumes
		static const size_t maxNumSCV = traits::maxNumSCV;

	///	type of SubControlVolume
		typedef typename traits::scv_type scv_type;

	///	max number of SubControlVolumeFaces
		static const size_t maxNumSCVF = traits::maxNumSCVF;

	/// max number of shape functions
		static const size_t maxNSH = traits::maxNSH;

	/// number of integration points
		static const size_t nip = 1;

	public:
	///	Sub-Control Volume Face structure
	/**
	 * Each finite element is cut by several sub-control volume faces. The idea
	 * is that the "dual" skeleton formed by the sub control volume faces of
	 * all elements again gives rise to a regular mesh with closed
	 * (lipschitz-bounded) control volumes. The SCVF are the boundary of the
	 * control volume. In computation the flux over each SCVF must be the same
	 * in both directions over the face in order to guarantee the conservation
	 * property.
	 */
		class SCVF
		{
			public:
			///	Number of corners of scvf
				static const size_t numCo = traits::NumCornersOfSCVF;

			public:
				SCVF() {}

			/// index of SubControlVolume on one side of the scvf
				inline size_t from() const {return From;}

			/// index of SubControlVolume on one side of the scvf
				inline size_t to() const {return To;}

			/// number of integration points on scvf
				inline size_t num_ip() const {return nip;}

			/// local integration point of scvf
				inline const MathVector<dim>& local_ip() const {return localIP;}

			/// global integration point of scvf
				inline const MathVector<worldDim>& global_ip() const {return globalIP;}

			/// normal on scvf (points direction "from"->"to"). Norm is equal to area
				inline const MathVector<worldDim>& normal() const {return Normal;}

			/// Transposed Inverse of Jacobian in integration point
				inline const MathMatrix<worldDim,dim>& JTInv() const {return JtInv;}

			/// Determinant of Jacobian in integration point
				inline number detJ() const {return detj;}

			/// number of shape functions
				inline size_t num_sh() const {return numSH;}

			/// value of shape function i in integration point
				inline number shape(size_t sh) const {return vShape[sh];}

			/// vector of shape functions in ip point
				inline const number* shape_vector() const {return vShape;}

			/// value of local gradient of shape function i in integration point
				inline const MathVector<dim>& local_grad(size_t sh) const
					{UG_ASSERT(sh < num_sh(), "Invalid index"); return vLocalGrad[sh];}

			/// vector of local gradients in ip point
				inline const MathVector<dim>* local_grad_vector() const {return vLocalGrad;}

			/// value of global gradient of shape function i in integration point
				inline const MathVector<worldDim>& global_grad(size_t sh) const
					{UG_ASSERT(sh < num_sh(), "Invalid index"); return vGlobalGrad[sh];}

			/// vector of gloabl gradients in ip point
				inline const MathVector<worldDim>* global_grad_vector() const {return vGlobalGrad;}

			/// number of corners, that bound the scvf
				inline size_t num_corners() const {return numCo;}

			/// return local corner number i
				inline const MathVector<dim>& local_corner(size_t co) const
					{UG_ASSERT(co < num_corners(), "Invalid index."); return vLocPos[co];}

			/// return glbal corner number i
				inline const MathVector<worldDim>& global_corner(size_t co) const
					{UG_ASSERT(co < num_corners(), "Invalid index."); return vGloPos[co];}

			private:
			// 	let outer class access private members
				friend class DimFV1IBGeometry<dim, worldDim>;

			// This scvf separates the scv with the ids given in "from" and "to"
			// The computed normal points in direction from->to
				size_t From, To;

			//	The normal on the SCVF pointing (from -> to)
				MathVector<worldDim> Normal; // normal (incl. area)

			// ordering is:
			// 1D: edgeMidPoint
			// 2D: edgeMidPoint, CenterOfElement
			// 3D: edgeMidPoint, Side one, CenterOfElement, Side two
				MathVector<dim> vLocPos[numCo]; // local corners of scvf
				MathVector<worldDim> vGloPos[numCo]; // global corners of scvf
				MidID vMidID[numCo]; // dimension and id of object, that's midpoint bounds the scvf

			// scvf part
				MathVector<dim> localIP; // local integration point
				MathVector<worldDim> globalIP; // global integration point

			// shapes and derivatives
				size_t numSH;
				number vShape[maxNSH]; // shapes at ip
				MathVector<dim> vLocalGrad[maxNSH]; // local grad at ip
				MathVector<worldDim> vGlobalGrad[maxNSH]; // global grad at ip
				MathMatrix<worldDim,dim> JtInv; // Jacobian transposed at ip
				number detj; // Jacobian det at ip
		};

	///	sub control volume structure
		class SCV
		{
			public:
			/// Number of corners of scv
				static const size_t numCo = traits::NumCornersOfSCV;

			public:
				SCV() {};

			/// volume of scv
				inline number volume() const {return Vol;}

			/// number of corners, that bound the scv
				inline size_t num_corners() const {return numCo;}

			/// return local corner number i
				inline const MathVector<dim>& local_corner(size_t co) const
					{UG_ASSERT(co < num_corners(), "Invalid index."); return vLocPos[co];}

			/// return global corner number i
				inline const MathVector<worldDim>& global_corner(size_t co) const
					{UG_ASSERT(co < num_corners(), "Invalid index."); return vGloPos[co];}

			/// node id that this scv is associated to
				inline size_t node_id() const {return nodeId;}

			/// number of integration points
				inline size_t num_ip() const {return nip;}

			/// local integration point of scv
				inline const MathVector<dim>& local_ip() const {return vLocPos[0];}

			/// global integration point
				inline const MathVector<worldDim>& global_ip() const {return vGloPos[0];}

			/// Transposed Inverse of Jacobian in integration point
				inline const MathMatrix<worldDim,dim>& JTInv() const {return JtInv;}

			/// Determinant of Jacobian in integration point
				inline number detJ() const {return detj;}

			/// number of shape functions
				inline size_t num_sh() const {return numSH;}

			/// value of shape function i in integration point
				inline number shape(size_t sh) const {return vShape[sh];}

			/// vector of shape functions in ip point
				inline const number* shape_vector() const {return vShape;}

			/// value of local gradient of shape function i in integration point
				inline const MathVector<dim>& local_grad(size_t sh) const
					{UG_ASSERT(sh < num_sh(), "Invalid index"); return vLocalGrad[sh];}

			/// vector of local gradients in ip point
				inline const MathVector<dim>* local_grad_vector() const {return vLocalGrad;}

			/// value of global gradient of shape function i in integration point
				inline const MathVector<worldDim>& global_grad(size_t sh) const
					{UG_ASSERT(sh < num_sh(), "Invalid index"); return vGlobalGrad[sh];}

			/// vector of global gradients in ip point
				inline const MathVector<worldDim>* global_grad_vector() const {return vGlobalGrad;}

			private:
			// 	let outer class access private members
				friend class DimFV1IBGeometry<dim, worldDim>;

			//  node id of associated node
				size_t nodeId;

			//	volume of scv
				number Vol;

			//	local and global positions of this element
				MathVector<dim> vLocPos[numCo]; // local position of node
				MathVector<worldDim> vGloPos[numCo]; // global position of node
				MidID vMidID[numCo]; // dimension and id of object, that's midpoint bounds the scv

			// shapes and derivatives
				size_t numSH;
				number vShape[maxNSH]; // shapes at ip
				MathVector<dim> vLocalGrad[maxNSH]; // local grad at ip
				MathVector<worldDim> vGlobalGrad[maxNSH]; // global grad at ip
				MathMatrix<worldDim,dim> JtInv; // Jacobian transposed at ip
				number detj; // Jacobian det at ip
		};

	///	boundary face
		class BF
		{
			public:
			/// Number of corners of bf
				static const size_t numCo = traits::NumCornersOfSCVF;

			public:
				BF() {}

			/// index of SubControlVolume of the bf
				inline size_t node_id() const {return nodeId;}

			/// number of integration points on bf
				inline size_t num_ip() const {return nip;}

			/// local integration point of bf
				inline const MathVector<dim>& local_ip() const {return localIP;}

			/// global integration point of bf
				inline const MathVector<worldDim>& global_ip() const {return globalIP;}

			/// outer normal on bf. Norm is equal to area
				inline const MathVector<worldDim>& normal() const {return Normal;} // includes area

			/// volume of bf
				inline number volume() const {return Vol;}

			/// Transposed Inverse of Jacobian in integration point
				inline const MathMatrix<worldDim, dim>& JTInv() const {return JtInv;}

			/// Determinant of Jacobian in integration point
				inline number detJ() const {return detj;}

			/// number of shape functions
				inline size_t num_sh() const {return numSH;}

			/// value of shape function i in integration point
				inline number shape(size_t sh) const
					{UG_ASSERT(sh < num_sh(), "Invalid index"); return vShape[sh];}

			/// vector of local gradients in ip point
				inline const number* shape_vector() const {return vShape;}

			/// value of local gradient of shape function i in integration point
				inline const MathVector<dim>& local_grad(size_t sh) const
					{UG_ASSERT(sh < num_sh(), "Invalid index"); return vLocalGrad[sh];}

			/// vector of local gradients in ip point
				inline const MathVector<dim>* local_grad_vector() const {return vLocalGrad;}

			/// value of global gradient of shape function i in integration point
				inline const MathVector<worldDim>& global_grad(size_t sh) const
					{UG_ASSERT(sh < num_sh(), "Invalid index"); return vGlobalGrad[sh];}

			/// vector of global gradients in ip point
				inline const MathVector<worldDim>* global_grad_vector() const {return vGlobalGrad;}

			/// number of corners, that bound the scvf
				inline size_t num_corners() const {return numCo;}

			/// return local corner number i
				inline const MathVector<dim>& local_corner(size_t co) const
					{UG_ASSERT(co < num_corners(), "Invalid index."); return vLocPos[co];}

			/// return global corner number i
				inline const MathVector<worldDim>& global_corner(size_t co) const
					{UG_ASSERT(co < num_corners(), "Invalid index."); return vGloPos[co];}

			private:
			/// let outer class access private members
				friend class DimFV1IBGeometry<dim, worldDim>;

			// 	id of scv this bf belongs to
				size_t nodeId;

			// ordering is:
			// 1D: edgeMidPoint
			// 2D: edgeMidPoint, CenterOfElement
			// 3D: edgeMidPoint, Side one, CenterOfElement, Side two
				MathVector<dim> vLocPos[numCo]; // local corners of bf
				MathVector<worldDim> vGloPos[numCo]; // global corners of bf
				MidID vMidID[numCo]; // dimension and id of object, that's midpoint bounds the bf

			// 	scvf part
				MathVector<dim> localIP; // local integration point
				MathVector<worldDim> globalIP; // global integration point
				MathVector<worldDim> Normal; // normal (incl. area)
				number Vol; // volume of bf

			// 	shapes and derivatives
				size_t numSH;
				number vShape[maxNSH]; // shapes at ip
				MathVector<dim> vLocalGrad[maxNSH]; // local grad at ip
				MathVector<worldDim> vGlobalGrad[maxNSH]; // global grad at ip
				MathMatrix<worldDim,dim> JtInv; // Jacobian transposed at ip
				number detj; // Jacobian det at ip
		};

	public:
	/// construct object and initialize local values and sizes
		DimFV1IBGeometry() : m_pElem(NULL), m_roid(ROID_UNKNOWN) {};

	/// adapt integration points for elements cut by the inner boundary
		void adapt(GridObject* elem, const MathVector<TWorldDim>* vCornerCoords,
					const ISubsetHandler* ish = NULL);

	/// adapt normals for elements cut by the inner boundary
		void adapt_normals(GridObject* elem, const MathVector<TWorldDim>* vCornerCoords,
					const ISubsetHandler* ish = NULL);

	/// adapt integration points for elements cut by the inner boundary
		void adapt_integration_points(GridObject* elem, const MathVector<TWorldDim>* vCornerCoords,
					const ISubsetHandler* ish = NULL);

	///	update local data
		void update_local_data();

	/// update data for given element
		void update(GridObject* elem, const MathVector<worldDim>* vCornerCoords,
		            const ISubsetHandler* ish = NULL);

	/// update boundary data for given element
		void update_boundary_faces(GridObject* elem,
		                           const MathVector<worldDim>* vCornerCoords,
		                           const ISubsetHandler* ish = NULL);

	/// get vector of corners for current element
		const MathVector<worldDim>* corners() const {return m_vvGloMid[0];}

	/// number of SubControlVolumeFaces
		inline size_t num_scvf() const {return m_numSCVF;};

	/// const access to SubControlVolumeFace number i
		inline const SCVF& scvf(size_t i) const
			{UG_ASSERT(i < num_scvf(), "Invalid Index."); return m_vSCVF[i];}

	/// number of SubControlVolumes
		// do not use this method to obtain the number of shape functions,
		// since this is NOT the same for pyramids; use num_sh() instead.
		inline size_t num_scv() const {return m_numSCV;}

	/// const access to SubControlVolume number i
		inline const SCV& scv(size_t i) const
			{UG_ASSERT(i < num_scv(), "Invalid Index."); return m_vSCV[i];}

	/// number of shape functions
		inline size_t num_sh() const {return m_nsh;};

	public:
	/// returns number of all scvf ips
		size_t num_scvf_ips() const {return m_numSCVF;}

	/// returns all ips of scvf as they appear in scv loop
		const MathVector<dim>* scvf_local_ips() const {return m_vLocSCVF_IP;}

	/// returns all ips of scvf as they appear in scv loop
		const MathVector<worldDim>* scvf_global_ips() const {return m_vGlobSCVF_IP;}

	/// returns number of all scv ips
		size_t num_scv_ips() const {return m_numSCV;}

	/// returns all ips of scv as they appear in scv loop
		const MathVector<dim>* scv_local_ips() const {return &(m_vvLocMid[0][0]);}

	/// returns all ips of scv as they appear in scv loop
		const MathVector<worldDim>* scv_global_ips() const {return &(m_vvGloMid[0][0]);}


	protected:
	//	global and local ips on SCVF
		MathVector<worldDim> m_vGlobSCVF_IP[maxNumSCVF];
		MathVector<dim> m_vLocSCVF_IP[maxNumSCVF];

	public:
	/// add subset that is interpreted as boundary subset.
		inline void add_boundary_subset(int subsetIndex) {m_mapVectorBF[subsetIndex];}

	/// removes subset that is interpreted as boundary subset.
		inline void remove_boundary_subset(int subsetIndex) {m_mapVectorBF.erase(subsetIndex);}

	/// reset all boundary subsets
		inline void clear_boundary_subsets() {m_mapVectorBF.clear();}

	/// number of registered boundary subsets
		inline size_t num_boundary_subsets() {return m_mapVectorBF.size();}

	/// number of all boundary faces
		inline size_t num_bf() const
		{
			typename std::map<int, std::vector<BF> >::const_iterator it;
			size_t num = 0;
			for ( it=m_mapVectorBF.begin() ; it != m_mapVectorBF.end(); it++ )
				num += (*it).second.size();
			return num;
		}

	/// number of boundary faces on subset 'subsetIndex'
		inline size_t num_bf(int si) const
		{
			typename std::map<int, std::vector<BF> >::const_iterator it;
			it = m_mapVectorBF.find(si);
			if(it == m_mapVectorBF.end()) return 0;
			else return (*it).second.size();
		}

	/// returns the boundary face i for subsetIndex
		inline const BF& bf(int si, size_t i) const
		{
			typename std::map<int, std::vector<BF> >::const_iterator it;
			it = m_mapVectorBF.find(si);
			if(it == m_mapVectorBF.end()) UG_THROW("DimFV1IBGeom: No BndSubset "<<si);
			return (*it).second[i];
		}

	/// returns reference to vector of boundary faces for subsetIndex
		inline const std::vector<BF>& bf(int si) const
		{
			typename std::map<int, std::vector<BF> >::const_iterator it;
			it = m_mapVectorBF.find(si);
			if(it == m_mapVectorBF.end()) return m_vEmptyVectorBF;
			return (*it).second;
		}

	protected:
		std::map<int, std::vector<BF> > m_mapVectorBF;
		std::vector<BF> m_vEmptyVectorBF;

	private:
	///	pointer to current element
		GridObject* m_pElem;

	///	current reference object id
		ReferenceObjectID m_roid;

	///	current number of scv
		size_t m_numSCV;

	///	current number of scvf
		size_t m_numSCVF;

	/// current number of shape functions
		size_t m_nsh;

	///	max number of geometric objects in a dimension
	// 	(most objects in 1 dim, i.e. number of edges, but +1 for 1D)
		static const int maxMid = maxNumSCVF + 1;

	///	local and global geom object midpoints for each dimension
		MathVector<dim> m_vvLocMid[dim+1][maxMid];
		MathVector<worldDim> m_vvGloMid[dim+1][maxMid];

	///	SubControlVolumeFaces
		SCVF m_vSCVF[maxNumSCVF];

	///	SubControlVolumes
		SCV m_vSCV[maxNumSCV];

	public:
	/// new members for adaptions
		MathVector<worldDim> vSCVF_PosOnEdge[maxNumSCVF]; 	// global position of scvf on edges
		MathVector<worldDim> m_MidPoint; 				// global position of the midpoint, connecting each entry of PosOnEdges to a scvf

};



#include "fv1ib_geom_impl.h"

} // end ug namespace

#endif /* FV1IB_GEOM_H_ */
