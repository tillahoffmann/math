#ifndef STAN_MATH__PRIM_META_PARTIALS_TYPE_HPP
#define STAN_MATH__PRIM_META_PARTIALS_TYPE_HPP

namespace stan {

template <typename T>
struct partials_type {
  typedef T type;
};

}  // namespace stan
#endif
