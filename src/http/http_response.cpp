// Copyright (c) 2012-2017, The CryptoNote developers, The Bytecoin developers
// Copyright (c) 2014-2018, The Monero Project
// Copyright (c) 2018-2019, The TurtleCoin Developers
// Copyright (c) 2019, The Kryptokrona Developers
//
// Please see the included LICENSE file for more information.

#include "http_response.h"

#include <stdexcept>

namespace
{

    const char *getStatusString(mevacoin::HttpResponse::HTTP_STATUS status)
    {
        switch (status)
        {
        case mevacoin::HttpResponse::STATUS_200:
            return "200 OK";
        case mevacoin::HttpResponse::STATUS_404:
            return "404 Not Found";
        case mevacoin::HttpResponse::STATUS_500:
            return "500 Internal Server Error";
        default:
            throw std::runtime_error("Unknown HTTP status code is given");
        }

        return ""; // unaccessible
    }

    const char *getErrorBody(mevacoin::HttpResponse::HTTP_STATUS status)
    {
        switch (status)
        {
        case mevacoin::HttpResponse::STATUS_404:
            return "Requested url is not found\n";
        case mevacoin::HttpResponse::STATUS_500:
            return "Internal server error is occurred\n";
        default:
            throw std::runtime_error("Error body for given status is not available");
        }

        return ""; // unaccessible
    }

} // namespace

namespace mevacoin
{

    HttpResponse::HttpResponse()
    {
        status = STATUS_200;
        headers["Server"] = "CryptoNote-based HTTP server";
        headers["Connection"] = "keep-alive";
    }

    void HttpResponse::setStatus(HTTP_STATUS s)
    {
        status = s;

        if (status != HttpResponse::STATUS_200)
        {
            setBody(getErrorBody(status));
        }
    }

    void HttpResponse::addHeader(const std::string &name, const std::string &value)
    {
        headers[name] = value;
    }

    void HttpResponse::setBody(const std::string &b)
    {
        body = b;
        if (!body.empty())
        {
            headers["Content-Length"] = std::to_string(body.size());
        }
        else
        {
            headers.erase("Content-Length");
        }
    }

    std::ostream &HttpResponse::printHttpResponse(std::ostream &os) const
    {
        os << "HTTP/1.1 " << getStatusString(status) << "\r\n";

        for (auto pair : headers)
        {
            os << pair.first << ": " << pair.second << "\r\n";
        }
        os << "\r\n";

        if (!body.empty())
        {
            os << body;
        }

        return os;
    }

} // namespace mevacoin
