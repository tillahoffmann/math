#ifndef STAN_MATH_PRIM_FUN_FFT_HPP
#define STAN_MATH_PRIM_FUN_FFT_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/fun/Eigen.hpp>
#include <unsupported/Eigen/FFT>
#include <Eigen/Dense>
#include <complex>
#include <type_traits>
#include <vector>

namespace stan {
namespace math {

  /**
   * Return the discrete Fourier transform of the specified complex
   * vector.  The input vector may be considered to be in the time
   * domain and the output will be in the frequency domain. 
   *
   * Given an input complex vector `x[0:N-1]` of size `N`, the discrete
   * Fourier transform computes entries of the resulting complex
   * vector `y[0:N-1]` by 
   *
   * ```
   * y[n] = SUM_{i < N} x[i] * exp(n * i * -2 * pi * sqrt(-1) / N)
   * ```
   * 
   * If `y` is of size zero, the result is a size zero vector.
   *
   * @tparam V type of complex vector argument
   * @param x complex time domain vector to transform
   * @return discrete Fourier transform of `x`
   */
  template <typename V, require_eigen_vector_vt<is_complex, V>* = nullptr>
  inline Eigen::Matrix<scalar_type_t<V>, -1, 1> fft(const V& x) {
    Eigen::Matrix<scalar_type_t<V>, -1, 1> xv = x;
    if (xv.size() <= 1) return xv;
    Eigen::FFT<typename scalar_type_t<V>::value_type> fft;
    return fft.fwd(xv);
  }


  /**
   * Return the inverse discrete Fourier transform of the specified
   * complex vector.  The input may be considered to be in the
   * frequency domain and the output will be in the time domain.
   *
   * Given an input complex vector `y[0:N-1]` of size `N`, the inverse
   * discrete Fourier transform computes entries of the resulting
   * complex vector `x[0:N-1]` by 
   *
   * ```
   * x[n] = SUM_{i < N} y[i] * exp(n * i * 2 * pi * sqrt(-1) / N)
   * ```
   *
   * The only difference between the discrete DFT and its inverse is
   * the sign of the exponent.
   *
   * If the input is size zero, the output will be size zero.
   *
   * @tparam V type of complex vector argument
   * @param y complex frequency domain vector to inverse transform
   * @return inverse discrete Fourier transform of `x`
   */
  template <typename V, require_eigen_vector_vt<is_complex, V>* = nullptr>
  inline Eigen::Matrix<scalar_type_t<V>, -1, 1> inv_fft(const V& y) {
    Eigen::Matrix<scalar_type_t<V>, -1, 1> yv = y;
    if (y.size() <= 1) return yv;
    Eigen::FFT<typename scalar_type_t<V>::value_type> fft;
    return fft.inv(yv);
  }

  /**
   * Return the two-dimensional discrete Fourier transform of the
   * specified complex matrix.  The 2D discrete Fourier transform
   * first runs the DFT on the each row, then on each column of the
   * result.
   * 
   * @tparam M type of complex matrix argument
   * @param x complex time-domain matrix 
   * @return discrete 2D Fourier transform of `x`
   */
  template <typename M, require_eigen_vt<is_complex, M>* = nullptr>
  inline Eigen::Matrix<scalar_type_t<M>, -1, -1> fft2(const M& x) {
    Eigen::Matrix<scalar_type_t<M>, -1, -1> y(x.rows(), x.cols());
    for (int i = 0; i < y.rows(); ++i)
      y.row(i) = fft(x.row(i));
    for (int j = 0; j < y.cols(); ++j)
      y.col(j) = fft(y.col(j).eval());
    return y;
  }

  /**
   * Return the two-dimensional inverse discrete Fourier transform of
   * the specified complex matrix.  The 2D inverse discrete Fourier
   * transform first runs the 1D inverse Fourier transform on the
   * columns, and then on the resulting rows.  The composition of the
   * FFT and inverse FFT (or vice-versa) is the identity.
   * 
   * @tparam M type of complex matrix argument
   * @param x complex frequency-domain matrix 
   * @return inverse discrete 2D Fourier transform of `x`
   */
  template <typename M, require_eigen_vt<is_complex, M>* = nullptr>
  inline Eigen::Matrix<scalar_type_t<M>, -1, -1> inv_fft2(const M& y) {
    Eigen::Matrix<scalar_type_t<M>, -1, -1> x(y.rows(), y.cols());
    for (int j = 0; j < x.cols(); ++j)
      x.col(j) = inv_fft(y.col(j));
    for (int i = 0; i < x.rows(); ++i)
      x.row(i) = inv_fft(x.row(i));
    return y;
  }

  
}  // namespace math
}  // namespace stan

#endif
