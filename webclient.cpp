/* 
 * File:   webclient.cpp
 * Author: pwx
 * 
 * Created on 2012年2月18日, 下午7:37
 */

#include "webclient.h"
#include "exceptions.h"
#include <boost/algorithm/string.hpp>

namespace PwxGet {
    bool parseProxy(const string &proxy, string &optProxy, long &optType) {
        size_t m = proxy.find("://");
        
        optProxy = proxy;
        optType = CURLPROXY_HTTP;
        
        if (m == string::npos) {
            optType = CURLPROXY_HTTP;
        } else {
            string protocol = optProxy.substr(0, m);
            optProxy = optProxy.substr(m+3);
            if (protocol.empty() || protocol == "http") optType = CURLPROXY_HTTP;
            else if (protocol == "http10") optType = CURLPROXY_HTTP_1_0;
            else if (protocol == "socks4") optType = CURLPROXY_SOCKS4;
            else if (protocol == "socks4a") optType = CURLPROXY_SOCKS4A;
            else if (protocol == "socks5") optType = CURLPROXY_SOCKS5;
            else if (protocol == "socks5h") optType = CURLPROXY_SOCKS5_HOSTNAME;
            else return false;
        }
        
        m = optProxy.rfind("/");
        if (m != string::npos) {
            optProxy = optProxy.substr(0, m);
        }
        
        return !optProxy.empty();
    }
    
    // init & dispose
    WebClient::WebClient(WebClient::DataWriter &writer, size_t sheetSize) : _writer(writer),
            _sheetSize(sheetSize) {
        // create curl object
        curl = curl_easy_init();
        if (!curl) {
            throw WebError("CURL object cannot be initialized.");
        }
        _errmsg.resize(CURL_ERROR_SIZE);
        
        // default settings
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 256L);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, _errmsg.c_str());
        
        // write cache
        _buffer.reserve(sheetSize);
        _buffer.clear();
    }
    
    WebClient::~WebClient() {
        if (curl) {
            curl_easy_cleanup(curl);
            curl = NULL;
        }
    }
    
    // set & get parameters
    void WebClient::setUrl(const string& url) {
        _url = url;
        curl_easy_setopt(curl, CURLOPT_URL, _url.c_str());
    }
    
    void WebClient::setProxy(const string &proxy) {
        if (!parseProxy(proxy, _proxyServer, _proxyType)) {
            throw ArgumentError("proxy", proxy + " is not a valid proxy.");
        }
        curl_easy_setopt(curl, CURLOPT_PROXY, _proxyServer.c_str());
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE, _proxyType);
        _proxy = proxy;
    }
    
    void WebClient::setCookies(const string& cookies) {
        _cookies = cookies;
        curl_easy_setopt(curl, CURLOPT_COOKIE, _cookies.c_str());
    }
    
    void WebClient::reset() {
        setUrl("");
        setProxy("");
        setCookies("");
    }
    
    CURLcode WebClient::perform() {
        return curl_easy_perform(curl);
    }
}

