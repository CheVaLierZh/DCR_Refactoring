#ifndef DCR_CORE_ORACLE_H_
#define DCR_CORE_ORACLE_H_
#include <cstdlib>
#include <string>
#include <vector>
#include <utility>
// only supported with g++ >= 3.3.1
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>

#include "core/table.h"
#include "core/subset_query.h"

namespace dcr {

class Table;
class SubsetQuery;

class Oracle {

public:
    Oracle(const Table& table, const SubsetQuery& query) {
        std::vector<size_t> ret = table.Find(query);
        for (size_t node_idx: ret) {
            nodes_.insert(node_idx);
        }
    }

    bool InSubgraph(size_t node_id) const {
        return nodes_.find(node_id) != nodes_.end();
    }

    size_t SampleNode() const {
        return *nodes_.find_by_order(rand() % nodes_.size());
    }

    size_t NumberofNodesInSubgraph() const {
        return nodes_.size();
    }


private:
    __gnu_pbds::tree<size_t, 
                    __gnu_pbds::null_type, std::less<size_t>,
                    __gnu_pbds::rb_tree_tag,
                    __gnu_pbds::tree_order_statistics_node_update> nodes_;
};

}  // dcr
#endif  // DCR_CORE_ORACLE_H_