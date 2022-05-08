#ifndef STAN_MATH_REV_FUN_INV_PHI_LOG_HPP
#define STAN_MATH_REV_FUN_INV_PHI_LOG_HPP

#include <stan/math/rev/meta.hpp>
#include <stan/math/rev/core.hpp>
#include <stan/math/prim/fun/constants.hpp>
#include <stan/math/prim/fun/inv_Phi_log.hpp>
#include <cmath>

namespace stan {
namespace math {

/**
 * The inverse of unit normal cumulative density function.
 *
 * The derivative is the reciprocal of unit normal density function,
 *
 * @param p Probability
 * @return The unit normal inverse cdf evaluated at p
 */
inline var inv_Phi_log(const var& log_p) {
  return make_callback_var(inv_Phi_log(log_p.val()), [log_p](auto& vi) mutable {
    log_p.adj() += vi.adj() * SQRT_TWO_PI  / std::exp(-0.5 * vi.val() * vi.val());
  });
}

/**
 * Return the elementwise inverse of unit normal cumulative density function.
 *
 * @tparam T a `var_value` with inner Eigen type
 * @param p Probability vector
 * @return Elementwise unit normal inverse cdf
 */
template <typename T, require_var_matrix_t<T>* = nullptr>
inline auto inv_Phi_log(const T& log_p) {
  return make_callback_var(inv_Phi_log(log_p.val()), [log_p](auto& vi) mutable {
    log_p.adj().array() += vi.adj().array() * SQRT_TWO_PI
                       / (-0.5 * vi.val().array().square()).exp() ;
  });
}

}  // namespace math
}  // namespace stan
#endif
