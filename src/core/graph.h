#ifndef DCR_CORE_GRAPH_H_
#define DCR_CORE_GRAPH_H_

#include <cstdio>
#include <vector>
#include <unordered_map>
#include "core/subset_query.h"

namespace dcr {

class SubsetQuery;


class Graph {
public:

    Graph() = delete;

	Graph(Table *table, k_quasi_count=0): table_(table),k_quasi_count_(k_quasi_count) {
		unsigned int seed = (unsigned int)time(NULL);
		rsu_ = new RandomSequenceOfUnique(seed, seed + 1);
        Initialize();
	}

    Graph(const Graph&) = delete;

    Graph& operator=(const Graph&) = delete;

    ~Graph() {
        delete rsu_;
    }

    std::vector<size_t> VertexCoverBllp();

    std::vector<size_t> VertexCoverTelp();

    // std::vector<size_t> VertexCoverQtlp(int k, double epsilon);

    double InconsistencyDegree(double epsilon, const SubsetQuery&);

    std::string ToString() {
        std::stringstream ss;
        ss << "The graph contains vertex num: " << nodes_.size() << ", edge num: " << edges_.size();
        return ss.str();
    }

    void PersistOSR(std::vector<size_t>& except_idxs);

private:

    class Edge {
    public:
        Edge(size_t u, size_t v, size_t edge_id, size_t ranking): u_(u), v_(v), edge_id_(edge_id), ranking_(ranking) {}

        Edge() = default;

        bool operator < (const Edge& other) const {
            return ranking_ < other.ranking_;
        }

        bool operator==(const Edge& other) const {
            return this == &other;
        }

        size_t GetId() {
            return edge_id_;
        }

        size_t u_;
        size_t v_;
        size_t edge_id_;
        size_t ranking_;
    };


    class Node {
    public:
        Node(size_t node_id, size_t color = -1, double lp = -1.0, size_t k_quasi = 0): node_id_(node_id), color_(color), lp_(lp), k_quasi_(k_quasi) {}

        Node() = default;

        void AddEdge(size_t v_node_id, size_t edge_id, size_t ranking) {
            edges_.emplace_back(node_id_, v_node_id, edge_id, ranking);
        }

        void SortEdge() {
            std::sort(edges_.begin(), edges_.end());
        }

        Edge GetEdge(int v_node_id) {
            for (Edge e : edges_) if (e.v_ == v_node_id) return e;
        }

        size_t EdgeSize() {
            return edges_.size();
        }

        void Clear() {
            edges_.clear();
        }

        void RemoveEdge(size_t node_id) {
            auto iter = edges_.begin();
            while (iter != edges_.end()) {
                if (iter->v_ == node_id) {
                    iter = edges_.erase(iter);
                } else {
                    iter++;
                }
            }
        }

        bool operator==(const Node& other) const {
            return this.node_id_ == other.node_id_;
        }

        size_t GetId() {
            return node_id_;
        }

        size_t node_id_;
        std::vector<Edge> edges_;
        size_t color_;
        double lp_;
        int k_quasi_;
    };

    size_t k_quasi_count_;

    std::vector<Node> nodes_;
    std::vector<Edge> edges_;

    Table* table_;

    std::unordered_map<size_t, bool> vertex_cover_;
    std::unordered_map<size_t, bool> matching_;

    RandomSequenceOfUnique* rsu_;

    void Initialize() {
        size_t total_num = table_->GetTotalRowNum();
        for (size_t i = 0; i < total_num; i++) {
            AddNode(i);
        }

        iter = table_->GetIterator();
        while (iter.HasNext()) {
            Record r = iter.Next();
            size_t r_node_idx = r.GetRowIndex();

            std::vector<size_t> conflict_nodes_idx = table_->FindConflict(r);
            for (size_t node_idx: confict_nodes_idx) {
                if (node_idx > r_node_idx) {
                    AddEdge(r_node_idx, node_idx, edges_.size(), rsu_->Next());
                }
            }
        }

        for (size_t i = 0; i < nodes_.size(); i++) {
            nodes_[i].SortEdge();
        }
    }

    void AddNode(size_t node_id) {
        nodes_.emplace_back(node_id);
    }

    void AddEdge(size_t u, size_t v, size_t edge_id, size_t ranking) {
        nodes_[u].AddEdge(v, edge_id, ranking);
        nodes_[v].AddEdge(u, edge_id, ranking);
        edges_.emplace_back(u, v, edge_id, ranking);
    }

    void Coloring();

    size_t MostColor();

    void LpSolver();

    std::vector<size_t> EliminateTriangles();

    bool IsTriangle(const Node& u, const Node& v, size_t* w) const;

    void DelTriangle(size_t u, size_t v, size_t w);

    bool InVertexcover(size_t node_id, const Oracle& oracle);

    bool InMatching(size_t u, const Edge& edge, const Oracle& oracle);
};


}  // namespace dcr

#endif  // DCR_CORE_GRAPH_H_