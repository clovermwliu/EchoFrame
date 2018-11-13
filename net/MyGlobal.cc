//
//  myglobal.cpp
//  MyFramewrok
//
//  Created by mingweiliu on 2017/2/21.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#include "MyGlobal.h"

namespace MF {
    //tcp -h 127.0.0.1 -p 12345 -n test_server -t 3000
    MyEndpoint MyEndpointHelper::StringToEndpoint(const std::string& str) {
        MyEndpoint ep;
        auto vec = MyCommon::split<std::string>(str);
        if (vec.empty()) {
            return ep;
        }
        
        //1. 解析协议类型
        auto proto = MyCommon::lower(MyCommon::trim(vec[0]));
        if (proto == "tcp") {
            ep.protocol_ = kTCP;
        } else if (proto == "udp") {
            ep.protocol_ = kUDP;
        } else if (proto == "unix") {
            ep.protocol_ = kUNIX;
        } else {
            return ep;
        }
        
        if (vec.size() - 1 % 2 ==  0) { //如果参数个数不对，那么就退出
            return ep;
        }
        
        for (uint32_t i = 1; i < vec.size(); i += 2) {
            auto k = MyCommon::lower(MyCommon::trim(MyCommon::trim(vec[i]), "-"));
            auto v = MyCommon::lower(MyCommon::trim(vec[i+1]));
            if (k == "h") {
                ep.host_ = v;
            } else if (k == "p") {
                ep.port_ = MyCommon::strto<uint16_t>(v);
            } else if (k == "n") {
                ep.name_ = MyCommon::trim(vec[i+1]);
            } else if (k == "t") {
                ep.timeout_ = MyCommon::strto<uint32_t>(v);
            }
        }
        
        return ep;
    }
    
    std::string MyEndpointHelper::EndpointToString(const MyEndpoint& ep) {
        std::string rv;
        
        if (ep.protocol_ == kTCP) {
            rv.append("tcp ");
        } else if (ep.protocol_ == kUDP) {
            rv.append("udp ");
        } else if (ep.protocol_ == kUNIX) {
            rv.append("unix ");
        }
        
        rv.append("-h ").append(ep.host_).append(" ");
        rv.append("-p ").append(MyCommon::tostr(ep.port_)).append(" ");
        rv.append("-n ").append(ep.name_).append(" ");
        rv.append("-t ").append(MyCommon::tostr(ep.timeout_)).append(" ");
        return rv;
    }
}
