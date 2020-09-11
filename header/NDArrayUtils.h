#pragma once

#include "NDArray.h"
#include "Index.h"

#include <vector>
#include <numeric>
#include <cassert>
#include <iostream>
#include <fstream>
#include <map>


int maxAbsElement(const std::vector<int>& r);

std::vector<int> diff(const std::vector<int>& x, const std::vector<int>& y);
std::vector<double> diff(const std::vector<double>& x, const std::vector<double>& y);

typedef std::map<int, std::vector<double>> Map;

template<typename T, typename U>
void diff(const NDArray<T>& x, const NDArray<U>& y, NDArray<double>& d)
{
  // TODO check x y and d sizes match
  for (Index index(x.sizes()); !index.end(); ++index)
  {
	d[index] = x[index] - y[index];
  }
}

bool allZeros(const std::vector<std::vector<int>>& r);

template<typename T>
T sum(const std::vector<T>& v)
{
  return std::accumulate(v.begin(), v.end(), T(0));
}

template<typename T>
T product(const std::vector<T>& v)
{
  return std::accumulate(v.begin(), v.end(), T(1), std::multiplies<T>());
}

template<typename T>
T sum(const NDArray<T>& a)
{
  return std::accumulate(a.rawData(), a.rawData() + a.storageSize(), T(0));
}

template<typename T>
T min(const NDArray<T>& a)
{
  T minVal = std::numeric_limits<T>::max();
  for (Index i(a.sizes()); !i.end(); ++i)
  {
	minVal = std::min(minVal, a[i]);
  }
  return minVal;
}

template<typename T>
T max(const NDArray<T>& a)
{
  T minVal = std::numeric_limits<T>::max();
  for (Index i(a.sizes()); !i.end(); ++i)
  {
	minVal = std::min(minVal, a[i]);
  }
  return minVal;
}

// TODO move printing somehwere else
template<typename T>
void print(const std::vector<T>& v, std::ostream& ostr = std::cout)
{
  for (size_t i = 0; i < v.size(); ++i)
  {
	ostr << v[i] << ", ";
  }
  ostr << std::endl;
}

template<typename T>
void print(T* p, size_t n, size_t breakAt = 1000000, std::ostream& ostr = std::cout)
{
  for (size_t i = 0; i < n; ++i)
  {
	ostr << p[i] << ", ";
	if (!((i+1) % breakAt))
	  ostr << std::endl;
  }
  ostr << std::endl;
}

template<typename T>
void roundEstimates(std::vector<T>&est)
{
	double adj = 0;
	for (size_t i = 0; i < est.size(); ++i)
	{
		double diff = adj + (est.at(i) - floor(est.at(i)));
		if (diff >= 0.5) {
			adj = diff - 1;
			est.at(i) = ceil(est.at(i));
		}
		else {
			adj = diff;
			est.at(i) = floor(est.at(i));
		}

	}
}

//overloaded print method - Shailesh Tamrakar
template<typename T>
void print(T* p, const std::vector<int>sizes, Map &m_est, size_t breakAt = 1000000, std::ostream& ostr = std::cout)
{
	std::ofstream myfile;
	myfile.open("ipf.csv", std::ios::app);
 
	size_t  row_size = sizes.at(0);
	size_t col_size = sizes.at(1);

	
  for(size_t i = 0; i < row_size; i++)
	{
		std::vector<double>est;
		for(size_t j = 0; j < col_size; j++)
		{
			//ostr << (int)p[col_size*i+j] << "\t";
			myfile << p[col_size*i+j] << ",";
			est.push_back(p[col_size*i+j]);
		}
		
		roundEstimates(est);

		m_est.insert(std::make_pair(i+1, est));
		//ostr << std::endl;
		myfile << std::endl;
	}
  myfile << std::endl;

  myfile.close();
}



template<typename T>
bool isZero(const std::vector<T>& v)
{
  for (size_t i = 0; i < v.size(); ++i)
  {
	if (v[i] != 0)
	  return false;
  }
  return true;
}

template<typename T>
T marginalProduct(const std::vector<std::vector<T>>& m, const std::vector<int64_t>& idx)
{
  assert(m.size() == idx.size());
  T p = T(1);

  for (size_t d = 0; d < m.size(); ++d)
  {
	p *= m[d][idx[d]];
  }
  return p;
}

// Reduce n-D array to 1-D sums
template<typename T>
std::vector<T> reduce(const NDArray<T>& input, size_t orient)
{
  // return reduced;
  // check valid orientation
  assert(orient < input.dim());

  std::vector<T> sums(input.size(orient), 0);

  // Index indexer(input.sizes(), std::make_pair(orient, 0));
  // for (; !indexer.end(); ++indexer)
  // {
  //   // Pass index in directly to avoid
  //   typename NDArray<T>::ConstIterator it(input, orient, indexer);
  //   for(size_t i = 0; !it.end(); ++it, ++i)
  //   {
  //     sums[i] += *it;
  //   }
  // }

  // this is MUCH slower!!!
  
  Index indexer(input.sizes(), std::make_pair(-1,-1));
  for (; !indexer.end(); ++indexer)
  {
	sums[indexer[orient]] += input[indexer];
  }

  
  return sums;
}


// // Reduce n-D array to 1-D sums
// template<typename T>
// std::vector<T> reduce(const NDArray<T>& input, size_t orient)
// {
//   const size_t n = input.size(orient);
//   assert(orient < input.dim());

//   std::vector<T> sums(n, T(0));

//   // Loop over elements in the orient dimension
//   for (size_t i = 0; i < n; ++i)
//   {
//     // sum over elements in all orther dims
//     for (Index index(input.sizes(), { orient, i }); !index.end(); ++index)
//     {
//       sums[i] += input[index];
//     }
//   }

//   return sums;
// }

// // Reduce n-D array to 1-D sums
// template<typename T>
// std::vector<T> reduce(const NDArray<T>& input, size_t orient)
// {
//   const size_t n = input.size(orient);
//   assert(orient < input.dim());

//   std::vector<T> sums(n, T(0));

//   Index index(input.sizes());
//   const int64_t& k = index[orient];
//   for (; !index.end(); ++index)
//   {
//     sums[k] += input[index];
//   }

//   return sums;
// }


// Reduce n-D array to m-D sums (where m<n)
template<typename T>
NDArray<T> reduce(const NDArray<T>& input, const std::vector<int64_t>& preservedDims)
{
  const size_t reducedDim = preservedDims.size();
  // check valid orientation
  assert(reducedDim < input.dim());

  std::vector<int64_t> preservedSizes(reducedDim);
  for (size_t d = 0; d < reducedDim; ++d)
  {
	preservedSizes[d] = input.sizes()[preservedDims[d]];
  }

  NDArray<T> reduced(preservedSizes);
  reduced.assign(T(0));

  Index index(input.sizes());
  MappedIndex rIndex(index, preservedDims);
  for (; !index.end(); ++index)
  {
	reduced[rIndex] += input[index];
  }

  return reduced;
}


// take a D-1 dimensional slice at element index in orientation O
template<typename T>
NDArray<T> slice(const NDArray<T>& input, std::pair<int64_t, int64_t> index)
{
  if ((size_t)index.first >= input.dim())
	throw std::runtime_error("dimension out of bounds in slice");
  if (index.second >= input.sizes()[index.first])
	throw std::runtime_error("index out of bounds in slice");

  std::vector<int64_t> remainingSizes;
  remainingSizes.reserve(input.dim() - 1);
  for (size_t i = 0; i < input.dim(); ++i)
  {
	if (i != (size_t)index.first)
	{
	  remainingSizes.push_back(input.sizes()[i]);
	}
  }
  NDArray<T> output(remainingSizes);
  Index inputIndex(input.sizes(), index);
  Index outputIndex(output.sizes());
  for(;!inputIndex.end(); ++inputIndex, ++outputIndex)
  {
	output[outputIndex] = input[inputIndex];
  }
  return output;
}


// Converts a D-dimensional population array into a list with D columns and pop rows
template<typename T>
std::vector<std::vector<int>> listify(const size_t pop, const NDArray<T>& t)
{
  std::vector<std::vector<int>> list(t.dim(), std::vector<int>(pop));
  Index index(t.sizes());

  size_t pindex = 0;
  while (!index.end())
  {
	for (T i = 0; i < t[index]; ++i)
	{
	  const std::vector<int64_t>& ref = index;
	  for (size_t j = 0; j < t.dim(); ++j)
	  {
		list[j][pindex] = ref[j];
	  }
	  ++pindex;
	}
	++index;
  }
  return list;
}

