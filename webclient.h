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
        	DataBuffer();
            DataBuffer(size_t capacity);
            DataBuffer(const DataBuffer &other);
            virtual ~DataBuffer() throw();
            inline size_t capacity() const throw() { return _cap; }
            inline char *data() throw() { return _d; }
            inline const char *data() const throw() { return _d; }
            inline size_t length() const throw() { return _len; }
            void clear();
            void resize(size_t capacity);

            void append(const void *data, size_t len);
            void safeAppend(const void *data, size_t len);
            template <typename T>
            void appendValue(const T& value) { append(&value, sizeof(T)); }
            template <typename T>
            void safeAppendValue(const T& value) { safeAppend(&value, sizeof(T)); }

            void get(void *dst, size_t start, size_t count, size_t *p=NULL);
            void safeGet(void *dst, size_t start, size_t count, size_t *p=NULL);
            void getString(size_t start, size_t n, string &dst, size_t *p=NULL);
            void safeGetString(size_t start, size_t n, string &dst, size_t *p=NULL);
            template <typename T>
            void getValue(size_t pos, T& value, size_t *p=NULL)
            { get(&value, pos, sizeof(value), p); }
            template <typename T>
            void safeGetValue(size_t pos, T& value, size_t *p=NULL)
            { safeGet(&value, pos, sizeof(value), p); }
        protected:
            char *_d;
            size_t _cap, _len;
        };
        
        class DataWriter {
        public:
            virtual size_t write(char *ptr, size_t size, size_t nmemb) = 0;
            virtual void clear() = 0;
        };
        
        class DummyDataWriter : public DataWriter {
        public:
        	inline virtual size_t write(char *ptr, size_t size, size_t nmemb) {
        		return nmemb * size;
        	}
        	inline virtual void clear() {}
        };

        class BufferDataWriter : public DataWriter {
        public:
            inline BufferDataWriter() : DataWriter() {}
            inline BufferDataWriter(size_t capacity) : DataWriter() {
            	_d.reserve(capacity);
            }
            inline virtual size_t write(char *ptr, size_t size, size_t nmemb) {
                _d.append(ptr, size*nmemb);
                return size*nmemb;
            }
            inline virtual void clear() { _d.clear(); }
            inline string &data() throw() { return _d; }
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
        void terminate();
        
        const string errorMessage() const throw() { return string(_errmsg.data()); }
        void clearError() throw() { _errmsg.clear(); }
        
        DataWriter &dataWriter() throw() { return _writer; }
        int getHttpCode();
        long long getResponseLength();
        const string getResponseUrl();
        bool supportRange() { return _supportRange; }
        double getDownloadSpeed();
        
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

