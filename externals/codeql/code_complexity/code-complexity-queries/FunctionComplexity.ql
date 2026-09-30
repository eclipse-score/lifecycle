/**
 * @name High Cyclomatic Complexity in Functions
 * @description Functions exceeding cyclomatic complexity threshold
 * @kind problem
 * @id cpp/cyclomatic-complexity-function
 * @problem.severity warning
 * @tags maintainability complexity testability
 */

import cpp

int getComplexityThreshold() {
  result = 20
}

from Function f, int complexity
where
  strictcount(f.getEntryPoint()) = 1 and
  f.fromSource() and
  complexity = f.getMetrics().getCyclomaticComplexity() and
  complexity > getComplexityThreshold()
select f,
  "Function has cyclomatic complexity " + complexity +
  ", exceeding threshold of " + getComplexityThreshold()
