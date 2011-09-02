----------------------------------------------------------
--
--   Lua - Script to perform the Laplace-Problem
--
--   Author: Martin Rupp / Andreas Vogel
--
----------------------------------------------------------
-- SetOutputProcessRank(0)
SetOutputProfileStats(false)

ug_load_script("ug_util.lua")

-- constants
if util.HasParamOption("-3d") then
	dim = 3
else
	dim = 2
end

-- choose dimension and algebra
InitUG(dim, CPUAlgebraSelector());

if dim == 2 then
	gridName = util.GetParam("-grid", "unit_square_01/unit_square_01_tri_2x2.ugx")	
	-- gridName = "unit_square/unit_square_quads_8x8.ugx"
end
if dim == 3 then
	gridName = util.GetParam("-grid", "unit_square/unit_cube_hex.ugx")
	-- gridName = "unit_square/unit_cube_tets_regular.ugx"
end

-- choose number of total Refinements (incl. pre-Refinements)
numRefs = util.GetParamNumber("-numRefs", 1)

-- choose number of pre-Refinements (before sending grid onto different processes)	
numPreRefs = util.GetParamNumber("-numPreRefs", math.min(5, numRefs-2))

maxBase = util.GetParamNumber("-maxBase", 1000)

RAepsilon = util.GetParamNumber("-RAepsilon", 1)
RAalpha = util.GetParamNumber("-RAalpha", 0)

epsx = util.GetParamNumber("-epsx", 1)
epsy = util.GetParamNumber("-epsy", 1)

bFileOutput = true
bOutput = false

if util.HasParamOption("-RSAMG") then
	bRSAMG = true
else
	bRSAMG = false
end

print("Parameters: ")
print("    numPreRefs = "..numPreRefs)
print("    numRefs = "..numRefs)
print("    maxBase = "..maxBase)
print("    dim = "..dim)
print("    gridName = "..gridName)
print("    RAepsilon = "..RAepsilon)
print("    RAalpha = "..RAalpha.." degree")
RAalpha = RAalpha * (2*math.pi/360)
print("    RAalpha = "..RAalpha.." grad")
print("    epsx = "..epsx)
print("    epsy = "..epsy)

function writeln(...)
	write(...)
	write("\n")
end

function my_assert(condition, text)
	if condition == false then
		error(text)
	end
end

--------------------------------
-- User Data Functions (begin)
--------------------------------
	function cbDirichletBnd2d(x, y, t)
		return true, 0		
	end
	dirchletBnd2d = util.CreateLuaBoundaryNumber("cbDirichletBnd"..dim.."d", dim)
	
	
	function cbSinRhs2d(x, y, t)
		local s = 2*math.pi
		return s*s*(math.sin(s*x) + math.sin(s*y))
	end
	sinRhs2d = util.CreateLuaUserNumber("cbSinRhs2d", 2)
	
	
	function cbSinDirichletBnd2d(x, y, t)
		local s = 2*math.pi
		return true, math.sin(s*x) + math.sin(s*y)
	end
	sinDirchletBnd2d = util.CreateLuaBoundaryNumber("cbSinDirichletBnd2d", 2)
	
	-- anisotropic diffusion in corners
	function cbAnisoDiffTensor2d(x, y, t)
		local fac1 = 1/1+exp(alpha*(x+y-1)) 
		local fac2 = 1- fac1
		return	fac1*epsx+fac2, 0, 
				0, fac1+fac2*epsy
	end
	anisoDiffTensor2d = util.CreateLuaUserMatrix("cbAnisoDiffTensor2d", 2)

	
	function CreateRotatedAnisotropyMatrix2d(alpha, epsilon)
		local sinalpha = math.sin(alpha)
		local cosalpha = math.cos(alpha)
		RAmat = ConstUserMatrix2d()
		-- print((sinalpha*sinalpha + epsilon*cosalpha*cosalpha)..", "..(1-epsilon)*sinalpha*cosalpha)
		-- print((1-epsilon)*sinalpha*cosalpha..", "..epsilon*sinalpha*sinalpha + cosalpha*cosalpha)
		return util.CreateConstUserMatrix2d(sinalpha*sinalpha + epsilon*cosalpha*cosalpha, (1-epsilon)*sinalpha*cosalpha,	(1-epsilon)*sinalpha*cosalpha, epsilon*sinalpha*sinalpha + cosalpha*cosalpha)		
	end	


	-- hedgehog diffusion 
	function cbHedgehogDiffTensor2d(x, y, t)
		if x<0 then 
			if y<0 then return 1.0, 0, 0, 1.0 
			else return epsx, 0, 0, 1.0 end
		else
			if y<0 then return 1.0, 0, 0, epsy 
			else return 1.0, -1.0, -1.0, 1.0 end
		end
		
		-- should never happen
		return 0, 0, 0, 0 
	end
	
	
	-- discontinuous coefficient
	function cbJumpDiffTensor2d(x, y, t)
		if (math.abs (x)<0.5 and math.abs(y)<0.5) then
			return	epsx, 0, 0, epsy
		else 
			return	1, 0, 0, 1
		end
	end	
	


problem = "rotatedAniso"
-------------------------------------------
--  Setup User Functions
-------------------------------------------
if problem == "rotatedAniso" then
diffusionMatrix = CreateRotatedAnisotropyMatrix2d(RAalpha, RAepsilon)
velocityField = util.CreateConstUserVector2d(0,0)
reaction = util.CreateConstUserNumber(0)
rhs = util.CreateConstUserNumber(0)
dirichlet = dirchletBnd2d
end

if problem == "hedgehog" then
diffusionMatrix = util.CreateLuaUserMatrix("cbHedgehogDiffTensor2d", 2)
velocityField = util.CreateConstUserVector2d(0,0)
reaction = util.CreateConstUserNumber(0)
rhs = util.CreateConstUserNumber(0)
dirichlet = dirchletBnd2d
end

if problem == "jump" then
diffusionMatrix = util.CreateLuaUserMatrix("cbJumpDiffTensor2d", 2)
velocityField = util.CreateConstUserVector2d(0,0)
reaction = util.CreateConstUserNumber(0)
rhs = util.CreateConstUserNumber(0)
dirichlet = dirchletBnd2d
end

--------------------------------
-- User Data Functions (end)
--------------------------------
tBefore = os.clock()

-- create Instance of a Domain
dom = Domain()
if util.LoadDomain(dom, gridName) == false then
print("Loading Domain failed.")
exit()
end

-- create Refiner
my_assert(numPreRefs < numRefs, "numPreRefs must be smaller than numRefs");

refiner = GlobalDomainRefiner(dom)
for i=1,numPreRefs do
refiner:refine()
end

-- Distribute the domain to all involved processes
my_assert(DistributeDomain(dom) == true, "Error while Distributing Grid.")

-- Perform post-refine
for i=numPreRefs+1,numRefs do
refiner:refine()
end

tGrid = os.clock()-tBefore

-- get subset handler
sh = dom:get_subset_handler()
my_assert(sh:num_subsets() == 2, "Domain must have 2 Subsets for this problem.")
sh:set_subset_name("Inner", 0)
sh:set_subset_name("DirichletBoundary", 1)
--sh:set_subset_name("NeumannBoundary", 2)

-- write grid to file for test purpose
-- SaveDomain(dom, "refined_grid.ugx")

-- create Approximation Space
approxSpace = util.CreateApproximationSpace(dom)
approxSpace:add_fct("c", "Lagrange", 1)
approxSpace:init()



-----------------------------------------------------------------
--  Setup FV Convection-Diffusion Element Discretization
-----------------------------------------------------------------
if dim == 2 then
upwind = WeightedUpwind2d(); 
else
upwind = WeightedUpwind3d();
end
upwind:set_weight(0.0)
elemDisc = util.CreateFV1ConvDiff(approxSpace, "c", "Inner")
my_assert(elemDisc:set_upwind(upwind), "could not set upwind")
elemDisc:set_diffusion_tensor(diffusionMatrix)
elemDisc:set_velocity_field(velocityField)
elemDisc:set_reaction(reaction)
elemDisc:set_source(rhs)

-----------------------------------------------------------------
--  Setup Neumann Boundary
-----------------------------------------------------------------

--neumannDisc = util.CreateNeumannBoundary(approxSpace, "Inner")
--neumannDisc:add(neumann, "c", "NeumannBoundary")

-----------------------------------------------------------------
--  Setup Dirichlet Boundary
-----------------------------------------------------------------

dirichletBND = util.CreateDirichletBoundary(approxSpace)
dirichletBND:add(dirichlet, "c", "DirichletBoundary")

-------------------------------------------
--  Setup Domain Discretization
-------------------------------------------

domainDisc = DomainDiscretization()
domainDisc:set_approximation_space(approxSpace)
domainDisc:add(elemDisc)
--domainDisc:add(neumannDisc)
domainDisc:add(dirichletBND)



-------------------------------------------
--  Algebra
-------------------------------------------
print ("Setting up Algebra Solver")

-- create operator from discretization
linOp = AssembledLinearOperator()
linOp:set_discretization(domainDisc)
linOp:set_dof_distribution(approxSpace:get_surface_dof_distribution())

-- get grid function
u = approxSpace:create_surface_function()
b = approxSpace:create_surface_function()

-- set initial value
u:set_random(-1.0, 1.0)

-- init Operator and set dirichlet values in start iterate
print ("Assemble Operator and dirichlet ... ")
linOp:init_op_and_rhs(b)
linOp:set_dirichlet_values(u)
print ("done")

-- write matrix for test purpose
if bOutput then
SaveMatrixForConnectionViewer(u, linOp, "Stiffness.mat")
SaveVectorForConnectionViewer(b, "Rhs.vec")
end

-- create algebraic Preconditioners

print ("create preconditioners... ")
jac = Jacobi()
jac:set_damp(0.8)
gs = GaussSeidel()
sgs = SymmetricGaussSeidel()
bgs = BackwardGaussSeidel()
ilu = ILU()
ilut = ILUT()


-- create Base Solver
baseConvCheck = StandardConvergenceCheck()
baseConvCheck:set_maximum_steps(500)
baseConvCheck:set_minimum_defect(1e-16)
baseConvCheck:set_reduction(1e-16)
baseConvCheck:set_verbose_level(false)

if false then
	base = LinearSolver()
	base:set_convergence_check(baseConvCheck)
	base:set_preconditioner(jac)
else
	base = LU()
end

-- create AMG ---
-----------------

if bRSAMG == false then
	print ("create FAMG... ")
	-- Testvectors for FAMG ---
	--------------------------	
	function CreateAMGTestvector(gridfunction, luaCallbackName, dim)
		local amgTestvector;
		amgTestvector = GridFunctionVectorWriter()
		amgTestvector:set_reference_grid_function(gridfunction)
		amgTestvector:set_user_data(util.CreateLuaUserNumber(luaCallbackName, dim))
		return amgTestvector	
	end
		
	function CreateAMGTestvectorDirichlet0(dirichletBND, approxSpace)
		local amgDirichlet0 = GridFunctionVectorWriterDirichlet0()
		amgDirichlet0:init(dirichletBND, approxSpace)
		return amgDirichlet0
	end
	
	amg = FAMGPreconditioner()	
	amg:set_delta(0.5)
	amg:set_theta(0.95)
	amg:set_aggressive_coarsening(false)
	amg:set_damping_for_smoother_in_interpolation_calculation(0.8)	
		
	-- add testvector which is 1 everywhere and only 0 on the dirichlet Boundary.
	testvectorwriter = CreateAMGTestvectorDirichlet0(dirichletBND, approxSpace)
	testvector = approxSpace:create_surface_function()
	testvectorwriter:update(testvector)	
	amg:add_vector_writer(testvectorwriter, 1.0)
	amg:set_testvector_damps(1)
	amg:set_damping_for_smoother_in_interpolation_calculation(0.8)
		
	if bOutput then
		amg:write_testvectors(true)
	end
	
		
	-- amg:set_debug_level_get_ratings(4)
	-- amg:set_debug_level_coloring(4)
	-- amg:set_debug_level_communicate_prolongation(4)
	-- amg:set_debug_level_overlap(4,4)
	-- amg:set_debug_level_precalculate_coarsening(4)
	-- amg:set_debug_level_calculate_parent_pairs(4)
else
	print ("create AMG... ")
	amg = RSAMGPreconditioner()
	-- amg:enable_aggressive_coarsening_A(2)
end


vectorWriter = GridFunctionPositionProvider()
vectorWriter:set_reference_grid_function(u)
amg:set_position_provider2d(vectorWriter)
if bOutput then
amg:set_matrix_write_path("/Users/mrupp/matrices/")
end

amg:set_num_presmooth(2)
amg:set_num_postsmooth(2)
amg:set_cycle_type(1)
amg:set_presmoother(jac)
amg:set_postsmoother(jac)
amg:set_base_solver(base)
amg:set_max_levels(20)

amg:set_min_nodes_on_one_processor(50000)
-- amg:set_preferred_nodes_on_one_processor(1000)
amg:set_max_nodes_for_base(maxBase)
amg:set_max_fill_before_base(0.7)
amg:set_fsmoothing(true)
amg:set_epsilon_truncation(0)
amg:tostring()


-- create Convergence Check
convCheck = StandardConvergenceCheck()
convCheck:set_maximum_steps(100)
convCheck:set_minimum_defect(1e-11)
convCheck:set_reduction(1e-12)

print("done.")
-- create Linear Solver
linSolver = LinearSolver()
linSolver:set_preconditioner(amg)
linSolver:set_convergence_check(convCheck)

-- Apply Solver

b2 = approxSpace:create_surface_function()
b2:assign(b)

-------------------------------------------
--  Apply Solver
-------------------------------------------
-- 1. init operator
print("Init operator (i.e. assemble matrix).")
tBefore = os.clock()
linOp:init()
tAssemble = os.clock()-tBefore

-- 2. init solver for linear Operator
print("Init solver for operator.")
linSolver:init(linOp)

-- 3. apply solver
print("Apply solver.")
tBefore = os.clock()
linSolver:apply_return_defect(u,b)
tSolve = os.clock()-tBefore
WriteGridFunctionToVTK(u, "Solution")

print("done")

printf = function(s,...)
	print(s:format(...))
end -- function

formatf = function(s, ...)
	return s:format(...)
end

function fsize (file)
	local current = file:seek()      -- get current position
    local size = file:seek("end")    -- get file size
    file:seek("set", current)        -- restore position
    return size
end

if bFileOutput and GetProcessRank() == 0 then
	output = io.open("output_"..os.date("y%Ym%md%d")..".txt", "a")
	if fsize(output) == 0 then 
		output:write("procs")
		output:write("\tnumRefs")
		output:write("\tndofs")
		output:write("\tsteps")
		output:write("\tlastReduction")
		output:write("\ttSetupAmg [ms]")
		output:write("\tc_A")
		output:write("\tc_G")
		output:write("\tused Levels")
		output:write("\ttSolve [s]")
		output:write("\ttGrid [s]")
		output:write("\ttAssemble [s]")
		output:write("\n")
	end
	output:write(GetNumProcesses())
	output:write("\t"..numRefs)
	output:write("\t"..amg:get_level_information(0):get_nr_of_nodes())
	output:write("\t"..convCheck:step())
	output:write("\t"..convCheck:defect()/convCheck:previous_defect())
	output:write("\t"..amg:get_timing_whole_setup_ms())
	output:write("\t"..amg:get_operator_complexity())
	output:write("\t"..amg:get_grid_complexity())
	output:write("\t"..amg:get_used_levels())
	output:write("\t"..tSolve)
	output:write("\t"..tGrid)
	output:write("\t"..tAssemble)
	output:write("\n")
	print(s)
	-- else
end


print("NumProcesses: "..GetNumProcesses())
print("numRefs: "..numRefs)
print("nr of nodes: "..amg:get_level_information(0):get_nr_of_nodes())
print("steps: "..convCheck:step())
print("last reduction: "..convCheck:defect()/convCheck:previous_defect())
print("tSetupAmg [ms]: "..amg:get_timing_whole_setup_ms())
print("c_A: "..amg:get_operator_complexity())
print("c_G: "..amg:get_grid_complexity())
print("used Levels: "..amg:get_used_levels())
print("tSolve [s]: "..tSolve)
print("tGrid [s]: "..tGrid)
print("tAssemble [s]: "..tAssemble)



if GetProfilerAvailable() == true then
	create_levelPN = GetProfileNode("c_create_AMG_level")
	
	function PrintParallelProfileNode(name)
		pn = GetProfileNode(name)
		t = pn:get_avg_total_time_ms()/to100 * 100
		tmin = ParallelMin(t)
		tmax = ParallelMax(t)
		printf("%s:\n%.2f %%, min: %.2f %%, max: %.2f %%", name, t, tmin, tmax)
	end
	
	if create_levelPN:is_valid() then
		if true then
			print(create_levelPN:call_tree())
			print(create_levelPN:child_self_time_sorted())
		end
		to100 = create_levelPN:get_avg_total_time_ms()
		PrintParallelProfileNode("create_OL2_matrix")
		PrintParallelProfileNode("CalculateTestvector")
		PrintParallelProfileNode("CreateSymmConnectivityGraph")
		PrintParallelProfileNode("calculate_all_possible_parent_pairs")
		PrintParallelProfileNode("color_process_graph")
		PrintParallelProfileNode("FAMG_recv_coarsening_communicate")
		PrintParallelProfileNode("update_rating")
		PrintParallelProfileNode("precalculate_coarsening")
		PrintParallelProfileNode("send_coarsening_data_to_processes_with_higher_color")
		PrintParallelProfileNode("communicate_prolongation")
		PrintParallelProfileNode("create_new_index")
		PrintParallelProfileNode("create_parent_index")
		PrintParallelProfileNode("create_interfaces")
		PrintParallelProfileNode("create_fine_marks")
		PrintParallelProfileNode("create_galerkin_product")
		PrintParallelProfileNode("CalculateNextTestvector")
	end

else
	print("Profiler not available.")
end	