#include "../UnitTesting.h"
#include "../Core.h"
#include "../Algorithms.h"
using namespace SimpleLib;

namespace
{
	// Plain data node - no longer implements an interface, the algorithm
	// now queries it via the virtual methods on TestClustering below.
	class TestNode
	{
	public:
		TestNode(int weight, bool keepWithPrecedents = false, bool shouldExecute = true)
			: m_weight(weight), m_keepWithPrecedents(keepWithPrecedents), m_shouldExecute(shouldExecute)
		{
		}

		void AddPrecedent(TestNode* p)
		{
			m_precedents.Add(p);
		}

		int m_weight;
		bool m_keepWithPrecedents;
		bool m_shouldExecute;
		List<TestNode*> m_precedents;
	};

	// Test-specific clustering algorithm, wiring the algorithm's virtual
	// callbacks up to TestNode's plain data members
	class TestClustering : public NodeClustering<TestNode>
	{
	public:
		TestClustering() : NodeClustering(50, 4) {}
		bool ShouldKeepNodeWithPrecedents(TestNode* node) override { return node->m_keepWithPrecedents; }
		bool ShouldExecuteNode(TestNode* node) override { return node->m_shouldExecute; }
		int GetNodeWeight(TestNode* node) override { return node->m_weight; }
		int GetNodePrecedentCount(TestNode* node) override { return node->m_precedents.GetCount(); }
		TestNode* GetNodePrecedent(TestNode* node, int index) override { return node->m_precedents[index]; }
	};

	// Returns the total number of nodes across every cluster in a plan
	int CountPlanNodes(TestClustering::Plan* plan)
	{
		int total = 0;
		for (int i = 0; i < plan->clusters.GetCount(); i++)
			total += plan->clusters[i]->nodes.GetCount();
		return total;
	}

	// Finds which cluster in a plan a given node ended up in
	TestClustering::Cluster* FindClusterContaining(TestClustering::Plan* plan, TestNode* n)
	{
		for (int i = 0; i < plan->clusters.GetCount(); i++)
		{
			auto* c = plan->clusters[i];
			for (int j = 0; j < c->nodes.GetCount(); j++)
				if (c->nodes[j] == n)
					return c;
		}
		return nullptr;
	}

	// Verifies the plan's cluster-level dependency graph is genuinely
	// acyclic (Kahn's algorithm, using only predCount + succs - everything
	// Plan/Cluster actually expose)
	bool IsPlanAcyclic(TestClustering::Plan* plan)
	{
		Map<TestClustering::Cluster*, int> remaining;
		List<TestClustering::Cluster*> ready;
		for (int i = 0; i < plan->clusters.GetCount(); i++)
		{
			auto* c = plan->clusters[i];
			remaining.Set(c, c->predCount);
			if (c->predCount == 0)
				ready.Add(c);
		}

		int processed = 0;
		while (!ready.IsEmpty())
		{
			auto* c = ready.Dequeue();
			processed++;
			for (int i = 0; i < c->succs.GetCount(); i++)
			{
				auto* s = c->succs[i];
				int r = remaining.Get(s, -1) - 1;
				remaining.Set(s, r);
				if (r == 0)
					ready.Add(s);
			}
		}

		return processed == plan->clusters.GetCount();
	}
}

Fact("NodeClustering Single Node")
{
	TestNode a(10);

	TestClustering nc;
	auto plan = nc.Clusterize(&a);
	Assert(plan != nullptr);
	Assert(plan->clusters.GetCount() == 1);
	Assert(plan->clusters[0]->nodes.GetCount() == 1);
	Assert(plan->clusters[0]->predCount == 0);
	delete plan;
}

Fact("NodeClustering Simple Chain Always Merges")
{
	// A 1-to-1 chain has no parallelism opportunity, so must always end
	// up as a single cluster regardless of weight
	TestNode a(1000);
	TestNode b(1000);
	TestNode c(1000);
	b.AddPrecedent(&a);
	c.AddPrecedent(&b);

	TestClustering nc;
	auto plan = nc.Clusterize(&c);
	Assert(plan != nullptr);
	Assert(plan->clusters.GetCount() == 1);
	Assert(plan->clusters[0]->nodes.GetCount() == 3);
	delete plan;
}

Fact("NodeClustering Diamond Shared Precedent Is Not A Cycle")
{
	// C is a shared precedent of both A and B, which both feed R. This
	// is the ordinary fan-out/fan-in shape the algorithm exists to
	// handle, not a circular reference.
	TestNode c(10);
	TestNode a(20);
	TestNode b(20);
	TestNode r(5);
	a.AddPrecedent(&c);
	b.AddPrecedent(&c);
	r.AddPrecedent(&a);
	r.AddPrecedent(&b);

	TestClustering nc;
	auto plan = nc.Clusterize(&r);
	Assert(plan != nullptr);
	Assert(CountPlanNodes(plan) == 4);
	delete plan;
}

Fact("NodeClustering Detects True Cycle")
{
	// A <-> B is a genuine cycle (client passed a bad graph) and must
	// still be rejected
	TestNode a(10);
	TestNode b(10);
	a.AddPrecedent(&b);
	b.AddPrecedent(&a);

	TestClustering nc;
	auto plan = nc.Clusterize(&a);
	Assert(plan == nullptr);
}

Fact("NodeClustering Preserves All Nodes Across A Wider Graph")
{
	// Two independent three-node chains converging on a shared sink -
	// every node supplied must appear exactly once, somewhere, in the
	// resulting plan, regardless of how the clusters end up partitioned
	List<OwnedPtr<TestNode>> allNodes;

	auto makeChain = [&](int len) -> TestNode*
	{
		TestNode* prev = nullptr;
		for (int i = 0; i < len; i++)
		{
			TestNode* n = new TestNode(500);
			allNodes.Add(n);
			if (prev)
				n->AddPrecedent(prev);
			prev = n;
		}
		return prev;
	};

	TestNode* tail1 = makeChain(3);
	TestNode* tail2 = makeChain(3);

	TestNode* sink = new TestNode(5);
	allNodes.Add(sink);
	sink->AddPrecedent(tail1);
	sink->AddPrecedent(tail2);

	TestClustering nc;
	auto plan = nc.Clusterize(sink);
	Assert(plan != nullptr);
	Assert(CountPlanNodes(plan) == allNodes.GetCount());
	delete plan;
}

Fact("NodeClustering Keeps Independent Heavy Branches Separate At A Shared Sink")
{
	// Two independent, equally heavy three-node chains converging on a
	// cheap shared sink. Since clusters are dispatched atomically (a
	// cluster can't start until ALL its precedent clusters are done),
	// merging either chain into the sink would couple the whole merged
	// cluster to the OTHER chain's completion time too - so the right
	// answer is to keep all three separate and let the chains run in
	// parallel, not collapse everything into one serial cluster.
	List<OwnedPtr<TestNode>> allNodes;

	auto makeChain = [&](int len) -> TestNode*
	{
		TestNode* prev = nullptr;
		for (int i = 0; i < len; i++)
		{
			TestNode* n = new TestNode(500);
			allNodes.Add(n);
			if (prev)
				n->AddPrecedent(prev);
			prev = n;
		}
		return prev;
	};

	TestNode* tail1 = makeChain(3);
	TestNode* tail2 = makeChain(3);

	TestNode* sink = new TestNode(5);
	allNodes.Add(sink);
	sink->AddPrecedent(tail1);
	sink->AddPrecedent(tail2);

	TestClustering nc;
	auto plan = nc.Clusterize(sink);
	Assert(plan != nullptr);
	Assert(CountPlanNodes(plan) == allNodes.GetCount());

	// Should be 3 clusters: the two chains, kept independent, plus the sink
	Assert(plan->clusters.GetCount() == 3);

	int branchClusters = 0;
	int sinkClusters = 0;
	for (int i = 0; i < plan->clusters.GetCount(); i++)
	{
		auto* c = plan->clusters[i];
		if (c->nodes.GetCount() == 3 && c->predCount == 0)
			branchClusters++;
		else if (c->nodes.GetCount() == 1 && c->predCount == 2)
			sinkClusters++;
	}
	Assert(branchClusters == 2);
	Assert(sinkClusters == 1);

	delete plan;
}

Fact("NodeClustering Weight Changes Propagate Through Unrelated Fan-Out Siblings")
{
	// X fans out to Y and Z, both of which feed a shared sink. Whichever
	// of Y/Z merges into X first grows X's weight - and the OTHER one
	// (an existing successor of X untouched by that specific merge's own
	// precedent/successor fixups) must still see X's updated weight when
	// its own merge decision is evaluated, and the sink must correctly
	// end up waiting on X's true, fully-grown weight either way.
	// Regression test for MergeClusters failing to mark a merged
	// cluster's *other*, unrelated successors dirty after its weight
	// (not topLevel) changes.
	TestNode x(100);
	TestNode y(500);
	TestNode z(10000);
	TestNode sink(1);
	y.AddPrecedent(&x);
	z.AddPrecedent(&x);
	sink.AddPrecedent(&y);
	sink.AddPrecedent(&z);

	TestClustering nc;
	auto plan = nc.Clusterize(&sink);
	Assert(plan != nullptr);
	Assert(CountPlanNodes(plan) == 4);
	delete plan;
}

Fact("NodeClustering Nodes Within A Cluster Are Topologically Ordered")
{
	TestNode a(1000);
	TestNode b(1000);
	TestNode c(1000);
	b.AddPrecedent(&a);
	c.AddPrecedent(&b);

	TestClustering nc;
	auto plan = nc.Clusterize(&c);
	Assert(plan != nullptr);
	Assert(plan->clusters.GetCount() == 1);

	auto& nodes = plan->clusters[0]->nodes;
	Assert(nodes.GetCount() == 3);
	Assert(nodes[0] == &a);
	Assert(nodes[1] == &b);
	Assert(nodes[2] == &c);
	delete plan;
}

Fact("NodeClustering KeepWithPrecedents Merges Single-Dependent Precedents")
{
	// m wants to stay grouped with its precedents; both p1 and p2 only
	// ever feed m, so this should reduce to one cluster
	TestNode p1(10);
	TestNode p2(10);
	TestNode m(50, true);
	m.AddPrecedent(&p1);
	m.AddPrecedent(&p2);

	TestClustering nc;
	auto plan = nc.Clusterize(&m);
	Assert(plan != nullptr);
	Assert(plan->clusters.GetCount() == 1);
	Assert(plan->clusters[0]->nodes.GetCount() == 3);
	delete plan;
}

Fact("NodeClustering ShouldExecuteNode Excludes Node From Plan But Keeps Topology")
{
	// b doesn't need to execute (eg: a no-op pass-through), but it still
	// takes part in clustering/scheduling as normal - it should simply be
	// left out of the emitted cluster's node list, with a and c still
	// correctly topologically ordered around the gap it leaves behind
	TestNode a(1000);
	TestNode b(1000, false, false);
	TestNode c(1000);
	b.AddPrecedent(&a);
	c.AddPrecedent(&b);

	TestClustering nc;
	auto plan = nc.Clusterize(&c);
	Assert(plan != nullptr);
	Assert(plan->clusters.GetCount() == 1);

	auto& nodes = plan->clusters[0]->nodes;
	Assert(nodes.GetCount() == 2);
	Assert(nodes[0] == &a);
	Assert(nodes[1] == &c);
	delete plan;
}

Fact("NodeClustering ShouldExecuteNode Excluded Shared Precedent Still Feeds Both Dependents")
{
	// c is a shared precedent that doesn't need to execute, but a and b
	// still both transitively depend on it - excluding it from the plan's
	// node list must not affect the cluster/edge structure built around it
	TestNode c(10, false, false);
	TestNode a(20);
	TestNode b(20);
	TestNode r(5);
	a.AddPrecedent(&c);
	b.AddPrecedent(&c);
	r.AddPrecedent(&a);
	r.AddPrecedent(&b);

	TestClustering nc;
	auto plan = nc.Clusterize(&r);
	Assert(plan != nullptr);
	// c is excluded from the emitted plan, so 3 of the 4 nodes remain
	Assert(CountPlanNodes(plan) == 3);
	delete plan;
}

Fact("NodeClustering Plan Orders Leaf Clusters First And Reports leafClusterCount")
{
	// Build a graph with a mix of leaf clusters (no precedents) and
	// non-leaf clusters (some precedents), spread across independent
	// branches - a plain post-order Finalize walk wouldn't necessarily
	// group all the leaves together at the front, so this exercises the
	// explicit sort/count rather than something that'd pass by accident
	TestNode a(500);
	TestNode b(500);
	TestNode c(500);
	c.AddPrecedent(&a);
	c.AddPrecedent(&b);

	TestNode d(500);
	TestNode e(500);

	TestNode sink(5);
	sink.AddPrecedent(&c);
	sink.AddPrecedent(&d);
	sink.AddPrecedent(&e);

	TestClustering nc;
	auto plan = nc.Clusterize(&sink);
	Assert(plan != nullptr);

	// Make sure the graph actually produced a mix of leaf and non-leaf
	// clusters - otherwise the ordering guarantee wouldn't be exercised
	int leafCount = 0, nonLeafCount = 0;
	for (int i = 0; i < plan->clusters.GetCount(); i++)
	{
		if (plan->clusters[i]->predCount == 0)
			leafCount++;
		else
			nonLeafCount++;
	}
	Assert(leafCount > 0);
	Assert(nonLeafCount > 0);

	// leafClusterCount must match, and every leaf cluster must be a
	// leading entry with every non-leaf cluster coming after it
	Assert(plan->leafClusterCount == leafCount);
	for (int i = 0; i < plan->clusters.GetCount(); i++)
	{
		if (i < plan->leafClusterCount)
			Assert(plan->clusters[i]->predCount == 0);
		else
			Assert(plan->clusters[i]->predCount > 0);
	}

	delete plan;
}

Fact("NodeClustering Never Produces A Cyclic Plan At A Diamond Shortcut")
{
	// A -> B direct, and A -> C -> B as an alternate route. Directly
	// merging A and B would fold the A->C->B path into a cycle (A and B
	// dispatching atomically while depending on each other). Whatever
	// sequence of merges the algorithm actually picks, the resulting
	// cluster-level graph must remain acyclic and must still cover every
	// node exactly once.
	TestNode a(0);
	TestNode c(1);
	TestNode b(1);
	c.AddPrecedent(&a);
	b.AddPrecedent(&a);
	b.AddPrecedent(&c);

	TestClustering nc;
	auto plan = nc.Clusterize(&b);
	Assert(plan != nullptr);
	Assert(CountPlanNodes(plan) == 3);
	Assert(IsPlanAcyclic(plan));
	delete plan;
}

Fact("NodeClustering KeepWithPrecedents Does Not Force-Merge A Shared Precedent")
{
	// Sink wants to stay grouped with its precedents, but only p2 is
	// structurally eligible (its only successor is Sink). p1 and p3 also
	// each feed a second node (y1/y3), so per the documented safety
	// precondition they must NOT be force-merged just because the flag is
	// set - they have to go through the normal weighted decision like any
	// other fan-in. p1/p3 are heavy chains and p2/sink are cheap, so the
	// weighted decision should keep the heavy branches split (mirroring
	// "NodeClustering Keeps Independent Heavy Branches Separate At A
	// Shared Sink") - but the one thing that must hold regardless of the
	// weighted outcome is that sink's cluster is NOT the full 10-node blob
	// a buggy force-merge (ignoring the single-successor precondition)
	// would produce.
	List<OwnedPtr<TestNode>> allNodes;
	auto makeChainNodes = [&](int len) -> TestNode*
	{
		TestNode* prev = nullptr;
		for (int i = 0; i < len; i++)
		{
			TestNode* n = new TestNode(500);
			allNodes.Add(n);
			if (prev)
				n->AddPrecedent(prev);
			prev = n;
		}
		return prev;
	};

	TestNode* p1 = makeChainNodes(3);
	TestNode* p2 = makeChainNodes(3);
	TestNode* p3 = makeChainNodes(3);

	TestNode* sink = new TestNode(5, true); // KeepWithPrecedents
	allNodes.Add(sink);
	sink->AddPrecedent(p1);
	sink->AddPrecedent(p2);
	sink->AddPrecedent(p3);

	TestNode* y1 = new TestNode(1);
	allNodes.Add(y1);
	y1->AddPrecedent(p1);

	TestNode* y3 = new TestNode(1);
	allNodes.Add(y3);
	y3->AddPrecedent(p3);

	TestNode* superSink = new TestNode(1);
	allNodes.Add(superSink);
	superSink->AddPrecedent(sink);
	superSink->AddPrecedent(y1);
	superSink->AddPrecedent(y3);

	TestClustering nc;
	auto plan = nc.Clusterize(superSink);
	Assert(plan != nullptr);
	Assert(CountPlanNodes(plan) == allNodes.GetCount());
	Assert(IsPlanAcyclic(plan));

	// p2 (single-successor precondition holds) must have force-merged with
	// sink; p1/p3 (each also feed y1/y3, breaking the precondition) must
	// not have - sink's cluster must be exactly {sink, p2's 3 nodes} = 4
	// nodes, not the full 10-node blob a buggy force-merge would produce
	auto* sinkCluster = FindClusterContaining(plan, sink);
	Assert(sinkCluster->nodes.GetCount() == 4);

	delete plan;
}
