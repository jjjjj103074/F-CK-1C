# Change Review

Use this shared procedure to review a defined change in F-CK-1C. It preserves
the independent **Standards** and **Spec** axes of the team's existing
`code-review` workflow. These project-specific instructions are self-contained;
reviewers do not need a contributor's personal skill installation.

Review is read-only unless the current task separately authorizes revisions.
Follow [project instructions](../../AGENTS.md), the
[contribution guide](../../CONTRIBUTING.md), and the
[PR workflow](pr-workflow.md).
Review analysis is not human approval and grants no permission to commit,
push, resolve human review threads or merge.

## Define the review snapshot

- Use the requested scope or the scope already established by the task. Ask
  only when the intended comparison cannot be determined from that context.
- For an uncommitted change, identify its baseline and include the selected
  staged, unstaged and new-file contents. A tracked-file diff alone omits new
  files. Review the selected worktree result, or the index result if staged
  content was requested; do not silently interchange them.
- For a commit, compare it with its parent and inspect the commit's actual
  contents. For a merge commit, state which parent is the baseline. For a
  range, state the endpoints and whether the baseline is a merge-base.
- For a branch or PR review, use its confirmed base and cumulative diff;
  include the commit list and relevant target-branch changes when assessing
  integration. Do not describe this as validation of only the latest commit.
- Record the baseline, target revision or worktree snapshot, selected files,
  diff source and any exclusions. A feature branch is one coherent work item;
  do not divide scope by author or presume another contributor's work unrelated.
- Inspect surrounding implementation, callers, contracts and tests where
  needed to understand the change. Changed lines define the review focus, not
  the only files reviewers may inspect.
- An invalid reference or unreadable diff is a review error. An empty selected
  diff means there is no change to review; it is not evidence that a change
  passed. If the contents change during review, identify affected conclusions
  and review the new snapshot before carrying them forward.

## Gather the two sources of truth

**Standards:** collect applicable project instructions, contribution rules,
architecture documents and module guidance. Cite actual rules rather than
inventing conventions. Let automated checks enforce mechanical formatting;
review their relevant results without duplicating style-only findings.

**Spec:** use confirmed user instructions and accepted scope decisions,
including the current conversation, plus relevant issue, design or contract
documents. Do not require an issue tracker when the requirements are already
available. Preserve explicit exclusions and subsequent scope changes.
If material requirements are missing or contradictory, report the gap and
ask only for the decision needed. An unavailable spec means that axis is
incomplete, not that the implementation satisfies its requirements.

## Run independent reviews

When isolated parallel reviewers are available, assign one to each axis.
Give both the same snapshot, diff, relevant context and source documents.
Give each its own brief below; do not provide the other reviewer's findings
or a proposed verdict before the independent passes finish.

If the environment cannot provide independent reviewers, explicitly report
that independent review was not performed. Do not represent a single agent's
two sequential readings as independent review or claim complete coverage.

### Standards brief

Check the change against documented rules. For each violation, cite the changed
file and line, the governing document and rule, and the concrete consequence.
Separate documented violations from discretionary design concerns.

Use these design heuristics when relevant; project rules take precedence.
Only report a concern with evidence of a practical cost in this change.

| Possible smell | Question to investigate |
|---|---|
| Mysterious Name | Does the name obscure the responsibility or meaning? |
| Duplicated Code | Would repeated behavior require coordinated fixes? |
| Feature Envy | Is behavior placed away from the data and responsibility it uses? |
| Data Clumps | Are related values repeatedly passed around separately? |
| Primitive Obsession | Does a raw value conceal a useful domain concept? |
| Repeated Switches | Is the same branching knowledge duplicated? |
| Shotgun Surgery | Does one concept force changes across unrelated locations? |
| Divergent Change | Does one module have unrelated reasons to change? |
| Speculative Generality | Is unused flexibility adding present complexity? |
| Message Chains | Does a caller depend on another module's internal structure? |
| Middle Man | Does a layer delegate without providing a useful boundary? |
| Refused Bequest | Does an implementation reject the contract it inherits? |

These are prompts for judgment, not mandatory refactors or automatic failures.

### Spec brief

Compare the change with the confirmed requirements. Identify missing or partial
behavior, incorrect implementations, and additions outside the accepted scope.
For each finding, cite the requirement and changed location, explain a concrete
trigger or counterexample, and state the expected versus actual behavior.
Inspect whether tests address those requirements and expose relevant failures;
tests that merely repeat the implementation are not independent correctness
evidence. Static review does not prove runtime behavior or replace execution.

## Report and recheck

Present **Standards** and **Spec** separately. Preserve each axis's conclusion;
do not combine them into one ranking that hides a failure on the other axis.
For each actionable finding, provide the file and line, requirement or rule,
evidence, impact and suggested correction. Label heuristic concerns as such.
State when an axis has no findings and disclose missing sources, unreviewed
scope or unavailable independence. No findings is not proof of correctness.

Report finding counts and the most consequential issue within each axis, plus
the reviewed snapshot. Keep execution results separate from review findings.
If revisions follow, recheck affected findings against the revised snapshot;
do not carry a previous clean conclusion over changed content without review.
Leave remaining work and PR readiness to the PR workflow.
