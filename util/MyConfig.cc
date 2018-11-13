//
//  myconfig2.cc
//  MyFramewrok
//
//  Created by mingweiliu on 2017/3/10.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#include "MyConfig.h"

namespace MF {
    bool MyXmlConfigFileParser::parse(const std::string& path, MyPropertyTree* tree) {
        bool rv = false;
        tree_ = tree;
        
        rv = doc_.LoadFile(path.c_str()) == XML_SUCCESS; //解析配置文件
        if (rv) {
            //解析第一行
            (void)visit(doc_.RootElement(), tree_->root());
        }
        
        return rv;
    }
    
    void MyXmlConfigFileParser::save(const std::string& path, MyPropertyTree* tree) {
        //访问tree
    }
    
    void MyXmlConfigFileParser::visit(XMLNode* node, MyPropertyTreeNode* parent) {
        if (!node || !parent) { //访问结束了
            return;
        }
        
        if (node->ToElement()) {
            auto v = node->ToElement();
            if (v->GetText()) {
                tree_->append<std::string>(parent, v->Name(), v->GetText());
                if (!node->NextSibling()) {
                    visit(node->Parent()->NextSibling(), parent->parent().get()); //解析parent的上层同级节点
                } else {
                    visit(node->NextSibling(), parent);
                }
            } else {
                if (!node->FirstChild()) {
                    visit(node->Parent()->NextSibling(), parent->parent().get()); //解析parent的上层同级节点
                } else {
                    //新建一个parent节点
                    auto next = std::make_shared<MyPropertyTreeNode>(node->Value());
                    parent->add_child(next);
                    visit(node->FirstChild(), next.get());
                }
            }
        }
        return;
    }
    
}
