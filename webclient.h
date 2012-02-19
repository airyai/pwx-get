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
        
        class BufferDataWriter : public DataWriter {
        public:
            BufferDataWriter() : DataWriter() {}
            virtual size_t write(char *ptr, size_t size, size_t nmemb) {
                _d.append(ptr, size*nmemb);
            }
            virtual void clear() { _d.clear(); }
            string &data() { return _d; }
        protected:
            string _d;
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
        void setRange(const string &range);
        const string &getRange() const throw() { return _range; }
        void setHeaderOnly(bool headerOnly);
        bool getHeaderOnly() const throw() { return _headerOnly; }
        
        bool valid() const throw() { return curl; }
        void reset();
        CURLcode perform();
        
        const string errorMessgae() const throw() { return string(_errmsg.c_str()); }
        void clearError() throw() { _errmsg[0] = 0; }
        
        DataWriter &dataWriter() throw() { return _writer; }
        int getHttpCode();
        long getResponseLength();
        const string getResponseUrl();
        
    protected:
        CURL *curl;
        DataWriter &_writer;
        size_t _sheetSize;
        string _buffer, _errmsg;
        string _url, _proxy, _proxyServer, _cookies, _range;
        long _proxyType;
        bool _headerOnly;
        long long _contentLength;
        
        static size_t write_body(char *ptr, size_t size, size_t nmemb, void *userdata);
        static size_t write_header(char *ptr, size_t size, size_t nmemb, void *userdata);
    };
}

#endif	/* WEBCLIENT_H */

