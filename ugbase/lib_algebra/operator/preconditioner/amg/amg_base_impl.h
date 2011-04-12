/**
 * \file amg_base.h
 *
 * \author Martin Rupp
 *
 * \date 01.12.2010
 *
 * implementation file for base amg functionality
 *
 * Goethe-Center for Scientific Computing 2010.
 */

#ifndef __H__LIB_DISCRETIZATION__AMG_SOLVER__AMG_BASE_IMPL_H__
#define __H__LIB_DISCRETIZATION__AMG_SOLVER__AMG_BASE_IMPL_H__

#include <fstream>
#include "stopwatch.h"
#include "amg_debug.h"

namespace ug{

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//----------------
//! creates MG Hierachy for with matrix_type A and temporary vectors for higher levels
//! @param A	matrix A.
template<typename TAlgebra>
bool amg_base<TAlgebra>::preprocess(matrix_type& mat)
{
	cleanup();
	m_A.resize(1);
	m_A[0] = &mat;
	init();
#ifdef UG_PARALLEL
	// set level 0 communicator
		com = &mat.get_communicator();
#endif

	return true;
}

template<typename TAlgebra>
bool amg_base<TAlgebra>::create_level_vectors(size_t level)
{
	size_t N = m_A[level]->num_rows();
	size_t iNrOfCoarse = m_A[level+1]->num_rows();
	// create vectors for AMG multigrid
	/////////////////////////////////////////

	m_vec3.resize(level+1);
	m_vec3[level] = new vector_type;
	m_vec3[level]->create(N);

	m_vec1.resize(level+2);
	m_vec1[level+1] = new vector_type;
	m_vec1[level+1]->create(iNrOfCoarse);

	m_vec2.resize(level+2);
	m_vec2[level+1] = new vector_type;
	m_vec2[level+1]->create(iNrOfCoarse);

#ifdef UG_PARALLEL
	m_vec3[level]->set_communicator(*com);
	m_vec1[level+1]->set_communicator(*com);
	m_vec2[level+1]->set_communicator(*com);

	m_vec3[level]->set_storage_type(PST_CONSISTENT);
	m_vec1[level+1]->set_storage_type(PST_ADDITIVE);
	m_vec2[level+1]->set_storage_type(PST_ADDITIVE);

	m_vec3[level]->set_layouts(m_A[level]->get_master_layout(), m_A[level]->get_slave_layout());
	m_vec1[level+1]->set_layouts(m_A[level+1]->get_master_layout(), m_A[level+1]->get_slave_layout());
	m_vec2[level+1]->set_layouts(m_A[level+1]->get_master_layout(), m_A[level+1]->get_slave_layout());
#endif


	//UG_LOG(std::endl << "created m_vec3 on level " << level << ", m_vec1 and m_vec2 on level" << level +1);

	// todo: set size for variable sized blockvectors
	/*for(size_t i=0; i<N; i++)
		if(nodes[i].isCoarse())
		{
			int rows = GetRows(A.begin_row(i).value());
			UG_ASSERT(newIndex[i] >= 0, "");
			SetSize((*m_vec1[level+1])[newIndex[i]], rows);
			SetSize((*m_vec2[level+1])[newIndex[i]], rows);
		}*/
	return true;
}

template<typename TAlgebra>
bool amg_base<TAlgebra>::init()
{
	AMG_PROFILE_FUNC();
	update_positions();

	// init m_amghelper for grid printing
	if(m_writeMatrices)
	{
		if(m_dbgPositions.size() > 0)
			m_amghelper.positions = &m_dbgPositions[0];
		m_amghelper.size = m_A[0]->num_rows();
		m_amghelper.parentIndex = &m_parentIndex;
		m_amghelper.dimension = m_dbgDimension;
	}

	UG_ASSERT(m_writeMatrices == false || m_dbgDimension != 0, "you need to provide position information for writing matrices");


	if(m_basesolver==NULL)
	{
		UG_LOG("amg_base::init(): No base solver selected. Call set_base_solver(basesolver) to set a base solver.\n");
		return false;
	}
	if(m_presmoother==NULL)
	{
		UG_LOG("amg_base::init(): No PreSmoother selected. Call set_presmoother(presmoother) to set a PreSmoother.\n");
		return false;
	}
	if(m_postsmoother==NULL)
	{
		UG_LOG("amg_base::init(): No PostSmoother selected. Call set_postsmoother(postsmoother) to set a PostSmoother.\n");
		return false;
	}



	UG_DLOG(LIB_ALG_AMG, 1, "Starting AMG Setup." << std::endl << std::endl);

	m_levelInformation.clear();


	stopwatch SWwhole;
	SWwhole.start();


	m_levelInformation.push_back(LevelInformation(0, m_A[0]->num_rows(), m_A[0]->total_num_connections()));

	size_t level=0;
	for(; level< m_maxLevels-1; level++)
	{
		m_SMO.resize(level+1);
		m_SMO[level].setmatrix(m_A[level]);

		m_presmoothers.resize(level+1);
		m_presmoothers[level] = m_presmoother->clone();
		m_presmoothers[level]->init(m_SMO[level]);

		m_postsmoothers.resize(level+1);
		if(m_presmoother == m_postsmoother)
			m_postsmoothers[level] = m_presmoothers[level];
		else
		{
			m_postsmoothers[level] = m_postsmoother->clone();
			m_postsmoothers[level]->init(m_SMO[level]);
		}

		double L = m_A[level]->num_rows();

		if(L < m_maxNodesForBase || m_A[level]->total_num_connections()/(L*L) > m_dMaxFillBeforeBase)
			break;
		//smoothem_R[level].init(*m_A[level]);

		m_A.resize(level+2);
		m_A[level+1] = new matrix_type;

		m_P.resize(level+1);
		m_P[level] = new matrix_type;
		m_R.resize(level+1);
		m_R[level] = new matrix_type;

#ifdef UG_PARALLEL
		m_A[level+1]->set_storage_type(PST_ADDITIVE);
		m_P[level]->set_storage_type(PST_ADDITIVE);
		m_R[level]->set_storage_type(PST_ADDITIVE);
#endif


		stopwatch SWwhole; SWwhole.start();
		create_AMG_level(*m_A[level+1], *m_R[level], *m_A[level], *m_P[level], level);
		SWwhole.stop();

		// finish
		/////////////////////////////////////////

		double nrOfCoarse = m_A[level+1]->num_rows();
		size_t nnzCoarse = m_A[level+1]->total_num_connections();
		IF_DEBUG(LIB_ALG_AMG, 1)
		{
			double nrOfFine = m_A[level]->num_rows();
			UG_DLOG(LIB_ALG_AMG, 1, "AH: nnz: " << nnzCoarse << " Density: " <<
					nnzCoarse/(nrOfCoarse*nrOfCoarse)*100.0 << "%, avg. nnz pre row: " << nnzCoarse/nrOfCoarse << std::endl);

			UG_DLOG(LIB_ALG_AMG, 1, "Coarsening rate: " << (100.0*nrOfCoarse)/nrOfFine << "%" << std::endl);

			UG_DLOG(LIB_ALG_AMG, 1, " level took " << SWwhole.ms() << " ms" << std::endl << std::endl);
		}

		m_levelInformation.push_back(LevelInformation(SWwhole.ms(), nrOfCoarse, nnzCoarse));

		create_level_vectors(level);
	}

	UG_ASSERT(block_traits< typename vector_type::value_type >::is_static, "dynamic not yet implemented");
	size_t static_nrUnknowns = block_traits< typename vector_type::value_type >::static_size;

	UG_DLOG(LIB_ALG_AMG, 1, "Creating level " << level << " (" << m_A[level]->num_rows() << " nodes, total "
			<< m_A[level]->num_rows()*static_nrUnknowns << " unknowns)" << std::endl << "Using Direct Solver on Matrix "
			<< m_A[level]->num_rows()*static_nrUnknowns << "x" << m_A[level]->num_rows()*static_nrUnknowns << ". ");

	stopwatch SW; SW.start();

	m_SMO.resize(level+1);
	m_SMO[level].setmatrix(m_A[level]);
	m_basesolver->init(m_SMO[level]);

	m_dTimingCoarseSolverSetupMS = SW.ms();
	UG_DLOG(LIB_ALG_AMG, 1, "Coarse Solver Setup took " << m_dTimingCoarseSolverSetupMS << "ms." << std::endl);

	m_usedLevels = level+1;
	UG_LOG("AMG Setup finished. Used Levels: " << m_usedLevels << ". ");
	m_dTimingWholeSetupMS = SWwhole.ms();
	UG_DLOG(LIB_ALG_AMG, 1, "AMG Setup took " << m_dTimingWholeSetupMS << " ms." << std::endl);

	// calc complexities
	double totalNNZs=0;
	double totalNrOfNodes=0;
	for(size_t i=0; i<m_usedLevels; i++)
	{
		totalNNZs += m_A[i]->total_num_connections();
		totalNrOfNodes += m_A[i]->num_rows();
	}

	m_dOperatorComplexity = totalNNZs / m_A[0]->total_num_connections();
	m_dGridComplexity = totalNrOfNodes / m_A[0]->num_rows();

	UG_DLOG(LIB_ALG_AMG, 1, "Operator Complexity: " << m_dOperatorComplexity << " grid complexity: "
			<< m_dGridComplexity << std::endl << std::endl);

	m_bInited = true;
	return true;
}


//!
//! amg constructor
template<typename TAlgebra>
amg_base<TAlgebra>::amg_base() :
	m_numPreSmooth(2),
	m_numPostSmooth(2),
	m_cycleType(1),

	m_maxLevels(100),
	m_usedLevels(0),

	m_presmoother(NULL),
	m_postsmoother(NULL),
	m_basesolver(NULL),

	m_pPositionProvider2d(NULL),
	m_pPositionProvider3d(NULL)
{
	m_vec4 = NULL;
	m_bInited = false;

	m_maxNodesForBase = 100;
	m_dMaxFillBeforeBase = 0.5;

	m_writeMatrices = false;
	m_bFSmoothing = false;

	m_dbgDimension = 0;

}


template<typename TAlgebra>
void amg_base<TAlgebra>::cleanup()
{
	for(size_t i=1; i < m_A.size(); i++) {
		if(m_A[i]) delete m_A[i]; m_A[i] = NULL;
	}


	for(size_t i=0; i < m_vec1.size(); i++) { if(m_vec1[i]) delete m_vec1[i]; m_vec1[i] = NULL; }
	for(size_t i=0; i < m_vec2.size(); i++) { if(m_vec2[i]) delete m_vec2[i]; m_vec2[i] = NULL; }
	for(size_t i=0; i < m_vec3.size(); i++) { if(m_vec3[i]) delete m_vec3[i]; m_vec3[i] = NULL; }

	for(size_t i=0; i < m_postsmoothers.size(); i++) {
		if(m_postsmoothers[i] && (i < m_presmoothers.size() && m_presmoothers[i] == m_postsmoothers[i]) == false)
			delete m_postsmoothers[i];
		m_postsmoothers[i] = NULL;
	}

	for(size_t i=0; i < m_presmoothers.size(); i++) {
		if(m_presmoothers[i])
			delete m_presmoothers[i];
		m_presmoothers[i] = NULL;
	}

	m_usedLevels = 0;
	m_bInited=false;
}
//!
//! amg destructor
template<typename TAlgebra>
amg_base<TAlgebra>::~amg_base()
{
	cleanup();
}


template<typename TAlgebra>
bool amg_base<TAlgebra>::f_smoothing(vector_type &corr, vector_type &d, size_t level)
{
	UG_ASSERT(level < is_fine.size(), "fine markers not available for level " << level);

	matrix_type &M = *m_A[level];
	UG_ASSERT(M.num_rows() == is_fine[level].size(), "fine markers do not match in size on level " << level);
	for(size_t i=0; i<M.num_rows(); i++)
	{
		if(is_fine[level][i])
			corr[i] = d[i]/M(i,i);
		else
			corr[i] = 0.0;
	}

	M.matmul_minus(d, corr);
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_correction_and_update_defect:
//------------------------------------

template<typename TAlgebra>
bool amg_base<TAlgebra>::get_correction_and_update_defect(vector_type &c, vector_type &d, size_t level)
{
	UG_ASSERT(c.size() == d.size() && c.size() == m_A[level]->num_rows(),
			"c.size = " << c.size() << ", d.size = " << d.size() << ", A.size = " << m_A[level]->num_rows() << ": not matching");

	const matrix_type &Ah = *(m_A[level]);

	c.set(0.0);
	if(level == m_usedLevels-1)
	{
		m_basesolver->apply_return_defect(c, d);
		return true;
	}

	vector_type &corr = *m_vec3[level];

	// presmooth
	// same as setting c.set(0.0).
	m_presmoothers[level]->apply_update_defect(c, d);
	for(size_t i=1; i < m_numPreSmooth; i++)
	{
		m_presmoothers[level]->apply_update_defect(corr, d);
		c += corr;
	}

	// pre f-smoothing
	if(m_bFSmoothing)
	{
		f_smoothing(corr, d, level);
		c+=corr;
	}

	vector_type &cH = *m_vec1[level+1];
	vector_type &dH = *m_vec2[level+1];

#ifdef UG_PARALLEL
	cH.set_storage_type(PST_CONSISTENT);
#endif

	// restrict defect
	// dH = m_R[level]*d;
	m_R[level]->apply(dH, d);

	// apply lmgc on coarser nodes
	if(level+1 == m_usedLevels-1)
		get_correction_and_update_defect(cH, dH, level+1);
	else
		for(int i=0; i< m_cycleType; i++)
			get_correction_and_update_defect(cH, dH, level+1);

	// interpolate correction
	// corr = m_P[level]*cH
	m_P[level]->apply(corr, cH);

	// add coarse grid correction to level correction
	// c += corr;

	c += corr;

	// update defect
	// d = d - Ah*corr
	Ah.matmul_minus(d, corr);

	// post f-smoothing
	if(m_bFSmoothing)
	{
		f_smoothing(corr, d, level);
		c+=corr;
	}

	// postsmooth
	for(size_t i=0; i < m_numPostSmooth; i++)
	{
		m_postsmoothers[level]->apply_update_defect(corr, d);
		c += corr;
	}

	return true;
}



template<typename TAlgebra>
bool amg_base<TAlgebra>::get_correction(vector_type &c, const vector_type &const_d)
{
	UG_ASSERT(c.size() == const_d.size() && c.size() == m_A[0]->num_rows(),
				"c.size = " << c.size() << ", d.size = " << const_d.size() << ", A.size = " << m_A[0]->num_rows() << ": not matching");


	if(m_vec4 == NULL)
		m_vec4 = new vector_type;

	m_vec4->resize(const_d.size());
#ifdef UG_PARALLEL
	m_vec4->set_layouts(c.get_master_layout(), c.get_slave_layout());
	m_vec4->set_storage_type(PST_ADDITIVE);
#endif
	vector_type &d = *m_vec4;

	d = const_d;
	return get_correction_and_update_defect(c, d);
}



template<typename TAlgebra>
bool amg_base<TAlgebra>::check(const vector_type &const_c, const vector_type &const_d)
{

	vector_type c, d;
	c = const_c;
	d = const_d;

#ifdef UG_PARALLEL
	if(!d.has_storage_type(PST_ADDITIVE) || !c.has_storage_type(PST_CONSISTENT))
	{
		UG_LOG("ERROR: In 'amg::check':Inadequate storage format of Vectors.\n");
		return false;
	}
#endif

	// build defect:  d := d_nl - J(u)*c_nl
//	m_A[0]->matmul_minus(d, c);

	for(size_t i=0; i<m_usedLevels-1; i++)
	{
		UG_LOG("\nLEVEL " << i << "\n\n");

		check_level(c, d, i);

		if(i+1 < m_usedLevels)
		{
			vector_type t; t = d;
			d.resize(m_A[i+1]->num_rows());
			c.resize(m_A[i+1]->num_rows());
			m_R[i]->apply(d, t);
			m_R[i]->apply(c, t);

		}

	}
	return true;
}

template<typename TAlgebra>
bool amg_base<TAlgebra>::writevec(std::string filename, const vector_type &d, size_t level)
{
	UG_ASSERT(m_writeMatrices, "");
	std::string name = (std::string(m_writeMatrixPath) + filename + ToString(level) + ".mat");
	AMGWriteToFile(*m_A[level], level, level, name.c_str(), m_amghelper);
	std::fstream f(name.c_str(), std::ios::out | std::ios::app);

	std::string name2 = (std::string(m_writeMatrixPath) + filename + ToString(level) + ".values");
	f << "v " << name2 << "\n";

	std::fstream file(name2.c_str(), std::ios::out);
	for(size_t i=0; i<d.size(); i++)
		file << m_amghelper.GetOriginalIndex(level, i) << " " << (d[i]) << std::endl;
	return true;
}


template<typename TAlgebra>
bool amg_base<TAlgebra>::check_level(vector_type &c, vector_type &d, size_t level)
{
	const matrix_type &Ah = *(m_A[level]);
	vector_type corr;


	//UG_LOG("preprenorm: " << d.two_norm() << std::endl);
	/*for(size_t i=0; i<5; i++)
		get_correction_and_update_defect(corr, d, level);*/

	double prenorm = d.two_norm();
	UG_LOG("Prenorm = " << prenorm << "\n");

	// presmooth
	// same as setting c.set(0.0).

	double n1 = d.two_norm(), n2;
	CloneVector(corr, c);
	m_presmoothers[level]->apply_update_defect(c, d);
	n2 = d.two_norm();	UG_LOG("presmoothing 1 " << ": " << n2/prenorm << "\t" <<n2/n1 << "\n");	n1 = n2;
	for(size_t i=1; i < m_numPreSmooth; i++)
	{
		m_presmoothers[level]->apply_update_defect(corr, d);
		c += corr;
		n2 = d.two_norm();	UG_LOG("presmoothing " << i+1 << ": " << n2/prenorm << "\t" <<n2/n1 << "\n");	n1 = n2;
	}

	if(m_bFSmoothing)
	{
		f_smoothing(corr, d, level);
		c+=corr;
		n2 = d.two_norm();	UG_LOG("pre f-smoothing: " << n2/prenorm << "\t" <<n2/n1 << "\n");	n1 = n2;
	}


	if(m_writeMatrices) writevec("AMG_dp", d, level);

	vector_type &cH = *m_vec1[level+1];
	vector_type &dH = *m_vec2[level+1];

	// restrict defect
	// dH = m_R[level]*d;

	m_R[level]->apply(dH, d);

	vector_type tc; tc.resize(cH.size());
	cH.set( 0.0);

	double nH1 = dH.norm();
	double preHnorm=nH1;
	size_t i;
	for(i=0; i<100; i++)
	{
		// apply lmgc on coarser nodes
		if(level+1 == m_usedLevels-1)
		{
			get_correction_and_update_defect(tc, dH, level+1);
			cH += tc;
		}
		else
			for(int i=0; i< m_cycleType; i++)
			{
				get_correction_and_update_defect(tc, dH, level+1);
				cH += tc;
			}

		double nH2 = dH.norm();
		if(i < 6)
		{	UG_LOG("coarse correction (on coarse) " << i+1 << ": " << nH2/nH1 << "\n"); nH1 = nH2; }
		if(nH2/preHnorm < 0.01) { UG_LOG("coarse solver reduced by 0.01 in iteration " << i << std::endl); break; }

	}

	// interpolate correction
	// corr = m_P[level]*cH

	m_P[level]->apply(corr, cH);

#ifdef UG_PARALLEL
	cH.set_storage_type(PST_CONSISTENT);
#endif

	// add coarse grid correction to level correction

	c += corr;

	//update defect
	// d = d - Ah*corr
	Ah.matmul_minus(d, corr);

	n2 = d.two_norm();	UG_LOG("complete coarse correction " << i+1 << ": " << n2/prenorm << "\t" <<n2/n1 << "\n");	n1 = n2;

	if(m_writeMatrices) writevec("AMG_dc", d, level);


	// post f-smoothing
	if(m_bFSmoothing)
	{
		f_smoothing(corr, d, level);
		c+=corr;
	}
	n2 = d.two_norm();	UG_LOG("post f-smoothing: " << n2/prenorm << "\t" <<n2/n1 << "\n");	n1 = n2;

	// postsmooth
	for(size_t i=0; i < m_numPostSmooth; i++)
	{
		m_postsmoothers[level]->apply_update_defect(corr, d);
		c += corr;
		n2 = d.two_norm();	UG_LOG("postsmoothing " << i+1 << ": " << n2/prenorm << "\t" <<n2/n1 << "\n");	n1 = n2;
	}


	double postnorm = d.two_norm();
	UG_LOG("Level " << level << " reduction: " << postnorm/prenorm << std::endl);

	if(m_writeMatrices) writevec("AMG_d", d, level);

	UG_LOG("\n\n");
	return true;
}



template<typename TAlgebra>
void amg_base<TAlgebra>::tostring() const
{
	UG_LOG("AMGBase:\n");
	UG_LOG(" nr of pre smoothing steps (nu1)  = " << m_numPreSmooth<< std::endl);
	UG_LOG(" nr of post smoothing steps (nu2) = " << m_numPostSmooth << std::endl);

	UG_LOG(" cycle type = ");
	if(m_cycleType == 1) { UG_LOG("V-cycle\n"); }
	else if(m_cycleType == 2) { UG_LOG("W-cycle\n"); }
	else { UG_LOG("gamma = " << m_cycleType << "\n"); }

	UG_LOG(" max levels = " << m_maxLevels << std::endl);

	if(m_presmoother) 	{UG_LOG(" presmoother is " << m_presmoother->name() << ".\n");}
	else				{UG_LOG(" no presmoother set!\n");}

	if(m_postsmoother) 	{UG_LOG(" postsmoother is " << m_postsmoother->name() << ".\n");}
	else				{UG_LOG(" no postsmoother set!\n");}

	if(m_basesolver)	{UG_LOG(" basesolver set\n");}
	else				{UG_LOG(" no basesolver set!\n");}
	UG_LOG("Using base solver for max. " << m_maxNodesForBase << " nodes or " << m_dMaxFillBeforeBase*100 << "% fill in.\n")

	if(m_writeMatrices)
		UG_LOG("Writing Matrices to \"" << m_writeMatrixPath << "\"\n");

	if(m_bInited)
	{
		UG_LOG("AMG is initialized.\n");
		UG_LOG("Used Levels: " << m_usedLevels << "\n");
	}
}


template<typename TAlgebra>
void amg_base<TAlgebra>::update_positions()
{
	UG_ASSERT(m_pPositionProvider2d == NULL || m_pPositionProvider3d == NULL, "specify EITHER positions 2d or 3d");
	if(m_pPositionProvider2d == NULL && m_pPositionProvider3d == NULL)
		return;
	if(m_pPositionProvider2d)
	{
		std::vector<MathVector<2> > pos;
		m_pPositionProvider2d->get_positions(pos);
		set_positions(&pos[0], pos.size());
	}
	if(m_pPositionProvider3d)
	{
		std::vector<MathVector<3> > pos;
		m_pPositionProvider3d->get_positions(pos);
		set_positions(&pos[0], pos.size());
	}
}


} // namespace ug

#endif //  __H__LIB_DISCRETIZATION__AMG_SOLVER__AMG_BASE_IMPL_H__
