#include "number_comparison.h"
#include <initializer_list>
#include <cstdlib>
#include <cmath>
#include <limits>
#include <cfloat>
#include <cstdio>
#include <random>

uint32_t ulp_dist(float a, float b) {
  const auto a_to_b=number_tests::f32_ulp_dist(a,b);
  const auto b_to_a=number_tests::f32_ulp_dist(b,a);
  if(a_to_b!=b_to_a) {
    std::abort();
  }
  return a_to_b;
}

uint64_t ulp_dist(double a, double b) {
  const auto a_to_b=number_tests::f64_ulp_dist(a,b);
  const auto b_to_a=number_tests::f64_ulp_dist(b,a);
  if(a_to_b!=b_to_a) {
    std::abort();
  }
  return a_to_b;
}

template<class Real>
void test_single_number(const Real y) {
  //get the preceding and following numbers (order is x<y<z)
  const Real x=std::nextafter(y,-std::numeric_limits<Real>::infinity());
  const Real z=std::nextafter(y,+std::numeric_limits<Real>::infinity());

  auto compare=[](const Real a, const Real b, const unsigned int expected_ulp) {
    // only compare if the numbers have the same exponent and sign,
    // otherwise we need to decide to measure |a-b| in ULP(a) or ULP(b)
    int a_exp,b_exp;
    const Real a_frac=std::frexp(a,&a_exp);
    const Real b_frac=std::frexp(b,&b_exp);
    if(a_exp!=b_exp)
      return ;
    if(std::signbit(a_frac)!=std::signbit(b_frac))
      return ;
    if(!std::isfinite(a) || !std::isfinite(b))
      return;
    auto reported=ulp_dist(a,b);
    if(reported!=expected_ulp)
      std::abort();
  };

  //distance to self must be zero
  compare(x,x,0);
  //distance to next must be one
  compare(x,y,1);
  compare(y,z,1);
  //distance to second next must be two
  compare(x,z,2);

}

void test_interesting_numbers() {
  for(double x : {-1.0,0.,1.,1e9,-1e9, M_PI,DBL_MIN,double(FLT_MIN),DBL_MAX,double(FLT_MAX)}) {
    test_single_number(x);
    test_single_number(static_cast<float>(x));
  }
}

template<class Real,class RNG>
void test_random_numbers(RNG& rng, size_t count) {
  for (size_t i=0;i<count;++i) {
   const auto tmp=rng();
   Real x=0;
   std::memcpy(&x,&tmp,std::min(sizeof(tmp),sizeof(x)));
   test_single_number(x);
  }
}

int main(int argc, char* argv[]) {
  test_interesting_numbers();

  // do some random numbers, seed with whatever is given as the first argument
  std::mt19937_64 rnd;
  if(argc>1) {
    std::seed_seq seed(argv[1],argv[1]+std::strlen(argv[1]));
    rnd.seed(seed);
  }
  test_random_numbers<double>(rnd,1000000);
  test_random_numbers<float>(rnd,1000000);
}
