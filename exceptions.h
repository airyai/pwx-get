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
        virtual const string message() const throw() {}
        
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
    
    class IOException : public RuntimeError {
    public:
        IOException(const string &path, const string &err=string()): 
                RuntimeError(err.empty()? path: err), _path(path) { }
        const string path() const throw() { return _path; }
        virtual ~IOException() throw () {}
    protected:
        string _path;
    };
    
    class BadIndexFile : public IOException {
    public:
        BadIndexFile(const string &path, const string &err=string()) : 
                IOException(path, err) {}
        virtual ~BadIndexFile() throw () {}
    };
    
    class SeekError : public IOException {
    public:
        SeekError(const string &path, size_t position, const string &err=string()) : 
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
        ArgumentError(const string &arg) throw() : LogicError(), _arg(arg) {}
        virtual const string message() const throw() {
            return "Argument " + this->_arg + " is invalid.";
        }
        const string arg() const throw() { return _arg ; }
        virtual ~ArgumentError() throw () {}
    protected:
        string _arg;
    };
    
    class OutOfRange : public ArgumentError {
    public:
        OutOfRange(const string &arg) throw() : ArgumentError(arg) {}
        virtual const string message() const throw() {
            return "Argument " + this->_arg + " out of range.";
        }
        virtual ~OutOfRange() throw () {}
    };
}



#endif	/* EXCEPTIONS_H */

