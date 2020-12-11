#ifndef STAN_MATH_REV_FUN_READ_CORR_L_HPP
#define STAN_MATH_REV_FUN_READ_CORR_L_HPP

#include <stan/math/prim/fun/read_corr_L.hpp>
#include <stan/math/rev/meta.hpp>
#include <stan/math/rev/core.hpp>
#include <stan/math/rev/core/operator_multiplication.hpp>
#include <cmath>
#include <complex>

namespace stan {
namespace math {

/**
 * Return the Cholesky factor of the correlation matrix of the
 * specified dimensionality corresponding to the specified
 * canonical partial correlations.
 *
 * It is generally better to work with the Cholesky factor rather
 * than the correlation matrix itself when the determinant,
 * inverse, etc. of the correlation matrix is needed for some
 * statistical calculation.
 *
 * <p>See <code>read_corr_matrix(Array, size_t, T)</code>
 * for more information.
 *
 * @tparam T type of input (must be a `var_value<T>` where `T`
 *  inherits from EigenBase)
 * @param CPCs The (K choose 2) canonical partial correlations in
 * (-1, 1).
 * @param K Dimensionality of correlation matrix.
 * @return Cholesky factor of correlation matrix for specified
 * canonical partial correlations.
 */
template <typename T, require_var_vector_t<T>* = nullptr>
auto read_corr_L(const T& CPCs, size_t K) {  // on (-1, 1)
  using ret_type = var_value<Eigen::MatrixXd>;

  if (K == 0) {
    return ret_type(Eigen::MatrixXd());
  }
  if (K == 1) {
    return ret_type(Eigen::MatrixXd::Identity(1, 1));
  }

  using std::sqrt;
  Eigen::ArrayXd acc(K - 1);
  acc.setOnes();
  // Cholesky factor of correlation matrix
  Eigen::MatrixXd L = Eigen::MatrixXd::Zero(K, K);

  size_t position = 0;
  size_t pull = K - 1;

  L(0, 0) = 1.0;
  L.col(0).tail(pull) = CPCs.val().head(pull);
  acc.tail(pull) = 1.0 - CPCs.val().head(pull).array().square();
  for (size_t i = 1; i < (K - 1); i++) {
    position += pull;
    pull = K - 1 - i;

    const auto& cpc_seg = CPCs.val().array().segment(position, pull);
    L(i, i) = sqrt(acc(i - 1));
    L.col(i).tail(pull) = cpc_seg * acc.tail(pull).sqrt();
    acc.tail(pull) = acc.tail(pull) * (1.0 - cpc_seg.square());
  }

  L(K - 1, K - 1) = sqrt(acc(K - 2));

  ret_type L_res = L;
  auto arena_acc = to_arena(acc);

  reverse_pass_callback([CPCs, arena_acc, K, L_res]() mutable {
    Eigen::ArrayXd acc_val = arena_acc;
    Eigen::ArrayXd acc_adj = Eigen::ArrayXd::Zero(K - 1);

    acc_adj(K - 2)
        += 0.5 * L_res.adj()(K - 1, K - 1) / L_res.val()(K - 1, K - 1);

    int position = CPCs.size() - 1;
    for (size_t i = K - 2; i > 0; --i) {
      int pull = K - 1 - i;

      const auto& cpc_seg_val = CPCs.val().array().segment(position, pull);
      auto cpc_seg_adj = CPCs.adj().array().segment(position, pull);
      acc_val.tail(pull) /= 1.0 - cpc_seg_val.square();
      cpc_seg_adj
          -= 2.0 * acc_adj.tail(pull) * acc_val.tail(pull) * cpc_seg_val;
      acc_adj.tail(pull)
          = (acc_adj.tail(pull).array() * (1.0 - cpc_seg_val.square())).eval();
      cpc_seg_adj
          += L_res.adj().array().col(i).tail(pull) * acc_val.tail(pull).sqrt();
      acc_adj.tail(pull) += 0.5 * L_res.adj().array().col(i).tail(pull)
                            * cpc_seg_val / acc_val.tail(pull).sqrt();
      acc_adj(i - 1) += 0.5 * L_res.adj()(i, i) / L_res.val()(i, i);

      position -= pull + 1;
    }

    int pull = K - 1;
    CPCs.adj().array().head(pull)
        -= 2.0 * acc_adj.tail(pull) * CPCs.val().array().head(pull);
    CPCs.adj().head(pull) += L_res.adj().col(0).tail(pull);
  });

  return L_res;
}

/**
 * Return the Cholesky factor of the correlation matrix of the
 * specified dimensionality corresponding to the specified
 * canonical partial correlations, incrementing the specified
 * scalar reference with the log absolute determinant of the
 * Jacobian of the transformation.
 *
 * <p>The implementation is Ben Goodrich's Cholesky
 * factor-based approach to the C-vine method of:
 *
 * <ul><li> Daniel Lewandowski, Dorota Kurowicka, and Harry Joe,
 * Generating random correlation matrices based on vines and
 * extended onion method Journal of Multivariate Analysis 100
 * (2009) 1989–2001 </li></ul>
 *
 * @tparam T type of input (must be a `var_value<T>` where `T`
 *  inherits from EigenBase)
 * @param CPCs The (K choose 2) canonical partial correlations in
 * (-1, 1).
 * @param K Dimensionality of correlation matrix.
 * @param log_prob Reference to variable to increment with the log
 * Jacobian determinant.
 * @return Cholesky factor of correlation matrix for specified
 * partial correlations.
 */
template <typename T1, typename T2, require_var_vector_t<T1>* = nullptr,
          require_stan_scalar_t<T2>* = nullptr>
auto read_corr_L(const T1& CPCs, size_t K, T2& log_prob) {
  using ret_val_type = Eigen::MatrixXd;
  using ret_type = var_value<Eigen::MatrixXd>;

  if (K == 0) {
    return ret_type(ret_val_type());
  }
  if (K == 1) {
    return ret_type(Eigen::MatrixXd::Identity(1, 1));
  }

  size_t pos = 0;
  double acc_val = 0;
  // no need to abs() because this Jacobian determinant
  // is strictly positive (and triangular)
  // see inverse of Jacobian in equation 11 of LKJ paper
  for (size_t k = 1; k <= (K - 2); k++) {
    for (size_t i = k + 1; i <= K; i++) {
      acc_val += (K - k - 1) * log1m(square(CPCs.val()(pos)));
      pos++;
    }
  }

  log_prob += 0.5 * acc_val;

  reverse_pass_callback([K, CPCs, log_prob]() mutable {
    double acc_adj = log_prob.adj() * 0.5;

    size_t pos = CPCs.size() - 2;
    for (size_t k = K - 2; k >= 1; --k) {
      for (size_t i = K; i >= k + 1; --i) {
        CPCs.adj()(pos) -= 2.0 * acc_adj * (K - k - 1) * CPCs.val()(pos)
                           / (1.0 - square(CPCs.val()(pos)));
        pos--;
      }
    }
  });

  return read_corr_L(CPCs, K);
}

}  // namespace math
}  // namespace stan
#endif
