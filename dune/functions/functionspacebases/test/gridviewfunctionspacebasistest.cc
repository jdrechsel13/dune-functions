// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#include <config.h>

#include <iostream>

#include <dune/common/exceptions.hh>
#include <dune/common/typetraits.hh>

#include <dune/geometry/quadraturerules.hh>

#include <dune/grid/yaspgrid.hh>

#include <dune/localfunctions/test/test-localfe.hh>

#include <dune/functions/functionspacebases/interpolate.hh>
#include <dune/functions/functionspacebases/pq1nodalbasis.hh>
#include <dune/functions/functionspacebases/pq2nodalbasis.hh>
#include <dune/functions/functionspacebases/pqknodalbasis.hh>

using namespace Dune;
using namespace Dune::Functions;

/** \param disabledLocalTests Allows to disable certain local tests (see dune-localfunctions/dune/localfunctions/test/test-localfe.hh)
 */
template <typename Basis>
void testScalarBasis(const Basis& feBasis,
                     bool isPartitionOfUnity,
                     char disabledLocalTests = DisableNone)
{
  static const int dim = Basis::GridView::dimension;

  //////////////////////////////////////////////////////////////////////////////////////
  //  Run the dune-localfunctions test for the LocalFiniteElement of each grid element
  //////////////////////////////////////////////////////////////////////////////////////

  typedef typename Basis::GridView GridView;
  GridView gridView = feBasis.gridView();

  typename Basis::LocalView localView(&feBasis);


  // Test the LocalFiniteElement
  for (auto it = gridView.template begin<0>(); it!=gridView.template end<0>(); ++it)
  {
    // Bind the local FE basis view to the current element
    localView.bind(*it);

    // The general LocalFiniteElement unit test from dune/localfunctions/test/test-localfe.hh
    const auto& lFE = localView.tree().finiteElement();
    testFE(lFE, disabledLocalTests);
  }

  /////////////////////////////////////////////////////////////////////////
  //  Make sure the basis is a partition of unity
  /////////////////////////////////////////////////////////////////////////
  if (isPartitionOfUnity)
  {
    for(const auto& e : elements(gridView))
    {
      // Bind the local FE basis view to the current element
      localView.bind(e);

      const auto& lFE = localView.tree().finiteElement();

      const QuadratureRule<double,dim>& quad = QuadratureRules<double,dim>::rule(e.type(), 3);
      std::vector<FieldVector<double,1> > values;
      for (size_t i=0; i<quad.size(); i++)
      {
        lFE.localBasis().evaluateFunction(quad[i].position(), values);
        double sum = std::accumulate(values.begin(), values.end(), 0.0);

        if (std::abs(sum-1.0) > 1e-5)
          DUNE_THROW(Exception, "Basis is no partition of unity, even though it is supposed to be!");
      }
    }
  }

  ////////////////////////////////////////////////////////////////////////////
  //  Make sure the basis does not give out constant zero shape functions
  ////////////////////////////////////////////////////////////////////////////
  for(const auto& e : elements(gridView))
  {
    // Bind the local FE basis view to the current element
    localView.bind(e);

    const auto& lFE = localView.tree().finiteElement();

    const QuadratureRule<double,dim>& quad = QuadratureRules<double,dim>::rule(e.type(), 3);
    std::vector<FieldVector<double,1> > values;
    std::vector<double> sumOfAbsValues(lFE.size());
    std::fill(sumOfAbsValues.begin(), sumOfAbsValues.end(), 0.0);
    for (size_t i=0; i<quad.size(); i++)
    {
      lFE.localBasis().evaluateFunction(quad[i].position(), values);
      // Sum the absolute values for each quadrature point
      for (size_t j=0; j<values.size(); j++)
        sumOfAbsValues[j] += std::fabs(values[j][0]);
    }

    for (size_t i=0; i<values.size(); i++)
      if ( sumOfAbsValues[i] <= 1e-5)
        DUNE_THROW(Exception, "Basis gives out a constant-zero shape function!");
  }




  auto indexSet = feBasis.indexSet();

  // Check whether the basis exports a type 'MultiIndex'
  typedef typename Basis::MultiIndex MultiIndex;

  // And this type must be indexable
  static_assert(is_indexable<MultiIndex>(), "MultiIndex must support operator[]");

  ///////////////////////////////////////////////////////////////////////////////////
  //  Check whether the global indices are in the correct range,
  //  and whether each global index appears at least once.
  ///////////////////////////////////////////////////////////////////////////////////

  std::vector<bool> seen(indexSet.size());
  std::fill(seen.begin(), seen.end(), false);

  auto localIndexSet = indexSet.localIndexSet();

  // Loop over all leaf elements
  for (auto it = gridView.template begin<0>(); it!=gridView.template end<0>(); ++it)
  {
    // Bind the local FE basis view to the current element
    localView.bind(*it);
    localIndexSet.bind(localView);

    for (size_t i=0; i<localView.tree().size(); i++)
    {
      if (localIndexSet.index(i)[0] < 0)
        DUNE_THROW(Exception, "Index is negative, which is not allowed");

      if (localIndexSet.index(i)[0] >= seen.size())
        DUNE_THROW(Exception, "Local index " << i
                           << " is mapped to global index " << localIndexSet.index(i)
                           << ", which is larger than allowed");

      seen[localIndexSet.index(i)[0]] = true;
    }
  }

  for (size_t i=0; i<seen.size(); i++)
    if (! seen[i])
      DUNE_THROW(Exception, "Index [" << i << "] does not exist as global basis vector");

  //////////////////////////////////////////////////////////////////////////////////////////
  // Interpolate the function f(x,y) = x wrt the basis, and check whether we get
  // the expected integral.
  //////////////////////////////////////////////////////////////////////////////////////////

  std::vector<double> x(indexSet.size());
  interpolate(feBasis, x, [](FieldVector<double,dim> x){ return x[0]; });

  // Objects required in the local context
  auto localIndexSet2 = feBasis.indexSet().localIndexSet();
  std::vector<double> localCoefficients(localView.maxSize());

  // Loop over elements and integrate over the function
  double integral = 0;
  for (auto it = gridView.template begin<0>(); it != gridView.template end<0>(); ++it)
  {
    localView.bind(*it);
    localIndexSet.bind(localView);
    localIndexSet2.bind(localView);

    // paranoia checks
    assert(localView.size() == localIndexSet.size());
    assert(&(localView.globalBasis()) == &(feBasis));
    assert(&(localIndexSet.localView()) == &(localView));

    assert(localIndexSet.size() == localIndexSet2.size());
    for (size_t i=0; i<localIndexSet.size(); i++)
      assert(localIndexSet.index(i) == localIndexSet2.index(i));

    // copy data from global vector
    localCoefficients.resize(localIndexSet.size());
    for (size_t i=0; i<localIndexSet.size(); i++)
      localCoefficients[i] = x[localIndexSet.index(i)[0]];

    // get access to the finite element
    typedef typename Basis::LocalView::Tree Tree;
    auto& treeImp = localView.tree();
    const typename Tree::Interface& tree = treeImp;

    auto& localFiniteElement = tree.finiteElement();

    // we have a flat tree...
    assert(localView.size() == tree.size());
    assert(localView.size() == tree.finiteElement().localBasis().size());

    // A quadrature rule
    const QuadratureRule<double, dim>& quad = QuadratureRules<double, dim>::rule(it->type(), 1);

    // Loop over all quadrature points
    for ( size_t pt=0; pt < quad.size(); pt++ ) {

      // Position of the current quadrature point in the reference element
      const FieldVector<double,dim>& quadPos = quad[pt].position();

      // The multiplicative factor in the integral transformation formula
      const double integrationElement = it->geometry().integrationElement(quadPos);

      // Evaluate all shape function values at this point
      std::vector<FieldVector<double,1> > shapeFunctionValues;
      localFiniteElement.localBasis().evaluateFunction(quadPos, shapeFunctionValues);

      // Actually compute the vector entries
      for (size_t i=0; i<localFiniteElement.localBasis().size(); i++)
      {
        integral += localCoefficients[tree.localIndex(i)] * shapeFunctionValues[i] * quad[pt].weight() * integrationElement;
      }
    }

    // unbind
    localIndexSet.unbind();
    localView.unbind();
  }

  if (std::abs(integral-0.5) > 1e-10)
    DUNE_THROW(Dune::Exception, "Error: integral value is wrong!");
}

template <int dim>
void testOnStructuredGrid()
{
  std::cout << "   +++++++++++  Testing on structured " << dim << "d grids  ++++++++++++" << std::endl;

  // Generate grid for testing
  typedef YaspGrid<dim> GridType;
  FieldVector<double,dim> l;
  std::fill(l.begin(), l.end(), 1.0);
  std::array<int,dim> elements;
  std::fill(elements.begin(), elements.end(), 2);
  GridType grid(l,elements);

  // Test whether PQ1FunctionSpaceBasis.hh can be instantiated on the leaf view
  typedef typename GridType::LeafGridView GridView;
  const GridView& gridView = grid.leafGridView();

  // Test PQ1NodalBasis
  PQ1NodalBasis<GridView> pq1Basis(gridView);
  testScalarBasis(pq1Basis, true);

  // Test PQ2NodalBasis
  PQ2NodalBasis<GridView> pq2Basis(gridView);
  testScalarBasis(pq2Basis, true);

  // Test PQKNodalBasis for k==3
  PQKNodalBasis<GridView, 3> pq3Basis(gridView);
  if (dim<3) // Currently not implemented for dim >= 3
    testScalarBasis(pq3Basis, true);

  // Test PQKNodalBasis for k==4
  PQKNodalBasis<GridView, 4> pq4Basis(gridView);
  if (dim<3) // Currently not implemented for dim >= 3
    testScalarBasis(pq4Basis, true);

}

int main (int argc, char* argv[]) try
{
  testOnStructuredGrid<1>();
  testOnStructuredGrid<2>();
  testOnStructuredGrid<3>();
  return 0;

} catch ( Dune::Exception &e )
{
  std::cerr << "Dune reported error: " << e << std::endl;
}
catch(...)
{
  std::cerr << "Unknown exception thrown!" << std::endl;
}
