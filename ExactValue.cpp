#include "ExactValue.hpp"
#include "Config.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <sstream>

void ExactTerm::simplify() {
  if (c == 0)
    c = 1;
  if (c < 0) {
    a = -a;
    c = -c;
  }
  if (a == 0 || b == 0) {
    a = 0;
    b = 1;
    c = 1;
    vars.clear();
    is_imaginary = false;
    return;
  }

  bool is_neg_b = (b < 0);
  long long temp_b = std::abs(b);
  long long d = 2;

  while (true) {
    long long d_pow = 1;
    bool overflow = false;
    for (int i = 0; i < root_degree; ++i) {
      if (d_pow > temp_b / d) {
        overflow = true;
        break;
      }
      d_pow *= d;
    }
    if (overflow || d_pow > temp_b)
      break;

    while (temp_b % d_pow == 0) {
      a *= d;
      temp_b /= d_pow;
    }
    d++;
  }

  if (is_neg_b) {
    if (root_degree % 2 != 0) {
      a = -a;
      b = temp_b;
    } else {
      is_imaginary = !is_imaginary;
      b = temp_b;
    }
  } else {
    b = temp_b;
  }

  std::vector<VariablePower> simplified_vars;
  for (auto &vp : vars)
    if (vp.power != 0)
      simplified_vars.push_back(vp);

  std::sort(simplified_vars.begin(), simplified_vars.end(),
            [](const VariablePower &x, const VariablePower &y) {
              return x.name < y.name;
            });

  vars.clear();
  for (const auto &vp : simplified_vars) {
    if (!vars.empty() && vars.back().name == vp.name)
      vars.back().power += vp.power;
    else
      vars.push_back(vp);
  }

  auto it =
      std::remove_if(vars.begin(), vars.end(),
                     [](const VariablePower &vp) { return vp.power == 0; });
  vars.erase(it, vars.end());

  long long g = std::gcd(std::abs(a), c);
  if (g > 0) {
    a /= g;
    c /= g;
  }
}
void ExactValue::simplify() {
  if (!symbolic_repr.empty() || is_approx)
    return;

  std::vector<ExactTerm> new_terms;
  for (auto &t : terms) {
    t.simplify();
    if (t.a == 0)
      continue;

    bool combined = false;
    for (auto &nt : new_terms) {
      if (are_like_terms(nt, t)) {
        long long new_a = nt.a * t.c + t.a * nt.c;
        long long new_c = nt.c * t.c;
        nt.a = new_a;
        nt.c = new_c;
        nt.simplify();
        combined = true;
        break;
      }
    }
    if (!combined)
      new_terms.push_back(t);
  }

  terms.clear();
  double dbl_acc = 0.0, imag_acc = 0.0;
  for (auto &nt : new_terms) {
    if (nt.a != 0) {
      terms.push_back(nt);
      double var_val = 1.0;
      double val = (static_cast<double>(nt.a) *
                    std::pow(static_cast<double>(nt.b), 1.0 / nt.root_degree) *
                    var_val) /
                   static_cast<double>(nt.c);
      if (nt.is_imaginary)
        imag_acc += val;
      else
        dbl_acc += val;
    }
  }
  cached_double = dbl_acc;
  cached_imag = imag_acc;
}

ExactValue make_exact(long long a, long long b, long long c, long long root,
                      double dbl, double imag) {
  ExactValue ev;
  ev.terms.push_back({a, b, c, root, {}, imag != 0.0});
  ev.cached_double = dbl;
  ev.cached_imag = imag;
  ev.simplify();
  return ev;
}

double to_double(const ExactValue &ev) { return ev.cached_double; }

ExactValue double_to_exact(double val) {
  if (std::isnan(val) || std::isinf(val))
    return make_exact(0, 1, 1, 2, 0.0);

  double rounded = std::round(val);
  if (std::abs(val - rounded) < 1e-9)
    val = rounded;

  long long sign = (val < 0) ? -1 : 1;
  double abs_val = std::abs(val);
  if (abs_val < 1e-9)
    return make_exact(0, 1, 1, 2, 0.0);

  long long c = 1000000;
  long long a = std::round(abs_val * c);
  return make_exact(sign * a, 1, c, 2, val);
}

bool is_terminating_decimal(long long denominator) {
  if (denominator <= 0)
    return false;
  while (denominator % 2 == 0)
    denominator /= 2;
  while (denominator % 5 == 0)
    denominator /= 5;
  return denominator == 1;
}

std::string format_double(double val) {
  std::ostringstream oss;
  oss << std::setprecision(10) << val;
  std::string s = oss.str();
  if (s.find('.') != std::string::npos) {
    while (!s.empty() && s.back() == '0')
      s.pop_back();
    if (!s.empty() && s.back() == '.')
      s.pop_back();
  }
  return s;
}

std::string format_complex(double real, double imag) {
  if (std::abs(real) < 1e-9 && std::abs(imag) < 1e-9)
    return "0";
  if (std::abs(real) < 1e-9) {
    if (std::abs(imag - 1.0) < 1e-9)
      return "i";
    if (std::abs(imag + 1.0) < 1e-9)
      return "-i";
    return format_double(imag) + "i";
  }
  if (std::abs(imag) < 1e-9)
    return format_double(real);

  std::string res = format_double(real);
  if (imag > 0) {
    if (std::abs(imag - 1.0) < 1e-9)
      res += " + i";
    else
      res += " + " + format_double(imag) + "i";
  } else {
    if (std::abs(imag + 1.0) < 1e-9)
      res += " - i";
    else
      res += " - " + format_double(std::abs(imag)) + "i";
  }
  return res;
}

bool has_variables(const ExactValue &ev) {
  for (const auto &t : ev.terms)
    if (!t.vars.empty())
      return true;
  return false;
}

std::string to_exact_string(const ExactValue &ev) {
  if (!ev.symbolic_repr.empty())
    return ev.symbolic_repr;
  if (ev.terms.empty())
    return "0";

  std::string res = "";
  for (size_t i = 0; i < ev.terms.size(); ++i) {
    const auto &t = ev.terms[i];
    long long a_print = t.a;

    if (i > 0) {
      if (t.a < 0) {
        res += " - ";
        a_print = -t.a;
      } else {
        res += " + ";
      }
    } else if (t.a < 0) {
      res += "-";
      a_print = -t.a;
    }

    std::string var_str = "";
    for (const auto &vp : t.vars) {
      var_str += vp.name;
      if (vp.power != 1)
        var_str += "^" + std::to_string(vp.power);
    }

    std::string i_str = t.is_imaginary ? "i" : "";
    std::string num_str = "";

    if (t.b == 1) {
      if (a_print == 1 && (!var_str.empty() || t.is_imaginary))
        num_str = i_str + var_str;
      else
        num_str = std::to_string(a_print) + i_str + var_str;
    } else {
      std::string coeff = "";
      if (a_print == 1)
        coeff = i_str + var_str;
      else
        coeff = std::to_string(a_print) + i_str + var_str;

      std::string rad_sym = get_root_symbol(t.root_degree);
      num_str = coeff + rad_sym + (USE_UNICODE ? "" : "(") +
                std::to_string(t.b) + (USE_UNICODE ? "" : ")");
    }

    if (t.c == 1) {
      res += num_str;
    } else {
      if (t.b == 1 && t.vars.empty())
        res += std::to_string(a_print) + i_str + "/" + std::to_string(t.c);
      else
        res += "(" + num_str + ")/" + std::to_string(t.c);
    }
  }
  return res.empty() ? "0" : res;
}

// Global variable storage for Assignments

bool are_like_terms(const ExactTerm &t1, const ExactTerm &t2) {
  if (t1.is_imaginary != t2.is_imaginary)
    return false;
  if (t1.b != t2.b || t1.root_degree != t2.root_degree)
    return false;
  if (t1.vars.size() != t2.vars.size())
    return false;
  for (size_t i = 0; i < t1.vars.size(); ++i) {
    if (!(t1.vars[i] == t2.vars[i]))
      return false;
  }
  return true;
}
