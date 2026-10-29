# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************

"""Sphinx extension providing the ``feat-req-verification-table`` directive.

Lists valid lifecycle feature requirements together with the verification
state of their derived component requirements. A plain ``needtable`` cannot
do this, because the state is not a field of the feature requirement: it is
derived from the test backlinks of the linked component requirements. The
table is therefore rendered after all needs are collected.
"""

from enum import IntEnum

from docutils import nodes
from sphinx.util.docutils import SphinxDirective
from sphinx_needs.data import SphinxNeedsData
from sphinx_needs.utils import row_col_maker

_FEATURE_ID = "feat__lifecycle"


class VerificationState(IntEnum):
    """Verification state of a requirement.

    The values define the sort order of the table: verified (green) first,
    then partly verified (yellow), then not verified (red).
    """

    VERIFIED = 0
    PARTLY_VERIFIED = 1
    NOT_VERIFIED = 2

    @property
    def label(self):
        """Text shown in the table, e.g. ``partly verified``."""
        return self.name.lower().replace("_", " ")

    @property
    def css_class(self):
        """CSS class colouring the row, e.g. ``verification-state-partly-verified``.

        The colours are defined in ``module_verification_report.rst``.
        """
        return "verification-state-" + self.name.lower().replace("_", "-")


class feat_req_verification_table(nodes.General, nodes.Element):
    """Placeholder node, replaced by the rendered table in ``_render_tables``."""


class FeatReqVerificationTableDirective(SphinxDirective):
    """The ``feat-req-verification-table`` directive. It takes no arguments."""

    def run(self):
        """Emit a placeholder, as the needs are not complete at read time."""
        return [feat_req_verification_table()]


def _comp_req_state(comp_req):
    """Return the verification state of a component requirement.

    - VERIFIED: at least one test fully verifies it
    - PARTLY_VERIFIED: it is only partially verified by tests
    - NOT_VERIFIED: no test verifies it
    """
    if comp_req.get("fully_verifies_back"):
        return VerificationState.VERIFIED
    if comp_req.get("partially_verifies_back"):
        return VerificationState.PARTLY_VERIFIED
    return VerificationState.NOT_VERIFIED


def _feat_req_state(comp_req_states):
    """Return the verification state of a feature requirement.

    The state is derived from the states of its component requirements:

    - no component requirements: NOT_VERIFIED
    - all component requirements share one state: that state
    - mixed states: PARTLY_VERIFIED
    """
    states = set(comp_req_states)
    if not states:
        return VerificationState.NOT_VERIFIED
    # All component requirements share the same state -> return that state
    if len(states) == 1:
        return states.pop()
    return VerificationState.PARTLY_VERIFIED


def _is_lifecycle_feat_req(need):
    """Return whether ``need`` is a valid feature requirement of the feature."""
    return (
        need["type"] == "feat_req"
        and need["status"] == "valid"
        and _FEATURE_ID in need.get("satisfied_by", [])
    )


def _collect_rows(needs):
    """Collect the table rows from all needs.

    Each row is a ``(state, feat_req, [(comp_req, comp_state), ...])`` tuple,
    where the list holds the component requirements derived from
    ``feat_req``. Rows are sorted by state (verified first) and then by title.
    """
    rows = []
    for feat_req in filter(_is_lifecycle_feat_req, needs.values()):
        comp_reqs = [
            needs[i]
            for i in feat_req.get("derived_from_back", [])
            if i in needs and needs[i]["type"] == "comp_req"
        ]
        states = [_comp_req_state(c) for c in comp_reqs]
        rows.append((_feat_req_state(states), feat_req, list(zip(comp_reqs, states))))
    rows.sort(key=lambda r: (r[0], r[1]["title"]))
    return rows


def _derived_from_back_entry(cell, comp_reqs):
    """Render the "Derived From Back" table cell.

    The cell reads ``comp_req_id (state); comp_req_id (state); ...``, with
    each id linked to its need. ``cell`` is the cell factory of
    ``_build_table``; only its link is taken over, not the whole cell.
    """
    paragraph = nodes.paragraph()
    for index, (comp_req, comp_state) in enumerate(comp_reqs):
        if index:
            paragraph += nodes.Text("; ")
        paragraph += cell(comp_req, "id")[0].children
        paragraph += nodes.Text(f" ({comp_state.label})")
    return nodes.entry("", paragraph)


def _make_row(cell, state, feat_req, comp_reqs):
    """Render one table row for a feature requirement.

    The ID cell reads ``feat_req_id (state)``, and the row's CSS class (see
    ``VerificationState.css_class``) colours it green, yellow or red.
    """
    id_entry = cell(feat_req, "id")
    id_entry[0] += nodes.Text(f" ({state.label})")
    row = nodes.row(classes=[state.css_class])
    row += id_entry
    row += cell(feat_req, "title")
    row += _derived_from_back_entry(cell, comp_reqs)
    return row


def _build_table(app, fromdocname, needs):
    """Build the complete docutils table for the document ``fromdocname``."""

    def cell(need, field):
        """Render ``need[field]`` as a table cell; ids are linked to the need."""
        make_ref = field == "id"
        return row_col_maker(app, fromdocname, needs, need, field, make_ref=make_ref)

    tgroup = nodes.tgroup(cols=3) # Table group with 3 columns
    # The HTML writer needs one colspec per column; widths are left to the browser.
    tgroup += [nodes.colspec() for _ in range(3)]
    header = nodes.row()
    for title in ("ID", "Title", "Derived From Back"):
        header += nodes.entry("", nodes.paragraph(text=title))
    tgroup += nodes.thead("", header)
    tgroup += nodes.tbody("", *(_make_row(cell, *row) for row in _collect_rows(needs)))
    return nodes.table("", tgroup, classes=["feat-req-verification-table"])


def _render_tables(app, doctree, fromdocname):
    """Replace all placeholder nodes in ``doctree`` by the rendered table.

    Connected to ``doctree-resolved``, which runs after all needs have been
    collected and their backlinks resolved.
    """
    needs = SphinxNeedsData(app.env).get_needs_view()
    for node in list(doctree.findall(feat_req_verification_table)):
        node.replace_self(_build_table(app, fromdocname, needs))


def setup(app):
    """Register the directive, its placeholder node and the render hook."""
    app.add_node(feat_req_verification_table)
    app.add_directive("feat-req-verification-table", FeatReqVerificationTableDirective)
    app.connect("doctree-resolved", _render_tables)
    return {"parallel_read_safe": True, "parallel_write_safe": True}
