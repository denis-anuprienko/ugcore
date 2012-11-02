/*
 * util.h
 *
 *  Created on: 24.05.2012
 *      Author: andreasvogel
 */

#ifndef __H__UG_BRIDGE__UTIL__
#define __H__UG_BRIDGE__UTIL__

#include "registry/registry.h"
#include "suffix_tag.h"

namespace ug{
namespace bridge{



template <typename Functionality>
void RegisterCommon(Registry& reg, std::string grp)
{
	Functionality::Common(reg,grp);
}

template <typename Functionality>
void RegisterDimensionDependent(Registry& reg, std::string grp)
{
#ifdef UG_DIM_1
	Functionality::template Dimension<1>(reg,grp);
#endif
#ifdef UG_DIM_2
	Functionality::template Dimension<2>(reg,grp);
#endif
#ifdef UG_DIM_3
	Functionality::template Dimension<3>(reg,grp);
#endif
}


} // end namespace bridge
} // end namespace ug

#define UG_REGISTRY_CATCH_THROW(grp)	\
		catch(UGRegistryError& ex) {\
			UG_ERR_LOG("### ERROR while registering functionality at '"<<(grp)<<"'. "\
					"Registration failed (using name " << ex.name << ").\n");\
			throw(ex);}

#endif /* __H__UG_BRIDGE__UTIL__ */
