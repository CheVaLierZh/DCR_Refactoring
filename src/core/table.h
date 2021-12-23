#ifndef DCR_CORE_TABLE_H_
#define DCR_CORE_TABLE_H_

#include <unordered_map>
#include <string>
#include <algorithm>
#include <sstream>
#include <vector>
#include "core/subset_query.h"

namespace dcr {

class SubsetQuery;


class Record {
public:

    inline std::vector<std::string> GetMultiFields(const std::vector<std::string>& attrs) const {
        std::vector<std::string> ret;
        for (auto attr : attrs) {
            ret.push_back(GetField(attr));
        }
        return ret;
    }

    inline std::string GetField(std::string attr) const {
        return content_[attr];
    }

    std::string ToString() {
        std::stringstream ss;
        for (auto& kv: content_) {
            ss << kv.first << ": " << kv.second << "\n";
        }
        return ss.str();
    }

    inline size_t GetRowIndex() {
    	return row_idx_;
    }

    Record() = default;

    explicit Record(size_t row_idx, const std::unordered_map<std::string, std::string>& content): row_idx_(row_idx), content_(content) {}

private:
	std::unordered_map<std::string, std::string> content_;
	size_t row_idx_;
};


class FunctionalDependency {
public:
    bool IsConflict(const Record& a, const Record& b) const {
        vector<string> a_attr_vals = a.GetMultiFields(fd_.first);
        vector<string> b_attr_vals = b.GetMultiFields(fd_.first);
        if (a_attr_vals == b_attr_vals) {
            a_attr_vals.clear();
            b_attr_vals.clear();
            a_attr_vals = a.GetMultiFields(fd_.second);
            b_attr_vals = b.GetMultiFields(fd_.second);
            if (a_attr_vals != b_attr_vals) {
                return true;
            }
        }
        return false;
    }

    inline std::vector<std::string> GetLeftHandAttrs() const {
        return fd_.first;
    }

    inline std::vector<std::string> GetRightHandAttrs() const {
        return fd_.second;
    }

    FunctionalDependency(const std::vector<std::string>& lhs, const std::vector<std::string>& rhs) {
        fd_.first = lhs;
        fd_.second = rhs;
    }

    FunctionalDependency() = default;

private:

    std::pair<std::vector<std::string>, std::vector<std::string>> fd_;
};


class TableIterator {
public:
    virtual bool HasNext() = 0;

    virtual Record Next() = 0;
};


class Table {
public:

    virtual TableIterator GetIterator() = 0;

    virtual std::vector<size_t> FindConflict(const Record& r) = 0;

    inline std::vector<std::string> GetTableAttrbutes() const {
    	return attrs_;
    }

    inline void SetTableName(const std::string& tablename): tablename_(tablename) {}

    inline void LoadFunctionalDependencies(const std::vector<FunctionalDependency>& fds): fds_(fds) {}

    virtual std::vector<size_t> Find(SubsetQuery&) = 0;

    inline std::string GetTableName() {
        return tablename_;
    }

protected:
	std::unordered_map<std::string, std::string> schema_;
	std::vector<FunctionalDependency> fds_;
    std::string tablename_;
};

}  // dcr


#endif  // DCR_CORE_TABLE_H_