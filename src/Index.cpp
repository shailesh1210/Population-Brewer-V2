
#include "Index.h"

#include "NDArrayUtils.h"

#include <algorithm>
#include <cassert>


// Omit the second argument to loop over all elements
Index::Index(const std::vector<int>& sizes, const std::pair<int, int>& fixed)
  : m_dim(sizes.size()), m_idx(sizes.size(), 0), m_sizes(sizes), m_fixed(fixed), m_atEnd(false)
{
  assert(m_sizes.size());
  if (m_fixed.first != Unfixed)
  {
	m_idx[m_fixed.first] = m_fixed.second;
  }
  m_storageSize = m_sizes[0];
  for (size_t i = 1; i < m_dim; ++i)
	m_storageSize *= m_sizes[i];
}

//TODO why TF linker errors when using Unfixed in init list????
Index::Index(const std::vector<int>& sizes, const std::vector<int>& values)
: m_dim(sizes.size()), m_idx(values), m_sizes(sizes), /*m_fixed({-1,-1}),*/ m_atEnd(false)
{
 
	m_fixed.first = -1; m_fixed.second = -1;
	assert(m_sizes.size());
  assert(m_idx.size() == m_sizes.size());
  m_storageSize = m_sizes[0];
  for (size_t i = 1; i < m_dim; ++i)
  {
	assert(m_idx[i] < m_sizes[i]);
	m_storageSize *= m_sizes[i];
  }
}


Index::Index(const Index& rhs)
  : m_dim(rhs.m_dim), m_idx(rhs.m_idx), m_sizes(rhs.m_sizes), m_fixed(rhs.m_fixed), m_storageSize(rhs.m_storageSize), m_atEnd(rhs.m_atEnd)
{
}

const std::vector<int>& Index::operator++()
{
  for (int i = m_dim - 1; i != -1ll; --i)
  {
	// ignore the iteration axis
	if (i == m_fixed.first) continue;

	++m_idx[i];
	if (m_idx[i] != m_sizes[i])
	  break;
	if (i == 0 || (m_fixed.first == 0 && i == 1))
	  m_atEnd = true;
	m_idx[i] = 0;
  }
  return m_idx;
}

// Implicitly cast to index vector
Index::operator const std::vector<int>&() const
{
  return m_idx;
}

size_t Index::size() const 
{
  return m_idx.size();
}

const std::vector<int>& Index::sizes() const
{
  return m_sizes;
}


// allow read-only access to individual values
const int& Index::operator[](size_t i) const
{
  return m_idx[i];
}

// allow modification of individual values
int& Index::operator[](size_t i)
{
  return m_idx[i];
}

// NB row-major offset calc is in NDArray itself

// need this for e.g. R where storage is column-major
size_t Index::colMajorOffset() const
{
  size_t ret = 0;
  size_t mult = m_storageSize;
  for (int i = m_dim-1; i >= 0; --i)
  {
	mult /= m_sizes[i];
	ret += mult * m_idx[i];
  }
  return ret;
}

void Index::reset()
{
  m_idx.assign(m_dim, 0);
  m_atEnd = false;
}

bool Index::end() const
{
  return m_atEnd;
}


MappedIndex::MappedIndex(const Index& idx, const std::vector<int>& mappedDimensions)
: m_dim(mappedDimensions.size()), m_sizes(m_dim), m_mappedIndex(m_dim), m_atEnd(idx.end())
{
  int n = idx.size();
  (void)n; // avoid compiler warning about unused variable when assert exands to nothing
  // TODO check mappedDimensions are unique 
  for (size_t d = 0; d < m_dim; ++d)
  {
	// check mappedDimensions are within dimension of index
	assert(mappedDimensions[d] < n);
	m_sizes[d] = idx.sizes()[mappedDimensions[d]];
	m_mappedIndex[d] = &const_cast<Index&>(idx)[mappedDimensions[d]];
  }
}


const MappedIndex& MappedIndex::operator++()
{
  for (int i = m_dim - 1; i != -1ll; --i)
  {
	++*m_mappedIndex[i];
	if (*m_mappedIndex[i] != m_sizes[i])
	  break;
	if (i == 0)
	  m_atEnd = true;
	*m_mappedIndex[i] = 0;
  }
  return *this;
}

// TODO better to overload NDArray to take Index types???
MappedIndex::operator const std::vector<int*>&() const
{
  return m_mappedIndex;
}

bool MappedIndex::end()
{
  return m_atEnd;
}

// std::vector<int> MappedIndex::excludeFrom(const std::vector<int>& dims, int excludedDim)
// {
//   std::vector<int> included;
//   included.reserve(dims.size() - 1);
//   for (int i = 0; i < (int)dims.size(); ++i)
//   {
//     if (i != excludedDim)
//       included.push_back(dims[i]);
//   } 
//   return included;
// }

