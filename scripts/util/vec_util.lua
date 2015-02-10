--[[
see also bridge/algebra_bridges/bridge_mat_vec_operations.h
These functions enable us to do some matrix stuff in lua e.g.

you will need to have gridFunctions for this to work:

x = GridFunction(approxSpace)
e = GridFunction(approxSpace)
d = GridFunction(approxSpace)
b = GridFunction(approxSpace)
temp = GridFunction(approxSpace)

-- and some assembled Matrix Operators
A = MatrixOperator()
B = MatrixOperator()


-- now you can calc some stuff
print(VecNorm(A*x-b))
-- or
print(VecProd(x, A*b))

-- or assign stuff
Dinv = MatrixDiagonalInverse(A)
Assign(e, Dinv*x)

-- VecNorm, VecScale and Assign all accept GridFunctions and VecScaleAdd-Objects
-- VecScaleAdd are of the form 
-- VSA_1 +/- VSA_2 +/- VSA_3 +/- ...
--- where VSA is a GridFunction, double*GridFunction, MatrixOperator*GridFunction or double*(MatrixOperator*GridFunction)

-- EXAMPLES:

-- Eigenvalue stuff

function rayleigh_quotient(u, A, B)
	return math.sqrt(VecProd(u, A*u)/VecProd(u, B*u))
end

function eigenvalue_defect(u, A, B)
	def = temp
	lambda = rayleigh_quotient(u, A, B)
	
	-- lua doesn't accept lambda*B*e. you have to use lambda*(B*e)
	Assign(def, A*e - lambda*(B*e))
end

-- smoothing stuff

function calcSmoothing(e, b)
	Se = GridFunction(approxSpace)
	Assign(Se, 1.0*e)

	-- perform 2 jacobi smooths
	solver=LinearSolver(Jacobi(0.6), ConvCheck(2, 1e-12, 1e-12, false) )	 
	solver:apply_return_defect(Se, b:clone())
	
	D = MatrixDiagonal(A)
	DInv = MatrixDiagonalInverse(A)
	
	-- check how amg norms changed
	Se1 = VecProd(A*Se, Se)
	e0 = VecProd(D*e, e)
	e1 = VecProd(A*e, e)
	e2 = VecProd(DInv*(A*e), A*e)
end

]]--

function __ug__TemplateExpressions_add(a, b)
	return VecScaleAddClass(1.0, a, 1.0, b)
end
function __ug__TemplateExpressions_sub(a, b)	
	return VecScaleAddClass(1.0, a, -1.0, b)
end

function __ug__TemplateExpressions_mul(a, b)
	if type(a) == "number" or type(b) == "number" then
		return VecScaleAddClass(a, b)
	else
		local aa = a
		local bb = b
		if ug_class_group(a) == "VecScaleAddClass" then
			aa = a:eval()
		end
		if ug_class_group(b) == "VecScaleAddClass" then
			bb = b:eval()
		end
		return VecProd(aa, bb)
	end		
end

function __ug__TemplateExpressions_div(a, b)
	return VecScaleAddClass(1/b, a)		
end
-------------------------------------------------------

function set_user_data_overloads2(name)
	-- request metatable for the classname
	mt = ug_get_metatable(name)
	if mt == nil then return end
	
	-- set __add function in metatable
	mt.__add = _G["__ug__TemplateExpressions_add"]

	-- set __sub function in metatable
	mt.__sub = _G["__ug__TemplateExpressions_sub"]
	
	-- set __mul function in metatable
	mt.__mul = _G["__ug__TemplateExpressions_mul"]
	
	-- set __mul function in metatable
	mt.__div = _G["__ug__TemplateExpressions_div"]
end

-- add GridFunctionData
for k, class in ipairs({"GridFunction"}) do
	-- loop algebra
	for j, algebra in ipairs({"CPU1", "CPU2", "CPU3", "CPU4", "CPUVAR"}) do
		-- loop dimensions
		for dim =  1,3 do
			-- only set if dimension is compiled (otherwise metatable does not exist)
			if ug_dim_compiled(dim) and ug_algebra_compiled(algebra) then
				set_user_data_overloads2(class..dim.."d"..algebra)
			end
		end
	end
end

for j, algebra in ipairs({"CPU1", "CPU2", "CPU3", "CPU4", "CPUVAR"}) do
	if ug_algebra_compiled(algebra) then
		set_user_data_overloads2("Vector"..algebra)
	end
end

