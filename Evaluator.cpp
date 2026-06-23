#include "Evaluator.hpp"
#include "Engine.hpp"
#include "Matrix.hpp"
#include <cmath>
#include <numbers>
#include <complex>

std::map<std::string, ExactValue> global_variables;
std::string current_target_var = "";

ExactValue lookup_trig(const std::string &func, int ref, bool positive,
                       ParserState &state) {
  ExactValue ev;
  bool undefined = false;

  if (func == "sin") {
    if (ref == 0)
      ev = make_exact(0, 1, 1, 2, 0.0);
    else if (ref == 30)
      ev = make_exact(1, 1, 2, 2, 0.5);
    else if (ref == 45)
      ev = make_exact(1, 2, 2, 2, 0.70710678118);
    else if (ref == 60)
      ev = make_exact(1, 3, 2, 2, 0.86602540378);
    else if (ref == 90)
      ev = make_exact(1, 1, 1, 2, 1.0);
  } else if (func == "cos") {
    if (ref == 0)
      ev = make_exact(1, 1, 1, 2, 1.0);
    else if (ref == 30)
      ev = make_exact(1, 3, 2, 2, 0.86602540378);
    else if (ref == 45)
      ev = make_exact(1, 2, 2, 2, 0.70710678118);
    else if (ref == 60)
      ev = make_exact(1, 1, 2, 2, 0.5);
    else if (ref == 90)
      ev = make_exact(0, 1, 1, 2, 0.0);
  } else if (func == "tan") {
    if (ref == 0)
      ev = make_exact(0, 1, 1, 2, 0.0);
    else if (ref == 30)
      ev = make_exact(1, 3, 3, 2, 0.57735026919);
    else if (ref == 45)
      ev = make_exact(1, 1, 1, 2, 1.0);
    else if (ref == 60)
      ev = make_exact(1, 3, 1, 2, 1.73205080757);
    else if (ref == 90)
      undefined = true;
  } else if (func == "csc") {
    if (ref == 0)
      undefined = true;
    else if (ref == 30)
      ev = make_exact(2, 1, 1, 2, 2.0);
    else if (ref == 45)
      ev = make_exact(1, 2, 1, 2, 1.41421356237);
    else if (ref == 60)
      ev = make_exact(2, 3, 3, 2, 1.15470053838);
    else if (ref == 90)
      ev = make_exact(1, 1, 1, 2, 1.0);
  } else if (func == "sec") {
    if (ref == 0)
      ev = make_exact(1, 1, 1, 2, 1.0);
    else if (ref == 30)
      ev = make_exact(2, 3, 3, 2, 1.15470053838);
    else if (ref == 45)
      ev = make_exact(1, 2, 1, 2, 1.41421356237);
    else if (ref == 60)
      ev = make_exact(2, 1, 1, 2, 2.0);
    else if (ref == 90)
      undefined = true;
  } else if (func == "cot") {
    if (ref == 0)
      undefined = true;
    else if (ref == 30)
      ev = make_exact(1, 3, 1, 2, 1.73205080757);
    else if (ref == 45)
      ev = make_exact(1, 1, 1, 2, 1.0);
    else if (ref == 60)
      ev = make_exact(1, 3, 3, 2, 0.57735026919);
    else if (ref == 90)
      ev = make_exact(0, 1, 1, 2, 0.0);
  }

  if (undefined) {
    state.error = ParseError::DIVIDE_BY_ZERO;
    return {};
  }

  if (!positive) {
    for (auto &t : ev.terms)
      t.a = -t.a;
    ev.cached_double = -ev.cached_double;
  }
  ev.simplify();
  return ev;
}


int simplify_ast(ParserState &state, int node_idx) {
  if (node_idx < 0 || node_idx >= state.ast_pool.size()) return node_idx;
  ASTNode &node = state.ast_pool[node_idx];

  if (node.type == ASTNodeType::FUNCTION || node.type == ASTNodeType::UNARY || node.type == ASTNodeType::BINARY) {
      if (node.left_idx != -1) node.left_idx = simplify_ast(state, node.left_idx);
      if (node.right_idx != -1) node.right_idx = simplify_ast(state, node.right_idx);
  }

  if (node.type == ASTNodeType::BINARY && node.op == '+') {
      const ASTNode& left = state.ast_pool[node.left_idx];
      const ASTNode& right = state.ast_pool[node.right_idx];
      if (left.type == ASTNodeType::FUNCTION && right.type == ASTNodeType::FUNCTION) {
          if ((left.func_name == "log" && right.func_name == "log") || 
              (left.func_name == "ln" && right.func_name == "ln")) {
              
              ASTNode mul_node;
              mul_node.type = ASTNodeType::BINARY;
              mul_node.op = '*';
              mul_node.left_idx = left.right_idx;
              mul_node.right_idx = right.right_idx;
              state.ast_pool.push_back(mul_node);
              int new_mul_idx = state.ast_pool.size() - 1;

              ASTNode log_node;
              log_node.type = ASTNodeType::FUNCTION;
              log_node.func_name = left.func_name;
              log_node.right_idx = new_mul_idx;
              state.ast_pool.push_back(log_node);
              return state.ast_pool.size() - 1;
          }
      }
  }

  if (node.type == ASTNodeType::BINARY && node.op == '-') {
      const ASTNode& left = state.ast_pool[node.left_idx];
      const ASTNode& right = state.ast_pool[node.right_idx];
      if (left.type == ASTNodeType::FUNCTION && right.type == ASTNodeType::FUNCTION) {
          if ((left.func_name == "log" && right.func_name == "log") || 
              (left.func_name == "ln" && right.func_name == "ln")) {
              
              ASTNode div_node;
              div_node.type = ASTNodeType::BINARY;
              div_node.op = '/';
              div_node.left_idx = left.right_idx;
              div_node.right_idx = right.right_idx;
              state.ast_pool.push_back(div_node);
              int new_div_idx = state.ast_pool.size() - 1;

              ASTNode log_node;
              log_node.type = ASTNodeType::FUNCTION;
              log_node.func_name = left.func_name;
              log_node.right_idx = new_div_idx;
              state.ast_pool.push_back(log_node);
              return state.ast_pool.size() - 1;
          }
      }
  }

  if (node.type == ASTNodeType::BINARY && node.op == '*') {
      const ASTNode& left = state.ast_pool[node.left_idx];
      const ASTNode& right = state.ast_pool[node.right_idx];
      if (right.type == ASTNodeType::FUNCTION && (right.func_name == "log" || right.func_name == "ln")) {
          ASTNode pow_node;
          pow_node.type = ASTNodeType::BINARY;
          pow_node.op = '^';
          pow_node.left_idx = right.right_idx;
          pow_node.right_idx = node.left_idx;
          state.ast_pool.push_back(pow_node);
          int new_pow_idx = state.ast_pool.size() - 1;

          ASTNode log_node;
          log_node.type = ASTNodeType::FUNCTION;
          log_node.func_name = right.func_name;
          log_node.right_idx = new_pow_idx;
          state.ast_pool.push_back(log_node);
          return state.ast_pool.size() - 1;
      }
  }

  
  // sin^2(x) -> 1 - cos^2(x)
  if (node.type == ASTNodeType::BINARY && node.op == '^') {
      const ASTNode& left = state.ast_pool[node.left_idx];
      const ASTNode& right = state.ast_pool[node.right_idx];
      if (left.type == ASTNodeType::FUNCTION && left.func_name == "sin" && 
          right.type == ASTNodeType::LITERAL && right.value.cached_double == 2.0) {
          
          ASTNode cos_node;
          cos_node.type = ASTNodeType::FUNCTION;
          cos_node.func_name = "cos";
          cos_node.right_idx = left.right_idx;
          state.ast_pool.push_back(cos_node);
          int new_cos_idx = state.ast_pool.size() - 1;

          ASTNode pow_node;
          pow_node.type = ASTNodeType::BINARY;
          pow_node.op = '^';
          pow_node.left_idx = new_cos_idx;
          pow_node.right_idx = node.right_idx;
          state.ast_pool.push_back(pow_node);
          int new_pow_idx = state.ast_pool.size() - 1;

          ASTNode one_node;
          one_node.type = ASTNodeType::LITERAL;
          one_node.value = make_exact(1, 1, 1, 2, 1.0);
          state.ast_pool.push_back(one_node);
          int new_one_idx = state.ast_pool.size() - 1;

          ASTNode sub_node;
          sub_node.type = ASTNodeType::BINARY;
          sub_node.op = '-';
          sub_node.left_idx = new_one_idx;
          sub_node.right_idx = new_pow_idx;
          state.ast_pool.push_back(sub_node);
          return state.ast_pool.size() - 1;
      }
  }

  return node_idx;
}


ExactValue evaluate(ParserState &state, int node_idx) {
  if (state.error != ParseError::NONE)
    return {};
  if (node_idx < 0 || node_idx >= static_cast<int>(state.ast_pool.size()))
    return {};

  const ASTNode &node = state.ast_pool[node_idx];

  if (node.type == ASTNodeType::LITERAL) {
    return substitute_variables(node.value, state);
  }
  if (node.type == ASTNodeType::UNARY) {
    if (node.op == '-') {
      ExactValue res = evaluate(state, node.right_idx);
      for (auto &t : res.terms)
        t.a = -t.a;
      res.cached_double = -res.cached_double;
      res.cached_imag = -res.cached_imag;
      res.simplify();
      return res;
    }
  }
  if (node.type == ASTNodeType::FUNCTION) {
    // Linear & Quadratic rational solver
    
    if (node.func_name == "solve") {
      const ASTNode& left_arg = state.ast_pool[node.left_idx];
      const ASTNode& right_arg = state.ast_pool[node.right_idx];

      if (left_arg.type == ASTNodeType::ARRAY && right_arg.type == ASTNodeType::ARRAY) {
          int num_eqs = left_arg.args.size();
          int num_vars = right_arg.args.size();

          if (num_eqs != num_vars) {
              state.error = ParseError::UNSUPPORTED_OPERATION;
              state.error_extra = "Number of equations must match number of variables";
              return {};
          }

          std::vector<std::string> vars;
          for (int i = 0; i < num_vars; ++i) {
              ExactValue var_ev = evaluate(state, right_arg.args[i]);
              if (var_ev.terms.size() == 1 && var_ev.terms[0].vars.size() == 1) {
                  vars.push_back(var_ev.terms[0].vars[0].name);
              } else {
                  state.error = ParseError::UNSUPPORTED_OPERATION;
                  state.error_extra = "Target variables must be singular unknowns";
                  return {};
              }
          }

          Matrix mat(num_eqs, num_vars + 1);

          for (int i = 0; i < num_eqs; ++i) {
              int eq_idx = simplify_ast(state, left_arg.args[i]);
              const ASTNode& eq_node = state.ast_pool[eq_idx];
              if (eq_node.type != ASTNodeType::BINARY || eq_node.op != '=') {
                  state.error = ParseError::UNSUPPORTED_OPERATION;
                  state.error_extra = "Array elements must be equations (e.g. x + y = 5)";
                  return {};
              }

              // Evaluate left and right separately, then subtract right from left to get Eq = 0
              ExactValue l_val = evaluate(state, eq_node.left_idx);
              ExactValue r_val = evaluate(state, eq_node.right_idx);
              ExactValue eq_val = subtract(l_val, r_val, state);

              ExactValue c; c.cached_double = 0.0;
              std::vector<ExactValue> coeffs(num_vars, make_exact(0, 1, 1, 2, 0.0));

              for (const auto& term : eq_val.terms) {
                  bool found = false;
                  for (int j = 0; j < num_vars; ++j) {
                      ExactTerm t = term;
                      auto it = std::find_if(t.vars.begin(), t.vars.end(), [&](const VariablePower &vp) { return vp.name == vars[j]; });
                      if (it != t.vars.end()) {
                          if (it->power != 1) {
                              state.error = ParseError::UNSUPPORTED_OPERATION;
                              state.error_extra = "Non-linear systems not supported yet";
                              return {};
                          }
                          t.vars.erase(it);
                          ExactValue coeff_val;
                          if (t.a != 0) {
                              coeff_val.terms.push_back(t);
                              coeff_val.simplify();
                          }
                          coeffs[j] = add(coeffs[j], coeff_val, state);
                          found = true;
                          break;
                      }
                  }
                  if (!found) {
                      ExactValue const_val;
                      const_val.terms.push_back(term);
                      const_val.simplify();
                      c = add(c, const_val, state);
                  }
              }

              for (int j = 0; j < num_vars; ++j) {
                  mat.set(i, j, coeffs[j]);
              }
              // -c
              ExactValue zero = make_exact(0, 1, 1, 2, 0.0);
              mat.set(i, num_vars, subtract(zero, c, state));
          }

          if (state.error != ParseError::NONE) return {};

          mat.rref(state);

          ExactValue res;
          res.symbolic_repr = "System Solutions: ";
          for (int i = 0; i < num_vars; ++i) {
              res.symbolic_repr += vars[i] + " = " + to_exact_string(mat.get(i, num_vars));
              if (i < num_vars - 1) res.symbolic_repr += ", ";
          }
          return res;
      }

      if (node.left_idx == -1 || node.right_idx == -1) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra =
            "solve requires 2 arguments: solve(equation, variable)";
        return {};
      }

      ExactValue var_val = evaluate(state, node.right_idx);
      std::string target_var = "";
      if (var_val.terms.size() == 1 && var_val.terms[0].vars.size() == 1 &&
          var_val.terms[0].a == 1 && var_val.terms[0].b == 1 &&
          var_val.terms[0].c == 1 && var_val.terms[0].root_degree == 2) {
        target_var = var_val.terms[0].vars[0].name;
      } else {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra =
            "second argument to solve must be a single variable (e.g., x)";
        return {};
      }

      int eq_idx = node.left_idx;
      eq_idx = simplify_ast(state, eq_idx);

      const ASTNode& eq_node = state.ast_pool[eq_idx];
      if (eq_node.type == ASTNodeType::BINARY && eq_node.op == '=') {
          const ASTNode& left = state.ast_pool[eq_node.left_idx];
          const ASTNode& right = state.ast_pool[eq_node.right_idx];
          if (left.type == ASTNodeType::FUNCTION && right.type == ASTNodeType::FUNCTION) {
              if (left.func_name == right.func_name && (left.func_name == "ln" || left.func_name == "log" || left.func_name.starts_with("log"))) {
                  ASTNode stripped_eq;
                  stripped_eq.type = ASTNodeType::BINARY;
                  stripped_eq.op = '=';
                  stripped_eq.left_idx = left.right_idx;
                  stripped_eq.right_idx = right.right_idx;
                  state.ast_pool.push_back(stripped_eq);
                  eq_idx = state.ast_pool.size() - 1;
              }
          }
      }

      current_target_var = target_var;
      ExactValue eq_val = evaluate(state, eq_idx);
      current_target_var = ""; // Reset target variable configuration

      if (state.error != ParseError::NONE)
        return {};

      // Clear division/rational relationships automatically by solving
      // Numerator = 0
      RationalValue rv = get_rational_form(eq_val, state);
      ExactValue solve_target = rv.num;

      ExactValue a, b, c;
      a.cached_double = 0.0;
      b.cached_double = 0.0;
      c.cached_double = 0.0;

      for (const auto &term : solve_target.terms) {
        int power = 0;
        ExactTerm new_term = term;
        auto it = std::find_if(
            new_term.vars.begin(), new_term.vars.end(),
            [&](const VariablePower &vp) { return vp.name == target_var; });
        if (it != new_term.vars.end()) {
          power = it->power;
          new_term.vars.erase(it);
        }

        ExactValue temp;
        if (new_term.a != 0) {
          temp.terms.push_back(new_term);
          temp.simplify();
        }

        if (power == 0)
          c = add(c, temp, state);
        else if (power == 1)
          b = add(b, temp, state);
        else if (power == 2)
          a = add(a, temp, state);
        else {
          state.error = ParseError::UNSUPPORTED_OPERATION;
          state.error_extra =
              "engine can only solve linear and quadratic equations";
          return {};
        }
      }

      if (state.error != ParseError::NONE)
        return {};

      bool is_a_zero = (a.terms.empty() && a.symbolic_repr.empty());
      bool is_b_zero = (b.terms.empty() && b.symbolic_repr.empty());

      ExactValue zero = make_exact(0, 1, 1, 2, 0.0);

      if (is_a_zero) {
        if (is_b_zero) {
          ExactValue res;
          res.symbolic_repr = "All real numbers (Identity) or No solution";
          return res;
        }
        ExactValue minus_c = subtract(zero, c, state);
        ExactValue sol = divide(minus_c, b, state);
        ExactValue res;
        res.symbolic_repr = target_var + " = " + to_exact_string(sol);
        return res;
      } else {
        ExactValue b_sq = power(b, 2, state);
        ExactValue four = make_exact(4, 1, 1, 2, 4.0);
        ExactValue ac = multiply(a, c, state);
        ExactValue four_ac = multiply(four, ac, state);
        ExactValue disc = subtract(b_sq, four_ac, state);

        ExactValue root_disc = compute_root(disc, 2, state);
        if (state.error != ParseError::NONE)
          return {};

        ExactValue minus_b = subtract(zero, b, state);
        ExactValue two = make_exact(2, 1, 1, 2, 2.0);
        ExactValue two_a = multiply(two, a, state);

        ExactValue num_plus = add(minus_b, root_disc, state);
        ExactValue num_minus = subtract(minus_b, root_disc, state);

        ExactValue sol1 = divide(num_plus, two_a, state);
        ExactValue sol2 = divide(num_minus, two_a, state);

        ExactValue res;
        std::string s1 = to_exact_string(sol1);
        std::string s2 = to_exact_string(sol2);

        if (s1 == s2) {
          res.symbolic_repr = target_var + " = " + s1;
        } else {
          res.symbolic_repr =
              target_var + " = " + s1 + " , " + target_var + " = " + s2;
        }
        return res;
      }
    }

    if (node.func_name == "approx") {
      ExactValue arg = evaluate(state, node.right_idx);
      ExactValue res;
      res.cached_double = arg.cached_double;
      res.cached_imag = arg.cached_imag;
      res.is_approx = true;
      return res;
    }

    if (node.func_name == "round") {
      if (node.left_idx == -1) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra =
            "round requires 2 arguments: round(decimals, value)";
        return {};
      }
      ExactValue decimals_ev = evaluate(state, node.left_idx);
      ExactValue val_ev = evaluate(state, node.right_idx);

      double decimals_dbl = std::round(to_double(decimals_ev));
      if (decimals_dbl < 0 || decimals_dbl > 15) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "decimals must be between 0 and 15";
        return {};
      }

      double multiplier = std::pow(10.0, decimals_dbl);
      double rounded_dbl =
          std::round(val_ev.cached_double * multiplier) / multiplier;
      double rounded_imag =
          std::round(val_ev.cached_imag * multiplier) / multiplier;

      ExactValue res;
      res.cached_double = rounded_dbl;
      res.cached_imag = rounded_imag;
      res.is_approx = true;
      return res;
    }

    if (node.func_name == "root" || node.func_name == "croot") {
      ExactValue arg = evaluate(state, node.right_idx);
      if (state.error != ParseError::NONE)
        return {};
      long long degree = (node.func_name == "root") ? 2 : 3;
      return compute_root(arg, degree, state);
    }

    if (node.func_name == "ln") {
      // Native Identity Check: ln(e^x) -> x
      const ASTNode &arg_node = state.ast_pool[node.right_idx];
      if (arg_node.type == ASTNodeType::BINARY && arg_node.op == '^') {
        ExactValue base_val = evaluate(state, arg_node.left_idx);
        if (base_val.symbolic_repr == "e") {
          return evaluate(state, arg_node.right_idx);
        }
      }

      ExactValue arg_val = evaluate(state, node.right_idx);
      if (state.error != ParseError::NONE)
        return {};

      const ASTNode &child_node = state.ast_pool[node.right_idx];
      if (child_node.type == ASTNodeType::LITERAL &&
          child_node.value.symbolic_repr == "e" &&
          child_node.value.terms.empty()) {
        return make_exact(1, 1, 1, 2, 1.0);
      }

      if (arg_val.cached_double <= 0.0 ||
          std::abs(arg_val.cached_imag) > 1e-9) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "log input must be positive real";
        return {};
      }
      if (std::abs(arg_val.cached_double - 2.718281828459) < 1e-9)
        return make_exact(1, 1, 1, 2, 1.0);

      double ln_val = std::log(arg_val.cached_double);
      ExactValue res = double_to_exact(ln_val);
      res.symbolic_repr = "ln(" + to_exact_string(arg_val) + ")";
      res.cached_double = ln_val;
      return res;
    }

    if (node.func_name == "log" || node.func_name.rfind("log", 0) == 0) {
      double base_val = 10.0;
      ExactValue base_ev = make_exact(10, 1, 1, 2, 10.0);
      ExactValue arg_val;

      if (node.left_idx != -1) {
        base_ev = evaluate(state, node.left_idx);
        arg_val = evaluate(state, node.right_idx);
        base_val = to_double(base_ev);
      } else {
        arg_val = evaluate(state, node.right_idx);
        if (node.func_name != "log") {
          std::string base_str = node.func_name.substr(3);
          try {
            base_val = std::stod(base_str);
            base_ev = double_to_exact(base_val);
          } catch (...) {
            state.error = ParseError::UNSUPPORTED_OPERATION;
            state.error_extra = "invalid log base label";
            return {};
          }
        }
      }
      if (state.error != ParseError::NONE)
        return {};

      // Native Identity Check: log_b(b^x) -> x
      const ASTNode &arg_node = state.ast_pool[node.right_idx];
      if (arg_node.type == ASTNodeType::BINARY && arg_node.op == '^') {
        ExactValue arg_base = evaluate(state, arg_node.left_idx);
        ExactValue diff = subtract(base_ev, arg_base, state);
        if (diff.terms.empty() && diff.symbolic_repr.empty()) {
          return evaluate(state, arg_node.right_idx);
        }
      }

      if (base_val <= 0.0 || base_val == 1.0 || arg_val.cached_double <= 0.0 ||
          std::abs(arg_val.cached_imag) > 1e-9) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        return {};
      }

      double p = std::log(arg_val.cached_double) / std::log(base_val);
      double rounded_p = std::round(p);
      if (std::abs(p - rounded_p) < 1e-9)
        return make_exact(static_cast<long long>(rounded_p), 1, 1, 2, p);

      ExactValue res = double_to_exact(p);
      res.symbolic_repr = "log" + to_exact_string(base_ev) + "(" +
                          to_exact_string(arg_val) + ")";
      res.cached_double = p;
      return res;
    }

    ExactValue arg_val = evaluate(state, node.right_idx);
    if (state.error != ParseError::NONE)
      return {};

    
    if (has_variables(arg_val)) {
        ExactValue sym_res;
        ExactTerm st;
        st.a = 1;
        st.b = 1;
        st.c = 1;
        st.root_degree = 2;
        st.vars.push_back({node.func_name + "(" + to_exact_string(arg_val) + ")", 1});
        sym_res.terms.push_back(st);
        sym_res.simplify();
        return sym_res;
    }

    double arg_dbl = to_double(arg_val);
    double norm_angle = std::fmod(arg_dbl, 360.0);
    if (norm_angle < 0)
      norm_angle += 360.0;
    double rounded_angle = std::round(norm_angle);

    if (std::abs(norm_angle - rounded_angle) < 1e-9 &&
        std::abs(arg_val.cached_imag) < 1e-9) {
      int int_angle = static_cast<int>(rounded_angle);
      if (int_angle % 30 == 0 || int_angle == 45 || int_angle == 135 ||
          int_angle == 225 || int_angle == 315) {
        int ref_angle = 0, quadrant = 1;
        if (int_angle >= 0 && int_angle <= 90) {
          ref_angle = int_angle;
          quadrant = 1;
        } else if (int_angle > 90 && int_angle <= 180) {
          ref_angle = 180 - int_angle;
          quadrant = 2;
        } else if (int_angle > 180 && int_angle <= 270) {
          ref_angle = int_angle - 180;
          quadrant = 3;
        } else {
          ref_angle = 360 - int_angle;
          quadrant = 4;
        }

        bool pos = true;
        if (node.func_name == "sin" || node.func_name == "csc")
          pos = (quadrant == 1 || quadrant == 2);
        else if (node.func_name == "cos" || node.func_name == "sec")
          pos = (quadrant == 1 || quadrant == 4);
        else if (node.func_name == "tan" || node.func_name == "cot")
          pos = (quadrant == 1 || quadrant == 3);

        ExactValue res = lookup_trig(node.func_name, ref_angle, pos, state);
        if (state.error == ParseError::NONE)
          return res;
      }
    }

    state.error = ParseError::NONE;
    std::complex<double> c_arg(arg_val.cached_double, arg_val.cached_imag);
    std::complex<double> c_res(0.0, 0.0);

    if (node.func_name == "sin")
      c_res = std::sin(c_arg * std::numbers::pi / 180.0);
    else if (node.func_name == "cos")
      c_res = std::cos(c_arg * std::numbers::pi / 180.0);
    else if (node.func_name == "tan")
      c_res = std::tan(c_arg * std::numbers::pi / 180.0);
    else if (node.func_name == "csc")
      c_res = 1.0 / std::sin(c_arg * std::numbers::pi / 180.0);
    else if (node.func_name == "sec")
      c_res = 1.0 / std::cos(c_arg * std::numbers::pi / 180.0);
    else if (node.func_name == "cot")
      c_res = 1.0 / std::tan(c_arg * std::numbers::pi / 180.0);
    else {
      state.error = ParseError::UNSUPPORTED_OPERATION;
      state.error_extra = "unknown function '" + node.func_name + "'";
      return {};
    }


    if (state.error == ParseError::NONE) {
        ExactValue final_res;
        final_res.cached_double = c_res.real();
        final_res.cached_imag = c_res.imag();
        final_res.symbolic_repr = node.func_name + "(" + to_exact_string(arg_val) + ")";
        return final_res;
    }
    // If it threw an error like "unknown function" or couldn't evaluate, clear error and return symbolic
    state.error = ParseError::NONE;
    state.error_extra = "";
    ExactValue sym_res;
    ExactTerm st;
    st.a = 1;
    st.b = 1;
    st.c = 1;
    st.root_degree = 2;
    st.vars.push_back({node.func_name + "(" + to_exact_string(arg_val) + ")", 1});
    sym_res.terms.push_back(st);
    sym_res.simplify();
    return sym_res;
  }

  if (node.type == ASTNodeType::BINARY) {

    ExactValue left_val = evaluate(state, node.left_idx);
    ExactValue right_val = evaluate(state, node.right_idx);
    if (state.error != ParseError::NONE)
      return {};

    switch (node.op) {
    case '=':
      return subtract(left_val, right_val, state);
    case '+':
      return add(left_val, right_val, state);
    case '-':
      return subtract(left_val, right_val, state);
    case '*':
      return multiply(left_val, right_val, state);
    case '/':
      return divide(left_val, right_val, state);
    case '^': {
      right_val.simplify();
      if (right_val.terms.size() != 1 || right_val.terms[0].b != 1 ||
          right_val.terms[0].c != 1 || right_val.terms[0].is_imaginary) {
        state.error = ParseError::UNSUPPORTED_OPERATION;
        state.error_extra = "exponent must be a clean integer value";
        return {};
      }
      return power(left_val, static_cast<int>(right_val.terms[0].a), state);
    }
    }
  }
  return {};
}

