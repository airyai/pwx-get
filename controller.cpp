/* 
 * File:   controller.cpp
 * Author: pwx
 * 
 * Created on 2012年2月20日, 下午8:17
 */

#include "controller.h"
#include "exceptions.h"

namespace PwxGet {
    /* PagedMemoryCache */
    PagedMemoryCache::PagedMemoryCache(FileBuffer &fileBuffer, size_t pageSize, 
            size_t pageCount) : _fb(fileBuffer), _sheetSize(fileBuffer.sheetSize()), 
            _pageSize(pageSize), _pageCount(pageCount) {
        // Create page array & queue
        _pages = new char*[pageCount];
        if (!_pages) throw OutOfMemoryError();
        
    }
    
    PagedMemoryCache::~PagedMemoryCache() throw() {
        if (_pages) {
            for (size_t i=0; i<_pageCount; i++) {
                if (_pages[i]) {
                    delete[] _pages[i];
                    _pages[i] = NULL;
                }
            }
            _pages = NULL;
        }
    }
    
    void PagedMemoryCache::commit(size_t sheet, const char *data) {}
    void PagedMemoryCache::flush() {}
}

