//
// Created by mingweiliu on 2018/11/8.
//

#include "net/server/MyFilter.h"

namespace MF {
    namespace Server {

        MyFilter::MyFilter(MyFilter *nextFilter) : nextFilter(nextFilter) {}

        MyFilter::MyFilter() {
            this->nextFilter = nullptr;
        }

        MyFilter::~MyFilter() {
            if (this->nextFilter != nullptr) {
                delete(this->nextFilter);
            }
        }

        bool MyFilter::doFilter(MF::Protocol::MyMessage *request, std::shared_ptr<MyContext> context) {
            bool rv = false;
            if (this->nextFilter != nullptr) {
                rv = this->nextFilter->invoke(request, context);
            }

            return rv && this->invoke(request, context);
        }
    }
}
