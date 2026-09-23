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

# Repo-local pytest config used by integration tests. It must be owned by this
# module so its runfiles short_path is main-repo relative; the score_itf default
# (@score_itf//:pytest.ini) resolves to an external "../score_itf+/..." path that
# is not found at test runtime in consumer repositories.
exports_files(["pytest.ini"])
