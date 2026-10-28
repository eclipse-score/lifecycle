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

extensions = ["score_sphinx_bundle"]

project = "Lifecycle and Health Management"
project_url = "https://eclipse-score.github.io/lifecycle/"
version = "0.0.0"
required_in_id = ["lifecycle"]

score_metrics_include_external_needs = True
score_metrics_requirement_types = "feat_req,comp_req,aou_req"
score_metrics_external_feature_ids = ["feat__lifecycle"]


# --- feat-req-verification-table directive ------------------------------------
# Lists valid lifecycle feature requirements together with the verification
# state of their derived component requirements. The table is rendered after
# all needs are collected, because the state depends on the backlinks of other
# needs (component requirements and test cases).

from docutils import nodes
from sphinx.util.docutils import SphinxDirective
from sphinx_needs.data import SphinxNeedsData

_FEATURE_ID = "feat__lifecycle"
_STATE_ORDER = {"verified": 0, "partly-verified": 1, "not-verified": 2}
# Node classes defined in conf.py cannot be pickled for parallel reads, so the
# placeholder is a plain container marked by this class.
_PLACEHOLDER_CLASS = "feat-req-verification-table-placeholder"


class FeatReqVerificationTableDirective(SphinxDirective):
    def run(self):
        return [nodes.container(classes=[_PLACEHOLDER_CLASS])]


def _base_id(need_id):
    return need_id.split("[", 1)[0]


def _comp_req_state(comp_req):
    if comp_req.get("fully_verifies_back"):
        return "verified"
    if comp_req.get("partially_verifies_back"):
        return "partly verified"
    return "not verified"


def _feat_req_state(comp_req_states):
    if comp_req_states and all(s == "verified" for s in comp_req_states):
        return "verified"
    if any(s != "not verified" for s in comp_req_states):
        return "partly-verified"
    return "not-verified"


def _need_reference(app, fromdocname, need):
    if need.get("is_external"):
        refuri = need.get("external_url") or ""
    else:
        refuri = app.builder.get_relative_uri(fromdocname, need["docname"])
        refuri += "#" + need["id"]
    ref = nodes.reference("", "", internal=not need.get("is_external"), refuri=refuri)
    ref += nodes.literal(text=need["id"])
    return ref


def _build_table(app, fromdocname, needs):
    rows = []
    for feat_req in needs.values():
        if (
            feat_req["type"] != "feat_req"
            or feat_req["status"] != "valid"
            or _FEATURE_ID
            not in [_base_id(i) for i in feat_req.get("satisfied_by", [])]
        ):
            continue
        comp_reqs = [
            needs[_base_id(i)]
            for i in feat_req.get("derived_from_back", [])
            if _base_id(i) in needs and needs[_base_id(i)]["type"] == "comp_req"
        ]
        states = [_comp_req_state(c) for c in comp_reqs]
        rows.append((_feat_req_state(states), feat_req, list(zip(comp_reqs, states))))
    rows.sort(key=lambda r: (_STATE_ORDER[r[0]], r[1]["title"]))

    table = nodes.table(classes=["feat-req-verification-table"])
    tgroup = nodes.tgroup(cols=3)
    table += tgroup
    for width in (20, 25, 55):
        tgroup += nodes.colspec(colwidth=width)
    thead = nodes.thead()
    tgroup += thead
    header = nodes.row()
    for title in ("ID", "Title", "Derived From Back"):
        header += nodes.entry("", nodes.paragraph(text=title))
    thead += header

    tbody = nodes.tbody()
    tgroup += tbody
    for state, feat_req, comp_reqs in rows:
        row = nodes.row(classes=["verification-state-" + state])
        feat_req_ref = _need_reference(app, fromdocname, feat_req)
        row += nodes.entry("", nodes.paragraph("", "", feat_req_ref))
        row += nodes.entry("", nodes.paragraph(text=feat_req["title"]))
        derived = nodes.paragraph()
        for index, (comp_req, comp_state) in enumerate(comp_reqs):
            if index:
                derived += nodes.Text("; ")
            derived += _need_reference(app, fromdocname, comp_req)
            derived += nodes.Text(f" ({comp_state})")
        row += nodes.entry("", derived)
        tbody += row
    return table


def _render_feat_req_verification_tables(app, doctree, fromdocname):
    placeholders = [
        node
        for node in doctree.findall(nodes.container)
        if _PLACEHOLDER_CLASS in node["classes"]
    ]
    if not placeholders:
        return
    needs = SphinxNeedsData(app.env).get_needs_view()
    for placeholder in placeholders:
        placeholder.replace_self(_build_table(app, fromdocname, needs))


def setup(app):
    app.add_directive("feat-req-verification-table", FeatReqVerificationTableDirective)
    app.connect("doctree-resolved", _render_feat_req_verification_tables)
