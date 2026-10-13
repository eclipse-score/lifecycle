/**
 * @name Number of Goto Statements in Functions
 * @description Functions with goto statement count exceeding threshold
 * @kind problem
 * @id cpp/goto-statements
 * @problem.severity warning
 * @tags maintainability complexity readability
 */

import cpp

int getGotoStatementsThreshold() {
  result = 0
}

from Function f, int gotoStatements
where
  strictcount(f.getEntryPoint()) = 1 and
  f.fromSource() and
  gotoStatements = count(GotoStmt g | g.getEnclosingFunction() = f) and
  gotoStatements > getGotoStatementsThreshold()
select f,
  "Function has " + gotoStatements +
  " goto statements, exceeding threshold of " + getGotoStatementsThreshold()