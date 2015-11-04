
#ifndef __H__UG__LIB_DISC__OPERATOR__LINEAR_OPERATOR__LINEAR_SOLVER__
#define __H__UG__LIB_DISC__OPERATOR__LINEAR_OPERATOR__LINEAR_SOLVER__
#include <iostream>
#include <string>

#include "lib_algebra/operator/interface/preconditioned_linear_operator_inverse.h"
#include "lib_algebra/operator/interface/linear_solver_profiling.h"
#ifdef UG_PARALLEL
	#include "lib_algebra/parallelization/parallelization.h"
#endif

namespace ug{

/// linear solver using abstract preconditioner interface
/**
 * This class is a linear iterating scheme, that uses any implementation
 * of the ILinearIterator interface to precondition the iteration.
 *
 * \tparam 		TAlgebra		algebra type
 */
template <typename TVector>
class LinearSolver
	: public IPreconditionedLinearOperatorInverse<TVector>
{
	public:
	///	Vector type
		typedef TVector vector_type;

	///	Base type
		typedef IPreconditionedLinearOperatorInverse<vector_type> base_type;

	///	constructors
		LinearSolver() : base_type() {}

		LinearSolver(SmartPtr<ILinearIterator<vector_type,vector_type> > spPrecond)
			: base_type ( spPrecond )  {}

		LinearSolver(SmartPtr<ILinearIterator<vector_type,vector_type> > spPrecond, SmartPtr<IConvergenceCheck<vector_type> > spConvCheck)
			: base_type ( spPrecond, spConvCheck)  {}

	protected:
		using base_type::convergence_check;
		using base_type::linear_operator;
		using base_type::preconditioner;
		using base_type::write_debug;

	public:
	///	returns the name of the solver
		virtual const char* name() const {return "Iterative Linear Solver";}

	///	returns if parallel solving is supported
		virtual bool supports_parallel() const
		{
			if(preconditioner().valid())
				return preconditioner()->supports_parallel();
			else return true;
		}

		/**
		 * Compute a correction c := B*d using one iterative step
		 * Internally the defect is updated d := d - A*c = b - A*(x+c)
		 * @param c
		 * @param d
		 */
		bool compute_correction(vector_type &c, vector_type &d)
		{
			if(preconditioner().valid()) {
				LS_PROFILE_BEGIN(LS_ApplyPrecond);

				if(!preconditioner()->apply_update_defect(c, d))
				{
					UG_LOG("ERROR in 'LinearSolver::apply': Iterator "
							"Operator applied incorrectly. Aborting.\n");
					return false;
				}
				LS_PROFILE_END(LS_ApplyPrecond);
			}
			return true;
		}

		void write_debugXCD(vector_type &x, vector_type &c, vector_type &d, int loopCnt, bool bWriteC)
		{
			char ext[20]; sprintf(ext, "_iter%03d", loopCnt);
			write_debug(d, std::string("LS_Defect_") + ext + ".vec");
			if(bWriteC) write_debug(c, std::string("LS_Correction_") + ext + ".vec");
			write_debug(x, std::string("LS_Solution_") + ext + ".vec");
		}

	///	solves the system and returns the last defect
		virtual bool apply_return_defect(vector_type& x, vector_type& b)
		{

			LS_PROFILE_BEGIN(LS_ApplyReturnDefect);

			#ifdef UG_PARALLEL
			if(!b.has_storage_type(PST_ADDITIVE) || !x.has_storage_type(PST_CONSISTENT))
				UG_THROW("LinearSolver::apply: Inadequate storage format of Vectors.");
			#endif

		// 	rename b as d (for convenience)
			vector_type& d = b;

		// 	build defect:  d := b - J(u)*x
			LS_PROFILE_BEGIN(LS_BuildDefect);
			linear_operator()->apply_sub(d, x);
			LS_PROFILE_END(LS_BuildDefect);

		// 	create correction
			LS_PROFILE_BEGIN(LS_CreateCorrection);
			SmartPtr<vector_type> spC = x.clone_without_values();
			vector_type& c = *spC;
			#ifdef UG_PARALLEL
				// this is ok if clone_without_values() inits with zeros
				c.set_storage_type(PST_CONSISTENT);
			#endif
			LS_PROFILE_END(LS_CreateCorrection);

			LS_PROFILE_BEGIN(LS_ComputeStartDefect);
			prepare_conv_check();
			convergence_check()->start(d);
			LS_PROFILE_END(LS_ComputeStartDefect);

			int loopCnt = 0;
			write_debugXCD(x, c, d, loopCnt, false);

		// 	Iteration loop
			while(!convergence_check()->iteration_ended())
			{
				if( !compute_correction(c, d) ) return false;

			// 	add correction to solution: x += c
				LS_PROFILE_BEGIN(LS_AddCorrection);
				x += c;
				LS_PROFILE_END(LS_AddCorrection);

				write_debugXCD(x, c, d, ++loopCnt, true);

			// 	compute norm of new defect (in parallel)
				LS_PROFILE_BEGIN(LS_ComputeNewDefectNorm);
				convergence_check()->update(d);
				LS_PROFILE_END(LS_ComputeNewDefectNorm);
			}

		//	write some information when ending the iteration
			if(!convergence_check()->post())
			{
				UG_LOG("ERROR in 'LinearSolver::apply': post-convergence-check "
						"signaled failure. Aborting.\n");
				return false;
			}

		//	end profiling of whole function
			LS_PROFILE_END(LS_ApplyReturnDefect);

		//	we're done
			return true;
		}

	protected:
	///	prepares the convergence check output
		void prepare_conv_check()
		{
			convergence_check()->set_name(name());
			convergence_check()->set_symbol('%');
			if(preconditioner().valid())
            {
                std::string s;
                if(preconditioner().valid())
                    s = std::string(" (Precond: ") + preconditioner()->name() + ")";
                else
                    s = " (No Preconditioner) ";
                convergence_check()->set_info(s);
            }
		}
};

} // end namespace ug

#endif /* __H__UG__LIB_DISC__OPERATOR__LINEAR_OPERATOR__LINEAR_SOLVER__ */
