// Index.h

#pragma once

#include <vector>

#include <cstddef>
#include <cstdint>

// Indexer for elements in n-D array, optionally holding one dimension constant
class Index
{
public:
  static const int Unfixed = -1;

  // Omit the second argument to loop over all elements
  explicit Index(const std::vector<int>& sizes, const std::pair<int, int>& fixed);
  
  // Create an index with a predefined position (all dims unfixed)
  Index(const std::vector<int>& sizes, const std::vector<int>& values);
  
  Index(const Index& rhs);

  // TODO return *this
  const std::vector<int>& operator++();

  // Implicitly cast to index vector
  operator const std::vector<int>&() const;

  // TODO rename to dim
  size_t size() const;

  const std::vector<int>& sizes() const;

  // allow read-only access to individual values
  const int& operator[](size_t i) const;

  // allow modification of individual values
  int& operator[](size_t i);

  // need this for e.g. R where storage is column-major
  // NB row-major offset calc is in NDArray itself
  size_t colMajorOffset() const;

  void reset();

  bool end() const;

public:
  size_t m_dim;
  std::vector<int> m_idx;
  std::vector<int> m_sizes;
  // Fixed point (dim, idx)
  std::pair<int, int> m_fixed;
  size_t m_storageSize;
  bool m_atEnd;
};


// Contains a mapping from a higher dimensionality to a lower one
class MappedIndex
{
public:
  MappedIndex(const Index& idx, const std::vector<int>& mappedDimensions);
  
  const MappedIndex& operator++();  

  // TODO better to overload NDArray to take Index types???
  operator const std::vector<int*>&() const;

  bool end();

  //static std::vector<int> excludeFrom(const std::vector<int>& dims, int excludedDim);

private:
  size_t m_dim;
  std::vector<int> m_sizes;
  std::vector<int*> m_mappedIndex;
  bool m_atEnd;
};

