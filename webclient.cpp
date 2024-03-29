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
        if (proxy.empty()) {
            optProxy = string();
            optType = CURLPROXY_HTTP;
            return true;
        }
        
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
    
    // DataBuffer
    WebClient::DataBuffer::DataBuffer() : _d(NULL), _cap(0), _len(0) {
    }
    WebClient::DataBuffer::DataBuffer(size_t capacity) : _d(NULL), _cap(0), _len(0) {
    	resize(capacity);
    }

    WebClient::DataBuffer::DataBuffer(const DataBuffer &other) : _d(NULL), _cap(0) {
    	resize(other.capacity());
    	memcpy(_d, other.data(), _cap);
    	_len = other.length();
    }

    WebClient::DataBuffer::~DataBuffer() throw() {
    	if (_d) delete [] _d;
    	_d = NULL;
    	_cap = _len = 0;
    }

    void WebClient::DataBuffer::append(const void *data, size_t len) {
    	memcpy(_d+_len, data, len); // No overflow check.
    	_len += len;
    }

    void WebClient::DataBuffer::safeAppend(const void *data, size_t len) {
    	if (_len+len > _cap) throw OutOfRange("len");
    	append(data, len);
    }

    void WebClient::DataBuffer::get(void *dst, size_t start, size_t count, size_t *p) {
    	memcpy(dst, _d+start, count);
    	if (p) *p += count;
    }

    void WebClient::DataBuffer::getString(size_t start, size_t n, string &dst, size_t *p) {
		dst.clear();
		dst.resize(n);
		get((char*)dst.data(), start, n, p);
	}

    void WebClient::DataBuffer::safeGetString(size_t start, size_t n, string &dst, size_t *p) {
    	if (start+n > _cap) throw OutOfRange("n");
    	getString(start, n, dst, p);
    }

    void WebClient::DataBuffer::safeGet(void *dst, size_t start, size_t count, size_t *p) {
    	if (start+count > _cap) throw OutOfRange("count");
    	get(dst, start, count, p);
    }

    void WebClient::DataBuffer::resize(size_t capacity) {
    	if (_d) delete [] _d;
    	_cap = _len = 0;
    	_d = new char[capacity];
    	if (!_d) throw OutOfMemoryError();
    	_cap = capacity;
    	memset(_d, 0, _cap);
    }

    void WebClient::DataBuffer::clear() {
    	memset(_d, 0, _cap);
    	_len = 0;
    }

    // init & dispose
    WebClient::WebClient(WebClient::DataWriter &writer, size_t sheetSize) :
    		curl(curl_easy_init()), _writer(writer), _sheetSize(sheetSize), _errmsg(CURL_ERROR_SIZE),
    		_url(), _proxy(), _proxyServer(), _baseCookies(), _range(), _proxyType(0), _headerOnly(false),
    		_verbose(false), _supportRange(false),_contentLength(-1), _totalLength(-1), _timeout(30),
    		_connectTimeout(120), _lowSpeedLimit(1), _lowSpeedTime(120) {
        // create curl object
        if (!curl) {
            throw WebError("CURL object cannot be initialized.");
        }
        
        // default settings
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 256L);
        curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, _errmsg.data());
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE , "");
        
        // settings for timeout
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L); // fix multi-thread
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, _timeout); 
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, _connectTimeout);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, _lowSpeedLimit);
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, _lowSpeedTime);
        
        // write cache
        //_buffer.clear();
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &write_body);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
        curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION , &write_header);
        curl_easy_setopt(curl, CURLOPT_HEADERDATA, this);
    }
    
    WebClient::~WebClient() {
    	terminate();
    }
    
    // timeout
    void WebClient::setTimeout(long timeout) {
        _timeout = timeout;
        if (!curl) return;
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, _timeout);
    }
    void WebClient::setConnectTimeout(long connectTimeout) {
        _connectTimeout = connectTimeout;
        if (!curl) return;
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, _connectTimeout);
    }
    void WebClient::setLowSpeedLimit(long lowSpeedLimit) {
        _lowSpeedLimit = lowSpeedLimit;
        if (!curl) return;
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, _lowSpeedLimit);
    }
    void WebClient::setLowSpeedTime(long lowSpeedTime) {
        _lowSpeedTime = lowSpeedTime;
        if (!curl) return;
        curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, _lowSpeedTime);
    }
    
    // set & get parameters
    void WebClient::setUrl(const string& url) {
        _url = url;
        if (!curl) return;
        curl_easy_setopt(curl, CURLOPT_URL, _url.empty()? NULL: _url.c_str());
    }
    
    void WebClient::setProxy(const string &proxy) {
        if (!curl) return;
        if (!parseProxy(proxy, _proxyServer, _proxyType)) {
            throw ArgumentError("proxy", proxy + " is not a valid proxy.");
        }
        curl_easy_setopt(curl, CURLOPT_PROXY, _proxyServer.empty()? NULL: _proxyServer.c_str());
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE, _proxyType);
        _proxy = proxy;
    }
    
    void WebClient::setCookies(const string& cookies) {
        _baseCookies = cookies;
        if (!curl) return;
        curl_easy_setopt(curl, CURLOPT_COOKIELIST, "ALL");
        curl_easy_setopt(curl, CURLOPT_COOKIE, _baseCookies.empty()? NULL: _baseCookies.c_str());
    }
    
    void WebClient::setRange(const string &range) {
        _range = range;
        if (!curl) return;
        curl_easy_setopt(curl, CURLOPT_RANGE, _range.empty()? NULL: _range.c_str());
    }
    
    void WebClient::setHeaderOnly(bool headerOnly) {
        _headerOnly = headerOnly;
        if (!curl) return;
        if (headerOnly) {
            curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
        } else {
            curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
        }
    }
    
    void WebClient::setVerbose(bool verbose) {
        _verbose = verbose;
        if (!curl) return;
        if (verbose) {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        } else {
            curl_easy_setopt(curl, CURLOPT_VERBOSE, 0);
        }
    }
    
    void WebClient::reset() {
        setUrl("");
        setProxy("");
        setCookies("");
        setRange("");
        setHeaderOnly(false);
        _errmsg.clear();
        _contentLength = -1;
        _totalLength = -1;
        _supportRange = false;
    }
    
    bool WebClient::perform(CURLcode *curlReturnCode) {
        _contentLength = -1;
        _totalLength = -1;
        _supportRange = false;
        _errmsg.clear();
        if (!curl) return false;
        //curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        //curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 256L);
        CURLcode ret = curl_easy_perform(curl);
        if (curlReturnCode) *curlReturnCode = ret;
        return (ret == CURLE_OK);
    }
    
    void WebClient::terminate() {
    	if (curl) {
    		curl_easy_cleanup(curl);
			curl = NULL;
    	}
    }

    size_t WebClient::write_body( char *ptr, size_t size, size_t nmemb, void *userdata) {
        return static_cast<WebClient*>(userdata)->_writer.write(ptr, size, nmemb);
    }
    
    
    size_t WebClient::write_header(char *ptr, size_t size, size_t nmemb, void *userdata) {
        const size_t MAX_CONSIDER = 1000;
        WebClient* wc = static_cast<WebClient*>(userdata);
        size_t headerSize = size*nmemb;

        do {
            if (headerSize > MAX_CONSIDER) break;
            string header = string(ptr, headerSize);
            
            // Content-Length
            if (boost::istarts_with(header, "Content-Length:")) {
                header = header.substr(15);
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
                wc->_totalLength = -1;
                wc->_supportRange = false;
            } else if (boost::istarts_with(header, "Accept-Ranges:")
                    && boost::icontains(header, "bytes")) {
                wc->_supportRange = true;
            } else if (boost::istarts_with(header, "Content-Range:")) {
            	size_t pos = header.rfind('/');
            	if (pos != string::npos) {
            		header = header.substr(pos+1);
            		boost::trim(header);
            		try {
						wc->_totalLength = boost::lexical_cast<long long>(header);
					} catch (...) {
						wc->_totalLength = -1;
					}
            	}
            }
        } while (false);
        
        return headerSize;
    }
    
    // get response info
    int WebClient::getHttpCode() {
        long code;
        if (!curl) return -1;
        if (curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code) != CURLE_OK) {
            return -1;
        }
        return code;
    }
    
    long long WebClient::getResponseLength() {
        return _contentLength;
    }
    
    long long WebClient::getFileSize() {
    	if (_totalLength >= 0) return _totalLength;
    	return _contentLength;
    }

    const string WebClient::getResponseUrl() {
        char *url = NULL;
        if (!curl) return string();
        if (curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url) != CURLE_OK)
            return string();
        return string(url);
    }

    double WebClient::getDownloadSpeed() {
    	double spd = 0;
        if (!curl) return 0.0;
    	if (curl_easy_getinfo(curl, CURLINFO_SPEED_DOWNLOAD, &spd) != CURLE_OK)
    		return 0.0;
    	return spd;
    }
}

