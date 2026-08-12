#pragma once

#include <math.h>
#include "../Threading/Atomic.h"

namespace SimpleLib
{


template <class TNode>
class NodeClustering
{
public:
	NodeClustering()
	{

	}

	~NodeClustering()
	{
		// Free whichever ClusterInfo instances survived to the end without
		// being absorbed by a merge (see MergeClusters, which frees the
		// ones that do get absorbed).
		for (auto iter = m_allClusters.Iterate(); iter.Next(); )
			delete iter.Get();
	}

	// Client supplied nodes that need to be clustered
	virtual bool ShouldKeepNodeWithPrecedents(TNode* node) = 0;
	virtual bool ShouldExecuteNode(TNode* node) = 0;
	virtual int GetNodeWeight(TNode* node) = 0;
	virtual int GetNodePrecedentCount(TNode* node) = 0;
	virtual TNode* GetNodePrecedent(TNode* node, int index) = 0;

	// Output of clustering algorithm describing a single cluster
	class Cluster
	{
	public:
		// List of client nodes in topological order
		List<TNode*> nodes;

		// List of successor clusters
		List<Cluster*> succs;

		// Number of precedent clusters
		int predCount = 0;

		// For client convenience - a counter of pending
		// unexecuted precedents
		Atomic<int> predCountPending;

		static int __cdecl CompareByPredCount(Cluster* a, Cluster* b)
		{
			// ascending - leaf clusters (predCount == 0) sort to the front
			return a->predCount - b->predCount;
		}
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

		// Number of leading entries in clusters that are leaf clusters
		// (predCount == 0, ie: ready to run immediately) - guaranteed
		// contiguous at the front since Clusterize sorts clusters by predCount
		int leafClusterCount = 0;
	};

	Plan* Clusterize(TNode* sinkNode)
	{
		assert(sinkNode != nullptr);
		
		// Build node info for the entire DAG
		auto sinkNodeInfo = GetNodeInfo(sinkNode);
		if (!sinkNodeInfo)
			return nullptr;	// Circular reference found

		// Build initial clusters
		auto sinkCluster = BuildInitialClusters(sinkNodeInfo, nullptr);

		// Compute top levels
		ComputeTopLevel(sinkCluster);

		// Compute bottom levels. ComputeBottomLevel is memoized (guarded by
		// bottomLevel < 0), so it's safe and cheap to just kick it off from
		// every cluster - m_allClusters is exactly the live cluster set at
		// this point, no need to separately hunt for "leaf" (root) clusters
		// to seed from.
		for (auto iter = m_allClusters.Iterate(); iter.Next();)
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

		// Create the plan.
		// Note: sinkCluster may have been merged (and freed) into another
		// cluster during the merge pass above - sinkNodeInfo->cluster is
		// kept up to date by every merge, so use that instead of the
		// possibly-stale sinkCluster pointer.
		Plan* plan = new Plan();
		Finalize(plan, sinkNodeInfo->cluster);

		// Finalize's post-order traversal doesn't guarantee every leaf
		// cluster precedes every non-leaf one globally (only that a
		// cluster's own precedents precede it) - sort leaves to the front
		// explicitly so callers can rely on that ordering
		plan->clusters.Sort(Cluster::CompareByPredCount);

		plan->leafClusterCount = 0;
		while (plan->leafClusterCount < plan->clusters.GetCount() &&
			plan->clusters[plan->leafClusterCount]->predCount == 0)
		{
			plan->leafClusterCount++;
		}

		return plan;
	}



protected:

	class ClusterInfo;

	struct NodeInfo
	{
		TNode* node = nullptr;
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
		int topLevel = -1;
		int bottomLevel = -1;

		void AddNode(NodeInfo* pNode, int nodeWeight)
		{
			nodes.Add(pNode);
			pNode->cluster = this;
			weight += nodeWeight;
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

		static int __cdecl CompareByLevel(Edge* a, Edge* b)
		{
			// descending order
			return (b->from->cluster->bottomLevel + b->to->cluster->topLevel) -
				(a->from->cluster->bottomLevel + a->to->cluster->topLevel);
		}

	};

	Map<TNode*, OwnedPtr<NodeInfo>> m_nodeInfos;
	Set<ClusterInfo*> m_dirtyClusters;
	Set<ClusterInfo*> m_allClusters;

	// Scratch collections for CanReach, reused across calls (it's called
	// extremely frequently from ReadyWidth) to avoid repeatedly
	// allocating/freeing their internal storage - Clear() resets contents
	// but keeps the underlying capacity
	Set<ClusterInfo*> m_canReachVisited;
	List<ClusterInfo*> m_canReachStack;

	// Same, for IsMergeCyclic (called once per candidate merge edge)
	Set<ClusterInfo*> m_mergeCyclicVisited;
	List<ClusterInfo*> m_mergeCyclicStack;

	NodeInfo* GetNodeInfo(TNode* node)
	{
		// Already created?
		NodeInfo* ni = m_nodeInfos.Get(node, nullptr);
		if (ni != nullptr)
		{
			// ni->node is only set once this node's precedents have all
			// finished being processed (see below). If it's still null here,
			// we've recursed back into a node that's still being built
			// further up the call stack - a genuine circular reference.
			// Otherwise, this is just a diamond/shared-precedent (fan-in)
			// node that's already been fully built - not a cycle.
			if (ni->node == nullptr)
			{
				// Circular?
				assert(ni->node == nullptr);
				return nullptr;
			}

			return ni;
		}

		// Create new Node Info
		ni = new NodeInfo();
		m_nodeInfos.Add(node, ni);

		// Get all precedents
		int predCount = GetNodePrecedentCount(node);
		for (int i = 0; i < predCount; i++)
		{
			NodeInfo* pred = GetNodeInfo(GetNodePrecedent(node, i));
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
		ClusterInfo* pCluster;
		if (into == nullptr)
		{
			pCluster = new ClusterInfo();
			m_allClusters.Add(pCluster);
		}
		else
		{
			pCluster = into;
		}

		// Add this node to the cluster
		pCluster->AddNode(node, GetNodeWeight(node->node));

		// Add precedent nodes, either to this cluster or to their own
		for (int i = 0; i < node->preds.GetCount(); i++)
		{
			NodeInfo* pred = node->preds[i];

			// Combine precedents into this cluster when they have only 
			// one successor (ie: this node) and either:
			//  - this node has only one precedent (ie: 1-to-1 chain)
			//  - this node wants to be kept its precedents
			if (pred->succs.GetCount() == 1 &&
				(node->preds.GetCount() == 1 || ShouldKeepNodeWithPrecedents(node->node)))
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
		m_mergeCyclicVisited.Clear();
		m_mergeCyclicStack.Clear();

		// Start from A's dependents, excluding the direct A->B edge(s).
		// If B is still reachable via some OTHER path, contracting A and B
		// would fold that alternate path back into a cycle through the
		// merged node.
		for (auto iter = A->succs.Iterate(); iter.Next(); )
		{
			ClusterInfo* s = iter.Get();
			if (s != B)
				m_mergeCyclicStack.Push(s);
		}

		while (!m_mergeCyclicStack.IsEmpty())
		{
			ClusterInfo* c = m_mergeCyclicStack.Pop();
			if (c == B)
				return true;		// alternate path found -> cycle

			if (m_mergeCyclicVisited.Contains(c))
				continue;
			m_mergeCyclicVisited.Add(c);

			m_mergeCyclicStack.AddMany(c->succs);
		}
		return false;	// no alternate path -> safe to merge
	}

	// Above this many precedents, the exact O(k^2) ancestor check below
	// (each check itself a graph walk) gets too expensive to run on every
	// candidate edge - fall back to the cheap approximation instead.
	static const int readyWidthPrecisionLimit = 12;

	// How many mutually-independent precedent clusters
	// feed this cluster (i.e. could genuinely run concurrently)
	int ReadyWidth(ClusterInfo* cluster)
	{
		int predCount = cluster->preds.GetCount();
		if (predCount > readyWidthPrecisionLimit)
		{
			// Cheap approximation: just count distinct pred clusters,
			// without filtering out ones that are transitively ancestors
			// of another (may overcount width for large fan-in points,
			// but stays cheap where it matters most)
			return predCount;
		}

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
	}


	bool CanReach(ClusterInfo* p, ClusterInfo* q)
	{
		m_canReachVisited.Clear();
		m_canReachStack.Clear();
		m_canReachStack.Push(p);
		while (!m_canReachStack.IsEmpty())
		{
			ClusterInfo* c = m_canReachStack.Pop();
			if (c == q)
				return true;
			if (m_canReachVisited.Contains(c))
				continue;
			m_canReachVisited.Add(c);
			m_canReachStack.AddMany(c->succs);
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

		// The merged cluster inherits ALL of B's precedents, not just A -
		// under the runtime's atomic per-cluster dispatch (a cluster only
		// starts once every one of its precedent clusters has completed),
		// it can't start until every one of them is done, not just A.
		int newTopLevel = A->topLevel;
		for (auto iter = B->preds.Iterate(); iter.Next(); )
		{
			ClusterInfo* pred = iter.Get();
			if (pred == A)
				continue;
			int w = pred->topLevel + pred->weight + dispatchOverhead;
			if (w > newTopLevel)
				newTopLevel = w;
		}

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
			target->AddNode(iter.Get(), GetNodeWeight(iter.Get()->node));
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

		// target's weight just changed, which every one of its successors'
		// topLevel depends on (pred->weight in UpdateTopLevel) - but the
		// forward-pass propagation in RecomputeLevels only re-visits a
		// cluster's successors when that cluster's own topLevel changes,
		// not when its weight does. A successor untouched by the fixups
		// above (i.e. one target already had before this merge, unrelated
		// to source) would otherwise silently keep a stale topLevel. Mark
		// them all dirty explicitly so they get recomputed too.
		for (auto iter = target->succs.Iterate(); iter.Next(); )
			m_dirtyClusters.Add(iter.Get());

		// Remove and delete the no longer used cluster
		m_allClusters.Remove(source);
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



	List<TNode*> TopologicalSortCluster(ClusterInfo* pCluster)
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
		List<TNode*> sorted;
		int processedCount = 0;
		while (!ready.IsEmpty())
		{
			NodeInfo* n = ready.Dequeue();
			processedCount++;

			if (ShouldExecuteNode(n->node))
				sorted.Add(n->node);

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

		// processedCount (not sorted.GetCount(), which excludes filtered-out
		// nodes) confirms every node in the cluster was actually reachable
		// via the topological walk - i.e. no missed cycle within the cluster
		assert(pCluster->nodes.GetCount() == processedCount);
		return sorted;
	}

	Cluster* Finalize(Plan* plan, ClusterInfo* cluster)
	{
		// Already finalized?
		if (cluster->planCluster != nullptr)
			return cluster->planCluster;

		// Create plan cluster
		cluster->planCluster = new Cluster();

		// Store nodes topologically
		cluster->planCluster->nodes = TopologicalSortCluster(cluster);

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