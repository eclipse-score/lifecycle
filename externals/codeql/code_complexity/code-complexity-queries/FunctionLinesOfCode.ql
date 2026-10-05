/**
 * @name High Lines of Code per Function
 * @description Functions exceeding lines-of-code threshold
 * @kind problem
 * @id cpp/lines-of-code-per-function
 * @problem.severity warning
 * @tags maintainability readability
 */

import cpp

int getLinesOfCodeThreshold() {
  result = 200
}

from Function f, int loc
where
  strictcount(f.getEntryPoint()) = 1 and
  f.fromSource() and
  loc = f.getMetrics().getNumberOfLinesOfCode() and
  loc > getLinesOfCodeThreshold()
select f,
  "Function has " + loc + " lines of code, exceeding threshold of " + getLinesOfCodeThreshold()
