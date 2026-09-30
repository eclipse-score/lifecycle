/**
 * @name Number of Recursive Paths in Functions
 * @description Functions with recursive call paths exceeding threshold
 * @kind problem
 * @id cpp/recursive-paths
 * @problem.severity warning
 * @tags maintainability complexity recursion
 */

import cpp

int getRecursivePathsThreshold() {
  result = 0
}

/**
 * Gets the number of recursive call paths through `f`. A recursive call path
 * is a function `g` that `f` calls directly and from which `f` can be reached
 * again through a (possibly empty) chain of calls. This covers both direct
 * recursion (`f` calls itself) and mutual recursion (`f` -> ... -> `f`).
 */
int getNumberOfRecursivePaths(Function f) {
  result =
    count(Function g |
      f.calls(g) and
      g.calls*(f)
    )
}

from Function f, int recursivePaths
where
  strictcount(f.getEntryPoint()) = 1 and
  f.fromSource() and
  recursivePaths = getNumberOfRecursivePaths(f) and
  recursivePaths > getRecursivePathsThreshold()
select f,
  "Function has " + recursivePaths +
  " recursive paths, exceeding threshold of " + getRecursivePathsThreshold()