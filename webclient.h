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
        class DataBuffer {
        public:
            DataBuffer(size_t capacity) : _cap(capacity), _d(new char[capacity]) {}
            DataBuffer(const DataBuffer &other) : _cap(other.capacity()),
                _d(new char[_cap]) { memcpy(_d, other.data(), _cap); }
            virtual ~DataBuffer() throw() { if (_d) { delete[] _d; } _d = NULL; _cap = 0; }
            size_t capacity() const throw() { return _cap; }
            char *data() { return _d; }
            const char *data() const { return _d; }
            void clear() { _d[0] = 0; }
        protected:
            char *_d;
            size_t _cap;
        };
        
        class DataWriter {
        public:
            virtual size_t write(char *ptr, size_t size, size_t nmemb) = 0;
            virtual void clear() = 0;
        };
        
        class BufferDataWriter : public DataWriter {
        public:
            BufferDataWriter() : DataWriter() {}
            virtual size_t write(char *ptr, size_t size, size_t nmemb) {
                _d.append(ptr, size*nmemb);
                return size*nmemb;
            }
            virtual void clear() { _d.clear(); }
            string &data() { return _d; }
        protected:
            string _d;
        };
        
        WebClient(DataWriter &writer, size_t sheetSize=DEFAULT_SHEET_SIZE);
        WebClient(const WebClient& orig);
        virtual ~WebClient();
        
        CURL *handle() throw() { return curl; }
        
        void setUrl(const string &url);
        const string &getUrl() const throw() { return _url; }
        void setProxy(const string &proxy);
        const string &getProxy() const throw() { return _proxy; }
        void setCookies(const string &cookies);
        const string &getCookies() const throw() { return _baseCookies; }
        void setRange(const string &range);
        const string &getRange() const throw() { return _range; }
        void setHeaderOnly(bool headerOnly);
        bool getHeaderOnly() const throw() { return _headerOnly; }
        void setVerbose(bool verbose);
        bool getVerbose() const throw() { return _verbose; }
        
        void setTimeout(long timeout);
        long getTimeout() const throw() { return _timeout; }
        void setConnectTimeout(long connectTimeout);
        long getConnectTimeout() const throw() { return _connectTimeout; }
        void setLowSpeedLimit(long lowSpeedLimit);
        long getLowSpeedLimit() const throw() { return _lowSpeedLimit; }
        void setLowSpeedTime(long lowSpeedTime);
        long getLowSpeedTime() const throw() { return _lowSpeedTime; }
        
        bool valid() const throw() { return curl; }
        void reset();
        bool perform(CURLcode *curlReturnCode = NULL);
        
        const string errorMessage() const throw() { return string(_errmsg.data()); }
        void clearError() throw() { _errmsg.clear(); }
        
        DataWriter &dataWriter() throw() { return _writer; }
        int getHttpCode();
        long getResponseLength();
        const string getResponseUrl();
        bool supportRange() { return _supportRange; }
        
    protected:
        CURL *curl;
        DataWriter &_writer;
        size_t _sheetSize;
        //DataBuffer _buffer;
        DataBuffer _errmsg;
        string _url, _proxy, _proxyServer, _baseCookies, _range;
        long _proxyType;
        bool _headerOnly, _verbose, _supportRange;
        long long _contentLength;
        long _timeout, _connectTimeout, _lowSpeedLimit, _lowSpeedTime;
        
        static size_t write_body(char *ptr, size_t size, size_t nmemb, void *userdata);
        static size_t write_header(char *ptr, size_t size, size_t nmemb, void *userdata);
    };
}

#endif	/* WEBCLIENT_H */

