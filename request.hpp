// Copyright (C) 2016  Anders Lövgren, BMC Computing Department, Uppsala University
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// --------------------------------------------------------------------------
// Author: Anders Lövgren <anders.lovgren@bmc.uu.se>
//
// File:   request.hpp
// Date:   5 Feb 2016
// --------------------------------------------------------------------------
//

#ifndef REQUEST_HPP
#define REQUEST_HPP

#include <string>
#include <curl/curl.h>
#include "response.hpp"

//
// Send AJAX request using cURL.
//
class Request
{
public:
    struct Exception : public std::exception
    {
        Exception(const std::string &message) : message(message), curlerr(CURLE_OK) {}
        Exception(const std::string &message, CURLcode curlerr) : message(message), curlerr(curlerr) {}
        std::string message;    // Message string.
        CURLcode curlerr;       // Error message from CURL.
    };

    const int Timeout = 60;     // Default timeout.

    Request();
    Request(const std::string &target);
    ~Request();

    void SetTarget(const std::string &target);
    const std::string & GetTarget() const;

    void SetSession(const std::string &session);
    const std::string & GetSession() const;

    void SetRole(const std::string &role);
    const std::string & GetRole() const;

    void SetTimeout(int timeout);
    int  GetTimeout() const;

    static bool HasClients();
    static int  GetClients();

    const Response * Send(std::string uri, std::string payload);
    const Response * Send(std::string uri, std::string role, std::string payload);

private:
    void Initialize();
    void Cleanup();

    Response response;

    std::string target;     // Target base URL.
    std::string session;    // Session string.
    std::string role;       // Role to use in request.

    int timeout;            // Request timeout.
    CURL *curl;             // CURL library handle.

    static int clients;     // Number of clients initialized.
};

inline void Request::SetTarget(const std::string &target)
{
    this->target = target;
}

inline const std::string &Request::GetTarget() const
{
    return target;
}

inline void Request::SetSession(const std::string &session)
{
    this->session = session;
}

inline const std::string &Request::GetSession() const
{
    return session;
}

inline void Request::SetRole(const std::string &role)
{
    this->role = role;
}

inline const std::string &Request::GetRole() const
{
    return role;
}

inline void Request::SetTimeout(int timeout)
{
    this->timeout = timeout;
}

inline int Request::GetTimeout() const
{
    return timeout;
}

#endif // REQUEST_HPP
