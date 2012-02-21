/* 
 * File:   exceptions.h
 * Author: pwx
 *
 * Created on 2012年2月18日, 下午2:51
 */

#ifndef EXCEPTIONS_H
#define	EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace PwxGet {
    using namespace std;
    
    class Exception : public exception {
    public:
        Exception() throw () {}
        virtual ~Exception() throw () {}
        virtual const string message() const throw() {
        	return string();
        }
        
        virtual const char* what() const throw() {
            return message().c_str();
        }
    };
    
    class RuntimeError : public Exception {
    public:
        RuntimeError(const string &err) throw () : Exception(), err(err) {}
        virtual const string message() const throw() {
            return this->err;
        }
        virtual ~RuntimeError() throw () {}
    protected:
        string err;
    };
    
    class OutOfMemoryError: public RuntimeError {
    public:
        OutOfMemoryError(const string &err) throw () : RuntimeError(err.empty()? "Memory exhausted.": err) {}
        virtual ~OutOfMemoryError() throw () {}
    };
    
    class AssertionError : public RuntimeError {
    public:
        AssertionError(const string &err) throw() : RuntimeError(err) {}
        virtual ~AssertionError() throw() {}
    };
    
    class BadIndex : public RuntimeError {
    public:
        BadIndex(const string &err) throw() : RuntimeError(err) {}
        virtual ~BadIndex() throw() {}
    };
    
    class IOException : public RuntimeError {
    public:
        IOException(const string &path, const string &err=string()) throw() : 
                RuntimeError(err.empty()? path: err), _path(path) { }
        const string path() const throw() { return _path; }
        virtual ~IOException() throw () {}
    protected:
        string _path;
    };
    
    class BadIndexFile : public IOException {
    public:
        BadIndexFile(const string &path, const string &err=string()) throw() : 
                IOException(path, err) {}
        virtual ~BadIndexFile() throw () {}
    };
    
    class SeekError : public IOException {
    public:
        SeekError(const string &path, size_t position, const string &err=string()) throw() : 
                IOException(path, err), _position(position) {}
        virtual ~SeekError() throw () {}
        size_t position() const throw() { return _position; }
    protected:
        size_t _position;
    };
    
    class LogicError : public Exception {
    public:
        LogicError() throw() : Exception() {}
        virtual ~LogicError() throw () {}
    };
    
    class ArgumentError : public LogicError {
    public:
        ArgumentError(const string &arg, const string &message) throw() : LogicError(), 
                _arg(arg), _message(message) {
            if (message.empty()) {
                _message = "Argument " + _arg + " is invalid.";
            }
        }
        virtual const string message() const throw() {
            return _message;
        }
        const string arg() const throw() { return _arg ; }
        virtual ~ArgumentError() throw () {}
    protected:
        string _arg, _message;
    };
    
    class OutOfRange : public ArgumentError {
    public:
        OutOfRange(const string &arg) throw() : ArgumentError(arg, 
                "Argument " + arg + " out of range.") {}
        virtual ~OutOfRange() throw () {}
    };
    
    class WebError: public RuntimeError {
    public:
        WebError(const string &err) throw() : RuntimeError(err) {}
        virtual ~WebError() throw () {}
    };
    
    class CurlError: public WebError {
    public:
        CurlError(int errorCode, const string &err) throw() :
        	WebError(err), _errorCode(errorCode) {}
        int errorCode() const throw() { return _errorCode; }
        virtual ~CurlError() throw() {}
    protected:
        int _errorCode;
    };
}



#endif	/* EXCEPTIONS_H */

