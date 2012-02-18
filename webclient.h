/* 
 * File:   webclient.h
 * Author: pwx
 *
 * Created on 2012年2月18日, 下午7:37
 */

#ifndef WEBCLIENT_H
#define	WEBCLIENT_H

#include <curl/curl.h>
#include <string>
#include "filebuffer.h"

namespace PwxGet {
    using namespace std;
    
    /**
     * Parse the proxy string into curl opt.
     * @param proxy: Proxy string. Like http://127.0.0.1:3128/. See curl manual.
     * @param optProxy: Store the host & port part of proxy.
     * @param optType: Store the proxy type.
     * @return Input is valid or not.
     */
    bool parseProxy(const string &proxy, string &optProxy, long &optType);

    class WebClient {
    public:
        class DataWriter {
        public:
            virtual size_t write(char *ptr, size_t size, size_t nmemb);
            virtual void clear();
        };
        
        WebClient(DataWriter &writer, size_t sheetSize=DEFAULT_SHEET_SIZE);
        WebClient(const WebClient& orig);
        virtual ~WebClient();
        
        void setUrl(const string &url);
        const string &getUrl() const throw() { return _url; }
        void setProxy(const string &proxy);
        const string &getProxy() const throw() { return _proxy; }
        void setCookies(const string &cookies);
        const string &getCookies() const throw() { return _cookies; }
        
        bool valid() const throw() { return curl; }
        void reset();
        CURLcode perform();
        
        const string errorMessgae() const throw() {
            const char *msg = _errmsg.c_str();
            return string(msg, strlen(msg));
        }
        void clearError() throw () {
            _errmsg[0] = 0;
        }
    protected:
        CURL *curl;
        DataWriter &_writer;
        size_t _sheetSize;
        string _buffer, _errmsg;
        string _url, _proxy, _proxyServer, _cookies;
        long _proxyType;
    };
}

#endif	/* WEBCLIENT_H */

