#pragma once

#include <math.h>

namespace SimpleLib
{

class NodeClustering
{
public:
	NodeClustering()
	{

	}

	~NodeClustering()
	{
	}

	// Client supplied nodes that need to be clustered
	struct INode
	{
		virtual bool KeepWithPrecedents() = 0;
		virtual int GetWeight() = 0;
		virtual int GetPrecedentCount() = 0;
		virtual INode* GetPrecedent(int index) = 0;
	};

	// Output of clustering algorithm describing a single cluster
	class Cluster
	{
	public:
		// List of client nodes in topological order
		List<INode*> nodes;

		// List of successor clusters
		List<Cluster*> succs;

		// Number of precedent clusters
		int predCount;
	};

	class Plan
	{
	public:
		Plan() 
		{

		}
		virtual ~Plan()
		{
			for (int i = 0; i < clusters.GetCount(); i++)
			{
				delete clusters[i];
			}
		}

		List<Cluster*> clusters;
	};

	Plan* Clusterize(INode* sinkNode)
	{
		// Build node info for the entire DAG
		auto sinkNodeInfo = GetNodeInfo(sinkNode);
		if (!sinkNodeInfo)
			return nullptr;	// Circular reference found

		// Build initial clusters
		auto sinkCluster = BuildInitialClusters(sinkNodeInfo, nullptr);

		// Find all leaf clusters
		Set<ClusterInfo*> leaves;
		FindLeafClusters(leaves, sinkCluster);

		// Compute top levels
		ComputeTopLevel(sinkCluster);

		// Compute bottom levels
		for (auto iter = leaves.Iterate(); iter.Next();)
			ComputeBottomLevel(iter.Get());

		// Build a list of all edges sorted by summed top/bottom level
		List<OwnedPtr<Edge>> allEdges;
		for (auto iter = m_nodeInfos.Iterate(); iter.Next(); )
		{
			NodeInfo* node = iter.GetValue();
			for (int j = 0; j < node->preds.GetCount(); j++)
			{
				allEdges.Add(new Edge(node->preds[j], node));
			}
		}
		allEdges.Sort(Edge::CompareByLevel);

		// Merge pass
		for (int i = 0; i < allEdges.GetCount(); i++)
		{
			Edge* e = allEdges[i];
			NodeInfo* u = e->from;
			NodeInfo* v = e->to;

			// Already merged?
			if (u->cluster == v->cluster)
				continue;

			// Can't merge?
			if (IsMergeCyclic((ClusterInfo*)u->cluster, (ClusterInfo*)v->cluster))
				continue;

			// Calculate the ready width
			int width = ReadyWidth((ClusterInfo*)v->cluster);

			int costIfSeparate = CriticalPathIfSeparate(u, v);
			int costIfMerged = CriticalPathIfMerged(u, v);

			if (width > workerCount)
			{
				int overSubscription = width - workerCount;

				// widthDiscount grows with oversubscription but should be
				// damped (e.g. sqrt or log), not linear — some queuing slack
				// is still useful for load-balancing short/long task variance,
				// per the earlier caveat. Tune against DISPATCH_OVERHEAD.
				costIfSeparate -= dispatchOverhead * (int)(log2(overSubscription + 1));
			}

			if (costIfMerged <= costIfSeparate)
			{
				MergeClusters((ClusterInfo*)u->cluster, (ClusterInfo*)v->cluster);
				RecomputeLevels();
			}
		}

		// Create the plan
		Plan* plan = new Plan();
		Finalize(plan, sinkCluster);

		return plan;
	}



protected:

	class ClusterInfo;

	struct NodeInfo
	{
		INode* node = nullptr;
		ClusterInfo* cluster = nullptr;
		int inDegree = 0;
		List<NodeInfo*> preds;
		List<NodeInfo*> succs;
	};

	class ClusterInfo
	{
	public:
		Cluster* planCluster = nullptr;
		int weight = 0;
		Set<ClusterInfo*> preds;
		Set<ClusterInfo*> succs;
		Set<NodeInfo*> nodes;
		int topLevel = 0;
		int bottomLevel = -1;

		void AddNode(NodeInfo* pNode)
		{
			nodes.Add(pNode);
			pNode->cluster = this;
			weight += pNode->node->GetWeight();
		}

		void AddPrecedent(ClusterInfo* p)
		{
			preds.Add(p);
		}

		void AddDependent(ClusterInfo* p)
		{
			succs.Add(p);
		}
	};

	class Edge
	{
	public:
		Edge(NodeInfo* f, NodeInfo* t) :
			from(f),
			to(t)
		{

		}
		NodeInfo* from;
		NodeInfo* to;

		static int __cdecl CompareByLevel(OwnedPtr<Edge> const& a, OwnedPtr<Edge> const& b)
		{
			// descending order
			return (((ClusterInfo*)b->from->cluster)->bottomLevel + ((ClusterInfo*)b->to->cluster)->topLevel) -
				(((ClusterInfo*)a->from->cluster)->bottomLevel + ((ClusterInfo*)a->to->cluster)->topLevel);
		}

	};

	Map<INode*, OwnedPtr<NodeInfo>> m_nodeInfos;
	Set<ClusterInfo*> m_dirtyClusters;

	NodeInfo* GetNodeInfo(INode* node)
	{
		// Already created?
		NodeInfo* ni = m_nodeInfos.Get(node, nullptr);
		if (ni != nullptr)
		{
			// If this trips it means there's a circular reference
			if (ni->node == node)
			{
				// Circular?
				assert(ni->node == node);
				return nullptr;
			}

			return ni;
		}

		// Create new Node Info
		ni = new NodeInfo();
		m_nodeInfos.Add(node, ni);

		// Get all precedents
		int predCount = node->GetPrecedentCount();
		for (int i = 0; i < predCount; i++)
		{
			NodeInfo* pred = GetNodeInfo(node->GetPrecedent(i));
			if (pred == nullptr)
				return nullptr;
			ni->preds.Add(pred);
			pred->succs.Add(ni);
		}

		// Finalize this node
		ni->node = node;
		return ni;
	}


	ClusterInfo* BuildInitialClusters(NodeInfo* node, ClusterInfo* into)
	{
		// If already in a cluster?
		if (node->cluster)
		{
			// Make sure dependent isn't trying to combine us
			// into their cluster
			assert(into == nullptr);

			// Already built
			return node->cluster;
		}

		// Which cluster?
		ClusterInfo* pCluster = into == nullptr ? new ClusterInfo() : into;

		// Add this node to the cluster
		pCluster->AddNode(node);

		// Add precedent nodes, either to this cluster or to their own
		for (int i = 0; i < node->preds.GetCount(); i++)
		{
			NodeInfo* pred = node->preds[i];

			// Combine precedents into this cluster when they have only 
			// one successor (ie: this node) and either:
			//  - this node has only one precedent (ie: 1-to-1 chain)
			//  - this node wants to be kept its precedents
			if (pred->succs.GetCount() == 1 &&
				(node->preds.GetCount() == 1 || node->node->KeepWithPrecedents()))
			{
				// Add to this cluster
				BuildInitialClusters(pred, pCluster);
			}
			else
			{
				// Add to new cluster
				ClusterInfo* pPredCluster = BuildInitialClusters(pred, nullptr);
				pCluster->AddPrecedent(pPredCluster);
				pPredCluster->AddDependent(pCluster);
			}
		}

		// Return the cluster
		return pCluster;
	}

	void FindLeafClusters(Set<ClusterInfo*>& leaves, ClusterInfo* cluster)
	{
		if (cluster->preds.IsEmpty())
		{
			leaves.Add(cluster);
		}
		else
		{
			for (auto iter = cluster->preds.Iterate(); iter.Next(); )
				FindLeafClusters(leaves, iter.Get());
		}
	}

	int ComputeTopLevel(ClusterInfo* cluster)
	{
		// Needs calc?
		if (cluster->topLevel < 0)
		{
			cluster->topLevel = 0;
			if (cluster->preds.IsEmpty())
			{
			}
			else
			{
				for (auto iter = cluster->preds.Iterate(); iter.Next(); )
				{
					ClusterInfo* pred = iter.Get();
					int w = ComputeTopLevel(pred) + pred->weight + dispatchOverhead;
					if (w > cluster->topLevel)
						cluster->topLevel = w;
				}
			}
		}

		// Return it
		return cluster->topLevel;
	}

	int ComputeBottomLevel(ClusterInfo* cluster)
	{
		// Needs calc?
		if (cluster->bottomLevel < 0)
		{
			cluster->bottomLevel = 0;
			for (auto iter = cluster->succs.Iterate(); iter.Next(); )
			{
				ClusterInfo* succ = iter.Get();
				int w = ComputeBottomLevel(succ) + dispatchOverhead;
				if (w > cluster->bottomLevel)
					cluster->bottomLevel = w;
			}

			cluster->bottomLevel += cluster->weight;
		}
		return cluster->bottomLevel;
	}

	int dispatchOverhead = 50;
	int workerCount = 4;

	// mergeIsCyclic — does contracting A and B create a cycle in the
	// cluster graph? (original node DAG is acyclic, but a bad sequence
	// of merges can make the *cluster* graph cyclic — classic pitfall)
	bool IsMergeCyclic(ClusterInfo* A, ClusterInfo* B)
	{
		Set<ClusterInfo*> visited;
		List<ClusterInfo*> stack;

		// Start from A's dependents, excluding the direct A->B edge(s).
		// If B is still reachable via some OTHER path, contracting A and B
		// would fold that alternate path back into a cycle through the
		// merged node.
		for (auto iter = A->succs.Iterate(); iter.Next(); )
		{
			ClusterInfo* s = iter.Get();
			if (s != B)
				stack.Push(s);
		}

		while (!stack.IsEmpty())
		{
			ClusterInfo* c = stack.Pop();
			if (c == B)
				return true;		// alternate path found -> cycle

			if (visited.Contains(c))
				continue;
			visited.Add(c);

			stack.AddMany(c->succs);
		}
		return false;	// no alternate path -> safe to merge
	}

	// How many mutually-independent precedent clusters
	// feed this cluster (i.e. could genuinely run concurrently)
	int ReadyWidth(ClusterInfo* cluster)
	{
		Set<ClusterInfo*> independent;
		for (auto i = cluster->preds.Iterate(); i.Next(); )
		{
			ClusterInfo* p = i.Get();
			bool IsAncestorOfOther = false;
			for (auto j = cluster->preds.Iterate(); j.Next(); )
			{
				ClusterInfo* q = j.Get();
				if (p != q && CanReach(p, q))
				{
					IsAncestorOfOther = true;
					break;
				}
			}
			if (!IsAncestorOfOther)
			{
				independent.Add(p);
			}
		}

		return independent.GetCount();

		// cheap approximation: skip the inner ancestor check and just
		// count distinct pred clusters if perf matters more than precision
	}


	bool CanReach(ClusterInfo* p, ClusterInfo* q)
	{
		Set<ClusterInfo*> visited;
		List<ClusterInfo*> stack;
		stack.Push(p);
		while (!stack.IsEmpty())
		{
			ClusterInfo* c = stack.Pop();
			if (c == q)
				return true;
			if (visited.Contains(c))
				continue;
			visited.Add(c);
			stack.AddMany(c->succs);
		}
		return false;
	}


	// current path length through edge u->v
	// assuming they stay in different clusters (i.e. just reads the
	// current cached levels — no simulation needed, this IS the
	// current state)
	int CriticalPathIfSeparate(NodeInfo* u, NodeInfo* v)
	{
		ClusterInfo* A = (ClusterInfo*)u->cluster;
		ClusterInfo* B = (ClusterInfo*)v->cluster;
		return A->topLevel + A->weight + dispatchOverhead + B->bottomLevel;
	}

	// criticalPathIfMerged — simulate the merge locally without
	// mutating the graph: what would the path length through the
	// merged cluster be?
	int CriticalPathIfMerged(NodeInfo* u, NodeInfo* v)
	{
		ClusterInfo* A = (ClusterInfo*)u->cluster;
		ClusterInfo* B = (ClusterInfo*)v->cluster;

		int mergedWeight = A->weight + B->weight;
		int newTopLevel = A->topLevel;

		int newBottomLevel = 0;
		for (auto iter = B->succs.Iterate(); iter.Next(); )
		{
			ClusterInfo* dep = iter.Get();
			int w = dep->bottomLevel + dispatchOverhead;
			if (w > newBottomLevel)
				newBottomLevel = w;
		}
		newBottomLevel += mergedWeight;

		return newTopLevel + newBottomLevel;
	}

	void MergeClusters(ClusterInfo* target, ClusterInfo* source)
	{
		// Move nodes from b into a
		for (auto iter = source->nodes.Iterate(); iter.Next(); )
		{
			target->AddNode(iter.Get());
		}

		// Merge precedents
		target->preds.AddMany(source->preds);

		// Merge dependents
		target->succs.AddMany(source->succs);

		// Fix up external precedents
		for (auto iter = source->preds.Iterate(); iter.Next(); )
		{
			ClusterInfo* p = iter.Get();
			p->succs.Add(target);
			p->succs.Remove(source);
			m_dirtyClusters.Add(p);
		}

		// Fix up external dependents
		for (auto iter = source->succs.Iterate(); iter.Next(); )
		{
			ClusterInfo* p = iter.Get();
			p->preds.Add(target);
			p->preds.Remove(source);
			m_dirtyClusters.Add(p);
		}

		target->preds.Remove(target);
		target->preds.Remove(source);
		target->succs.Remove(target);
		target->succs.Remove(source);
		m_dirtyClusters.Add(target);

		// Remove and delete the no longer used cluster
		delete source;
	}

	void RecomputeLevels()
	{
		// --- Forward pass: propagate topLevel changes downstream ---
		List<ClusterInfo*> queue;
		Set<ClusterInfo*> queued;

		queue.AddMany(m_dirtyClusters);
		queued.AddMany(m_dirtyClusters);

		while (!queue.IsEmpty())
		{
			ClusterInfo* c = queue.Dequeue();
			queued.Remove(c);

			int old = c->topLevel;
			UpdateTopLevel(c);

			if (c->topLevel != old)
			{
				// topLevel changed -> anything downstream might need updating too
				for (auto iter = c->succs.Iterate(); iter.Next(); )
				{
					ClusterInfo* s = iter.Get();
					if (!queued.Contains(s))
					{
						queue.Add(s);
						queued.Add(s);
					}
				}
			}
		}

		// --- Backward pass: propagate bottomLevel changes upstream ---
		queue.Clear();
		queued.Clear();
		queue.AddMany(m_dirtyClusters);
		queued.AddMany(m_dirtyClusters);

		while (!queue.IsEmpty())
		{
			ClusterInfo* c = queue.Dequeue();
			queued.Remove(c);

			int old = c->bottomLevel;
			UpdateBottomLevel(c);

			if (c->bottomLevel != old)
			{
				for (auto iter = c->preds.Iterate(); iter.Next(); )
				{
					ClusterInfo* p = iter.Get();
					if (!queued.Contains(p))
					{
						queue.Add(p);
						queued.Add(p);
					}
				}
			}
		}

		m_dirtyClusters.Clear();
	}



	// UpdateTopLevel — longest weighted path from any root TO this
	// cluster, exclusive of the cluster's own weight
	void UpdateTopLevel(ClusterInfo* cluster)
	{
		cluster->topLevel = 0;
		for (auto iter = cluster->preds.Iterate(); iter.Next(); )
		{
			ClusterInfo* pred = iter.Get();
			int w = pred->topLevel + pred->weight + dispatchOverhead;
			if (w > cluster->topLevel)
				cluster->topLevel = w;
		}
	}

	// UpdateBottomLevels — longest weighted path FROM this cluster
	// to any leaf, inclusive of the cluster's own weight
	void UpdateBottomLevel(ClusterInfo* cluster)
	{
		cluster->bottomLevel = 0;
		for (auto iter = cluster->succs.Iterate(); iter.Next(); )
		{
			ClusterInfo* dep = iter.Get();
			int w = dep->bottomLevel + dispatchOverhead;
			if (w > cluster->bottomLevel)
				cluster->bottomLevel = w;
		}
		cluster->bottomLevel += cluster->weight;
	}



	List<NodeInfo*> TopologicalSortCluster(ClusterInfo* pCluster)
	{
		// Calculate number of dependents within this cluster
		for (auto iter = pCluster->nodes.Iterate(); iter.Next(); )
		{
			NodeInfo* n = iter.Get();
			n->inDegree = 0;

			for (int j = 0; j < n->preds.GetCount(); j++)
			{
				NodeInfo* p = n->preds[j];
				if (pCluster->nodes.Contains(p))
					n->inDegree++;
			}
		}

		// Find all clusters with no internal precedents
		List<NodeInfo*> ready;
		for (auto iter = pCluster->nodes.Iterate(); iter.Next(); )
		{
			if (iter.Get()->inDegree == 0)
				ready.Add(iter.Get());
		}

		// Build topological order
		List<NodeInfo*> sorted;
		while (!ready.IsEmpty())
		{
			NodeInfo* n = ready.Dequeue();
			sorted.Add(n);

			for (int i = 0; i < n->succs.GetCount(); i++)
			{
				NodeInfo* d = n->succs[i];
				if (pCluster->nodes.Contains(d))
				{
					d->inDegree--;
					if (d->inDegree == 0)
						ready.Enqueue(d);
				}
			}
		}

		assert(pCluster->nodes.GetCount() == sorted.GetCount());
		return sorted;
	}

	void TryMergeNodes(NodeInfo* target, NodeInfo* source)
	{
		// Already in same cluster?
		if (target->cluster == source->cluster)
			return;

		// Don't create cycles
		if (IsMergeCyclic((ClusterInfo*)target->cluster, (ClusterInfo*)source->cluster))
			return;

		// Merge
		MergeClusters((ClusterInfo*)target->cluster, (ClusterInfo*)source->cluster);
	}

	Cluster* Finalize(Plan* plan, ClusterInfo* cluster)
	{
		// Already finalized?
		if (cluster->planCluster != nullptr)
			return cluster->planCluster;

		// Create plan cluster
		cluster->planCluster = new Cluster();

		// Store nodes topologically
		cluster->planCluster->nodes = TopologicalSortCluster(cluster)
			.Map<INode*>([](NodeInfo* const& ni) { return ni->node;  });

		// Store precedent count
		cluster->planCluster->predCount = cluster->preds.GetCount();

		// Finalize precedents
		for (auto iter = cluster->preds.Iterate(); iter.Next(); )
		{
			// Finalize it
			Cluster* predCluster = Finalize(plan, iter.Get());

			// Add this cluster as a successor
			predCluster->succs.Add(cluster->planCluster);
		}

		// Add cluster to the plan
		plan->clusters.Add(cluster->planCluster);

		// Done!
		return cluster->planCluster;
	}

};

}