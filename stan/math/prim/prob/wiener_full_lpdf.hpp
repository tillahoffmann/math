#ifndef STAN_MATH_PRIM_PROB_WIENER_FULL_LPDF_HPP
#define STAN_MATH_PRIM_PROB_WIENER_FULL_LPDF_HPP

#include <stan/math/prim/fun.hpp>
#include <stan/math/prim/functor/hcubature.hpp>
#include <stan/math/prim/prob/wiener5_lpdf.hpp>

namespace stan {
namespace math {
namespace internal {

/**
 * Calculate the derivative of the wiener7 density w.r.t. 'sw'
 *
 * @param y A scalar variable; the reaction time in seconds
 * @param a The boundary separation
 * @param v The drift rate
 * @param w The relative starting point
 * @param sv The inter-trial variability of the drift rate
 * @param sw The inter-trial variability of the relative starting point
 * @param log_error The log error tolerance
 * @return Gradient w.r.t. sw
 */
inline double wiener7_grad_sw(double y, double a, double v, double w, double sv,
                              double sw, double log_error) {
  double low = w - sw / 2.0;
  low = (0 > low) ? 0 : low;
  double high = w + sw / 2.0;
  high = (1 < high) ? 1 : high;

  const double lower_value = wiener5_density<true>(y, a, v, low, sv, log_error);
  const double upper_value
      = wiener5_density<true>(y, a, v, high, sv, log_error);
  return 0.5 * (lower_value + upper_value) / sw;
}

/**
 * Helper function for agnostically calling wiener5 functions
 * (to be integrated over) or directly calling wiener7 functions,
 * accounting for the different number of arguments.
 *
 * Specialisation for wiener5 functions
 *
 * @tparam GradSW Whether the wiener7 gradient function is passed
 * @tparam F Type of Gradient/density functor
 *
 * @param functor Gradient/density functor to apply
 * @param y A scalar variable; the reaction time in seconds
 * @param a The boundary separation
 * @param v The drift rate
 * @param w The relative starting point
 * @param t0 The non-decision time
 * @param sv The inter-trial variability of the drift rate
 * @param sw The inter-trial variability of the relative starting point
 * @param st0 The inter-trial variability of the non-decision time
 * @param log_error The log error tolerance
 * @return Functor applied to arguments
 */
template <bool GradSW, typename F, std::enable_if_t<!GradSW>* = nullptr>
inline double conditionally_grad_sw(const F& functor, double y_diff, double a,
                                    double v, double w, double sv, double sw,
                                    double log_error) {
  return functor(y_diff, a, v, w, sv, log_error);
}

/**
 * Helper function for agnostically calling wiener5 functions
 * (to be integrated over) or directly calling wiener7 functions,
 * accounting for the different number of arguments.
 *
 * Specialisation for wiener7 functions
 *
 * @tparam GradSW Whether the wiener7 gradient function is passed
 * @tparam F Type of Gradient/density functor
 *
 * @param functor Gradient/density functor to apply
 * @param y_diff A scalar variable; the reaction time minus the non-decision in
 * seconds
 * @param a The boundary separation
 * @param v The drift rate
 * @param w The relative starting point
 * @param t0 The non-decision time
 * @param sv The inter-trial variability of the drift rate
 * @param sw The inter-trial variability of the relative starting point
 * @param st0 The inter-trial variability of the non-decision time
 * @param log_error The log error tolerance
 * @return Functor applied to arguments
 */
template <bool GradSW, typename F, std::enable_if_t<GradSW>* = nullptr>
inline double conditionally_grad_sw(const F& functor, double y_diff, double a,
                                    double v, double w, double sv, double sw,
                                    double log_error) {
  return functor(y_diff, a, v, w, sv, sw, log_error);
}

/**
 * Implementation function for preparing arguments and functor to be passed
 * to the hcubature() function for calculating wiener7 parameters via
 * integration
 *
 * @tparam GradSW Whether the wiener7 gradient function is passed
 * @tparam Wiener7FunctorT Type of functor
 * @tparam Targs... Types of arguments in parameter pack
 *
 * @param wiener7_functor Gradient/density functor to apply
 * @param hcubature_err Error tolerance for calculation
 * @param args Additional arguments to be passed to the hcubature function
 * @return Wiener7 density or gradient calculated by integration
 */
template <bool GradSW, bool GradW7 = false, typename Wiener7FunctorT,
          typename... TArgs>
auto wiener7_integrate(const Wiener7FunctorT& wiener7_functor,
                       double hcubature_err, TArgs&&... args) {
  const auto wiener7_integrand_impl
      = [&](auto&& x, double y, double a, double v, double w, double t0,
            double sv, double sw, double st0, double log_error) {
          scalar_seq_view<decltype(x)> x_vec(x);
          const double sw_val = GradSW ? 0 : sw;
          const double new_w = sw_val ? w + sw_val * (x_vec[0] - 0.5) : w;
          const double new_t0 = sw_val ? (st0 ? t0 + st0 * x_vec[1] : t0)
                                       : (st0 ? t0 + st0 * x_vec[0] : t0);
          if (y - new_t0 <= 0) {
            return 0.0;
          } else {
            return conditionally_grad_sw<GradSW>(wiener7_functor, y - new_t0, a,
                                                 v, new_w, sv, sw, log_error);
          }
        };
  const auto functor = [&](auto&&... int_args) {
    return hcubature(wiener7_integrand_impl, int_args...);
  };
  return estimate_with_err_check<0, GradW7, 8>(functor, hcubature_err, args...);
}
}  // namespace internal

/** \ingroup prob_dists
 * The log of the first passage time density function for a (Wiener)
 * drift diffusion model with up to 7 parameters, where
 * \f$y\in \mathbb{R}_{+}\f$ is the reacion time, \f$a \in \mathbb{R}_{+}\f$
 * the boundary separation, \f$t_0 \in \mathbb{R}_{\geq 0}\f$ the non-decision
 time,
 * \f$w \in (0, 1)\f$ the relative starting point (aka a-priori bias),
 * \f$v \in \mathbb{R}\f$ the drifte rate, \f$s_v \in
 * \mathbb{R}_{\geq 0}\f$ the inter-trial variability of the drift rate,
 * \f$s_w \in [0, 1)\f$ the inter-trial  variability of the relative starting
 * point, and  \f$s_{t_0} \in \mathbb{R}_{\geq 0}\f$ the inter-trial variability
 * of
 * the non-decision time.
 *
 *
 \f{eqnarray*}{
 y &\sim& \text{wiener_full}(a,t_0,w,v,s_v,s_w,s_{t_0}) \\
 \log(p(y|a,v,w,t_0,s_v,s_w,s_{t_0})) &=& \log(\frac{1}{s_{t_0}}
 \int_{t_0}^{t_o + s_{t_0}} \frac{1}{s_{w}}\int_{w -0.5s_w}^{w + 0.5s_{w}}
 \int_{-\infty}^{\infty} p_3(y-\tau_0|a,\nu,\omega) \\
 &&\times\frac{1}{\sqrt{2\pi s_\nu^2}}
 \mathbb{e}^{-\frac{(\nu-v)^2}{2s_\nu^2}} \ d\nu \ d\omega \ d\tau_0) \\
 &=& \log(\frac{1}{s_{t_0}}
 \int_{t_0}^{t_o + s_{t_0}} \frac{1}{s_{w}}\int_{w -0.5s_w}^{w + 0.5s_{w}}
 M \times p_3(y-\tau_0|a,v,\omega) \ d\omega \ d\tau_0),
 \f}
 * where \f$M\f$ and \f$p_3()\f$ are defined, by using \f$t:=y-\tau_0\f$, as
 \f{eqnarray*}{
 M &:=& \frac{1}{\sqrt{1+s^2_v t}}
 \mathbb{e}^{av\omega+\frac{v^2t}{2}+\frac{s^2_v a^2
 \omega^2-2av\omega-v^2t}{2(1+s^2_vt)}} \text{ and} \\ p_3(t|a,v,w) &:=&
 \frac{1}{a^2} \mathbb{e}^{-a v w -\frac{v^2t}{2}} f(\frac{t}{a^2}|0,1,w), \f}
 * where  \f$f(t^*=\frac{t}{a^2}|0,1,w)\f$ has two forms
 \f{eqnarray*}{
 f_l(t^*|0,1,w) &=& \sum_{k=1}^{\infty} k\pi \mathbb{e}^{-\frac{k^2\pi^2t^*}{2}}
 \sin{(k \pi w)}\text{ and} \\
 f_s(t^*|0,1,w) &=& \sum_{k=-\infty}^{\infty} \frac{1}{\sqrt{2\pi (t^*)^3}}
 (w+2k) \mathbb{e}^{-\frac{(w+2k)^2}{2t^*}}, \f}
 * which are selected depending on the number of components \f$k\f$, needed to
 * guarantee a certain precision.
 *
 * Note that the parameterization for non-decision time and relative starting
 point is as follows:
 * \c t0 is the lower bound of the variability interval;
 * \c w is the mean of the variability interval.
 *
 * See \b Details below for more details on how to use \c wiener_full_lpdf().
 *
 * @tparam T_y type of scalar
 * @tparam T_a type of boundary separation
 * @tparam T_t0 type of non-decision time
 * @tparam T_w type of relative starting point
 * @tparam T_v type of drift rate
 * @tparam T_sv type of inter-trial variability of drift rate
 * @tparam T_sw type of inter-trial variability of relative starting point
 * @tparam T_st0 type of inter-trial variability of non-decision time
 *
 * @param y A scalar variable; the reaction time in seconds
 * @param a The boundary separation
 * @param t0 The non-decision time
 * @param w The relative starting point
 * @param v The drift rate
 * @param sv The inter-trial variability of the drift rate
 * @param sw The inter-trial variability of the relative starting point
 * @param st0 The inter-trial variability of the non-decision time
 * @param precision_derivatives Level of precision in estimation of partial
 derivatives
 * @return The log of the Wiener first passage time density with
 *  the specified arguments for upper boundary responses
 * @throw std::domain_error if non-decision time \c t0 is greater than reaction
 time \c y.
 * @throw std::domain_error if \c 1-sw/2 is smaller than or equal to \c w.
 * @throw std::domain_error if \c sw/2 is larger than or equal to \c w.
 *
 *
 *
 * **Details**
 *
 * The function can be called by
 @code
 target += wiener_full_lpdf(y, a, t0, w, v, sv, sw, st0);
 @endcode
 * or
 @code
 y ~ wiener_full(a, t0, w, v, sv, sw, st0);
 @endcode
 *
 * By default \c wiener_full_lpdf() gives the log of the
 * Wiener first-passage time probability density function for the \e upper
 response
 * boundary. To use the \e lower response boundary \c v and \c w must be changed
 to
 * \c -v and \c (1-w), respectively.
 *
 * \c sv, \c sw, \c st0, and \c t0 can be set to zero, indicating no inter-trial
 * variability in \f$v\f$, no inter-trial variability in \f$w\f$, no
 * inter-trial variability in \f$t_0\f$, and no non-decision time, respectively.
 * If \c t0 is zero, \c st0 must be zero as well. For example, when no
 inter-trial
 * variability for the relative starting point is needed one can write something
 like:
 @code
 target += wiener_full_lpdf(y, a, t0, w, v, sv, 0, st0)
 @endcode
 * If no inter-trial variability is needed at all one can write something like:
 @code
 target += wiener_full_lpdf(y, a, t0, w, v, 0, 0, 0)
 @endcode
 * If for some reason no non-decision time is assumed one can write something
 like:
 @code
 target += wiener_full_lpdf(y, a, 0, w, v, sv, sw, 0)
 @endcode
 *
 * To also control the precision in the estimation of the partial derivatives:
 @code
 target += wiener_full_lpdf(y, a, t0, w, v, sv, sw, st0, precision);
 @endcode
 *
 *
 * **References**
 * - Blurton, S. P., Kesselmeier, M., & Gondan, M. (2017). The first-passage
 * time distribution for the diffusion model with variable drift.
 * *Journal of Mathematical Psychology, 76*, 7–12.
 * https://doi.org/10.1016/j.jmp.2016.11.003
 * - Foster, K., & Singmann, H. (2021). Another Approximation of the
 First-Passage
 * Time Densities for the Ratcliff Diffusion Decision Model.
 * *arXiv preprint arXiv:2104.01902*
 * - Gondan, M., Blurton, S. P., & Kesselmeier, M. (2014). Even faster and even
 * more accurate first-passage time densities and distributions for the Wiener
 * diffusion model. *Journal of Mathematical Psychology, 60*, 20–22.
 * https://doi.org/10.1016/j.jmp.2014.05.002
 * - Hartmann, R., & Klauer, K. C. (2021). Partial derivatives for the
 * first-passage time distribution in Wiener diffusion models.
 * *Journal of Mathematical Psychology, 103*, 102550.
 * https://doi.org/10.1016/j.jmp.2021.102550
 * - Henrich, F., Hartmann, R., Pratz, V., Voss, A., & Klauer, K.C. (in press).
 * The Seven-parameter Diffusion Model: An Implementation in Stan for Bayesian
 * Analyses. *Behavior Research Methods*.
 * - Navarro, D. J., & Fuss, I. G. (2009). Fast and accurate calculations for
 * first-passage times in Wiener diffusion models.
 * *Journal of Mathematical Psychology, 53*(4), 222–230.
 * https://doi.org/10.1016/j.jmp.2009.02.003
 */
template <bool propto = false, typename T_y, typename T_a, typename T_t0,
          typename T_w, typename T_v, typename T_sv, typename T_sw,
          typename T_st0,
          typename ReturnT
          = return_type_t<T_y, T_a, T_t0, T_w, T_v, T_sv, T_sw, T_st0>>
inline ReturnT wiener_full_lpdf(const T_y& y, const T_a& a, const T_t0& t0,
                                const T_w& w, const T_v& v, const T_sv& sv,
                                const T_sw& sw, const T_st0& st0,
                                double precision_derivatives = 1e-4) {
  if (!include_summand<propto, T_y, T_a, T_v, T_w, T_t0, T_sv, T_sw,
                       T_st0>::value) {
    return 0;
  }

  using T_y_ref = ref_type_t<T_y>;
  using T_a_ref = ref_type_t<T_a>;
  using T_v_ref = ref_type_t<T_v>;
  using T_w_ref = ref_type_t<T_w>;
  using T_t0_ref = ref_type_t<T_t0>;
  using T_sv_ref = ref_type_t<T_sv>;
  using T_sw_ref = ref_type_t<T_sw>;
  using T_st0_ref = ref_type_t<T_st0>;

  const char* function_name = "wiener_full_lpdf";
  check_consistent_sizes(function_name, "Random variable", y,
                         "Boundary separation", a, "Drift rate", v,
                         "A-priori bias", w, "Nondecision time", t0,
                         "Inter-trial variability in drift rate", sv,
                         "Inter-trial variability in A-priori bias", sw,
                         "Inter-trial variability in Nondecision time", st0);
  check_consistent_size(function_name, "Random variable", y, 1);
  check_consistent_size(function_name, "Boundary separation", a, 1);
  check_consistent_size(function_name, "Drift rate", v, 1);
  check_consistent_size(function_name, "A-priori bias", w, 1);
  check_consistent_size(function_name, "Nondecision time", t0, 1);
  check_consistent_size(function_name, "Inter-trial variability in drift rate",
                        sv, 1);
  check_consistent_size(function_name,
                        "Inter-trial variability in A-priori bias", sw, 1);
  check_consistent_size(function_name,
                        "Inter-trial variability in Nondecision time", st0, 1);

  T_y_ref y_ref = y;
  T_a_ref a_ref = a;
  T_v_ref v_ref = v;
  T_w_ref w_ref = w;
  T_t0_ref t0_ref = t0;
  T_sv_ref sv_ref = sv;
  T_sw_ref sw_ref = sw;
  T_st0_ref st0_ref = st0;

  check_positive_finite(function_name, "Random variable", value_of(y_ref));
  check_positive_finite(function_name, "Boundary separation", value_of(a_ref));
  check_finite(function_name, "Drift rate", value_of(v_ref));
  check_less(function_name, "A-priori bias", value_of(w_ref), 1);
  check_greater(function_name, "A-priori bias", value_of(w_ref), 0);
  check_nonnegative(function_name, "Nondecision time", value_of(t0_ref));
  check_finite(function_name, "Nondecision time", value_of(t0_ref));
  check_nonnegative(function_name, "Inter-trial variability in drift rate",
                    value_of(sv_ref));
  check_finite(function_name, "Inter-trial variability in drift rate",
               value_of(sv_ref));
  check_bounded(function_name, "Inter-trial variability in A-priori bias",
                value_of(sw_ref), 0, 1);
  check_nonnegative(function_name,
                    "Inter-trial variability in Nondecision time",
                    value_of(st0_ref));
  check_finite(function_name, "Inter-trial variability in Nondecision time",
               value_of(st0_ref));

  if (size_zero(y, a, v, w, t0) || size_zero(sv, sw, st0)) {
    return 0;
  }
  const size_t N = max_size(y, a, v, w, t0, sv, sw, st0);
  if (!N) {
    return 0;
  }
  scalar_seq_view<T_y_ref> y_vec(y_ref);
  scalar_seq_view<T_a_ref> a_vec(a_ref);
  scalar_seq_view<T_v_ref> v_vec(v_ref);
  scalar_seq_view<T_w_ref> w_vec(w_ref);
  scalar_seq_view<T_t0_ref> t0_vec(t0_ref);
  scalar_seq_view<T_sv_ref> sv_vec(sv_ref);
  scalar_seq_view<T_sw_ref> sw_vec(sw_ref);
  scalar_seq_view<T_st0_ref> st0_vec(st0_ref);
  const size_t N_y_t0 = max_size(y, t0, st0);

  for (size_t i = 0; i < N_y_t0; ++i) {
    if (y_vec[i] <= t0_vec[i]) {
      std::stringstream msg;
      msg << ", but must be greater than nondecision time = " << t0_vec[i];
      std::string msg_str(msg.str());
      throw_domain_error(function_name, "Random variable", y_vec[i], " = ",
                         msg_str.c_str());
    }
  }
  size_t N_beta_sw = max_size(w, sw);
  for (size_t i = 0; i < N_beta_sw; ++i) {
    if (unlikely(w_vec[i] - .5 * sw_vec[i] <= 0)) {
      std::stringstream msg;
      msg << ", but must be smaller than 2*(A-priori bias) = " << 2 * w_vec[i];
      std::string msg_str(msg.str());
      throw_domain_error(function_name,
                         "Inter-trial variability in A-priori bias", sw_vec[i],
                         " = ", msg_str.c_str());
    }
    if (unlikely(w_vec[i] + .5 * sw_vec[i] >= 1)) {
      std::stringstream msg;
      msg << ", but must be smaller than 2*(1-A-priori bias) = "
          << 2 * (1 - w_vec[i]);
      std::string msg_str(msg.str());
      throw_domain_error(function_name,
                         "Inter-trial variability in A-priori bias", sw_vec[i],
                         " = ", msg_str.c_str());
    }
  }

  const double log_error_density = log(1e-6);        // precision for density
  const double error_bound = precision_derivatives;  // precision for
  // derivatives (controllable by user)
  const double log_error_derivative = log(error_bound);
  const double absolute_error_hcubature = 0.0;
  const double relative_error_hcubature
      = .9 * error_bound;  // eps_rel(Integration)
  const double log_error_absolute = log(1e-12);
  const int maximal_evaluations_hcubature = 6000;
  double log_density = 0.0;
  auto ops_partials = make_partials_propagator(y_ref, a_ref, t0_ref, w_ref,
                                               v_ref, sv_ref, sw_ref, st0_ref);
  ReturnT result = 0;

  // calculate density and partials
  for (size_t i = 0; i < N; i++) {
    if (sw_vec[i] == 0 && st0_vec[i] == 0) {
      result
          += wiener5_lpdf<propto>(y_vec[i], a_vec[i], t0_vec[i], w_vec[i],
                                  v_vec[i], sv_vec[i], precision_derivatives);
      continue;
    }
    const double y_val = y_vec.val(i);
    const double a_val = a_vec.val(i);
    const double v_val = v_vec.val(i);
    const double w_val = w_vec.val(i);
    const double t0_val = t0_vec.val(i);
    const double sv_val = sv_vec.val(i);
    const double sw_val = sw_vec.val(i);
    const double st0_val = st0_vec.val(i);
    const int dim = (sw_val != 0) + (st0_val != 0);
    check_positive(function_name,
                   "(Inter-trial variability in A-priori bias) + "
                   "(Inter-trial variability in nondecision time)",
                   dim);

    Eigen::VectorXd xmin = Eigen::VectorXd::Zero(dim);
    Eigen::VectorXd xmax = Eigen::VectorXd::Ones(dim);
    if (st0_val) {
      xmax[dim - 1] = fmin(1.0, (y_val - t0_val) / st0_val);
    }

    double hcubature_err = log_error_absolute - log_error_density + LOG_TWO + 1;
    const auto params
        = std::make_tuple(y_val, a_val, v_val, w_val, t0_val, sv_val, sw_val,
                          st0_val, log_error_absolute - LOG_TWO);
    double density = internal::wiener7_integrate<false>(
        [&](auto&&... args) {
          return internal::wiener5_density<true>(args...);
        },
        hcubature_err, params, dim, xmin, xmax, maximal_evaluations_hcubature,
        absolute_error_hcubature, relative_error_hcubature / 2);
    log_density += log(density);

    hcubature_err = log_error_absolute - log_error_derivative
                    + log(fabs(density)) + LOG_TWO + 1;

    // computation of derivative for t and precision check in order to give
    // the value as deriv_t to edge1 and as -deriv_t to edge5
    const double deriv_t_7
        = internal::wiener7_integrate<false>(
              [&](auto&&... args) {
                return internal::wiener5_grad_t<true>(args...);
              },
              hcubature_err, params, dim, xmin, xmax,
              maximal_evaluations_hcubature, absolute_error_hcubature,
              relative_error_hcubature / 2)
          / density;

    // computation of derivatives and precision checks
    double derivative;
    if (!is_constant_all<T_y>::value) {
      partials<0>(ops_partials)[i] = deriv_t_7;
    }
    if (!is_constant_all<T_a>::value) {
      partials<1>(ops_partials)[i]
          = internal::wiener7_integrate<false>(
                [&](auto&&... args) {
                  return internal::wiener5_grad_a<true>(args...);
                },
                hcubature_err, params, dim, xmin, xmax,
                maximal_evaluations_hcubature, absolute_error_hcubature,
                relative_error_hcubature / 2)
            / density;
    }
    if (!is_constant_all<T_t0>::value) {
      partials<2>(ops_partials)[i] = -deriv_t_7;
    }
    if (!is_constant_all<T_w>::value) {
      partials<3>(ops_partials)[i]
          = internal::wiener7_integrate<false, true>(
                [&](auto&&... args) {
                  return internal::wiener5_grad_w<true>(args...);
                },
                hcubature_err, params, dim, xmin, xmax,
                maximal_evaluations_hcubature, absolute_error_hcubature,
                relative_error_hcubature / 2)
            / density;
    }
    if (!is_constant_all<T_v>::value) {
      partials<4>(ops_partials)[i]
          = internal::wiener7_integrate<false>(
                [&](auto&&... args) {
                  return internal::wiener5_grad_v<true>(args...);
                },
                hcubature_err, params, dim, xmin, xmax,
                maximal_evaluations_hcubature, absolute_error_hcubature,
                relative_error_hcubature / 2)
            / density;
    }
    if (!is_constant_all<T_sv>::value) {
      partials<5>(ops_partials)[i]
          = internal::wiener7_integrate<false>(
                [&](auto&&... args) {
                  return internal::wiener5_grad_sv<true>(args...);
                },
                hcubature_err, params, dim, xmin, xmax,
                maximal_evaluations_hcubature, absolute_error_hcubature,
                relative_error_hcubature / 2)
            / density;
    }
    if (!is_constant_all<T_sw>::value) {
      if (sw_val == 0) {
        partials<6>(ops_partials)[i] = 0;
      } else {
        if (st0_val == 0) {
          derivative = internal::estimate_with_err_check<6>(
              [&](auto&&... args) {
                return internal::wiener7_grad_sw(args...);
              },
              hcubature_err, y_val - t0_val, a_val, v_val, w_val, sv_val,
              sw_val, log_error_absolute - LOG_TWO);
        } else {
          derivative = internal::wiener7_integrate<true>(
              [&](auto&&... args) {
                return internal::wiener7_grad_sw(args...);
              },
              hcubature_err, params, 1, xmin, xmax,
              maximal_evaluations_hcubature, absolute_error_hcubature,
              relative_error_hcubature / 2);
        }
        partials<6>(ops_partials)[i] = derivative / density - 1 / sw_val;
      }
    }
    if (!is_constant_all<T_st0>::value) {
      double f;
      if (st0_val == 0) {
        partials<7>(ops_partials)[i] = 0;
      } else if (y_val - (t0_val + st0_val) <= 0) {
        partials<7>(ops_partials)[i] = -1 / st0_val;
      } else {
        const double t0_st0 = t0_val + st0_val;
        if (sw_val == 0) {
          f = internal::estimate_with_err_check<5>(
              [&](auto&&... args) {
                return internal::wiener5_density<true>(args...);
              },
              log_error_derivative + log(st0_val), y_val - t0_st0, a_val, v_val,
              w_val, sv_val, log_error_absolute - LOG_TWO);
        } else {
          const double new_error = log_error_absolute - LOG_TWO;
          const auto& params_st = std::make_tuple(
              y_val, a_val, v_val, w_val, t0_st0, sv_val, sw_val, 0, new_error);
          f = internal::wiener7_integrate<false>(
              [&](auto&&... args) {
                return internal::wiener5_density<true>(args...);
              },
              hcubature_err, params_st, 1, xmin, xmax,
              maximal_evaluations_hcubature, absolute_error_hcubature,
              relative_error_hcubature / 2);
        }
        partials<7>(ops_partials)[i] = -1 / st0_val + f / st0_val / density;
      }
    }
  }
  return result + ops_partials.build(log_density);
}
}  // namespace math
}  // namespace stan
#endif
