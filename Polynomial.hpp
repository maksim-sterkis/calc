#pragma once
#include <map>
#include <string>
#include <vector>

struct Fraction {
  long long num;
  long long den;
  Fraction(long long n = 0, long long d = 1);
  void simplify();
  Fraction operator+(const Fraction &o) const;
  Fraction operator-(const Fraction &o) const;
  Fraction operator*(const Fraction &o) const;
  Fraction operator/(const Fraction &o) const;
  bool operator==(const Fraction &o) const;
  bool operator!=(const Fraction &o) const;
  bool is_zero() const;
};

struct Monomial {
  Fraction coeff;
  std::map<std::string, int> vars;

  // Lexicographical ordering: variables are alphabetically sorted by map.
  // We compare powers of variables in alphabetical order.
  // Return true if `this` < `o` lexicographically.
  bool operator<(const Monomial &o) const;
  bool operator==(const Monomial &o) const;

  bool divides(const Monomial &o) const;
  Monomial divide(const Monomial &o) const;
  Monomial multiply(const Monomial &o) const;
};

struct ExactValue;
class Polynomial {
public:
  std::vector<Monomial> terms;
  Polynomial(const ExactValue &ev);
  Polynomial();
  Polynomial(const Monomial &m);

  void simplify();

  Polynomial operator+(const Polynomial &o) const;
  Polynomial operator-(const Polynomial &o) const;
  Polynomial operator*(const Polynomial &o) const;
  Polynomial multiply(const Fraction &f) const;
  Polynomial multiply(const Monomial &m) const;

  bool is_zero() const;
  Monomial leading_term() const;

  std::string to_string() const;
};

std::pair<std::vector<Polynomial>, Polynomial>
multivariate_divide(const Polynomial &dividend,
                    const std::vector<Polynomial> &divisors);

Polynomial s_polynomial(const Polynomial &f, const Polynomial &g);

std::vector<Polynomial> buchberger(const std::vector<Polynomial> &F);
