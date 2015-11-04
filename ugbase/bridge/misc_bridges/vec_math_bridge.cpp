#include <string>
#include <sstream>

#include "bridge/bridge.h"
#include "bridge/util.h"
#include "common/math/ugmath.h"
#include "bridge/suffix_tag.h"

using namespace std;

namespace ug{

/// \defgroup vecmath_bridge VecMath Bridge
/// \ingroup misc_bridge
/// \{

////////////////////////////////////////////////////////////////////////////////
//	Some methods which assist in creating vectors
static SmartPtr<vector1> MakeVec(number x)
{
	return SmartPtr<vector1>(new vector1(x));
}

static SmartPtr<vector2> MakeVec(number x, number y)
{
	return SmartPtr<vector2>(new vector2(x, y));
}

static SmartPtr<vector3> MakeVec(number x, number y, number z)
{
	return SmartPtr<vector3>(new vector3(x, y, z));
}

static SmartPtr<vector4> MakeVec(number x, number y, number z, number w)
{
	return SmartPtr<vector4>(new vector4(x, y, z, w));
}

////////////////////////////////////////////////////////////////////////////////
namespace bridge{
////////////////////////////////////////////////////////////////////////////////
/*
template <int dim>
static void RegisterBridge_VecMath(Registry& reg, string grp)
{
	typedef MathVector<dim, number> vec_type;
	
	try
	{
		string suffix = GetDimensionSuffix<dim>();
		string tag = GetDimensionTag<dim>();

	//	register the class
		string vecName = "Vec";
		vecName.append(suffix);

		reg.add_class_<vec_type>(vecName, grp)
			.add_method("coord",
					static_cast<typename vec_type::value_type (vec_type::*)(size_t) const>(&vec_type::coord));
		reg.add_class_to_group(vecName, "Vec", tag);
	}
	UG_REGISTRY_CATCH_THROW(grp);
}
*/
////////////////////////////////////////////////////////////////////////////////
static void RegisterVecMathBridge_DimIndep(Registry& reg, string grp)
{
	try
	{
		{
			typedef MathVector<1, number> vec_type;
			reg.add_class_<vec_type>("Vec1d", grp)
				.add_constructor()
				.add_constructor<void (*)(number)>()
				.add_method("coord",
						static_cast<vec_type::value_type (vec_type::*)(size_t) const>(&vec_type::coord));
			//reg.add_class_to_group("Vec1d", "Vec", GetDimensionTag<1>());
		}
		{
			typedef MathVector<2, number> vec_type;
			reg.add_class_<vec_type>("Vec2d", grp)
				.add_constructor()
				.add_constructor<void (*)(number, number)>()
				.add_method("coord",
						static_cast<vec_type::value_type (vec_type::*)(size_t) const>(&vec_type::coord));
			//reg.add_class_to_group("Vec2d", "Vec", GetDimensionTag<2>());
		}
		{
			typedef MathVector<3, number> vec_type;
			reg.add_class_<vec_type>("Vec3d", grp)
				.add_constructor()
				.add_constructor<void (*)(number, number, number)>()
				.add_method("coord",
						static_cast<vec_type::value_type (vec_type::*)(size_t) const>(&vec_type::coord));
			//reg.add_class_to_group("Vec3d", "Vec", GetDimensionTag<3>());
		}
		{
			typedef MathVector<4, number> vec_type;
			reg.add_class_<vec_type>("Vec4d", grp)
				.add_constructor()
				.add_constructor<void (*)(number, number, number, number)>()
				.add_method("coord",
						static_cast<vec_type::value_type (vec_type::*)(size_t) const>(&vec_type::coord));
			//reg.add_class_to_group("Vec4d", "Vec", GetDimensionTag<4>());
		}

	//	register make-methods
		reg.add_function("MakeVec", static_cast<SmartPtr<vector1> (*)(number)>(
							&MakeVec), grp)
			.add_function("MakeVec", static_cast<SmartPtr<vector2> (*)(number, number)>(
							&MakeVec), grp)
			.add_function("MakeVec", static_cast<SmartPtr<vector3> (*)(number, number, number)>(
							&MakeVec), grp)
			.add_function("MakeVec", static_cast<SmartPtr<vector4> (*)(number, number, number, number)>(
							&MakeVec), grp);
	}
	UG_REGISTRY_CATCH_THROW(grp);
}


////////////////////////////////////////////////////////////////////////////////
void RegisterBridge_VecMath(Registry& reg, string parentGroup)
{
//	get group string
	string grp = parentGroup; grp.append("/Util/VecMath");

//	RegisterBridge_VecMath<1>(reg, grp);
//	RegisterBridge_VecMath<2>(reg, grp);
//	RegisterBridge_VecMath<3>(reg, grp);
//	RegisterBridge_VecMath<4>(reg, grp);
	RegisterVecMathBridge_DimIndep(reg, grp);
}

// end group vecmath_bridge
/// \}

}// end of namespace
}// end of namespace
