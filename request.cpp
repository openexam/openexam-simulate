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
// File:   request.cpp
// Date:   5 Feb 2016
// --------------------------------------------------------------------------
//

#include "request.hpp"

#include <curl/curl.h>
#include <curl/easy.h>

//
// Number of clients initialized.
//
int Request::clients = 0;

//
// Lock access to shared resources.
//
class Lock
{
    typedef pthread_mutex_t Mutex;

public:
    Lock(bool acquire = true) : locked(false)
    {
        if(acquire) {
            Acquire();
        }
    }

    ~Lock()
    {
        if(locked) {
            Release();
        }
    }

    bool IsLocked() const
    {
        return locked;
    }

    void Acquire()
    {
        if(pthread_mutex_lock(&lock) != 0) {
            perror("pthread_mutex_lock");
        } else {
            locked = true;
        }
    }

    void Release()
    {
        if(pthread_mutex_unlock(&lock) != 0) {
            perror("pthread_mutex_unlock");
        } else {
            locked = false;
        }
    }

private:
    bool locked;
    static Mutex lock;
};

Lock::Mutex Lock::lock = PTHREAD_MUTEX_INITIALIZER;

Request::Request()
    : target("http://localhost/openexam"), timeout(Timeout)
{
    Initialize();
}

Request::Request(const std::string &target)
    : target(target), timeout(Timeout)
{
    Initialize();
}

Request::~Request()
{
    Cleanup();
}

//
// Initialize CURL.
//
void Request::Initialize()
{
    //
    // Try acquire lock:
    //
    Lock lock;

    if(!lock.IsLocked()) {
        throw Exception("Failed acquire lock");
    }

    //
    // First client will initialize globally.
    //
    if(!clients) {
        if(curl_global_init(CURL_GLOBAL_ALL) != 0) {
            throw Exception("Failed initialize CURL globally");
        }
    }

    //
    // Initialize the library handle. We do this inside the
    // critical section to have correct client count.
    //
    if(!(curl = curl_easy_init())) {
        throw Exception("Failed initialize CURL library handle");
    }

    //
    // One more client:
    //
    ++clients;
}

//
// Cleanup CURL.
//
void Request::Cleanup()
{
    //
    // Try acquire lock:
    //
    Lock lock;

    if(!lock.IsLocked()) {
        throw Exception("Failed acquire lock");
    }

    //
    // One less client:
    //
    --clients;

    //
    // Cleanup library handle.
    //
    if(curl) {
        curl_easy_cleanup(curl);
    }

    //
    // Last client will cleanup globally.
    //
    if(!clients) {
        curl_global_cleanup();
    }

}

bool Request::HasClients()
{
    //
    // Try acquire lock:
    //
    Lock lock;

    if(!lock.IsLocked()) {
        throw Exception("Failed acquire lock");
    }

    return clients != 0;
}

int Request::GetClients()
{
    //
    // Try acquire lock:
    //
    Lock lock;

    if(!lock.IsLocked()) {
        throw Exception("Failed acquire lock");
    }

    return clients;
}

//
// Send request with payload to uri using default role.
//
const Response * Request::Send(std::string uri, std::string payload)
{
    return Send(uri, role, payload);
}

//
// Send request with payload to uri using given role.
//
const Response * Request::Send(std::string uri, std::string role, std::string payload)
{
    CURLcode code;
    curl_easy_reset(curl);

    if((code = curl_easy_setopt(curl, CURLOPT_COOKIE, (std::string("PHPSESSID=" + session)).c_str())) != CURLE_OK) {
        throw Exception("Failed set cookie (session ID) option", code);
    }
    if((code = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Response::WriteCallback)) != CURLE_OK) {
        throw Exception("Failed set write function option", code);
    }
    if((code = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response)) != CURLE_OK) {
        throw Exception("Failed set write data option", code);
    }
    if((code = curl_easy_setopt(curl, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_0)) != CURLE_OK) {
        throw Exception("Failed set HTTP protocol 1.0 option", code);
    }

    if((code = curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout)) != CURLE_OK) {
        throw Exception("Failed set timeout option", code);
    }
    if((code = curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 1)) != CURLE_OK) {
        throw Exception("Failed set low speed limit option", code);
    }
    if((code = curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 60)) != CURLE_OK) {
        throw Exception("Failed set low speed time option", code);
    }

    std::string url(target + "/" + role + "/" + uri);
    if((code = curl_easy_setopt(curl, CURLOPT_URL, url.c_str())) != CURLE_OK) {
        throw Exception("Failed set URL option", code);
    }
    if((code = curl_easy_setopt(curl, CURLOPT_POST, 1)) != CURLE_OK) {
        throw Exception("Failed set POST option", code);
    }
    if((code = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str())) != CURLE_OK) {
        throw Exception("Failed set POST fields option", code);
    }

    //
    // Return response from here is OK because curl_easy_perform() is
    // synchronious and will not return until request is completed.
    //
    if((code = curl_easy_perform(curl)) != CURLE_OK) {
        throw Exception("Failed call curl_easy_perform()", code);
    } else {
        return &response;
    }
}
