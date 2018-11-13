//
//  mytimeprovider.h
//  MF
//
//  Created by mingweiliu on 17/1/16.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef mytimeprovider_h
#define mytimeprovider_h

#include "MySingleton.h"

namespace MF {
    
    /// 单例模式提供时间相关的函数
    class MyTimeProvider  {
    public:
        /**
         *  @brief 获取当前时间戳，以秒为单位
         *
         *  @return 当前时间戳
         */
        static uint32_t now();
        
        /**
         *  @brief 当前时间戳，单位毫秒
         *
         *  @return 时间戳
         */
        static uint64_t nowms();
        
        /**
         *  @brief 获取当前时间的tm结构 gm时间
         *
         *  @return tm结构
         */
        static std::tm nowgmtm();
        
        /**
         *  @brief 获取当前时间, 本地时间
         *
         *  @return 当前时间
         */
        static std::tm nowlocaltm();
        
    protected:
    };
}

#endif /* mytimeprovider_h */
