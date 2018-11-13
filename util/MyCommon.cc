//
//  mycommon.cc
//  MF
//
//  Created by mingweiliu on 17/1/18.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#include "MyCommon.h"
#include "MyTimeProvider.h"
#include <fstream>

namespace MF {
    std::string MyCommon::trim(const std::string &str, const std::string &c) {
        return trimright(trimleft(str, c), c);
    }
    
    std::string MyCommon::trimleft(const std::string &str, const std::string& c) {
        
        std::string rv = str;
        
        while (true) {
            auto pos = rv.find(c);
            if (pos != 0) { //左侧不包含从0开始的c字符串
                break;
            }
            
            //找到了从0开始c
            rv.erase(pos, c.length());
        }
        
        return rv;
    }
    
    std::string MyCommon::trimright(const std::string& str, const std::string& c) {
        std::string rv = str;
        
        while (true) {
            auto pos = rv.rfind(c);
            if (pos == std::string::npos
                || pos + c.length() != rv.length()) { //如果c不是最后一个单词
                break;
            }
            
            //c是最后一个单词
            rv.erase(pos, c.length());
        }
        
        return rv;
    }
    
    std::string MyCommon::replace(const std::string& str, const std::string& o, const std::string& r, bool recursion) {
        std::string rv = str;
        
        std::size_t pos = 0;
        while (true) {
            pos = rv.find(o, pos);
            if (pos == std::string::npos) { //所有的字符串都已经替换完成了
                break;
            }
            
            //替换一个字符串
            rv.replace(pos, o.length(), r);
            pos += r.length(); //pos前移
        }
        
        return rv;
    }
    
    uint32_t MyCommon::count(const std::string& str, char c) {
        uint32_t rv = 0;
        for (auto it = str.begin(); it != str.end(); ++it) {
            if (*it == c) {
                rv++;
            }
        }
        return rv;
    }
    
    std::size_t MyCommon::hash(const std::string &str) {
        std::size_t rv = 0;
        std::hash<std::string> hasher;
        rv = hasher(str);
        return rv;
    }
    
    uint64_t MyCommon::snowflake(uint32_t wid, uint32_t sid) {
        if (wid > 1023 || sid > 4095) { //wid超出范围了
            return 0;
        }
        
        uint64_t uid = 0;
        
        //1. 获取当前时间
        uint64_t now = MyTimeProvider::nowms();
        uid = now << 22;
        
        //2, 设置wid
        uid |= (wid << 12);
        
        //3. 设置序列号
        uid |= (sid);
        
        return uid;
    }
    
    std::string MyCommon::loadfile(const std::string& file) {
        std::stringbuf sb;
        
        //1. 打开文件
        std::ifstream stream(file);
        if (!stream.good()) {
            throw MyException(strerror(errno));
        }
        
        //2. 读取数据
        stream >> &sb;
        
        //3. 返回数据
        return sb.str();
    }
    
    void MyCommon::savefile(const std::string& file, const std::string& str) {
        //1. 打开文件
        std::ofstream stream(file);
        if (!stream.good()) {
            throw MyException(strerror(errno));
        }
        
        //2. 写入数据
        stream << str;
    }
}
