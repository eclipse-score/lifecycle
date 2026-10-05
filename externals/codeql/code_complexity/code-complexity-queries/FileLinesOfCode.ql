/**
 * @name High Lines of Code in Files
 * @description Files exceeding lines-of-code threshold
 * @kind problem
 * @id cpp/lines-of-code-in-files
 * @problem.severity warning
 * @tags maintainability readability
 */

import cpp

int getFileLinesOfCodeThreshold() {
  result = 4000
}

from File f, int loc
where
  f.fromSource() and
  loc = f.getMetrics().getNumberOfLinesOfCode() and
  loc > getFileLinesOfCodeThreshold()
select f,
  "File has " + loc + " lines of code, exceeding threshold of " + getFileLinesOfCodeThreshold()
