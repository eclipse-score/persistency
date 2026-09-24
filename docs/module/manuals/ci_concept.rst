..
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

.. _persistency_ci_concept:

CI Concept
==========

.. contents:: Table of Contents
   :depth: 2
   :local:

Concept
-------

This document defines the CI pipeline for the Persistency module. It is both a design document and a manual: the same
concepts apply to all jobs, and each persona finds its instructions in a dedicated section.

Goals:

- **Merge queue as primary gate.** The merge queue is enabled and is the primary mechanism to protect the main branch.
- **Always green main.** All checks run on ``push`` to main and in the merge queue; the set of *required* (blocking)
  checks grows as jobs prove stable.
- **Maximal reuse.** Workflows are reused from the `cicd-workflows <https://github.com/eclipse-score/cicd-workflows>`_
  repository wherever possible. Gaps are first implemented locally in a well-factored, reusable form and upstreamed to
  cicd-workflows once proven.
- **No ``pull_request_target``.** The trusted/untrusted split is done with the ``pull_request`` + ``workflow_run``
  pattern instead (see :ref:`ci-trust-boundary`).

Pipeline Architecture
---------------------

.. mermaid::

   graph TD
     subgraph untrusted["Untrusted: pull_request event (no secrets)"]
       A[Fast checks: gitlint, format, copyright, clippy] --> B[Build + tests]
       B --> C[Docs build: artifacts produced]
     end
     subgraph trusted["Trusted chain: workflow_run (secrets, checkout by SHA)"]
       D[Label gate: approved-for-ci for forks] --> E[QNX integration test]
       D --> F[License check: DASH]
       D --> G[Docs publisher: preview + PR comment]
     end
     C --> D
     H[Merge queue / push to main] --> I[Full suite, base context]
     I --> J[Pages deploy of versioned docs]

.. _ci-trust-boundary:

Trust Boundary
--------------

Every job belongs to exactly one of two parts:

**Untrusted part** (``pull_request`` trigger)
  Runs automatically for all PRs, including fork PRs. Has no access to secrets and only a read-only token. Produces
  build results and docs artifacts but never publishes or accesses credentials.

**Trusted chain** (``workflow_run`` trigger)
  Started only after the corresponding untrusted workflow succeeded. Runs in the base repository context with secrets
  and a write token, and checks out the exact head SHA recorded from the original event (never a mutable ref name). This
  is where secrets-dependent jobs and publishing live.

For repository PRs the trusted chain runs directly; for fork PRs it is gated by the ``approved-for-ci`` label (see
:ref:`ci-codeowners-manual`).

Trigger Matrix
--------------

.. list-table::
   :header-rows: 1

   * - Event
     - Untrusted part
     - Trusted chain
     - Notes
   * - ``pull_request`` (fork or repo)
     - yes
     - only with ``approved-for-ci`` for forks; always for repo PRs
     - fast feedback for every push
   * - ``merge_group``
     - yes
     - yes
     - authoritative gate protecting main
   * - ``push`` to main
     - yes
     - yes
     - full suite + versioned docs Pages deploy
   * - ``workflow_dispatch``
     - yes
     - yes
     - manual runs, e.g. QNX

Promotion Ladder
----------------

All jobs run on PR, merge queue and push. Whether a job is *required* (blocks the merge queue) is decided by its
stability:

1. **Initially unrequired:** coverage (threshold still in introduction phase) and the QNX integration test.
2. **Promotion criterion:** a job becomes required once it has been green on ``main`` (daily/push runs) for a stable
   time window (e.g. two weeks).
3. Required checks are configured in branch protection; the merge queue waits only on required checks. Flaky jobs are
   demoted rather than tolerated.

Reuse Policy
------------

- Check `cicd-workflows <https://github.com/eclipse-score/cicd-workflows>`_ first; prefer reusable workflows with minor
  updates over local copies.
- Persistency-specific needs (QNX tooling, Rust toolchain pins, Bazel targets) stay local, but implemented as reusable
  ``workflow_call`` workflows with in-repo parameters.
- Proven generic workflows are upstreamed to cicd-workflows; persistency then switches back to the reusable one.
- All third-party actions and cicd-workflows refs are pinned to full commit SHAs; the pinned ref is updated
  deliberately, not silently.

Good-Practice Checklist
-----------------------

- **CodeQL** for Rust/C++ security scanning (scheduled + PR).
- **Renovate/Dependabot** for dependency and action updates; renovate config exists in cicd-workflows for org-wide
  reuse.
- **Action pinning** to full commit SHAs, audited on update.
- **Docs cleanup**: preview deployments are cleaned up when the PR closes (see cicd-workflows ``docs-cleanup.yml``).
- **Secrets hygiene**: each secret is reachable only from the trusted chain; the inventory is documented here and kept
  up to date.

.. _ci-pr-creator-manual:

Manual: PR Creators
-------------------

Applies whether you open a PR from a fork or from the repository itself.

- Open your PR as usual. The untrusted part (fast checks, build, tests, docs build) runs automatically on every push.
- Repository PRs: the trusted chain (license check, QNX test, docs preview link) runs automatically, no action needed.
- Fork PRs: a codeowner must apply the ``approved-for-ci`` label before the trusted chain runs. Ask for it in the PR
  once your PR is reviewed and you expect no further pushes that would invalidate the approval.
- After a rebase or new push, the previous ``approved-for-ci`` approval is invalidated and must be re-applied.
- The trusted chain posts a documentation preview link as a PR comment; use it to check for missing or broken links
  before asking for review.
- If a check fails only in the merge queue, the PR is evicted from the queue; fix and re-queue. Non-required (unstable)
  checks never block you.

.. _ci-codeowners-manual:

Manual: Codeowners
------------------

- The ``approved-for-ci`` label authorizes the trusted chain (secrets, publishing) for a fork PR at its current head
  commit.
- Only codeowners can apply it; applying it is a distinct act from review approval - review first, then label.
- The label is validated against the head commit: if the PR is rebased or new commits are pushed after labelling, the
  trusted chain refuses to run until the label is re-applied. There is no stale pending approval to hunt for.
- Auditing: every label application is visible in the PR timeline, together with the SHA it was valid for.

Manual: CI Developers
---------------------

For people extending or modifying CI behavior.

Adding or changing a job
~~~~~~~~~~~~~~~~~~~~~~~~

1. Decide the trust level first: does the job need secrets or publish rights?
   - yes: it belongs to the trusted chain.
   - no: it belongs to the untrusted part and must work on fork checkouts.

2. Wire the trigger events according to the trigger matrix; every job runs on PR, merge queue and push unless the reuse
   policy says otherwise.
3. Prefer a reusable workflow from cicd-workflows; if a gap exists, implement locally as a ``workflow_call``-based
   reusable workflow and open an upstream PR afterwards.

Trusted chain implementation rules
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- ``workflow_run`` resolves the PR via the API using the head SHA from the original ``pull_request`` run; checkout
  always uses that exact SHA.
- Fork PRs: verify ``approved-for-ci`` is present and was applied *after* the head commit date; otherwise skip with an
  explanatory comment.
- Never check out ref names (e.g. ``refs/pull/N/head``) in trusted context; only SHAs that passed the untrusted part.

Upstreaming
~~~~~~~~~~~

Generic improvements land in persistency first (fast to validate), then are proposed to
`cicd-workflows <https://github.com/eclipse-score/cicd-workflows>`_. The persistency caller then switches to the shared
workflow. Keep local solutions short-lived; the goal is that persistency's ``.github/workflows/`` contains only thin
caller wrappers plus persistency-specific jobs.

Secrets inventory
~~~~~~~~~~~~~~~~~

============================= ======================= ======================
Secret                        Used by                 Reach
============================= ======================= ======================
``SCORE_QNX_LICENSE``         QNX integration test    trusted chain only
``SCORE_QNX_USER``            QNX integration test    trusted chain only
``SCORE_QNX_PASSWORD``        QNX integration test    trusted chain only
``ECLIPSE_GITLAB_API_TOKEN``  DASH license check      trusted chain only
============================= ======================= ======================

Pages deployment uses the trusted context on ``push`` to main / release only.
