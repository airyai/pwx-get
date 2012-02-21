/* 
 * File:   controller.h
 * Author: pwx
 *
 * Created on 2012年2月20日, 下午8:17
 */

#ifndef CONTROLLER_H
#define	CONTROLLER_H

#include <string>
#include <queue>
#include <list>
#include <stack>
#include <map>
#include <boost/thread.hpp>
#include "filebuffer.h"
#include "webclient.h"

namespace PwxGet {
    using namespace std;
    
    const size_t DEFAULT_PAGE_SIZE = 64; // DEFAULT_SHEET_SIZE * DEFAULT_PAGE_SIZE == 4M
    const size_t DEFAULT_PAGE_COUNT = 16; // DEFAULT_PAGE_COUNT * DEFAULT_PAGE_SIZE == 64M
    const size_t DEFAULT_SCAN_COUNT = 128;
    
    // Data are written into device in mostly continous sheets, which I call them pages.
    class PagedMemoryCache {
    public:
        /**
            * 
            * @param sheetSize: File sheet size.
            * @param pageSize: Count of sheets in a single page (the minimum unit to flush)
            * @param pageCount: Maximum page count)
            */
        PagedMemoryCache(FileBuffer &fileBuffer, size_t pageSize=DEFAULT_PAGE_SIZE, 
                size_t pageCount=DEFAULT_PAGE_COUNT);
        virtual ~PagedMemoryCache() throw();
        void commit(size_t sheet, const char *data);
        void flush();

        inline size_t pageSize() const throw() { return _pageSize; }
        inline size_t pageCount() const throw() { return _pageCount; }

        inline size_t createdPageCount() const throw() { return _createdPage; }
        inline size_t emptyPageCount() const throw() { return _empty.size(); }
        inline size_t workPageCount() const throw() { return _works.size(); }

    protected:
        // One Sheet Page
        class SheetPage {
        public:
        	SheetPage(size_t startSheet, size_t sheetSize, size_t pageSize, size_t done);
            inline virtual ~SheetPage();
            inline char *getSheet(size_t index) {
            	return buffer.data() + sheetSize * index;
            }
            inline void clear();
            inline char *data() { return buffer.data(); }
            size_t startSheet, sheetSize, pageSize, done;
            byte* usedSheets;
        protected:
            WebClient::DataBuffer buffer;
        };
        typedef map<size_t, SheetPage*> PageMap;
        typedef stack<SheetPage*> PageStack;
        typedef queue<SheetPage*> PageQueue;
        typedef list<SheetPage*> PageList;
        
        FileBuffer &_fb;
        size_t _sheetSize, _pageSize, _pageCount;
        size_t _createdPage;
        PageMap _pageMap; // Map page indexes to SheetPage instances.
        PageStack _empty; // empty pages
        PageList _works; // working pages
        
        SheetPage *openPage(size_t pageIndex);
        size_t beforeClosePage(SheetPage *page); // return pageIndex
    };
    
    class SheetCtl {
    public:
        SheetCtl(FileBuffer &fileBuffer, size_t pageSize=DEFAULT_PAGE_SIZE,
                size_t pageCount=DEFAULT_PAGE_COUNT, size_t scanCount=DEFAULT_SCAN_COUNT);
        virtual ~SheetCtl() throw();
        bool fetch(size_t &sheet, size_t &token);
        /**
         * Write data into one sheet.
         * Note: whether the sheet is complete or not, pls commit chunks exactly the size as sheetSize.
         * 
         * @param sheet: Sheet index.
         * @param data: Data chunk.
         */
        void commit(size_t sheet, size_t token, const char *data);
        void rollback(size_t sheet, size_t token);
        void flush();
        bool allDone() const;
        
        inline PagedMemoryCache &cache() throw() { return _cache; }
        inline FileBuffer &fileBuffer() throw() { return _fb; }
        inline size_t scanCount() const throw() { return _scanCount; }

    protected:
        typedef queue<size_t> IndexQueue;
        typedef boost::recursive_mutex Mutex;
        static const size_t DUMMY_TOKEN = 0x0;

        Mutex _mutex;
        FileBuffer &_fb;
        const byte *_sheetIndex;
        PagedMemoryCache _cache;
        size_t _sheetCount;
        size_t _scanCount, _nextscan; 	// The next sheet to be scanned.

        // TODO: Use "token" to control timeout.
        IndexQueue _works;	// Sheets to be processed.
        IndexQueue _rollbacks; // Sheets rolled back.
    };
}

#endif	/* CONTROLLER_H */

