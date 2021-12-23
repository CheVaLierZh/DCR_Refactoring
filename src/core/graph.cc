#include "core/graph.h"
#include <fstream>


namespace dcr {
using std::string;
using std::vector;
using std::unordered_map;

vector<size_t> Graph::VertexCoverBllp() {
    std::cout << "Start processing vertex cover bllp..." << std::endl;
    vector<size_t> vc;
    if (edges_.size() == 0) {
        return vc;
    }

    Coloring();
    LpSolver();
    size_t color = MostColor();
    for (size_t i = 0; i < nodes_.size(); i++)
        if (nodes_[i].lp_ == 1 || (nodes_[i].lp_ == 0.5 && nodes_[i].color_ != color))
            vc.push_back(nodes_[i].GetId());

    // erase solution trace
    for (Node& node: nodes_) {
        node.lp_ = -1.0;
        node.color_ = -1;
    }
    return vc;
}

vector<size_t> Graph::VertexCoverTelp() {
    std::cout << "Start processing vertex cover telp..." << std::endl;
    vector<size_t> vc = EliminateTriangles();
    if (edges_.size() == 0) {
        return vc;
    }

    Coloring();
    LpSolver();
    size_t color = MostColor();
    for (int i = 0; i < nodes_.size(); i++)
        if (nodes_[i].lp_ == 1 || (nodes_[i].lp_ == 0.5 && nodes_[i].color_ != color))
            vc.push_back(nodes_[i].GetId());

    std::sort(vc.begin(), vc.end());
    vc.erase(std::unique(vc.begin(), vc.end()), vc.end());
    
    // erase solution trace
    for (Node& node: nodes_) {
        node.lp_ = -1.0;
        node.color_ = -1;
    }
    return vc;
}

void Graph::Coloring() {
    size_t color = 0;
    bool done = false;
    while (!done) {
        done = true;
        for (size_t i = 0; i < nodes_.size(); i++) {
            if (nodes_[i].color_ == -1) {
                nodes_[i].color_ = color;
                vector<Edge> edges = nodes_[i].edges_;
                for (int j = 0; j < edges.size(); j++) {
                    if (nodes_[edges[j].v_].color_ == color) {
                        nodes_[i].color_ = -1;
                        done = false;
                        break;
                    }
                }
            }
        }
        color++;
    }
}

size_t Graph::MostColor()
{
    unordered_map<size_t, size_t> counts;
    for (size_t i = 0; i < nodes_.size(); i++) {
        //  cout << sum_graph.get_color(i) << endl;
        if (nodes_[i].lp_ == 0.5)
            if (counts.count(nodes_[i].color_) == 0)
                counts[nodes_[i].color_] = 1;
            else
                counts[nodes_[i].color_] += 1;
    }

    size_t max = 0, color = 0;
    for (auto iter = counts.begin(); iter != counts.end(); iter++) {
        // cout << iter->first << "," << iter->second << endl;
        if (iter->second > max) {
            max = iter->second;
            color = iter->first;
        }
    }
    // cout << color << "," << max << endl;
    return color;
}

void Graph::LpSolver() {
    int m = edges_.size();
    int n = nodes_.size();
initialize:
    glp_prob* lp;
    lp = glp_create_prob();
    glp_set_obj_dir(lp, GLP_MIN);

auxiliary_variables_rows:
    glp_add_rows(lp, m);
    for (int i = 1; i <= m; i++)
        glp_set_row_bnds(lp, i, GLP_LO, 1.0, 0.0);

variables_columns:
    glp_add_cols(lp, n);
    for (int i = 1; i <= n; i++)
        glp_set_col_bnds(lp, i, GLP_DB, 0.0, 1.0);

to_minimize:
    for (int i = 1; i <= n; i++)
        glp_set_obj_coef(lp, i, 1.0);

constrant_matrix:
    long long size = m * n;
    int* ia = new int[size + 1];
    int* ja = new int[size + 1];
    double* ar = new double[size + 1];
    for (int i = 1; i <= m; i++) {
        Edge edge = edges_[i - 1];
        for (int j = 1; j <= n; j++) {
            int k = (i - 1) * n + j;
            ia[k] = i;
            ja[k] = j;
            if (j - 1 == edge.u_ || j - 1 == edge.v_)
                ar[k] = 1.0;
            else
                ar[k] = 0.0;
        }
    }
    glp_load_matrix(lp, size, ia, ja, ar);

calculate:
    glp_simplex(lp, NULL);
    // glp_exact(lp, NULL);

output:
    // cout << glp_get_obj_val(lp) << endl;
    for (int i = 1; i <= n; i++) {
        nodes_[i - 1].lp_ = glp_get_col_prim(lp, i);
    }

cleanup:
    delete[] ia;
    delete[] ja;
    delete[] ar;
    glp_delete_prob(lp);
}

vector<size_t> Graph::EliminateTriangles() {
    vector<size_t> vertexcover;
    size_t sum = 0;
    for (auto iter = edges_.begin(); iter != edges_.end();) {
        Node& u = nodes_[iter->u_];
        Node& v = nodes_[iter->v_];
        if (u.EdgeSize() != 0 && v.EdgeSize() != 0) {
            size_t w;
            if (IsTriangle(u, v, &w)) {
                sum++;
                DelTriangle(u.node_id_, v.node_id_, w);
                vertexcover.push_back(u.node_id_);
                vertexcover.push_back(v.node_id_);
                vertexcover.push_back(w);
                iter = edges_.begin();
            } else
                iter++;
        } else {
            iter++;
        }
    }

    return vertexcover;
}

bool Graph::IsTriangle(const Node& u, const Node& v, size_t* w) const {
    for (const Edge& edge_u : u.edges_)
        for (const Edge& edge_v : v.edges_)
            if (edge_u.v_ == edge_v.v_) {
                *w = edge_u.v_;
                return true;
            }

    return false;
}

void Graph::DelTriangle(size_t u, size_t v, size_t w) {
    nodes_[u].Clear();
    nodes_[v].Clear();
    nodes_[w].Clear();
    for (auto& node : nodes_) {
        node.RemoveEdge(u);
        node.RemoveEdge(v);
        node.RemoveEdge(w);
    }
    auto iter = edges_.begin();
    while (iter != edges_.end()) {
        if (iter->u_ == u || iter->v_ == u)
            iter = edges_.erase(iter);
        else if (iter->u_ == v || iter->v_ == v)
            iter = edges_.erase(iter);
        else if (iter->u_ == w || iter->v_ == w)
            iter = edges_.erase(iter);
        else
            iter++;
    }
}

double Graph::InconsistencyDegree(double epsilon, const SubsetQuery& query) {
    Oracle oracle(*table_, query);
    std::cout << "Oracle for query completed! Number of nodes in subgraph: " << oracle.NumberofNodesInSubgraph() << std::endl;
    if (oracle.NumberofNodesInSubgraph() == 0) {
        return 0.0;
    }
    double epsilon = query.GetEpsilon();
    size_t sample_number_threshold = ceil((double) 8 / (epsilon * epsilon));
    size_t vc_size = 0;
    for (size_t i = 0; i < sample_number_threshold; ++i) {
        size_t node_id = oracle.SampleNode();
        if (InVertexcover(node_id, oracle)) {
            vc_size++;
        }
    }

    std::cout << "Vertex cover problem completed! The solution of vertex-cover: " << vc_size << std::endl;
    matching_.clear();
    vertex_cover_.clear();
    return (double)(vc_size) / (double)(sample_number_threshold);
}

bool Graph::InVertexcover(size_t node_id, const Oracle& oracle) {
    if (vertex_cover_.count(node_id) != 0) {
        return vertex_cover_[node_id];
    }
    const Node& node = nodes_[node_id];
    for (const Edge& edge : node.edges_) {
        if (oracle.InSubgraph(edge.v_)) {
            if (InMatching(node_id, edge, oracle)) {
                vertex_cover_[node_id] = true;
                return true;
            }
        }
    }
    vertex_cover_[node_id] = false;
    return false;
}

bool Graph::InMatching(size_t u, const Edge& edge, const Oracle& oracle) {
    if (matching_.count(edge.edge_id_) != 0) {
        return matching_[edge.edge_id_];
    }

    const Node& node_u = nodes_[u];
    const Node& node_v = nodes_[edge.v_];
    int k_1 = 0, k_2 = 0;
    while (node_u.edges_[k_1] < edge || node_v.edges_[k_2] < edge) {
        if (node_u.edges_[k_1] < node_v.edges_[k_2]) {
            if (oracle.InSubgraph(node_u.edges_[k_1].v_)) {
                if (InMatching(u, node_u.edges_[k_1], oracle)) {
                    matching_[edge.edge_id_] = false;
                    return false;
                }
            }
            k_1++;
        } else {
            if (oracle.InSubgraph(node_v.edges_[k_2].v_)) {
                if (InMatching(edge.v_, node_v.edges_[k_2], oracle)) {
                    matching_[edge.edge_id_] = false;
                    return false;
                }
            }
            k_2++;
        }
    }
    matching_[edge.edge_id_] = true;
    return true;
}

}  // namespace dcr