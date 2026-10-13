/**
 * @name High Number of Parameters per Function
 * @description Functions exceeding parameter count threshold
 * @kind problem
 * @id cpp/number-of-parameters-per-function
 * @problem.severity warning
 * @tags maintainability design readability
 */

import cpp

int getParameterThreshold() {
  result = 5
}

from Function f, int params
where
  strictcount(f.getEntryPoint()) = 1 and
  f.fromSource() and
  params = f.getMetrics().getNumberOfParameters() and
  params > getParameterThreshold()
select f,
  "Function has " + params + " parameters, exceeding threshold of " + getParameterThreshold()