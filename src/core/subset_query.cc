#include <algorithm>
#include <stack>
#include <iostream>
#include "core/subset_query.h"

namespace dcr {
using std::string;
using std::vector;
using std::stack;

SubsetQuery::Node* SubsetQuery::ConstructQueryTree(const string& query) {
    stack<SubsetQuery::Node*> st1;
    stack<int> st2;

    int pivot = 0;
    int pos = 0;
    while (true) {
        unsigned int pos1 = query.find(" and ", pivot);
        unsigned int pos2 = query.find(" or ", pivot);
        unsigned int pos3 = query.find("(", pivot);
        unsigned int pos4 = query.find(")", pivot);
        pos = std::min(pos1, std::min(pos2, std::min(pos3, pos4)));
        
        if (pos >= query.size()) {
            pos = query.size();
            SubsetQuery::Tuple t = Parse(query.substr(pivot, pos-pivot));

            SubsetQuery::Node* node = new SubsetQuery::Node(t.attr_name_, t.val_, t.op_);
            st1.push(node);
            nodes_.push_back(node);
            break;
        }

        switch (query[pos]) { 
            case ' ': {
                if (st2.size() < 1) {
                    throw InvalidExpressionException(query.substr(0, pos));
                }
                int c = st2.top();
                if (c == 0 || c == 1) {
                    st2.pop();
                    if (st1.size() < 2) {
                        throw InvalidExpressionException(query.substr(0, pos));
                    }
                    SubsetQuery::Node* right = st1.top();
                    st1.pop();
                    SubsetQuery::Node* left = st1.top();
                    st1.pop();

                    SubsetQuery::Node* node = new SubsetQuery::Node(c);
                    node->left_ = left;
                    node->right_ = right;
                    st1.push(node);
                    nodes_.push_back(node);
                }

                pos = pos + 1;
                if (query[pos] == 'a') {
                    st2.push(0);
                    pivot = pos + 4;
                } else {
                    st2.push(1);
                    pivot = pos + 3;
                }
                break;
            }
            case '(': {
                st2.push(2);
                pivot = pos + 1;
                break;
            }
            case ')': {
                if (st2.size() < 1) {
                    throw InvalidExpressionException(query.substr(0, pos));
                }
                while (true) {
                    int c = st2.top();
                    if (c == 2) {
                        break;
                    }
                    st2.pop();
                    if (st1.size() < 2) {
                        throw InvalidExpressionException(query.substr(0, pos));
                    }
                    SubsetQuery::Node* right = st1.top();
                    st1.pop();
                    SubsetQuery::Node* left = st1.top();
                    st1.pop();

                    SubsetQuery::Node* node = new SubsetQuery::Node(c);
                    node->left_ = left;
                    node->right_ = right;
                    st1.push(node);
                    nodes_.push_back(node);
                    if (st2.size() < 1) {
                        throw InvalidExpressionException(query.substr(0, pos));
                    }
                }
                st2.pop();
                pivot = pos + 1;
                break;
            }
        }
    }

    if (st2.size() >= 1 || (st2.size() > 0 && st2.top() != 0 && st2.top() != 1)) {
        throw InvalidExpressionException(query.substr(0, pos));
    } else if (st2.size() == 1) {
        int c = st2.top();
        st2.pop();

        if (st1.size() < 2) {
            throw InvalidExpressionException(query.substr(0, pos));
        }
        SubsetQuery::Node* right = st1.top();
        st1.pop();
        SubsetQuery::Node* left = st1.top();
        st1.pop();

        SubsetQuery::Node* node = new SubsetQuery::Node(c);
        node->left_ = left;
        node->right_ = right;
        st1.push(node);
        nodes_.push_back(node);
    }

    SubsetQuery::Node* root = st1.top();
    st1.pop();
    return root;
}


SubsetQuery::Tuple SubsetQuery::Parse(const string& s) {
    SubsetQuery::Tuple t;
    int start = 0;
    int end = s.size();
    int tmp_pivot = -1;
    for (int i = start; i < end; ++i) {
        switch (s[i]) {
            case '>': {
                t.attr_name_ = TrimCopy(s.substr(start, i - start));
                if (s[i+1] == '=') {
                    t.op_ = 1;
                    tmp_pivot = i+2;
                } else {
                    t.op_ = 0;
                    tmp_pivot = i+1;
                }
                break;
            }
            case '=': {
                t.attr_name_ = TrimCopy(s.substr(start, i - start));
                t.op_ = 2;
                tmp_pivot = i+1;
                break;
            }
            case '!': {
                if (s[i+1] == '=') {
                    t.attr_name_ = TrimCopy(s.substr(start, i - start));
                    t.op_ = 3;
                    tmp_pivot = i+2;
                }
                break;
            }
            case '<': {
                if (s[i+1] == '=') {
                    t.attr_name_ = TrimCopy(s.substr(start, i - start));
                    t.op_ = 4;
                    tmp_pivot = i+2;
                } else {
                    t.attr_name_ = TrimCopy(s.substr(start, i - start));
                    t.op_ = 5;
                    tmp_pivot = i+1;
                }
                break;
            }
        }
        if (tmp_pivot != -1) {
            break;
        }
    }

    t.val_ = TrimCopy(s.substr(tmp_pivot, end - tmp_pivot));
    return t;
}


bool SubsetQuery::SatisfyRange(const Record& r) const {
    if (root_ == nullptr) {
        return true;    
    } else {
        return root_->Satisfy(r);
    }
}


SubsetQuery::~SubsetQuery() {
    for (SubsetQuery::Node* node : nodes_) {
        delete node;
    }
}


}  // dcr