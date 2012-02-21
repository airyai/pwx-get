/* 
 * File:   controller.cpp
 * Author: pwx
 * 
 * Created on 2012年2月20日, 下午8:17
 */

#include "sheetctl.h"
#include "exceptions.h"

namespace PwxGet {
    /* PagedMemoryCache */
	PagedMemoryCache::SheetPage::SheetPage(size_t startSheet, size_t sheetSize, size_t pageSize, size_t done) :
                            startSheet(startSheet), sheetSize(sheetSize),
                            pageSize(pageSize), done(done), usedSheets(new byte[pageSize]),
                            buffer(pageSize * sheetSize) {
                        memset(usedSheets, 0, sizeof(usedSheets));
                    }
	PagedMemoryCache::SheetPage::~SheetPage() {
		delete [] usedSheets;
	}

	void PagedMemoryCache::SheetPage::clear() {
		done = 0;
		memset(buffer.data(), buffer.capacity(), 0);
		memset(usedSheets, 0, sizeof(usedSheets));
	}

    PagedMemoryCache::PagedMemoryCache(FileBuffer &fileBuffer, size_t pageSize, 
            size_t pageCount) : _fb(fileBuffer), _sheetSize(fileBuffer.sheetSize()), 
            _pageSize(pageSize), _pageCount(pageCount), _createdPage(0),
            _empty(), _works() {
    }
    
    PagedMemoryCache::~PagedMemoryCache() throw() {
        // empty stack & queue
        flush();
        
        while (!_empty.empty()) {
            delete _empty.top();
            _empty.pop();
        }
        _createdPage = 0;
    }
    
    void PagedMemoryCache::flush() {
    	// flush pages
        PageList::iterator it = _works.begin();
        while (it != _works.end()) {
            beforeClosePage(*it);
            _empty.push(*it);
            it++;
        }
        _works.clear();
        _pageMap.clear();
        // flush fileBuffer
        _fb.flush();
    }
    
    // TODO: add a lot of exception process!!!
    PagedMemoryCache::SheetPage *PagedMemoryCache::openPage(size_t pageIndex) {
        PageMap::iterator it = (_pageMap.find(pageIndex));
        SheetPage *page = NULL;
        
        if (it != _pageMap.end()) return it->second;
        if (!_empty.empty()) {
            page = _empty.top();
            _empty.pop();
            _works.push_back(page);
            _pageMap[pageIndex] = page;
            page->startSheet = pageIndex * _pageSize;
            return page;
        }
        if (_createdPage < _pageCount) {
            page = new SheetPage(pageIndex*_pageSize, _sheetSize, _pageSize, 0);
            _pageMap[pageIndex] = page;
            _works.push_back(page);
            ++_createdPage;
            return page;
        }
        
        page = _works.front(); _works.remove(page);
        _pageMap.erase(_pageMap.find(beforeClosePage(page)));
        _pageMap[pageIndex] = page;
        _works.push_back(page);
        page->startSheet = pageIndex * _pageSize;
        return page;
    }
    
    size_t PagedMemoryCache::beforeClosePage(SheetPage *page) {
        do {
            if (page->done == page->pageSize) {
                _fb.write((byte*)page->data(), page->startSheet, page->pageSize);
                break;
            }
            size_t i = 0, j;
            while (i < page->pageSize) {
                while (i < page->pageSize && !page->usedSheets[i]) ++i;
                j = i;
                while (j < page->pageSize && page->usedSheets[j]) ++j;
                if (i < page->pageSize) {
                    _fb.write((byte*)page->getSheet(i), page->startSheet+i, j-i);
                    i = j;
                }
            }
        } while (false);
        size_t pageIndex = page->startSheet / _pageSize;
        page->clear();
        return pageIndex;
    }
    
    void PagedMemoryCache::commit(size_t sheet, const char *data) {
        SheetPage *page = openPage(sheet / _pageSize);
        size_t i = sheet - page->startSheet;
        
        if (!page->usedSheets[i]){
            ++page->done;
            page->usedSheets[i] = 1;
        }
        memcpy(page->getSheet(i), data, _sheetSize);
        
        if (page->done == _pageSize) {
            _pageMap.erase(_pageMap.find(beforeClosePage(page)));
            _works.remove(page);
            _empty.push(page);
        }
    }

    /* Controller */
    SheetCtl::SheetCtl(FileBuffer &fileBuffer, size_t pageSize, size_t pageCount,
    		size_t scanCount) : _mutex(), _fb(fileBuffer), _sheetIndex(_fb.index()),
    		_cache(_fb, pageSize, pageCount), _sheetCount(_fb.sheetCount()),
    		_scanCount(scanCount), _nextscan(0), _works(), _rollbacks() {
    }

    SheetCtl::~SheetCtl() throw() {

    }

    bool SheetCtl::allDone() const {
    	return (_rollbacks.empty() && _works.empty() && _nextscan >= _sheetCount);
    }

    bool SheetCtl::fetch(size_t &sheet, size_t &token) {
    	Mutex::scoped_lock mylock(_mutex);
    	bool ret = false;

    	do {
    		// select from rolled back sheets
    		if (!_rollbacks.empty()) {
    			sheet = _rollbacks.front();
    			token = DUMMY_TOKEN;
    			ret = true;
    			_rollbacks.pop();
    			break;
    		}
    		// select from scanned sheets
    		if (!_works.empty()) {
    			sheet = _works.front();
    			token = DUMMY_TOKEN;
    			ret = true;
    			_works.pop();
    			break;
    		}
    		// emit next scan
    		size_t start = _nextscan;
    		while (start < _sheetCount && !_sheetIndex[start]) {
    			++start;
    		}
    		if (start == _sheetCount) break;
    		size_t end = _nextscan, limit = start+_scanCount;
    		while (end < _sheetCount && end < limit && _sheetIndex[start]) {
    			++end;
    		}
    		for (size_t i=start; i<end; i++) {
    			_works.push(i);
    		}
    		_nextscan = end;
    		// select available works
    		_nextscan = _works.front();
    		token = DUMMY_TOKEN;
    		ret = true;
    		_works.pop();
    		break;
    	} while (false);

    	return ret;
    }

    void SheetCtl::commit(size_t sheet, size_t token, const char *data) {
    	// temporarily ignore token
    	Mutex::scoped_lock mylock(_mutex);
    	_cache.commit(sheet, data);
    }

    void SheetCtl::rollback(size_t sheet, size_t token) {
    	// temporarily ignore token
    	Mutex::scoped_lock mylock(_mutex);
    	_rollbacks.push(sheet);
    }

    void SheetCtl::flush() {
    	Mutex::scoped_lock mylock(_mutex);
    	_cache.flush();
    }
}

