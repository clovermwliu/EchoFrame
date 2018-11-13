//
//  mycommon.h
//  MF
//
//  Created by mingweiliu on 17/1/18.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#ifndef mycommon_h
#define mycommon_h

#include <stdio.h>
#include <string>
#include <vector>
#include <type_traits>
#include <locale>

#include "MyException.h"

#include <glog/logging.h>

namespace MF {
    
    /// 用于类型的转换
    class MyCast {
    public:
        
        class MyCastException : public MyException {
        public:
            explicit MyCastException(const std::string& str, int32_t error = 0)
            : MyException(str, error) {
                
            }

            ~MyCastException() override = default;
        };
        /**
         *  @brief 将const T 类型转转换成T类型, 不能转换非引用和指针之外的类型，不能用于func类型转换
         *
         *  @param v 源值
         *
         *  @return 转换之后的值
         */
        template <typename T> static T* to_none_const(const T* v) {
            return const_cast<T*>(v);
        }
        
        template <typename T> static T& to_none_const(const T& v) {
            return const_cast<T&>(v);
        }
        
        
        //类型转换
        /**
         *  @brief 转换数字类型(不是同一类型的数字), 包含的类型参考c11标准手册
         *
         *  @param v 原值
         *
         *  @return 转换之后的值
         */
        template<typename T, typename F>
        static typename std::enable_if<
                std::is_integral<T>::value
                && std::is_integral<F>::value
                && !std::is_same<std::decay<T>, std::decay<F>>::value, T>::type to(const F& v) {
            //1. 检查F的值是超过了T的上线
            if (v > std::numeric_limits<T>::max()) {
                LOG(ERROR) << "numeric limits overflow" << std::endl;
                return 0x0;
            }
            
            //2. 进行转换
            return static_cast<T>(v);
        }

        /**
         *  @brief 转换数字类型(不是同一类型的数字), 包含的类型参考c11标准手册
         *
         *  @param v 原值
         *
         *  @return 转换之后的值, 如果超过了最大值，那么返回0
         */
        template<typename T, typename F>
        static typename std::enable_if<
                std::is_floating_point<T>::value
                && std::is_floating_point<F>::value
                && !std::is_same<std::decay<T>, std::decay<F>>::value, T>::type to(const F& v) {
            //1. 检查F的值是超过了T的上线
            if (v > std::numeric_limits<T>::max()) {
                LOG(ERROR) << "numeric limit overflow" << std::endl;
                return 0x0;
            }
            
            //2. 进行转换
            return static_cast<T>(v);
        }
        
        /**
         *  @brief 转换两个完全相同的类型
         *
         *  @param v 原值
         *
         *  @return 转换之后的值
         */
        template<typename T, typename F>
        static typename std::enable_if<std::is_same<std::decay<T>, std::decay<F>>::value, T>::type to(const F& v) {
            return v;
        }
        
        /**
         *  @brief 数字型转换成std::string
         *
         *  @param v 数字
         *
         *  @return 字符串
         */
        template<typename T, typename F>
        static typename std::enable_if<std::is_integral<F>::value
        && std::is_same<T, std::string>::value, T>::type to(const F& v) {
            return std::to_string(v);
        }
        
        /**
         *  @brief 数字型转换成std::string
         *
         *  @param v 数字
         *
         *  @return 字符串
         */
        template<typename T, typename F>
        static typename std::enable_if<std::is_floating_point<F>::value
        && std::is_same<T, std::string>::value, T>::type to(const F& v) {
            return std::to_string(v);
        }
        
        /**
         *  @brief char*转换成std::string
         *
         *  @param v 数字
         *
         *  @return 字符串
         */
        template<typename T, typename F>
        static typename std::enable_if<std::is_same<const char*, typename std::decay<F>::type >::value, T>::type to(F v) {
            return std::string(v);
        }
        
        template<typename T, typename F>
        static typename std::enable_if<std::is_same<char*, typename std::decay<F>::type >::value, T>::type to(F v) {
            return std::string(v);
        }
        
        /**
         *  @brief std::string 转换成 unsigned数字
         *
         *  @param v 字符串
         *
         *  @return 数字
         */
        template<typename T, typename F>
        static typename std::enable_if<
                std::is_same<F, std::string>::value
                && std::is_integral<T>::value
                && std::is_unsigned<T>::value, T>::type to(const F& v) {

            try {
                //1. 转换成unsgiend long long
                auto tmp = std::stoull(v);
                //2. 转换成对应的数字
                return MyCast::to<T>(tmp);
            } catch (std::out_of_range& e) {
                LOG(ERROR) << "out of range:" << e.what() << std::endl;
            } catch (std::invalid_argument& e) {
                LOG(ERROR) << "invalid argument:" << e.what() << std::endl;
            }
            return 0x0;
        }
        
        /**
         *  @brief std::string 转换成 signed数字
         *
         *  @param v 字符串
         *
         *  @return 数字
         */
        template<typename T, typename F>
        static typename std::enable_if<
                std::is_same<F, std::string>::value
                && std::is_integral<T>::value
                && std::is_signed<T>::value, T>::type to(const F& v) {
            try {
                //1. 转换成unsgiend long long
                auto tmp = std::stoll(v);
                //2. 转换成对应的数字
                return MyCast::to<T>(tmp);
            } catch (std::out_of_range& e) {
                LOG(ERROR) << "out of range: " << e.what() << std::endl;
            } catch (std::invalid_argument& e) {
                LOG(ERROR) << "invalid argument: " << e.what() << std::endl;
            }
        }
        
        /**
         *  @brief std::string 转换成 浮点型数字
         *
         *  @param v 字符串
         *
         *  @return 数字
         */
        template<typename T, typename F>
        static typename std::enable_if<std::is_same<F, std::string>::value
        && std::is_floating_point<T>::value, T>::type to(const F& v) {
            try {
                //1. 转换成unsgiend long long
                auto tmp = std::stold(v);
                //2. 转换成对应的数字
                return MyCast::to<T>(tmp);
            } catch (std::out_of_range& e) {
                LOG(ERROR) << "out of range: " << e.what() << std::endl;
            } catch (std::invalid_argument& e) {
                LOG(ERROR) << "invalid argument: " << e.what() << std::endl;
            }
        }
    };
    
    class MyCommon {
    public:
        
        /**
         *  @brief trim 掉指定的字符串
         *
         *  @param str 需要trim的字符串
         *  @param c   trim掉的字符串
         *
         *  @return trim之后的字符串
         */
        static std::string trim(const std::string& str, const std::string& c = " ");
        
        /**
         *  @brief trim左边的固定字符串
         *
         *  @param str 源字符串
         *  @param c   需要trim的字符串
         *
         *  @return trim之后的字符串
         */
        static std::string trimleft(const std::string& str, const std::string& c = " ");
        
        /**
         *  @brief trim右边的固定字符串
         *
         *  @param str 源字符串
         *  @param c   需要trim的字符串
         *
         *  @return trim之后的字符串
         */
        static std::string trimright(const std::string& str, const std::string& c = " ");
        
        /**
         *  @brief 将字符串中某一个字符串替换成另外一个字符串
         *
         *  @param str       源字符串
         *  @param o         需要替换的字符串
         *  @param r         被替换的字符串
         *  @param recursion 替换所有找到的结果
         *
         *  @return 替换之后的字符串
         */
        static std::string replace(const std::string& str, const std::string& o, const std::string& r, bool recursion = true);
        
        /**
         *  @brief 计算字符串中某个字符的个数
         *
         *  @param str 字符串
         *  @param c   字符
         *
         *  @return 个数
         */
        static uint32_t count(const std::string& str, char c);
        
        /**
         *  @brief 对字符串做hash
         *
         *  @param str 源字符串
         *
         *  @return hash code
         */
        static std::size_t hash(const std::string& str);
        
        /**
         *  @brief 将字符串拆分成某种格式的容器, 容器必须支持push_back操作
         *
         *  @param str          源字符串
         *  @param delimiter    分隔符
         *  @param ignore_blank 忽略纯空格字符串
         *
         *  @return 拆分的容器
         */
        template <typename T>
        static std::vector<T> split(const std::string& str, const std::string& delimiter = " ", bool ignore_blank = true) {
            std::vector<T> rv;
            
            std::size_t pos = 0;
            std::size_t last_pos = 0;
            std::string tmp;
            bool exit = false;
            while (!exit) {
                pos = str.find(delimiter, last_pos);
                if (pos == std::string::npos) { //最后一个单词
                    tmp = str.substr(last_pos, str.length() - last_pos);
                    exit = true;
                } else {
                    tmp = str.substr(last_pos, pos - last_pos);
                    pos += 1;
                    last_pos = pos;
                }
                
                if (ignore_blank && tmp.empty()) {
                    continue;
                }
                
                rv.push_back(strto<T>(tmp));
            }
            
            return rv;
        }
        
        /**
         *  @brief 将字符串拆分成某种格式的容器, 容器必须支持push_back操作
         *
         *  @param str          源字符串
         *  @param delimiter    分隔符
         *  @param ignore_blank 忽略纯空格字符串
         *
         *  @return 拆分的容器
         */
        template <typename T, template<class, class> class D>
        static D<T, std::allocator<T>> splitto(const std::string& str, const std::string& delimiter = " ", bool ignore_blank = true) {
            D<T, std::allocator<T>> rv;
            
            std::size_t pos = 0;
            std::size_t last_pos = 0;
            std::string tmp;
            bool exit = false;
            while (!exit) {
                pos = str.find(delimiter, last_pos);
                if (pos == std::string::npos) { //最后一个单词
                    tmp = str.substr(last_pos, str.length() - last_pos);
                    exit = true;
                } else {
                    tmp = str.substr(last_pos, pos - last_pos);
                    pos += 1;
                    last_pos = pos;
                }
                
                if (ignore_blank && tmp.empty()) {
                    continue;
                }
                
                rv.push_back(strto<T>(tmp));
            }
            
            return rv;
        }
        /**
         *  @brief 将T类型的值转换成字符串
         *
         *  @param v 源值
         *
         *  @return 转换后的值
         */
        template<typename T> static std::string tostr(const T& v) {
            return MyCast::to<std::string>(v);
        }
        
        /**
         *  @brief 将字符串类型转换成T类型, 如果转换失败，则返回0
         *
         *  @param v 源值
         *
         *  @return 转换后的值
         */
        template<typename T> static T strto(const std::string& v) {
            try {
                return MyCast::to<T>(v);
            } catch (MyCast::MyCastException& e) {
                return 0;
            }
        }
        /**
         *  @brief 将字符串转换成大学
         *
         *  @param str 源字符串
         *
         *  @return 转换后的字符串
         */
        static std::string upper(const std::string& str) {
            std::string rv = str;
            const auto& f = std::use_facet<std::ctype<char> >(std::locale());
            f.toupper(const_cast<char*>(&(rv[0])), &rv[0] + rv.size());
            return rv;
        }
        
        /**
         *  @brief 将字符串转换成小写
         *
         *  @param str 源字符串
         *
         *  @return 转换后的字符串
         */
        static std::string lower(const std::string& str) {
            std::string rv = str;
            const auto& f = std::use_facet<std::ctype<char> >(std::locale());
            f.tolower(const_cast<char*>(&(rv[0])), &rv[0] + rv.size());
            return rv;
        }
        
        /**
         *  @brief snowflake算法生成一个唯一的序列号
         *  @param wid 工作机器序列号
         *  @return 序列号 0 --> 生成失败 
         */
        static uint64_t snowflake(uint32_t wid, uint32_t sid);
        
        /**
         *  @brief 读取一个文件
         *
         *  @param file 文件路径
         *
         *  @return 读取到的内容
         */
        static std::string loadfile(const std::string& file);
        
        /**
         *  @brief 将字符串保存到文件
         *
         *  @param file 文件
         *  @param str  字符串
         */
        static void savefile(const std::string& file, const std::string& str);
        
        /**
         *  @brief 判断是否是digit
         *
         *  @param c 字符
         *
         *  @return true 是digit false 不是digit
         */
        static bool isdigit(char c) {
            return c >= '0' && c <= '9';
        }
        
        /**
         *  @brief 判断是否是digit字符串
         *
         *  @param str 字符串
         *
         *  @return true 是 false 不是
         */
        static bool is_digit_str(const std::string& str) {
            for (auto i = 0;i < str.length(); ++i) {
                if (!isdigit(str[i])) {
                    return false;
                }
            }
            
            return true;
        }
        
        /**
         *  @brief 是否是英文字符
         *
         *  @param c 字符
         *
         *  @return true 是 false 不是
         */
        static bool ischaracter(char c) {
            return (c>= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        }
        
    protected:
    private:
    };
}

#endif /* mycommon_h */

