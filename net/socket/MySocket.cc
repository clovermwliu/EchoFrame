//
//  mysocket.cc
//  MF
//
//  Created by mingweiliu on 17/1/16.
//  Copyright © 2017年 mingweiliu. All rights reserved.
//

#include "MySocket.h"
#include <errno.h>

namespace MF {
    namespace Socket {

        MySocket::~MySocket() {
            close();

        }
        //初始化socket
        int32_t MySocket::socket(uint8_t domain, int32_t type, int32_t protocol) {
            fd = ::socket(domain, type, protocol);
            this->domain = domain;
            this->type = type;
            this->protocol = protocol;
            return fd > 0 ? 0 : -1;
        }

        int32_t MySocket::bind(const std::string &host, uint16_t port) {
            sockaddr_in addr;
            bzero(&addr, sizeof(addr));
            
            addr.sin_family = domain;
            addr.sin_addr.s_addr = inet_addr(host.c_str());
            addr.sin_port = htons(port);
            
            local_.host = host;
            local_.port = port;
            
            //设置reuse
            setReuseAddr(true);
            
            return ::bind(fd, (struct sockaddr*)(&addr), sizeof(struct sockaddr));
        }
        
        int32_t MySocket::bind(uint16_t port) {
            return bind("0.0.0.0", port);
        }
        
        int32_t MySocket::listen(uint32_t backlog){
            return ::listen(fd, backlog);
        }
        
        int32_t MySocket::connect(const std::string &host, uint16_t port) {
            struct sockaddr_in addr;
            bzero(&addr, sizeof(struct sockaddr_in));
            
            addr.sin_family = domain;
            addr.sin_addr.s_addr = inet_addr(host.c_str());
            addr.sin_port = htons(port);
            
            remote.host = host;
            remote.port = port;
            
            int32_t rv = ::connect(fd, (struct sockaddr*)(&addr), sizeof(struct sockaddr));
            
            if (rv != 0) { //如果出错了，则抛出异常
                auto err = errno;
                if (err == EINPROGRESS) {
                    isConnected = false;
                    rv = 0;
                } else {
                    rv = -1;
                }
            } else {
                isConnected = true;
            }

            return rv;
        }

        int32_t MySocket::getConnectResult() const {
            int32_t error = 0;
            uint32_t len = sizeof(error);
            getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len);
            return error;
        }
        
        int32_t MySocket::accept(MySocket* client) {
            
            struct sockaddr_in raddr;
            bzero(&raddr, sizeof(struct sockaddr_in));
            socklen_t len = sizeof(raddr);
            
            auto fd = ::accept(this->fd,(struct sockaddr*)(&raddr), &len);
            if (fd == 0) {
                return -1;
            }
            client->fd = fd;
            client->isConnected = true;
            
            client->remote.host = inet_ntoa(raddr.sin_addr);
            client->remote.port = ntohs(raddr.sin_port);
            return 0;
        }
        
        int32_t MySocket::write(void* buffer, uint32_t length) {
            return static_cast<int32_t >(::write(fd, buffer, length));
        }
        
        int32_t MySocket::writeTo(const std::string &host, uint16_t port, void *buffer, uint32_t length) {
            
            struct sockaddr_in addr;
            bzero(&addr, sizeof(addr));
            addr.sin_family = domain;
            addr.sin_addr.s_addr = inet_addr(host.c_str());
            addr.sin_port = htons(port);
            
            return static_cast<int32_t >(
                    writeTo((const struct sockaddr *) (&addr), buffer, length));
        }
        
        int32_t MySocket::writeTo(const struct sockaddr *addr, void *buffer, uint32_t length) {
            return static_cast<int32_t >(
                    ::sendto(fd, buffer, length, 0, (const struct sockaddr*)(&addr), sizeof(struct sockaddr)));
        }
        
        int32_t MySocket::read(void *buffer, uint32_t size) {
            return static_cast<int32_t >(::read(fd, buffer, size));
        }
        
        int32_t MySocket::readFrom(struct sockaddr *addr, socklen_t *addr_len, void *buffer, uint32_t size) {
            return static_cast<int32_t >(::recvfrom(fd, buffer, size, 0, addr, addr_len));
        }

        int32_t MySocket::peekFrom(struct sockaddr *addr, socklen_t *addrLen) {
            char buf[1] = {0};
            uint32_t len = 1;
            return static_cast<uint32_t >(::recvfrom(fd, buf, len, MSG_PEEK, addr, addrLen));
        }
        
        void MySocket::setSockOpt(int32_t level, int32_t option_name, int32_t value) {
            setsockopt(fd, level, option_name, (void*)(&value), sizeof(value));
        }
    }
    
    
}
