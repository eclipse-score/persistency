# *******************************************************************************
# Copyright (c) 2025 Contributors to the Eclipse Foundation
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

project = "S-CORE Persistency"
project_url = "https://eclipse-score.github.io/persistency/"

extensions = [
    "score_sphinx_bundle",
]

include_patterns = [
    "index.rst",
    "docs/**",
    "score/**",
]

exclude_patterns = [
    # The following entries are not required when building the documentation via 'bazel
    # build //:docs', as that command runs in a sandboxed environment. However, when
    # building the documentation via 'bazel run //:docs' or esbonio, these
    # entries are required to prevent the build from failing.
    "bazel-*",
    ".venv*",
    "_build",
    "**/CONTRIBUTION.md",
    "**/README.md",
    "**/.github/**",
    "**/improvement.md"
]

required_in_id = ["persistency", "kvs"]
