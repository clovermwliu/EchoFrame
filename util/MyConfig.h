//
//  myconfig2.h
//  MyFramewrok
//
//  Created by mingweiliu on 2017/3/10.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myconfig2_h
#define myconfig2_h

#include <tinyxml2.h>
#include "MyCommon.h"
#include "MyPropertyTree.h"
#include "MyFactory.h"
#include "MySingleton.h"

namespace MF {
    using namespace tinyxml2;
    
    //配置文件解析器
    class MyConfigFileParser {
        
    public:
        /**
         *  @brief 解析配置文件
         *
         *  @param path 配置文件路径
         *  @param tree 属性树
         *
         *  @return true 成功 false 失败
         */
        virtual bool parse(const std::string& path, MyPropertyTree* tree) = 0;
        
        /**
         *  @brief 保存配置文件
         *
         *  @param path 路径
         *  @param tree 属性树
         */
        virtual void save(const std::string& path, MyPropertyTree* tree) = 0;
    protected:
        MyPropertyTree* tree_; //树
    private:
    };
    
    /// xml配置文件解析
    class MyXmlConfigFileParser : public MyConfigFileParser {
    public:
        virtual bool parse(const std::string& path, MyPropertyTree* tree) override;
        
        virtual void save(const std::string& path, MyPropertyTree* tree) override;
    protected:
        //访问xml节点
        void visit(XMLNode* node, MyPropertyTreeNode* parent);
    private:
        XMLDocument doc_; //xml解析器
    };
    
    class MyConfig {
    public:
        
        //构造解析器
        MyConfig(std::shared_ptr<MyConfigFileParser> parser) {
            parser_ = parser;
        }
        
        //解析配置文件
        bool LoadFile(const std::string& path) {
            return parser_->parse(path, &tree_);
        }
        /**
         *  @brief 获取属性树
         *
         *  @return 属性树
         */
        MyPropertyTree* operator -> () {
            return &tree_;
        }
        
        MyPropertyTree* operator *() {
            return &tree_;
        }
        
        /**
         *  @brief 保存配置文件
         *
         *  @param file 配置我恩建
         */
        void SaveFile(const std::string& file) {
            
        }
    protected:
    private:
        std::shared_ptr<MyConfigFileParser> parser_; //配置文件解析器
        MyPropertyTree tree_; //属性树
    };
    
    class MyConfigManager : public MySingleton<MyConfigManager> {
    public:
        MyConfigManager() {
            auto func = [] (std::shared_ptr<MyConfigFileParser> ptr) -> std::shared_ptr<MyConfig> {
                return std::make_shared<MyConfig>(ptr);
            };
            factory_.RegisterCreator(MAKE_NAME(MyConfig2), std::move(func));
        }
        
        //加载配置文件
        template<typename T, class... Args>
        std::shared_ptr<MyConfig> LoadFile(const std::string& path, Args... args) {
            std::shared_ptr<MyConfig> config = factory_.MakeObject(MAKE_NAME(MyConfig2), std::make_shared<T>(args...));
            if(!config->LoadFile(path)) {
                config.reset();
            }
            
            return config;
        }
    protected:
    private:
        typedef MyPointerFactory<std::string, std::shared_ptr<MyConfig>, std::shared_ptr<MyConfigFileParser>> MyConfigFactory;
      
        MyConfigFactory factory_; //工厂
    };
}

#endif /* myconfig2_h */
