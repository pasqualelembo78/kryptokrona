// Copyright (c) 2012-2017, The MevaCoin developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "tcp_connector.h"
#include <cassert>
#include <stdexcept>

#include <fcntl.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <syst/interrupted_exception.h>
#include <syst/ipv4_address.h>
#include "dispatcher.h"
#include "error_message.h"
#include "tcp_connection.h"

namespace syst
{

    namespace
    {

        struct TcpConnectorContextExt : public OperationContext
        {
            int connection;
        };

    }

    TcpConnector::TcpConnector() : dispatcher(nullptr)
    {
    }

    TcpConnector::TcpConnector(Dispatcher &dispatcher) : dispatcher(&dispatcher), context(nullptr)
    {
    }

    TcpConnector::TcpConnector(TcpConnector &&other) : dispatcher(other.dispatcher)
    {
        if (other.dispatcher != nullptr)
        {
            assert(other.context == nullptr);
            context = nullptr;
            other.dispatcher = nullptr;
        }
    }

    TcpConnector::~TcpConnector()
    {
    }

    TcpConnector &TcpConnector::operator=(TcpConnector &&other)
    {
        dispatcher = other.dispatcher;
        if (other.dispatcher != nullptr)
        {
            assert(other.context == nullptr);
            context = nullptr;
            other.dispatcher = nullptr;
        }

        return *this;
    }

    TcpConnection TcpConnector::connect(const Ipv4Address &address, uint16_t port)
    {
        assert(dispatcher != nullptr);
        assert(context == nullptr);
        if (dispatcher->interrupted())
        {
            throw InterruptedException();
        }

        std::string message;
        int connection = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (connection == -1)
        {
            message = "socket failed, " + lastErrorMessage();
        }
        else
        {
            sockaddr_in bindAddress;
            bindAddress.sin_family = AF_INET;
            bindAddress.sin_port = 0;
            bindAddress.sin_addr.s_addr = INADDR_ANY;
            if (bind(connection, reinterpret_cast<sockaddr *>(&bindAddress), sizeof bindAddress) != 0)
            {
                message = "bind failed, " + lastErrorMessage();
            }
            else
            {
                int flags = fcntl(connection, F_GETFL, 0);
                if (flags == -1 || fcntl(connection, F_SETFL, flags | O_NONBLOCK) == -1)
                {
                    message = "fcntl failed, " + lastErrorMessage();
                }
                else
                {
                    sockaddr_in addressData;
                    addressData.sin_family = AF_INET;
                    addressData.sin_port = htons(port);
                    addressData.sin_addr.s_addr = htonl(address.getValue());
                    int result = ::connect(connection, reinterpret_cast<sockaddr *>(&addressData), sizeof addressData);
                    if (result == -1)
                    {
                        if (errno == EINPROGRESS)
                        {

                            ContextPair contextPair;
                            TcpConnectorContextExt connectorContext;
                            connectorContext.interrupted = false;
                            connectorContext.context = dispatcher->getCurrentContext();
                            connectorContext.connection = connection;

                            contextPair.readContext = nullptr;
                            contextPair.writeContext = &connectorContext;

                            epoll_event connectEvent;
                            connectEvent.events = EPOLLOUT | EPOLLRDHUP | EPOLLERR | EPOLLONESHOT;
                            connectEvent.data.ptr = &contextPair;
                            if (epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_ADD, connection, &connectEvent) == -1)
                            {
                                message = "epoll_ctl failed, " + lastErrorMessage();
                            }
                            else
                            {
                                context = &connectorContext;
                                dispatcher->getCurrentContext()->interruptProcedure = [&]
                                {
                                    TcpConnectorContextExt *connectorContext1 = static_cast<TcpConnectorContextExt *>(context);
                                    if (!connectorContext1->interrupted)
                                    {
                                        if (close(connectorContext1->connection) == -1)
                                        {
                                            throw std::runtime_error("TcpListener::stop, close failed, " + lastErrorMessage());
                                        }

                                        connectorContext1->interrupted = true;
                                        dispatcher->pushContext(connectorContext1->context);
                                    }
                                };

                                dispatcher->dispatch();
                                dispatcher->getCurrentContext()->interruptProcedure = nullptr;
                                assert(dispatcher != nullptr);
                                assert(connectorContext.context == dispatcher->getCurrentContext());
                                assert(contextPair.readContext == nullptr);
                                assert(context == &connectorContext);
                                context = nullptr;
                                connectorContext.context = nullptr;
                                if (connectorContext.interrupted)
                                {
                                    throw InterruptedException();
                                }

                                if (epoll_ctl(dispatcher->getEpoll(), EPOLL_CTL_DEL, connection, NULL) == -1)
                                {
                                    message = "epoll_ctl failed, " + lastErrorMessage();
                                }
                                else
                                {
                                    if ((connectorContext.events & (EPOLLERR | EPOLLHUP)) != 0)
                                    {
                                        int result = close(connection);
                                        if (result)
                                        {
                                        }
                                        assert(result != -1);

                                        throw std::runtime_error("TcpConnector::connect, connection failed");
                                    }

                                    int retval = -1;
                                    socklen_t retValLen = sizeof(retval);
                                    int s = getsockopt(connection, SOL_SOCKET, SO_ERROR, &retval, &retValLen);
                                    if (s == -1)
                                    {
                                        message = "getsockopt failed, " + lastErrorMessage();
                                    }
                                    else
                                    {
                                        if (retval != 0)
                                        {
                                            message = "getsockopt failed, " + lastErrorMessage();
                                        }
                                        else
                                        {
                                            return TcpConnection(*dispatcher, connection);
                                        }
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        return TcpConnection(*dispatcher, connection);
                    }
                }
            }

            int result = close(connection);
            if (result)
            {
            }
            assert(result != -1);
        }

        throw std::runtime_error("TcpConnector::connect, " + message);
    }

}
