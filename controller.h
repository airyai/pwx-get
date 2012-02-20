/* 
 * File:   controller.h
 * Author: pwx
 *
 * Created on 2012年2月20日, 下午8:17
 */

#ifndef CONTROLLER_H
#define	CONTROLLER_H

#include <string>
#include <boost/thread.hpp>
#include "filebuffer.h"
#include "webclient.h"

namespace PwxGet {
    using namespace std;
    
    class PagedMemoryCache {
    public:
        /**
            * 
            * @param sheetSize: File sheet size.
            * @param pageSize: Count of sheets in a single page (the minimum unit to flush)
            * @param pageCount: Maximum page count)
            */
        PagedMemoryCache(FileBuffer &fileBuffer, size_t pageSize, size_t pageCount);
        virtual ~PagedMemoryCache() throw();
        void commit(size_t sheet, const char *data);
        void flush();
    protected:
        FileBuffer &_fb;
        size_t _sheetSize, _pageSize, _pageCount;
        char **_pages;
    };
    
    class Controller {
    public:

        
        const size_t NOSHEET = (size_t)-1;
        
        Controller(FileBuffer &fileBuffer);
        virtual ~Controller() throw();
        
        size_t getSheet();
        void commit(size_t sheet, const char *data);
        void rollback(size_t sheet);
        
    protected:
        boost::recursive_mutex _mutex;
        FileBuffer &_fb;
        PagedMemoryCache &_cache;
    };
}

#endif	/* CONTROLLER_H */

