#ifndef DCR_CORE_SUBSET_QUERY_H_
#define DCR_CORE_SUBSET_QUERY_H_

#include <string>
#include <vector>
#include <exception>
#include "core/table.h"


namespace dcr {

class SubsetQuery {
public:
    /*
        query language satisfy at least following grammar: 
            S = (A and | or S) | A | A and S | A or S
            A = attr >|>=|=|!=|<=|< B,
            B = [all letters except >|>=|=|!=|<=|<|and|or ]
            attr = [all letters except >|>=|=|!=|<=|< ]
    */

	virtual bool Satisfy(Record& r);

	virtual std::string ToString();

	SubsetQuery(const std::string& str) {
		query_ = str;
		if (query.size() > 0) {
			root_ = ConstructQueryTree(query_);
		} else {
			root_ = nullptr;
			query_ = "1 = 1";
		}
	}

private:

    class Tuple {
    public:
        bool operator==(const Tuple& other) const {
            return this == &other;
        }

        Tuple() = default;

        std::string attr_name_;
        std::string val_;
        int op_;
    };


    class Node {
    public:
        Node() = default;

        bool operator==(const Node& other) const {
            return this == &other;
        }

        Node(std::string& attr, std::string& val, int op): attr_(attr), val_(val), op_(op) {
            concate_ = -1;
            left_ = nullptr;
            right_ = nullptr;
        }

        explicit Node(int concate) {
            concate_ = concate;
            attr_ = "";
            val_ = "";
            op_ = -1;
            left_ = nullptr;
            right_ = nullptr;
        }

        bool Satisfy(const Record& r) const {
            bool ret = true;
            if (concate_ == -1) {
                std::string attr_val = r.GetField(attr_);
                switch (op_) {
                    case 0: {  // >
                        if (attr_val <= val_) {
                            ret = false;
                        }
                        break;
                    }
                    case 1: {  // >= 
                        if (attr_val < val_) {
                            ret = false;
                        }
                        break;
                    }
                    case 2: {  // = 
                        if (attr_val != val_) {
                            ret = false;
                        }
                        break;
                    }
                    case 3: { // !=
                        if (attr_val == val_) {
                            ret = false;
                        }
                        break;
                    }
                    case 4: { // <= 
                        if (attr_val > val_) {
                            ret = false;
                        }
                        break;
                    }
                    case 5: { // <
                        if (attr_val >= val_) {
                            ret = false;
                        }
                        break;
                    }
                }
            } else if (concate_ == 0) {
                ret = left_->Satisfy(r) && right_->Satisfy(r);
            } else if (concate_ == 1) {
                ret = left_->Satisfy(r) || right_->Satisfy(r);
            }

            return ret;
        }

        std::string attr_;
        std::string val_;
        int op_;
        int concate_;

        Node* left_;
        Node* right_;
    };

    Node* ConstructQueryTree(const std::string& query);

    Tuple Parse(const std::string& s);

    Node* root_;
    std::vector<Node*> nodes_;
    std::string query_;
};

} // dcr 