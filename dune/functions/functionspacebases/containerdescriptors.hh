// -*- tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi: set et ts=4 sw=2 sts=2:
#ifndef DUNE_FUNCTIONS_FUNCTIONSPACEBASES_CONTAINERDESCRIPTORS_HH
#define DUNE_FUNCTIONS_FUNCTIONSPACEBASES_CONTAINERDESCRIPTORS_HH

#include <array>
#include <cassert>
#include <functional>
#include <type_traits>
#include <vector>

#include <dune/common/filledarray.hh>
#include <dune/common/tuplevector.hh>
#include <dune/common/typeutilities.hh>


/**
 * \file containerdescriptor.hh
 * \brief Lightweight representation of (hierarchical) size and block structure extracted
 * from a bsis to describe data structures like containers that can be accessed by
 * multi-indices provided by the basis.
 *
 * The structure of a container-descriptor is a reduced container interface:
 * \code
  struct [Container]Descriptor
  {
    template<class Index>
    [SubContainerDescriptor] operator[](Index i) const;  // return the i-th sub-container-descriptor

    [static constexpr] std::size_t size() [const];  // return the number of children
  };
 * \endcode
 *
 * With the `operator[]` you can access the children. The `Index` type is either
 * an integral value or an `integral_constant` for tuple nodes.
 *
 * Size is either a static property, or a runtime value.
 **/

namespace Dune::Functions::ContainerDescriptors {

//! Fallback container descriptor if nothing else fits
struct Unknown {};


//! The node in the descriptor tree representing a value placeholder
struct Value
{
  //! The child access method is only available for the interface, but should not be called.
  template<class Index>
  Value operator[] (const Index&) const { return {}; }

  //! A value placeholder does not have any sub-descriptors, thus its size is zero.
  static constexpr std::size_t size () { return 0; }
};

//! Descriptor with all children of possibly different type
template<class... Children>
using Tuple = Dune::TupleVector<Children...>;

//! Generate a descriptor in case the children are not all of the same type.
//! \relates Tuple
template<class Child0, class... Children,
  std::enable_if_t<(sizeof...(Children) > 0), int> = 0,
  std::enable_if_t<(...|| (not std::is_same_v<Child0, Children>)), int> = 0>
auto makeDescriptor (Child0 child0, Children... children)
{
  using Descriptor = Tuple<Child0,Children...>;
  return Descriptor{std::move(child0),std::move(children)...};
}


//! Descriptor for arrays with all children of the same type and static size.
template<class Child, std::size_t n>
using Array = std::array<Child, n>;

//! Generate a descriptor in case the children are all of the same type.
template<class Child0, class... Children,
  std::enable_if_t<(std::is_same_v<Child0, Children> &&...), int> = 0>
auto makeDescriptor (Child0 child, Children... children)
{
  using Descriptor = Array<Child0,1+sizeof...(Children)>;
  return Descriptor{std::move(child),std::move(children)...};
}


//! Descriptor for vectors with all children of the same type and dynamic size.
template<class Child>
using Vector = std::vector<Child>;

//! Descriptor for arrays with all children identical and the number of children a static size.
template<class Child, std::size_t n>
struct UniformArray
{
  //! Default constructor. Is enable if the child-type is default constructible.
  template<class C = Child,
    std::enable_if_t<std::is_default_constructible_v<C>, int> = 0>
  UniformArray ()
    : child_{}
  {}

  //! Constructor that stores a single child only.
  explicit UniformArray (Child child)
    : child_{std::move(child)}
  {}

  //! Access the i'th child that is always the same, i.e., `child_`.
  template<class Index>
  const Child& operator[] (const Index& /*i*/) const { return child_; }

  //! The static size information, i.e., number of children.
  static constexpr std::size_t size () { return n; }

private:
  Child child_;
};

//! Alias for a uniform array storing value placeholders
template<std::size_t n>
using FlatArray = UniformArray<Value,n>;

//! Generate a uniform descriptor in case the size is a static constant
template<class Child, std::size_t n>
auto makeUniformDescriptor (std::integral_constant<std::size_t,n>, Child child)
{
  return UniformArray<Child,n>{std::move(child)};
}


//! Uniform descriptor with dynamic size.
template<class Child>
struct UniformVector
{
  //! Default constructor with size. Is enable if the child-type is default constructible.
  template<class C = Child,
    std::enable_if_t<std::is_default_constructible_v<C>, int> = 0>
  explicit UniformVector (std::size_t size)
    : size_{size}
    , child_{}
  {}

  //! Constructor that stores the size and a single child only.
  UniformVector (std::size_t size, Child child)
    : size_{size}
    , child_{std::move(child)}
  {}

  //! Access the i'th child that is always the same, i.e., `child_`.
  template<class Index>
  const Child& operator[] (const Index& /*i*/) const { return child_; }

  //! The dynamic size information, i.e., number of children.
  std::size_t size () const { return size_; }

private:
  std::size_t size_;
  Child child_;
};

//! Alias for a uniform vector storing value placeholders
using FlatVector = UniformVector<Value>;

//! Generate a uniform descriptor in case the size is a dynamic value
template<class Child>
auto makeUniformDescriptor (std::size_t n, Child child)
{
  return UniformVector<Child>{n,std::move(child)};
}

} // end namespace Dune::Functions::ContainerDescriptors

#endif // DUNE_FUNCTIONS_FUNCTIONSPACEBASES_CONTAINERDESCRIPTORS_HH
