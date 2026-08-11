#include "../UnitTesting.h"
#include "../Core.h"
#include "../Algorithms.h"
using namespace SimpleLib;

namespace
{
	// Simple INode implementation for tests - owns its own list of
	// precedents so graphs can be wired up directly in each test.
	class TestNode : public NodeClustering::INode
	{
	public:
		TestNode(int weight, bool keepWithPrecedents = false)
			: m_weight(weight), m_keepWithPrecedents(keepWithPrecedents)
		{
		}

		void AddPrecedent(TestNode* p)
		{
			m_precedents.Add(p);
		}

		bool KeepWithPrecedents() override { return m_keepWithPrecedents; }
		int GetWeight() override { return m_weight; }
		int GetPrecedentCount() override { return m_precedents.GetCount(); }
		NodeClustering::INode* GetPrecedent(int index) override { return m_precedents[index]; }

	private:
		int m_weight;
		bool m_keepWithPrecedents;
		List<TestNode*> m_precedents;
	};

	// Returns the total number of nodes across every cluster in a plan
	int CountPlanNodes(NodeClustering::Plan* plan)
	{
		int total = 0;
		for (int i = 0; i < plan->clusters.GetCount(); i++)
			total += plan->clusters[i]->nodes.GetCount();
		return total;
	}
}

Fact("NodeClustering Single Node")
{
	TestNode a(10);

	NodeClustering nc;
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

	NodeClustering nc;
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

	NodeClustering nc;
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

	NodeClustering nc;
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

	NodeClustering nc;
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

	NodeClustering nc;
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

	NodeClustering nc;
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

	NodeClustering nc;
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

	NodeClustering nc;
	auto plan = nc.Clusterize(&m);
	Assert(plan != nullptr);
	Assert(plan->clusters.GetCount() == 1);
	Assert(plan->clusters[0]->nodes.GetCount() == 3);
	delete plan;
}
