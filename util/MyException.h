//
//  myexception.h
//  MF
//
//  Created by mingweiliu on 17/1/16.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef myexception_h
#define myexception_h

#include <exception>
#include <string>
#include <errno.h>

namespace MF {
    class MyException : public std::exception {
    public:
        
    explicit MyException(const std::string& str, int32_t error = 0) {
        error_str_ = str;
        error_no_ = error;
    }
    
    ~MyException() = default;
    
    virtual const char* what() const _NOEXCEPT {
        return error_str_.c_str();
    }
    
    virtual int32_t error_no() const {
        return error_no_;
    }
    protected:
        std::string error_str_; //错误描述
        int32_t error_no_; //错误码
    };
}

#endif /* myexception_h */
