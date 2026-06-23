#include "Matrix.hpp"
#include "Engine.hpp"

Matrix::Matrix(int r, int c) : rows(r), cols(c) {
  ExactValue zero = make_exact(0, 1, 1, 2, 0.0);
  data.resize(r, std::vector<ExactValue>(c, zero));
}

void Matrix::set(int r, int c, const ExactValue &val) { data[r][c] = val; }

ExactValue Matrix::get(int r, int c) const { return data[r][c]; }

bool Matrix::rref(ParserState &state) {
  int lead = 0;
  for (int r = 0; r < rows; r++) {
    if (cols <= lead)
      return true;

    int i = r;
    while (data[i][lead].cached_double == 0.0 &&
           data[i][lead].symbolic_repr.empty() && data[i][lead].terms.empty()) {
      i++;
      if (rows == i) {
        i = r;
        lead++;
        if (cols == lead)
          return true;
      }
    }

    std::swap(data[i], data[r]);

    ExactValue lv = data[r][lead];
    if (lv.cached_double != 0.0 || !lv.terms.empty() ||
        !lv.symbolic_repr.empty()) {
      for (int c = 0; c < cols; c++) {
        data[r][c] = divide(data[r][c], lv, state);
      }
    }

    for (int i = 0; i < rows; i++) {
      if (i != r) {
        ExactValue lv_i = data[i][lead];
        for (int c = 0; c < cols; c++) {
          ExactValue mult = multiply(lv_i, data[r][c], state);
          data[i][c] = subtract(data[i][c], mult, state);
        }
      }
    }
    lead++;
  }
  return true;
}
