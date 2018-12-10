//
//  mypropertytree.cc
//  MyFramewrok
//
//  Created by mingweiliu on 2017/3/6.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#include "MyPropertyTree.h"

namespace MF {
    
    MyPropertyTree::MyPropertyTree() {
        root_ = std::make_shared<MyPropertyTreeNode>("root");
    }
    
    MyPropertyTree::MyPropertyTree(const MyPropertyTree&  r) {
        //多个property tree 共享一个root
        root_ = r.root_;
    }
    
    MyPropertyTree::~MyPropertyTree() {
        root_.reset();
    }
    
    MyPropertyTree& MyPropertyTree::operator = (const MyPropertyTree& r) {
        if (this == &r) {
            return *this;
        }
        root_ = r.root_;
        return *this;
    }
    
    uint32_t MyPropertyTree::array_size(const std::string& path, const std::string& k) const {
        uint32_t rv = 0;
        //1. 解析path
        auto vpath = MyCommon::split<std::string>(path, "/");
        if (vpath.empty()) {
            throw MyException("path is empty");
        }
        
        //2. 获取数据
        std::shared_ptr<MyPropertyTreeNode> child = root_;
        for (auto it = vpath.begin(); it != vpath.end(); ++it) {
            child = child->get_child(*it);
            if (!child) { //没有找到
                child = nullptr;
                break;
            }
        }
        
        //3. 获取数据长度
        if (child && child->is_array()) {
            rv = child->array_size();
        }
        
        return rv;
    }
    
    std::vector<std::string> MyPropertyTree::GetAllKeys(const std::string& path) {
        std::vector<std::string> v;
        
        //1. 解析path
        auto vpath = MyCommon::split<std::string>(path, "/");
        if (vpath.empty()) {
            return v;
        }
        
        //2. 获取数据
        std::shared_ptr<MyPropertyTreeNode> child = root_;
        for (auto it = vpath.begin(); it != vpath.end(); ++it) {
            child = child->get_child(*it);
            if (!child) { //没有找到
                child = nullptr;
                break;
            }
        }
        
        //3. 获取child包含的节点
        for (auto it = child->childs.begin(); it != child->childs.end(); ++it) {
            v.push_back(it->second->propertyPtr->k);
        }
        
        return v;
    }
}
