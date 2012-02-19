/* 
 * File:   webclient.cpp
 * Author: pwx
 * 
 * Created on 2012年2月18日, 下午7:37
 */

#include "webclient.h"
#include "exceptions.h"
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

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
    WebClient::WebClient(WebClient::DataWriter &writer, size_t sheetSize) : curl(curl_easy_init()),
            _writer(writer), _sheetSize(sheetSize), _buffer(), _errmsg(), _url(), _proxy(), 
            _proxyServer(), _cookies(), _range(), _proxyType(0), _headerOnly(false), _contentLength(-1) {
        // create curl object
        if (!curl) {
            throw WebError("CURL object cannot be initialized.");
        }
        _errmsg.resize(CURL_ERROR_SIZE);
        
        // default settings
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 256L);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, _errmsg.c_str());
        
        // settings for timeout
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // fix multi-thread
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L); 
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, 512L); // 0.5kb/s is lowest
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, 10L); // wait for 10s
        
        // write cache
        _buffer.reserve(sheetSize);
        _buffer.clear();
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_body);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION , &write_header);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
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
    
    void WebClient::setRange(const string &range) {
        _range = range;
        curl_easy_setopt(curl, CURLOPT_RANGE, _range.empty()? NULL: _range.c_str());
    }
    
    void WebClient::reset() {
        setUrl("");
        setProxy("");
        setCookies("");
        setRange("");
    }
    
    CURLcode WebClient::perform() {
        _contentLength = -1;
        return curl_easy_perform(curl);
    }
    
    size_t WebClient::write_body( char *ptr, size_t size, size_t nmemb, void *userdata) {
        return static_cast<WebClient*>(userdata)->_writer.write(ptr, size, nmemb);
    }
    
    
    size_t WebClient::write_header(char *ptr, size_t size, size_t nmemb, void *userdata) {
        const int MAX_CONSIDER = 1000;
        WebClient* wc = static_cast<WebClient*>(userdata);
        size_t headerSize = size*nmemb;

        do {
            if (headerSize > MAX_CONSIDER) break;
            string header = string(ptr, headerSize);
            
            // Content-Length
            if (boost::istarts_with(header, "Content-Length:")) {
                header.substr(15);
                boost::trim(header);
                try {
                    wc->_contentLength = boost::lexical_cast<long long>(header);
                } catch (...) {
                    wc->_contentLength = -1;
                }
            } else if (boost::istarts_with(header, "HTTP ") 
                    && header.size() > 5 && (header[5] >= '0' && header[5] <= '9')) {
                // Fast test if is HTTP \d+ .*
                wc->_contentLength = -1;
            }
        } while (false);
        
        return headerSize;
    }
    
    // get response info
    int WebClient::getHttpCode() {
        long code;
        if (!curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code)) {
            return -1;
        }
        return code;
    }
    
    long WebClient::getResponseLength() {
        return _contentLength;
    }
    
    const string WebClient::getResponseUrl() {
        char buffer[2048];
        if (!curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, buffer))
            return string();
        return string(buffer);
    }
}
