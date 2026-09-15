<!-- ----------------------------------------------------------------------------
  Copyright (c) 2026 Contributors to the Eclipse Foundation

  See the NOTICE file(s) distributed with this work for additional
  information regarding copyright ownership.

  This program and the accompanying materials are made available under the
  terms of the Apache License Version 2.0 which is available at
  https://www.apache.org/licenses/LICENSE-2.0

  SPDX-License-Identifier: Apache-2.0
----------------------------------------------------------------------------- -->

# Trusted-chain entry points: direct call from on-pr plus gated workflow_run for forks

Jobs that need secrets or publish rights (the trusted chain, see
`docs/module/manuals/ci_concept.rst`) are implemented once as reusable
`workflow_call` workflows and get exactly two entry points:

1. A direct call from the repository's PR workflow (`on-pr.yml`), filtered by
   `head.repo.full_name == github.repository` so only same-repository PRs,
   `merge_group`, and `push` to main reach it.
2. A thin `workflow_run` listener for fork PRs only: it resolves the PR via the
   API using the head SHA recorded by the original `pull_request` event and
   applies the `approved-for-ci` gate: the label must be present on the PR and
   its `labeled` timeline event must be newer than the last `synchronize`
   event (any push, rebase, or force-push invalidates the approval; no commit
   timestamps are compared). If the gate rejects, the run is skipped with an
   explanatory PR comment rather than failing red. On success it calls the
   same reusable workflow, checking out the exact SHA.

The fork gate itself is a local reusable `workflow_call` workflow from the
start (not inline in the listener), because it is the piece every trusted-chain
job (docs publisher, QNX test) will reuse and the first candidate to upstream
to cicd-workflows.

We chose this split over a single `workflow_run` listener handling all events
because same-repository runs then run natively in the base context with their
own event context (no SHA reconstruction needed for merge queue or push), while
the fork gate — the only genuinely dangerous path — stays small enough to
audit. The alternative of one monolithic listener would have made the
non-fork paths depend on the same reconstruction logic and the same
single-workflow failure domain. The reusable `workflow_call` core is shared by
both entry points, so behavior cannot drift between them; this shape is also
the candidate for upstreaming to cicd-workflows.

DASH license check (`license_check.yml`) is the pilot: it is trusted-only (no
untrusted part exists, since every run needs `ECLIPSE_GITLAB_API_TOKEN`).
