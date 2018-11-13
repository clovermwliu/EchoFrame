//
//  mypropertytree.h
//  MyFramewrok
//
//  Created by mingweiliu on 2017/3/6.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef mypropertytree_h
#define mypropertytree_h

#include <string>
#include <map>

#include "util/MyVariant.h"

namespace MF {
    //节点类型
    typedef enum enumPropertyType : uint32_t {
        kPropertyTypeString = 0,
        kPropertyTypeBool = 1,
        kPropertyTypeNull = 2,
        kPropertyTypeIntegral = 3,
        kPropertyTypeFloating = 4,
        kPropertyTypeArray = 5,
        kPropertyTypeTrunk = 99, //树干节点
    }PropertyType;
    
    //Property值
    typedef MyVariant<uint64_t, bool, double, std::string> PropertyValue;
    
    /**
     *  @brief 定义property
     */
    struct MyProperty {
        std::string k_; //字符串的key
        PropertyValue v_; //value
        template<typename T>
        MyProperty(const std::string& k, const T& v) {
            k_ = k;
            v_.set(v);
        }
    };
    //属性树节点
    struct MyPropertyTreeNode : std::enable_shared_from_this<MyPropertyTreeNode>{
        std::shared_ptr<MyProperty> property_; //属性
        PropertyType type_{kPropertyTypeTrunk}; //属性
        std::shared_ptr<MyPropertyTreeNode> parent_{nullptr}; //父节点
        std::map<std::string, std::shared_ptr<MyPropertyTreeNode>> childs_;//子节点
        
        MyPropertyTreeNode(const std::string& key) {
            property_ = std::make_shared<MyProperty>(key, false);
        }
        
        //写入一个节点
        template<typename T>
        void set(const T& v) {
            if(std::is_same<typename std::decay<T>::type, bool>::value) {
                type_ = kPropertyTypeBool;
            } else if (std::is_same<typename std::decay<T>::type, std::string>::value) {
                type_ = kPropertyTypeString;
            } else if (std::is_floating_point<typename std::decay<T>::type>::value) {
                type_ = kPropertyTypeFloating;
            } else if (std::is_integral<typename std::decay<T>::type>::value) {
                type_ = kPropertyTypeIntegral;
            } else {
                throw MyException("property is not supported");
            }
            property_->v_.set(v);
        }
        
        //写入array
        template<typename T>
        void push_back(const T& v) {
            if(type_ != kPropertyTypeTrunk && type_ != kPropertyTypeArray)
                throw MyException("Property is not array"); //非array
            
            //1. 获取索引
            uint32_t idx = MyCast::to<uint32_t>(childs_.size());
            
            //2. 写入一个子节点
            std::shared_ptr<MyPropertyTreeNode> child = std::make_shared<MyPropertyTreeNode>(MyCommon::tostr(idx));
            child->set<typename std::decay<T>::type>(v);
            child->parent_ = shared_from_this();
            childs_[child->property_->k_] = child;
            type_ = kPropertyTypeArray;
        }
        
        //获取值
        template<typename T>
        typename std::decay<T>::type get() {
            if(type_ == kPropertyTypeTrunk
               &&  type_ == kPropertyTypeArray
               && type_ == kPropertyTypeNull)
                throw MyException("Property incorrect"); //错误的类型
            return property_->v_.as<T>();
        }
        
        //获取array
        template<typename T>
        typename std::decay<T>::type at(uint32_t idx) {
            if(type_ != kPropertyTypeArray)
                throw MyException("Property is not array");
            
            auto it = childs_.find(MyCommon::tostr(idx));
            if(it == childs_.end())
                throw MyException("index overflow");
            
            return it->second->get<typename std::decay<T>::type>();
        }
        
        template<typename T>
        std::vector<typename std::decay<T>::type> get_array() {
            if(type_ != kPropertyTypeArray)
                throw MyException("Property is not array");
            
            std::vector<typename std::decay<T>::type> v;
            for(auto it = childs_.begin(); it != childs_.end(); ++it) {
                v.push_back(it->second->get<T>());
            }
            
            return v;
        }
        
        uint32_t array_size() const {
            return static_cast<uint32_t>(childs_.size());
        }
        
        //增加一个child
        void add_child(std::shared_ptr<MyPropertyTreeNode> child) {
            child->parent_ = shared_from_this();
            childs_[child->property_->k_] = child;
        }
        
        //获取一个节点
        std::shared_ptr<MyPropertyTreeNode> get_child(const std::string& k) {
            std::shared_ptr<MyPropertyTreeNode> child = nullptr;
            auto it = childs_.find(k);
            if(it != childs_.end()) { //获取子节点
                child = it->second;
            }
            
            return child;
        }
        
        std::shared_ptr<MyPropertyTreeNode> parent() {
            return parent_;
        }
        
        
        //判断类型
        bool is_integral() const {
            return type_ == kPropertyTypeIntegral;
        }
        
        bool is_bool() const {
            return type_ == kPropertyTypeBool;
        }
        
        bool is_string() const {
            return type_ == kPropertyTypeString;
        }
        
        bool is_floating() const {
            return type_ == kPropertyTypeFloating;
        }
        
        bool is_null() const {
            return type_ == kPropertyTypeNull;
        }
        
        bool is_array() const {
            return type_ == kPropertyTypeArray;
        }
        
        bool is_trunk() const {
            return type_ == kPropertyTypeTrunk;
        }
    };
    
    //属性树
    class MyPropertyTree {
    public:
        /**
         *  @brief 构造函数
         */
        MyPropertyTree();
        
        MyPropertyTree(const MyPropertyTree& r);
        
        /**
         *  @brief 析构函数
         */
        ~MyPropertyTree();
        
        /**
         *  @brief 赋值函数
         *
         *  @param r 右值
         *
         *  @return 左值
         */
        MyPropertyTree& operator = (const MyPropertyTree& r);
        
        /**
         *  @brief 获取一个配置项的值
         *
         *  @param path 配置项路径
         *
         *  @return 数据值
         */
        template<typename T>
        typename std::decay<T>::type get(const std::string& path, const std::string& k, const T& v);
        
        //获取array
        template<typename T>
        typename std::decay<T>::type at(const std::string& path, const std::string& k, uint32_t idx);
        
        template<typename T>
        std::vector<typename std::decay<T>::type> get_array(const std::string& path, const std::string& k);
        
        
        /**
         *  @brief 获取root节点
         *
         *  @return root节点
         */
        MyPropertyTreeNode* root() {
            return root_.get();
        }
        
        /**
         *  @brief 设置一个key
         *
         *  @param path 路径path
         *  @param k    key
         *  @param v    value
         */
        template<typename T>
        void set(const std::string& path, const std::string& k, const T& v);
        
        template<typename T>
        void append(MyPropertyTreeNode* parent, const std::string& k, const T& v);
        
        /**
         *  @brief array增加一个值
         *
         *  @param path 路径
         *  @param k    key
         *  @param v    value
         */
        template<typename T>
        void push_back(const std::string& path, const std::string& k, const T& v);
        
        //获取array元素个数
        uint32_t array_size(const std::string& path, const std::string& k) const ;
        
        /**
         *  @brief 获取一个path下所有的property, 包括leaf和trunk
         *
         *  @param path 路径
         *
         *  @return 所有的property
         */
        std::vector<std::string> GetAllKeys(const std::string& path);
        
        //获取数据
        bool get_bool(const std::string& path, const std::string& k, bool v = false) {
            return get<bool>(path, k, v);
        }
        
        uint64_t get_integral(const std::string& path, const std::string& k, uint64_t v = 0) {
            return get<uint64_t>(path, k, v);
        }
        
        std::string get_string(const std::string& path, const std::string& k, const std::string& v = "") {
            return get<std::string>(path, k, v);
        }
        
        double get_floating(const std::string& path, const std::string& k, double d = 0) {
            return get<double>(path, k, d);
        }
        
        //array相关操作
        bool array_get_bool(const std::string& path, const std::string& k, uint32_t idx) {
            return at<bool>(path, k, idx);
        }
        
        uint64_t array_get_integral(const std::string& path, const std::string& k, uint32_t idx) {
            return at<uint64_t>(path, k, idx);
        }
        
        std::string array_get_string(const std::string& path, const std::string& k, uint32_t idx) {
            return at<std::string>(path, k, idx);
        }
        
        double array_get_floating(const std::string& path, const std::string& k, uint32_t idx) {
            return at<double>(path, k, idx);
        }
        
    protected:
    private:
        std::shared_ptr<MyPropertyTreeNode> root_; //属性树
    };
    
    //取值函数
    template<typename T>
    typename std::decay<T>::type MyPropertyTree::get(const std::string& path, const std::string& k, const T& v) {
        typename std::decay<T>::type rv = v; // 默认值
        
        //1. 解析path
        auto vpath = MyCommon::split<std::string>(path, "/");
        if (vpath.empty()) {
            return rv;
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
        
        //3. 判断是否找到了对应的节点
        if (!child) {
            return v;
        }
        
        //4. 获取子节点
        child = child->get_child(k);
        if (child ) { //找到了
            rv = child->property_->v_.as<T>();
        }
        return rv;
    }
    
    template<typename T>
    typename std::decay<T>::type MyPropertyTree::at(const std::string& path, const std::string& k, uint32_t idx) {
        typename std::decay<T>::type rv;
        
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
        
        //3. 判断是否找到了对应的节点
        if (!child) {
            throw MyException("path not found");
        }
        
        //4. 获取子节点
        child = child->get_child(k);
        if (!child ) { //找到了
            throw MyException("key not found");
        }
        rv = child->at<typename std::decay<T>::type>(idx);
        return rv;
    }
    
    template<typename T>
    std::vector<typename std::decay<T>::type> MyPropertyTree::get_array(const std::string& path, const std::string& k) {
        std::vector<typename std::decay<T>::type> rv;
        
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
        
        //3. 判断是否找到了对应的节点
        if (!child) {
            throw MyException("path not found");
        }
        
        //4. 获取子节点
        child = child->get_child(k);
        if (!child ) { //找到了
            throw MyException("key not found");
        }
        
        rv = child->get_array<typename std::decay<T>::type>();
        
        return rv;
    }
    
    
    //赋值函数
    template<typename T>
    void MyPropertyTree::set(const std::string& path, const std::string& k, const T& v) {
        //1. 解析path
        auto vpath = MyCommon::split<std::string>(path, "/");
        if (vpath.empty()) {
            return;
        }
        
        //2. 生成child
        std::shared_ptr<MyPropertyTreeNode> child = nullptr;
        std::shared_ptr<MyPropertyTreeNode> parent = root_;
        for (auto it = vpath.begin(); it != vpath.end(); ++it) {
            child = parent->get_child(*it);
            if (!child) { //没有子节点
                child = std::make_shared<MyPropertyTreeNode>(*it);
                parent->add_child(child);
            }
            parent = child;
        }
        
        //3. 生成property
        auto node = std::make_shared<MyPropertyTreeNode>(k);
        node->set(v);
        parent->add_child(node);
    }
    
    template<typename T>
    void MyPropertyTree::append(MyPropertyTreeNode* parent, const std::string& k, const T& v){
        //3. 生成property
        auto node = std::make_shared<MyPropertyTreeNode>(k);
        node->set(v);
        parent->add_child(node);
    }
    
    template<typename T>
    void MyPropertyTree::push_back(const std::string& path, const std::string& k, const T& v) {
        //1. 解析path
        auto vpath = MyCommon::split<std::string>(path, "/");
        if (vpath.empty()) {
            return;
        }
        
        //2. 生成child
        std::shared_ptr<MyPropertyTreeNode> child = nullptr;
        std::shared_ptr<MyPropertyTreeNode> parent = root_;
        for (auto it = vpath.begin(); it != vpath.end(); ++it) {
            child = parent->get_child(*it);
            if (!child) { //没有子节点
                child = std::make_shared<MyPropertyTreeNode>(*it);
                parent->add_child(child);
            }
            parent = child;
        }
        
        //3. 获取property
        auto node = parent->get_child(k);
        if (!node) {
            node = std::make_shared<MyPropertyTreeNode>(k);
        }
        node->push_back(v);
        parent->add_child(node);
    }
    
}

#endif /* mypropertytree_h */
