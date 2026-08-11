# Cantabile Audio Engine — Execution Planner Algorithm

## Purpose

The execution planner converts a DAG of audio processing nodes (plugins, MIDI
processors, mixers, grouping nodes, etc.) into an **execution plan**: a set of
node groups ("clusters"), each of which runs sequentially on a single worker
thread, with clusters themselves running in parallel across a thread pool
where the graph allows it.

The core problem: naive parallelization (dispatch every node independently
to the thread pool) introduces scheduling/dispatch overhead per node. For
cheap nodes (MIDI processing, simple mixing), this overhead can exceed the
actual cost of the work, making the plan slower than running serially. The
planner's job is to group nodes into coarser units of work — clusters —
that are cheap enough to dispatch individually, but not so coarse that real
parallelism opportunities in the graph are lost.

The output of the planner is: for each cluster, an ordered list of nodes to
execute sequentially on one worker thread, plus the dependency relationships
between clusters that let a runtime scheduler know when each cluster becomes
ready to run.

## Core concepts

**Node**: the smallest unit of audio processing (a plugin instance, a MIDI
filter, a mixer channel, a grouping/routing node, etc.). Each node has:
- A **weight**, an integer estimate of its execution cost. Heavier nodes
  (e.g. plugins, weight ~100) are more expensive to run than lighter ones
  (e.g. MIDI processing, weight ~1). Grouping/placeholder nodes with no
  real work have weight 0.
- A set of **precedents** (nodes that must execute before it) and
  **dependents** (nodes that depend on it) — this is the DAG structure.
- Optional **execution wants**, flags a node can set to influence how it's
  grouped. Currently the main one is "keep with precedents" — a request
  that the node stay in the same execution group as its direct precedents
  whenever structurally possible (used for things like a multichannel
  mixer wanting its per-channel mixer nodes on the same thread as itself).

**Cluster**: a group of one or more nodes that will execute sequentially,
in dependency order, on a single worker thread. Clusters have their own
derived weight (sum of member node weights) and their own dependency edges
to other clusters (derived from the node-level edges crossing cluster
boundaries). The final execution plan is a DAG of clusters, not nodes —
the node-level DAG is "coarsened" into a cluster-level DAG.

**Dispatch overhead**: a constant representing the fixed cost of handing a
unit of work to the thread pool and synchronizing across a thread boundary
(vs. just continuing execution in the same thread). This is the cost that
justifies merging nodes together — every cluster boundary crossed at
runtime pays this cost once.

**Worker count**: the number of physical worker threads available at
runtime. Used to judge whether splitting work at a given point in the
graph can actually yield concurrency, or whether there simply aren't
enough workers to make use of extra parallelism.

## Why clustering, not scheduling, is the hard part

There are two distinct problems:

1. **Partitioning**: deciding which nodes belong in which cluster (a
   static decision, made once when the plan is built).
2. **Dispatch**: at runtime, deciding which ready cluster a free worker
   thread picks up next (a dynamic decision, made continuously as
   execution proceeds).

The planner is entirely concerned with problem 1. Problem 2 is a simple
ready-queue/refcount scheduler at runtime: a cluster becomes eligible to
run once all its predecessor clusters have completed; workers pull ready
clusters off a queue as they free up. This is standard and doesn't need
sophisticated logic — the planner's job is to make sure the *clusters*
handed to that scheduler are well-sized.

## Grouping rules, in order of precedence

Three different forces influence whether two adjacent nodes should end up
in the same cluster:

### 1. Chains always merge (no real choice involved)

If node A has exactly one dependent B, and B has exactly one precedent A
(i.e. a simple 1-to-1 relationship with nothing else attached at that
point), there is no parallelism opportunity between them — B can't start
before A finishes regardless of how they're grouped, since B strictly
depends on A. Splitting them into separate clusters only adds dispatch
overhead for zero benefit. So this case is **always** merged, unconditionally,
regardless of accumulated weight. A chain of five heavy plugins in a row
is still one cluster — there's nothing to parallelize by splitting it.

This case is also structurally guaranteed **safe** from a graph-cycle
perspective: because B is A's only dependent, there's no way for an
alternate path to exist between the two that a merge could fold into a
cycle. (See "why merges can create cycles," below.)

### 2. Keep-with-precedents (soft, opt-in)

A node can declare it wants to stay grouped with its direct precedents —
used for tightly-coupled node families, e.g. a multichannel mixer and its
per-channel mixer inputs, where nothing else in the graph ever depends on
the channel nodes except the mixer itself. In practice this flag is only
used in situations where, structurally, the flagged node's precedents also
only ever have that one dependent — meaning this case reduces to the same
"single dependent" shape as chain-merging, and is similarly guaranteed
graph-safe by construction, given that usage pattern.

### 3. Fan-out: the actual decision point

The interesting case is where a node's precedent has *multiple* dependents
that lead to genuinely independent subgraphs — a real fork in the graph
where two branches could execute concurrently. Here there's an actual
trade-off: keep them separate (pay dispatch overhead, gain real
concurrency) or merge them (serialize the branches onto one thread, save
the overhead). This is where node weight and dispatch overhead are
compared directly: is the potential time saved by running the branches
concurrently worth the fixed cost of dispatching them separately?

This comparison is made per-candidate-merge using two cost estimates:

- **Cost if kept separate**: the length of the critical path if the two
  branches run as independent clusters — accounting for the dispatch
  overhead paid to hand them off separately.
- **Cost if merged**: the length of the critical path if they're combined
  into one cluster and run sequentially on one thread — no dispatch
  overhead between them, but no concurrency either.

Whichever is cheaper wins. This is evaluated locally, edge by edge, rather
than as some kind of global graph partition — see "the clustering
algorithm" below for how this is made tractable.

## Why merges can create cycles

The node-level DAG is acyclic by construction. But contracting two
clusters into one can introduce a cycle at the *cluster* level even though
the underlying node graph never had one. This happens when there's an
alternate path between the two clusters being merged, other than the
direct edge motivating the merge — a "diamond" shape: cluster A feeds
cluster B directly, but also feeds some other cluster C which itself feeds
B. If A and B are merged into one cluster M, the A→C and C→B edges become
M→C and C→M — a two-cluster cycle, even though nothing in the original
graph was cyclic.

Because of this, every candidate merge (except the structurally-guaranteed
safe single-dependent case above) must be checked for whether an alternate
path already exists between the two clusters before committing to the
merge. If one exists, the merge is skipped.

## Top level and bottom level

To make local merge/split decisions consistently reflect their impact on
overall execution time, every cluster tracks two derived values:

- **Top level**: the length of the longest path (in weighted time,
  including dispatch overhead at each cluster boundary) from any starting
  point in the graph up to, but not including, this cluster. Represents
  "how long before this cluster could possibly start."
- **Bottom level**: the length of the longest path from this cluster,
  including its own weight, forward to the end of the graph. Represents
  "how long everything downstream of this cluster, plus this cluster
  itself, will take once this cluster starts."

Together, top level + bottom level for a given cluster describes the
length of the longest path through the graph that passes through that
cluster — i.e., how critical that cluster is to overall completion time.
These values are what the cost comparisons in the fan-out decision (above)
are actually built from, and they need to stay up to date as merging
proceeds, since merging two clusters changes the weight and shape of the
graph around them.

## Width-awareness: accounting for limited worker count

Even where the fan-out cost comparison suggests keeping branches separate
is worthwhile, that's only true if there's an available worker thread to
actually run each branch concurrently. If a given point in the graph has
more ready, mutually-independent precedent branches than there are worker
threads, some of them are guaranteed to sit in a queue rather than run
truly concurrently — so keeping them maximally split buys dispatch
overhead with no corresponding concurrency benefit beyond the worker
count.

This is folded into the same cost comparison as a discount applied to the
"cost if separate" side of the equation, scaled by how much the width at
that point exceeds the worker count — but deliberately *damped* rather
than an all-or-nothing cutoff. Some oversubscription past the worker count
is still useful in practice: if task durations vary, having a few more
ready branches than workers gives the runtime scheduler flexibility to
keep workers busy via queuing/load-balancing as short tasks finish early,
rather than being locked into a coarser split where one long branch stalls
a worker while others sit idle. So the discount grows with oversubscription
but tapers off, rather than forcing cluster count down to exactly the
worker count.

## The clustering algorithm: start fine, merge up

Rather than starting with the whole graph as one cluster and deciding
where to cut it apart (which requires reasoning about the entire graph's
structure at every candidate cut point), the algorithm works in the
opposite direction: start with the finest possible partition and greedily
merge clusters together where doing so is beneficial. This is the standard
approach from parallel task scheduling literature (task clustering
algorithms such as Sarkar's algorithm and Dominant Sequence Clustering),
adapted here with the node-weight/dispatch-overhead cost model described
above.

Concretely, this happens in two phases:

**Phase 1 — initial cluster construction.** Starting from the DAG's root
(the final output node) and walking backward through precedents, nodes are
grouped into initial clusters wherever a merge is unconditionally safe and
beneficial: chain relationships (rule 1, above) and keep-with-precedents
relationships (rule 2). This produces a first cut at the cluster graph that
already eliminates all the "obviously should be merged" cases before any
weighted decision-making begins.

**Phase 2 — greedy weighted merging.** All remaining edges between
clusters (i.e., the fan-out cases from rule 3) are considered in priority
order — highest-priority edges first, where priority reflects how close an
edge is to the overall critical path of the graph (an edge connecting two
clusters that are both already "important" to total execution time is
considered before a less consequential one). For each edge, if the two
clusters aren't already merged and merging wouldn't create a cycle, the
cost-if-separate vs. cost-if-merged comparison (including the width
discount) decides whether to merge them. This continues until every edge
has been considered.

Because merging two clusters changes their combined weight and position in
the graph, the top level / bottom level values need to be kept consistent
as merging proceeds — recomputed either fully (for the initial build) or
incrementally (only re-touching clusters whose values could plausibly have
changed as a result of a specific merge) as the algorithm runs.

## Output: what the plan actually consists of

Once clustering is complete, the plan consists of:

- A set of clusters, each containing an ordered list of nodes (respecting
  the original node-level dependency order within that cluster) — this
  ordered list is the literal sequence of operations a worker thread
  executes for that cluster.
- The dependency relationships between clusters (which clusters must
  complete before a given cluster can start) — this is what a runtime
  dispatcher uses to know when a cluster becomes eligible to run.

At runtime, execution is driven by a simple ready-queue scheduler: clusters
with no remaining incomplete predecessors are ready to run; as each
finishes, its dependents' remaining-precedent counts are decremented, and
any that reach zero become newly ready. Worker threads pull ready clusters
from the queue as they become free. This part of the system is
intentionally simple — all of the complexity is front-loaded into building
a well-partitioned cluster graph during planning, so that the runtime
dispatch logic can stay cheap and allocation-free, suitable for a real-time
audio thread.
