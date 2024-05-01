#ifndef STAN_MATH_PRIM_FUN_SCALAR_SEQ_VIEW_HPP
#define STAN_MATH_PRIM_FUN_SCALAR_SEQ_VIEW_HPP

#include <stan/math/prim/meta.hpp>
#include <stan/math/prim/fun/num_elements.hpp>
#include <stan/math/prim/fun/sequential_index.hpp>

namespace stan {
namespace internal {
template <typename T>
using require_nested_t = require_t<
  math::disjunction<math::is_tuple<T>,
                    math::conjunction<is_std_vector<T>,
                                      is_container<value_type_t<T>>>>>;
}
/**
 * scalar_seq_view provides a uniform sequence-like wrapper around either a
 * scalar or a sequence of scalars.
 *
 * @tparam C the container type; will be the scalar type if wrapping a scalar
 * @tparam T the scalar type
 */
template <typename C, typename = void>
class scalar_seq_view;

template <typename C>
class scalar_seq_view<C, require_eigen_t<C>> {
 public:
  template <typename T,
            typename = require_same_t<plain_type_t<T>, plain_type_t<C>>>
  explicit scalar_seq_view(T&& c) : c_(std::forward<T>(c)) {}

  /**
   * Segfaults if out of bounds.
   * @param i index
   * @return the element at the specified position in the container
   */
  inline auto operator[](size_t i) const { return c_.coeff(i); }
  inline auto& operator[](size_t i) { return c_.coeffRef(i); }

  inline auto size() const noexcept { return c_.size(); }

  inline const value_type_t<C>* data() const noexcept { return c_.data(); }
  inline value_type_t<C>* data() noexcept { return c_.data(); }

  template <typename T = C, require_st_arithmetic<T>* = nullptr>
  inline decltype(auto) val(size_t i) const {
    return c_.coeffRef(i);
  }

  template <typename T = C, require_st_autodiff<T>* = nullptr>
  inline decltype(auto) val(size_t i) const {
    return c_.coeffRef(i).val();
  }

 private:
  plain_type_t<C> c_;
};

template <typename C>
class scalar_seq_view<C, require_var_matrix_t<C>> {
 public:
  template <typename T,
            typename = require_same_t<plain_type_t<T>, plain_type_t<C>>>
  explicit scalar_seq_view(T&& c) : c_(std::forward<T>(c)) {}

  /**
   * Segfaults if out of bounds.
   * @param i index
   * @return the element at the specified position in the container
   */
  inline auto operator[](size_t i) const { return c_.coeff(i); }
  inline const auto* data() const noexcept { return c_.vi_; }
  inline auto* data() noexcept { return c_.vi_; }

  inline auto size() const noexcept { return c_.size(); }

  template <typename T = C, require_st_autodiff<T>* = nullptr>
  inline auto val(size_t i) const {
    return c_.val().coeff(i);
  }

  template <typename T = C, require_st_autodiff<T>* = nullptr>
  inline auto& val(size_t i) {
    return c_.val().coeffRef(i);
  }

 private:
  std::decay_t<C> c_;
};

template <typename C>
class scalar_seq_view<C, require_std_vector_vt<is_stan_scalar, C>> {
 public:
  template <typename T,
            typename = require_same_t<plain_type_t<T>, plain_type_t<C>>>
  explicit scalar_seq_view(T&& c) : c_(std::forward<T>(c)) {}

  /**
   * Segfaults if out of bounds.
   * @param i index
   * @return the element at the specified position in the container
   */
  inline auto operator[](size_t i) const { return c_[i]; }
  inline auto& operator[](size_t i) { return c_[i]; }
  inline auto size() const noexcept { return c_.size(); }
  inline const auto* data() const noexcept { return c_.data(); }

  template <typename T = C, require_st_arithmetic<T>* = nullptr>
  inline decltype(auto) val(size_t i) const {
    return c_[i];
  }

  template <typename T = C, require_st_autodiff<T>* = nullptr>
  inline decltype(auto) val(size_t i) const {
    return c_[i].val();
  }

 private:
  std::decay_t<C> c_;
};

template <typename C>
class scalar_seq_view<C, internal::require_nested_t<C>> {
 public:
  template <typename T>
  explicit scalar_seq_view(T&& c)
      : c_(std::forward<T>(c)), size_(math::num_elements(c_)) {}

  inline auto size() const noexcept { return size_; }

  inline auto operator[](size_t i) const {
    return math::sequential_index(i, std::forward<decltype(c_)>(c_));
  }

  inline auto& operator[](size_t i) {
    return math::sequential_index(i, std::forward<decltype(c_)>(c_));
  }

  inline const auto* data() const noexcept { return c_.data(); }

  template <typename T = C, require_st_arithmetic<T>* = nullptr>
  inline decltype(auto) val(size_t i) const {
    return this[i];
  }

  template <typename T = C, require_st_autodiff<T>* = nullptr>
  inline decltype(auto) val(size_t i) const {
    return this[i].val();
  }

 private:
  std::decay_t<C> c_;
  size_t size_;
};

template <typename C>
class scalar_seq_view<C, require_t<std::is_pointer<C>>> {
 public:
  template <typename T,
            typename = require_same_t<plain_type_t<T>, plain_type_t<C>>>
  explicit scalar_seq_view(const T& c) : c_(c) {}

  /**
   * Segfaults if out of bounds.
   * @param i index
   * @return the element at the specified position in the container
   */
  inline auto operator[](size_t i) const { return c_[i]; }
  inline auto size() const noexcept {
    static_assert(1, "Cannot Return Size of scalar_seq_view with pointer type");
  }
  inline const auto* data() const noexcept { return &c_[0]; }

  template <typename T = C, require_st_arithmetic<T>* = nullptr>
  inline decltype(auto) val(size_t i) const {
    return c_[i];
  }

  template <typename T = C, require_st_autodiff<T>* = nullptr>
  inline decltype(auto) val(size_t i) const {
    return c_[i].val();
  }

 private:
  const C& c_;
};

/**
 * This specialization handles wrapping a scalar as if it were a sequence.
 *
 * @tparam T the scalar type
 */
template <typename C>
class scalar_seq_view<C, require_stan_scalar_t<C>> {
 public:
  explicit scalar_seq_view(const C& t) noexcept : t_(t) {}

  inline auto operator[](size_t /* i */) const { return t_; }
  inline auto& operator[](size_t /* i */) { return t_; }

  template <typename T = C, require_st_arithmetic<T>* = nullptr>
  inline decltype(auto) val(size_t /* i */) const noexcept {
    return t_;
  }

  template <typename T = C, require_st_autodiff<T>* = nullptr>
  inline decltype(auto) val(size_t /* i */) const noexcept {
    return t_.val();
  }

  static constexpr auto size() { return 1; }
  inline const auto* data() const noexcept { return &t_; }
  inline auto* data() noexcept { return &t_; }

 private:
  std::decay_t<C> t_;
};
}  // namespace stan
#endif
