#include "lispylist.h"
#include <algorithm>
#include <iostream>
#include <iterator>

int main() {
  // y := 7 6 5
  //           \.
  //      x := 4 3 2 1
  //           /
  //   z := 9 8

  using List = LispyList<int>;
  List x = List().prepend(1).prepend(2).prepend(3).prepend(4);
  List y = x.prepend(5).prepend(6).prepend(7);
  List z = x.prepend(8).prepend(9);

  std::cout << "x: ";
  std::copy(x.begin(), x.end(), std::ostream_iterator<int>(std::cout, " "));
  std::cout << "\ny: ";
  std::copy(y.begin(), y.end(), std::ostream_iterator<int>(std::cout, " "));
  std::cout << "\nz: ";
  std::copy(z.begin(), z.end(), std::ostream_iterator<int>(std::cout, " "));
  std::cout << '\n';

  x = List();
  y = List();
}
