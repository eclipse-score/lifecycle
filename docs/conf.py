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

import sys
from pathlib import Path

# Make local extensions next to this file importable.
sys.path.insert(0, str(Path(__file__).parent))

extensions = ["score_sphinx_bundle"]
# Only available in lifecycle's own doc build (see //:BUILD).
if (Path(__file__).parent / "feat_req_verification_table.py").exists():
    extensions.append("feat_req_verification_table")

project = "Lifecycle and Health Management"
project_url = "https://eclipse-score.github.io/lifecycle/"
version = "0.0.0"
required_in_id = ["lifecycle"]

score_metrics_include_external_needs = True
score_metrics_requirement_types = "feat_req,comp_req,aou_req"
score_metrics_external_feature_ids = ["feat__lifecycle"]
