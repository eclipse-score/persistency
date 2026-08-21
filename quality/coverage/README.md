<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Coverage (consumer layer)

Unified C++ + Rust coverage via the shared LLVM pipeline in
`@score_tooling//coverage`. This directory only holds what is specific to
this repository:

| File | Purpose |
|---|---|
| `BUILD` | `score_coverage_scope` (the production targets in scope) + `score_coverage_reporter` (LLVM tool wiring) |
| `coverage.bazelrc` | the `llvm_cov` config, imported from the root `.bazelrc` |
| `coverage_justifications.yaml` | reviewed justifications for intentionally uncovered lines (`COV_JUSTIFIED` markers) |

## Usage

```bash
bazel coverage --config=llvm_cov //score/... --build_tests_only

bazel run @score_tooling//coverage:generate_coverage_html -- \
    --yaml quality/coverage/coverage_justifications.yaml --testlogs-subdir src

xdg-open coverage_linux/index.html
```

> Do NOT combine `--config=llvm_cov` with `--config=per-x86_64-linux`:
> the GCC toolchain would win resolution and produce no coverage data.

Untested in-scope files (e.g. `kvs_tool.rs`) appear at exact 0%; the gate
compares *effective* coverage (covered + justified) against
`COVERAGE_THRESHOLD` (CI: report-only during introduction, see
`.github/workflows/coverage.yml`).

Full documentation lives with the pipeline:
[adoption guide](https://github.com/eclipse-score/tooling/blob/main/coverage/README.md)
and
[mechanism deep-dive](https://github.com/eclipse-score/tooling/blob/main/coverage/COVERAGE_GUIDE.md).
