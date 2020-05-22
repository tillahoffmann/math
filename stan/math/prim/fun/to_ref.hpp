#ifndef STAN_MATH_PRIM_FUN_TO_REF_HPP
#define STAN_MATH_PRIM_FUN_TO_REF_HPP

#include <stan/math/prim/meta.hpp>

namespace stan {
namespace math {

/**
 * This evaluates expensive Eigen expressions. If given expression involves no
 * calculations this is a no-op that should be optimized away.
 * @tparam T argument type
 * @param a argument
 * @return optionally evaluated argument
 */
template <typename T>
inline ref_type_t<T&&> to_ref(T&& a) {
  return std::forward<T>(a);
}

/**
 * No-op that should be optimized away.
 * @tparam Cond condition
 * @tparam T argument type
 * @param a argument
 * @return argument
 */
template <bool Cond, typename T,
          require_not_t<conjunction<is_eigen<T>, is_var<value_type_t<T>>,
                                    bool_constant<Cond>>>* = nullptr>
inline T to_ref_if(T&& a) {
  return std::forward<T>(a);
}

/**
 * If the condition is true, converts Eigen argument into `Eigen::Ref`. This
 * evaluates expensive expressions.
 * @tparam Cond condition
 * @tparam T argument type (Eigen expression)
 * @param a argument
 * @return argument converted to `Eigen::Ref`
 */
template <bool Cond, typename T, require_eigen_vt<is_var, T>* = nullptr,
          std::enable_if_t<Cond>* = nullptr>
inline ref_type_t<T&&> to_ref_if(T&& a) {
  return std::forward<T>(a);
}

}  // namespace math
}  // namespace stan
#endif
