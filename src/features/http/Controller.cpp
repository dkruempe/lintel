#include "base_library/features/http/Controller.h"
void Controller::registerMethods(std::shared_ptr<httplib::Server> &server) {
    for (const auto &[type, methods] : m_methods) {
        for (auto &method : methods) {
            switch (type) {
                case Get:
                    if (method.m_handler != nullptr) {
                        server->Get(method.m_pattern.c_str(), *method.m_handler);
                    }
                    break;
                case Put:
                    if (method.m_handler != nullptr) {
                        server->Put(method.m_pattern.c_str(), *method.m_handler);
                    } else if (method.m_handlerWithContentReader != nullptr) {
                        server->Put(method.m_pattern.c_str(),
                                    *method.m_handlerWithContentReader);
                    }
                    break;
                case Post:
                    if (method.m_handler != nullptr) {
                        server->Post(method.m_pattern.c_str(), *method.m_handler);
                    } else if (method.m_handlerWithContentReader != nullptr) {
                        server->Post(method.m_pattern.c_str(),
                                     *method.m_handlerWithContentReader);
                    }
                    break;
                case Delete:
                    if (method.m_handler != nullptr) {
                        server->Delete(method.m_pattern.c_str(), *method.m_handler);
                    } else if (method.m_handlerWithContentReader != nullptr) {
                        server->Delete(method.m_pattern.c_str(),
                                       *method.m_handlerWithContentReader);
                    }
                    break;
            }
        }
    }
}