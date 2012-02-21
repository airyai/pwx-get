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

    protected:
        // One Sheet Page
        class SheetPage {
        public:
            SheetPage(size_t startSheet, size_t sheetSize, size_t pageSize, size_t done) : 
                    sheetSize(sheetSize), startSheet(startSheet),
                    pageSize(pageSize), done(done), buffer(pageSize * sheetSize),
                    usedSheets(new byte[pageSize]) {
                memset(usedSheets, 0, sizeof(usedSheets));
            }
            virtual ~SheetPage() {
                delete [] usedSheets;
            }
            char *getSheet(size_t index) { return buffer.data() + sheetSize * index; }
            void clear() { 
                done = 0; 
                memset(usedSheets, 0, sizeof(usedSheets));
            }
            char *data() { return buffer.data(); }
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
    
    class Controller {
    public:

        
        static const size_t NOSHEET = (size_t)-1;
        
        Controller(FileBuffer &fileBuffer);
        virtual ~Controller() throw();
        
        size_t getSheet();
        /**
         * Write data into one sheet.
         * Note: whether the sheet is complete or not, pls commit chunks exactly the size as sheetSize.
         * 
         * @param sheet: Sheet index.
         * @param data: Data chunk.
         */
        void commit(size_t sheet, const char *data);
        void rollback(size_t sheet);
        
    protected:
        boost::recursive_mutex _mutex;
        FileBuffer &_fb;
        PagedMemoryCache &_cache;
    };
}

#endif	/* CONTROLLER_H */

