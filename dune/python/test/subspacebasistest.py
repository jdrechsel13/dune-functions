import numpy as np

import dune.grid
import dune.functions as functions
from dune.functions import defaultGlobalBasis, subspaceBasis, Lagrange, Power, Composite

import basistest        # The general test suite for function space bases
import interpolatetest  # The general test suite for interpolation into bases


# Test all currently supported global bases in dimension `dimension`.
# Only the size of the global bases are tested.
def test(dimension):
    lowerLeft = [-1] * dimension
    upperRight = [1] * dimension
    elements = [1] * dimension

    grid = dune.grid.structuredGrid(lowerLeft,upperRight,elements)

    # Test with first-order Lagrange basis vector field
    basis1 = defaultGlobalBasis(grid, Power(Lagrange(order=1), dimension))

    for i in range(dimension):
      basis1_i = subspaceBasis(basis1, i)
      basistest.checkBasis(basis1_i)
      interpolatetest.checkConstantInterpolation(basis1_i, 0)
      interpolatetest.checkBasisFunctionInterpolation(basis1_i)



    # Test with lowest order Taylor-Hood basis
    taylorHoodBasis = defaultGlobalBasis(grid, Composite(Power(Lagrange(order=2), dimension), Lagrange(order=1)))

    # Check velocity subspace
    velocityBasis = subspaceBasis(taylorHoodBasis, 0)
    basistest.checkBasis(velocityBasis)
    interpolatetest.checkConstantInterpolation(velocityBasis, np.zeros(dimension))
    interpolatetest.checkBasisFunctionInterpolation(velocityBasis)

    # Check pressure subspace
    pressureBasis = subspaceBasis(taylorHoodBasis, 1)
    basistest.checkBasis(pressureBasis)
    interpolatetest.checkConstantInterpolation(pressureBasis, 0)
    interpolatetest.checkBasisFunctionInterpolation(pressureBasis)

    # Check velocity component subspaces obtained by passing multiple indices at once
    for i in range(dimension):
      velocityBasis_i = subspaceBasis(taylorHoodBasis, 0, i)
      basistest.checkBasis(velocityBasis_i)
      interpolatetest.checkConstantInterpolation(velocityBasis_i, 0)
      interpolatetest.checkBasisFunctionInterpolation(velocityBasis_i)

    # Check velocity component subspaces obtained via nested subspaceBasis call
    for i in range(dimension):
      velocityBasis_i = subspaceBasis(velocityBasis, i)
      basistest.checkBasis(velocityBasis_i)
      interpolatetest.checkConstantInterpolation(velocityBasis_i, 0)
      interpolatetest.checkBasisFunctionInterpolation(velocityBasis_i)

# Run tests for grids of dimension 2
test(2)
