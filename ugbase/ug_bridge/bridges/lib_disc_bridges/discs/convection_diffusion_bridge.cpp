/*
 * convection_diffusion_bridge.cpp
 *
 *  Created on: 12.05.2011
 *      Author: andreasvogel
 */

// extern headers
#include <iostream>
#include <sstream>

// include bridge
#include "../../../ug_bridge.h"
#include "../../../registry.h"

// lib_algebra includes
#include "lib_algebra/algebra_selector.h"
#include "lib_algebra/algebra_types.h"
#include "lib_algebra/operator/operator_util.h"
#include "lib_algebra/operator/operator_interface.h"
#include "lib_algebra/operator/operator_inverse_interface.h"

// lib_disc includes
#include "lib_discretization/domain.h"
#include "lib_discretization/function_spaces/grid_function.h"
#include "lib_discretization/function_spaces/grid_function_space.h"
#include "lib_discretization/dof_manager/p1conform/p1conform.h"

#include "lib_discretization/spatial_discretization/elem_disc/convection_diffusion/fe1/fe1_convection_diffusion.h"
#include "lib_discretization/spatial_discretization/elem_disc/convection_diffusion/fv1/convection_diffusion.h"

namespace ug
{

namespace bridge
{

template <typename TDomain, typename TAlgebra, typename TDoFDistribution>
void RegisterConvectionDiffusionObjects(Registry& reg, const char* parentGroup)
{
//	typedef domain
	typedef TDomain domain_type;
	typedef TDoFDistribution dof_distribution_type;
	typedef TAlgebra algebra_type;
	typedef typename algebra_type::vector_type vector_type;
	typedef typename algebra_type::matrix_type matrix_type;
	static const int dim = domain_type::dim;

	std::stringstream grpSS; grpSS << parentGroup << "/" << dim << "d";
	std::string grp = grpSS.str();

#ifdef UG_PARALLEL
		typedef ParallelGridFunction<GridFunction<domain_type, dof_distribution_type, algebra_type> > function_type;
#else
		typedef GridFunction<domain_type, dof_distribution_type, algebra_type> function_type;
#endif

/////////////////////////////////////////////////////////////////////////////
// Elem Disc
/////////////////////////////////////////////////////////////////////////////

//	Convection Diffusion Finite Volume
	{
		typedef FVConvectionDiffusionElemDisc<domain_type, algebra_type> T;
		typedef IDomainElemDisc<domain_type, algebra_type> TBase;
		std::stringstream ss; ss << "FV1ConvectionDiffusion" << dim << "d";
		reg.add_class_<T, TBase >(ss.str().c_str(), grp.c_str())
			.add_constructor()
			.add_method("set_diffusion_tensor", &T::set_diffusion)
			.add_method("set_velocity_field", &T::set_velocity)
			.add_method("set_reaction", &T::set_reaction)
			.add_method("set_source", &T::set_source)
			.add_method("set_mass_scale", &T::set_mass_scale)
			.add_method("set_upwind", &T::set_upwind)
			.add_method("get_concentration", &T::get_concentration)
			.add_method("get_concentration_grad", &T::get_concentration_grad);
	}

//	Convection Diffusion Finite Element
	{
		typedef FE1ConvectionDiffusionElemDisc<domain_type, algebra_type> T;
		typedef IDomainElemDisc<domain_type, algebra_type> TBase;
		std::stringstream ss; ss << "FE1ConvectionDiffusion" << dim << "d";
		reg.add_class_<T, TBase >(ss.str().c_str(), grp.c_str())
			.add_constructor()
			.add_method("set_diffusion_tensor", &T::set_diffusion)
			.add_method("set_velocity_field", &T::set_velocity)
			.add_method("set_reaction", &T::set_reaction)
			.add_method("set_source", &T::set_source)
			.add_method("set_mass_scale", &T::set_mass_scale);
	}
}


template <typename TAlgebra, typename TDoFDistribution>
bool RegisterConvectionDiffusionDisc(Registry& reg, const char* parentGroup)
{
	typedef TDoFDistribution dof_distribution_type;
	typedef TAlgebra algebra_type;
	typedef typename algebra_type::vector_type vector_type;
	typedef typename algebra_type::matrix_type matrix_type;

	try
	{
	//	get group string
		std::string grp = parentGroup; grp.append("/Discretization");

#ifdef UG_DIM_1
	//	Domain dependend part 1D
		{
			typedef Domain<1, MultiGrid, MGSubsetHandler> domain_type;
			RegisterConvectionDiffusionObjects<domain_type, algebra_type, dof_distribution_type>(reg, grp.c_str());
		}
#endif

#ifdef UG_DIM_2
	//	Domain dependend part 2D
		{
			typedef Domain<2, MultiGrid, MGSubsetHandler> domain_type;
			RegisterConvectionDiffusionObjects<domain_type, algebra_type, dof_distribution_type>(reg, grp.c_str());
		}
#endif

#ifdef UG_DIM_3
	//	Domain dependend part 3D
		{
			typedef Domain<3, MultiGrid, MGSubsetHandler> domain_type;
			RegisterConvectionDiffusionObjects<domain_type, algebra_type, dof_distribution_type>(reg, grp.c_str());
		}
#endif
	}
	catch(UG_REGISTRY_ERROR_RegistrationFailed ex)
	{
		UG_LOG("### ERROR in RegisterConvectionDiffusionDiscs: "
				"Registration failed (using name " << ex.name << ").\n");
		return false;
	}

	return true;
}

bool RegisterDynamicConvectionDiffusionDisc(Registry& reg, int algebra_type, const char* parentGroup)
{
	bool bReturn = true;

	switch(algebra_type)
	{
	case eCPUAlgebra:		 		bReturn &= RegisterConvectionDiffusionDisc<CPUAlgebra, P1ConformDoFDistribution>(reg, parentGroup); break;
	case eCPUBlockAlgebra2x2: 		bReturn &= RegisterConvectionDiffusionDisc<CPUBlockAlgebra<2>, GroupedP1ConformDoFDistribution>(reg, parentGroup); break;
//	case eCPUBlockAlgebra3x3: 		bReturn &= RegisterConvectionDiffusionDisc<CPUBlockAlgebra<3>, GroupedP1ConformDoFDistribution>(reg, parentGroup); break;
//	case eCPUBlockAlgebra4x4: 		bReturn &= RegisterConvectionDiffusionDisc<CPUBlockAlgebra<4>, GroupedP1ConformDoFDistribution>(reg, parentGroup); break;
//	case eCPUVariableBlockAlgebra: 	bReturn &= RegisterConvectionDiffusionDisc<CPUVariableBlockAlgebra, GroupedP1ConformDoFDistribution>(reg, parentGroup); break;
	default: UG_ASSERT(0, "Unsupported Algebra Type");
				UG_LOG("Unsupported Algebra Type requested.\n");
				return false;
	}

	return bReturn;
}

}//	end of namespace ug
}//	end of namespace interface
