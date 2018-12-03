//
// Created by mingweiliu on 2018/11/30.
//

#include "route/MyRouteHandler.h"

namespace MF {
    namespace Route{

        int32_t MyRegisterHandler::doHandler(const std::unique_ptr<Protocol::MyMessage> &request,
                                             std::unique_ptr<Protocol::MyMessage> &response,
                                             std::shared_ptr<Server::MyContext> context) {
            auto req = dynamic_cast<MyRouteMessage*>(request.get());
            LOG(INFO) << "receive register message: "
            << req->getPayload<RegisterReq>()->ShortDebugString() << std::endl;
            auto rspMsg = std::unique_ptr<RegisterRsp>(new RegisterRsp());
            rspMsg->set_code(kResultCodeSuccess);
            rspMsg->set_nodeid("test_node_id");
            auto rsp = dynamic_cast<Protocol::MyMessage*>(
                    new MyRouteMessage(kCommandCodeRegister, std::move(rspMsg)));
            response = std::move(std::unique_ptr<Protocol::MyMessage>(rsp));
            return kHandleResultSuccess;
        }

        int32_t MyOperateHandler::doHandler(const std::unique_ptr<Protocol::MyMessage> &request,
                                            std::unique_ptr<Protocol::MyMessage> &response,
                                            std::shared_ptr<Server::MyContext> context) {
            auto req = dynamic_cast<MyRouteMessage*>(request.get());
            LOG(INFO) << "receive operate message: "
                      << req->getPayload<OperateReq>()->ShortDebugString() << std::endl;
            auto rspMsg = std::unique_ptr<OperateRsp>(new OperateRsp());
            rspMsg->set_code(kResultCodeSuccess);
            rspMsg->set_nodeid("test_node_id");
            rspMsg->set_status(req->getPayload<OperateRsp>()->status());
            auto rsp = dynamic_cast<Protocol::MyMessage*>(
                    new MyRouteMessage(kCommandCodeOperate, std::move(rspMsg)));
            response = std::move(std::unique_ptr<Protocol::MyMessage>(rsp));
            return kHandleResultSuccess;
        }

        int32_t MyPullTableHandler::doHandler(const std::unique_ptr<Protocol::MyMessage> &request,
                                              std::unique_ptr<Protocol::MyMessage> &response,
                                              std::shared_ptr<Server::MyContext> context) {
            auto req = dynamic_cast<MyRouteMessage*>(request.get());
            LOG(INFO) << "receive pull table message: "
                      << req->getPayload<PullTableReq>()->ShortDebugString() << std::endl;
            auto rspMsg = std::unique_ptr<PullTableRsp>(new PullTableRsp());
            rspMsg->set_version(1);
            rspMsg->set_strategy(kStrategyRoundRoad);
            auto rsp = dynamic_cast<Protocol::MyMessage*>(
                    new MyRouteMessage(kCommandCodePullTable, std::move(rspMsg)));
            response = std::move(std::unique_ptr<Protocol::MyMessage>(rsp));
            return kHandleResultSuccess;
        }

        int32_t MyHeartbeatHandler::doHandler(const std::unique_ptr<Protocol::MyMessage> &request,
                                              std::unique_ptr<Protocol::MyMessage> &response,
                                              std::shared_ptr<Server::MyContext> context) {

            auto req = dynamic_cast<MyRouteMessage*>(request.get());
            LOG(INFO) << "receive heartbeat message: "
                      << req->getPayload<Heartbeat>()->ShortDebugString() << std::endl;
            auto rspMsg = std::unique_ptr<Heartbeat>(new Heartbeat());
            auto rsp = dynamic_cast<Protocol::MyMessage*>(
                    new MyRouteMessage(kCommandCodeHeartBeat, std::move(rspMsg)));
            response = std::move(std::unique_ptr<Protocol::MyMessage>(rsp));
            return kHandleResultSuccess;
        }
    }
}