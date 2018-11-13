//
//  mytimeprovider.cc
//  MF
//
//  Created by mingweiliu on 17/1/16.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#include <chrono>
#include <iomanip>

#include "MyTimeProvider.h"

namespace MF {
    uint32_t MyTimeProvider::now() {
        return (uint32_t)(std::chrono::time_point_cast<std::chrono::seconds>(
                std::chrono::system_clock::now()).time_since_epoch().count());
    }
    
    uint64_t MyTimeProvider::nowms() {
        return (std::chrono::time_point_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now()).time_since_epoch().count());
    }
    
    std::tm MyTimeProvider::nowgmtm() {
        std::time_t now = std::time_t(nullptr);
        return *std::gmtime(&now);
    }
    
    std::tm MyTimeProvider::nowlocaltm() {
        std::time_t now = std::time_t(nullptr);
        return *std::localtime(&now);
    }
}
