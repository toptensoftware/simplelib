#include "../UnitTesting.h"
#include "../Core.h"
#include "../Algorithms.h"
#include <stdio.h>
#include <chrono>
using namespace SimpleLib;

namespace
{
	// Plain data node - no longer implements an interface, the algorithm
	// now queries it via the virtual methods on PerfClustering below.
	class PerfNode
	{
	public:
		PerfNode(int weight) : m_weight(weight) {}

		void AddPrecedent(PerfNode* p) { m_precedents.Add(p); }

		int m_weight;
		List<PerfNode*> m_precedents;
	};

	// Test-specific clustering algorithm, wiring the algorithm's virtual
	// callbacks up to PerfNode's plain data members
	class PerfClustering : public NodeClustering<PerfNode>
	{
	public:
		bool ShouldKeepNodeWithPrecedents(PerfNode* node) override { return false; }
		bool ShouldExecuteNode(PerfNode* node) override { return true; }
		int GetNodeWeight(PerfNode* node) override { return node->m_weight; }
		int GetNodePrecedentCount(PerfNode* node) override { return node->m_precedents.GetCount(); }
		PerfNode* GetNodePrecedent(PerfNode* node, int index) override { return node->m_precedents[index]; }
	};

	// Small deterministic PRNG (xorshift32) so the generated graph - and
	// therefore the timing - is reproducible between runs
	class Rng
	{
	public:
		Rng(uint32_t seed) : m_state(seed ? seed : 1) {}

		uint32_t Next()
		{
			m_state ^= m_state << 13;
			m_state ^= m_state >> 17;
			m_state ^= m_state << 5;
			return m_state;
		}

		int Range(int maxExclusive) { return (int)(Next() % (uint32_t)maxExclusive); }

	private:
		uint32_t m_state;
	};

	// Builds a DAG of roughly nodeCount nodes, mixing long 1-to-1 chains,
	// fan-out (one node feeding several) and fan-in/convergence (several
	// nodes feeding one) - loosely modelling the shape of a real audio
	// graph rather than any one pathological shape. Returns the single
	// sink node that transitively reaches every node created; allNodes
	// owns them all.
	PerfNode* BuildRandomDag(int nodeCount, List<OwnedPtr<PerfNode>>& allNodes, uint32_t seed)
	{
		Rng rng(seed);

		auto makeNode = [&]() -> PerfNode*
		{
			// Mostly cheap nodes (MIDI/mixer-ish) with occasional heavier
			// ones (plugin-ish) - weighted well above dispatch overhead so
			// the merge/keep-separate decision is actually contested,
			// rather than everything trivially collapsing into one cluster
			int weight = (rng.Range(10) == 0) ? 500 + rng.Range(2000) : 20 + rng.Range(80);
			PerfNode* n = new PerfNode(weight);
			allNodes.Add(n);
			return n;
		};

		// Current set of branch tips with nothing depending on them yet
		List<PerfNode*> frontier;
		frontier.Add(makeNode());

		while (allNodes.GetCount() < nodeCount)
		{
			int action = rng.Range(10);
			if (action < 5)
			{
				// Extend a random tip by one node (chain)
				int i = rng.Range(frontier.GetCount());
				PerfNode* n = makeNode();
				n->AddPrecedent(frontier[i]);
				frontier.ReplaceAt(i, n);
			}
			else if (action < 8)
			{
				// Fan out: pick a tip and branch it into 2-4 new tips
				int i = rng.Range(frontier.GetCount());
				PerfNode* hub = frontier[i];
				int branches = 2 + rng.Range(3);

				PerfNode* first = makeNode();
				first->AddPrecedent(hub);
				frontier.ReplaceAt(i, first);

				for (int b = 1; b < branches && allNodes.GetCount() < nodeCount; b++)
				{
					PerfNode* n = makeNode();
					n->AddPrecedent(hub);
					frontier.Add(n);
				}
			}
			else
			{
				// Converge: merge 2-4 tips into a single new node
				if (frontier.GetCount() < 2)
					continue;

				int mergeCount = 2 + rng.Range(3);
				if (mergeCount > frontier.GetCount())
					mergeCount = frontier.GetCount();
				PerfNode* n = makeNode();
				for (int m = 0; m < mergeCount; m++)
				{
					int i = rng.Range(frontier.GetCount());
					n->AddPrecedent(frontier[i]);
					frontier.RemoveAt(i);
				}
				frontier.Add(n);
			}
		}

		// Converge everything left in the frontier into a single sink
		PerfNode* sink = makeNode();
		for (int i = 0; i < frontier.GetCount(); i++)
			sink->AddPrecedent(frontier[i]);

		return sink;
	}
}

Fact("NodeClustering Performance")
{
	const int nodeCount = 200;
	const int iterations = 3;

	List<OwnedPtr<PerfNode>> allNodes;
	PerfNode* sink = BuildRandomDag(nodeCount, allNodes, 12345);

	printf("NodeClustering Performance: %d nodes\n", allNodes.GetCount());

	{
		int totalEdges = 0, roots = 0, fanins = 0;
		Map<PerfNode*, int> useCount;
		for (int i = 0; i < allNodes.GetCount(); i++)
		{
			PerfNode* n = allNodes[i];
			int pc = n->m_precedents.GetCount();
			totalEdges += pc;
			if (pc == 0) roots++;
			if (pc > 1) fanins++;
			for (int j = 0; j < pc; j++)
			{
				PerfNode* p = n->m_precedents[j];
				useCount.Set(p, useCount.Get(p, 0) + 1);
			}
		}
		int fanouts = 0;
		for (auto iter = useCount.Iterate(); iter.Next(); )
			if (iter.GetValue() > 1) fanouts++;
		printf("diag: edges=%d roots=%d fanins=%d fanouts=%d\n", totalEdges, roots, fanins, fanouts);
	}

	for (int i = 0; i < iterations; i++)
	{
		PerfClustering nc;

		auto start = std::chrono::high_resolution_clock::now();
		auto plan = nc.Clusterize(sink);
		auto end = std::chrono::high_resolution_clock::now();

		Assert(plan != nullptr);

		double ms = std::chrono::duration<double, std::milli>(end - start).count();
		printf("  iteration %d: %.2f ms -> %d clusters\n", i, ms, plan->clusters.GetCount());

		delete plan;
	}
}
