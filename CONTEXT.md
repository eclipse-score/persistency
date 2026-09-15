# CI Domain Glossary

Ubiquitous language for the Persistency CI pipeline. Implementation details
live in `docs/module/manuals/ci_concept.rst`, not here.

## Terms

### Untrusted part
The set of CI jobs triggered by `pull_request` that run without secrets and
with a read-only token, safe for fork checkouts. Produces build results and
docs artifacts.

### Trusted chain
The set of jobs triggered by `workflow_run` after the untrusted part succeeds.
Runs in the base repository context with secrets and a write token, and checks
out only exact head SHAs from the original event.

### approved-for-ci
A label applied by codeowners to a fork PR that permits the trusted chain to
run for the current head commit. Invalidated by any new push or rebase.

### Merge queue gate
The authoritative CI run on the temporary batch merge commit created by the
GitHub merge queue. Only required checks block the queue.

### Promotion
The act of making a proven-stable job a required (blocking) check. Criteria:
green on main over a stable time window.

### Reuse-first
The policy that generic CI functionality is taken from the cicd-workflows
repository; gaps are implemented locally in reusable form and upstreamed once
proven.

### Eviction
Removal of a PR from the merge queue batch after a required check failed on
the batch head commit. The PR returns to unmerged state with the failure
charged to it.
