#include <stan/math/rev.hpp>
#include <test/unit/math/rev/util.hpp>
#include <gtest/gtest.h>
#include <cmath>
#include <limits>

TEST(MathFunctions, a) {
  stan::math::var a = 1;
  a.vi_->adj_ = 5.0;

  auto val_func = [&](auto&& x1, auto&& x2) { return x1 + x2; };
  auto grad_func1 = [&](auto&& x1, auto&& x2) { return x1 - x2; };
  auto grad_func2 = [&](auto&& x1, auto&& x2) { return x2 - x1; };
  stan::math::var res = stan::math::user_gradients(std::make_tuple(a,a),val_func,std::make_tuple(grad_func1,grad_func2));
  res.grad();
  EXPECT_FLOAT_EQ(a.adj(), 20.0);
}
