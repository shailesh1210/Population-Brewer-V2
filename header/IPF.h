// IPF.h
// C++ implementation of multidimensional* iterative proportional fitting
// marginals are 1d in this implementation

#pragma once

#include "NDArray.h"


#include <vector>
#include <cmath>
#include <map>

namespace deprecated {

class IPF
{
public:

  typedef std::map<int, std::vector<double>> Map;
	// construct from fractional marginals
  IPF(const NDArray<double>& seed, const std::vector<std::vector<double>>& marginals);

  // construct from integer marginals
  IPF(const NDArray<double>& seed, const std::vector<std::vector<int>>& marginals);

  //IPF(const IPF&) = delete;
//  IPF(IPF&&) = delete;

  //IPF& operator=(const IPF&) = delete;
  //IPF& operator=(IPF&&) = delete;

  virtual ~IPF() { }

  //void solve(const NDArray<double>& seed);
  Map solve(const NDArray<double>& seed);

  virtual size_t population() const;

  const NDArray<double>& result() const;

  const std::vector<std::vector<double>> errors() const;

  double maxError() const;

  virtual bool conv() const;

  virtual size_t iters() const;

protected:

  bool computeErrors(std::vector<std::vector<double>>& diffs);

  static const size_t s_MAXITER = 1000;

  NDArray<double> m_result;
  std::vector<std::vector<double>> m_marginals;
  std::vector<std::vector<double>> m_errors;

  size_t m_population;
  size_t m_iters;
  bool m_conv;
  const double m_tol;
  double m_maxError;
};


}



