/* 
 * File:   filebuffer.h
 * Author: pwx
 *
 * Created on 2012年2月18日, 下午2:32
 */

#ifndef FILEBUFFER_H
#define	FILEBUFFER_H

#include <string>
#include <cstdio>
#include "exceptions.h"

namespace PwxGet {
    using namespace std;
    
    const int DEFAULT_SHEET_SIZE = 81920;
    const int DEFAULT_CACHE_SIZE = 4194304;
    typedef unsigned char byte;
    
    /**
     * Read all content from a file.
     * @param path: File path.
     * @return File content.
     */
    string readfile(const string& path);
    void writefile(const string &path, const string &cnt);

    /**
     * Sheeted file buffer.
     * 
     * The filebuffer does not manage memory cache for continous sheet. It 
     * only provides a general interface to read and write large files and 
     * record whether a certain sheet is availble.
     */
    class FileBuffer {
    public:
        /**
         * Create a filebuffer instance.
         * @param path: The destination file path.
         * @param size: The destination file size.
         * @param fileIndex: Sheet index for the destination file.
         * @param sheetSize: Sheet size for the destination file.
         */
        FileBuffer(const string &path, size_t size, const string &indexPath, 
                int sheetSize = DEFAULT_SHEET_SIZE);
        FileBuffer(const FileBuffer& orig);
        virtual ~FileBuffer() throw();
        
        const byte *index() const throw ();
        
        void close();
        void flush();
        
        size_t write(byte *buffer, size_t startSheet, size_t sheetCount);
        size_t read(byte *buffer, size_t startSheet, size_t sheetCount);
        void erase(size_t startSheet, size_t sheetCount);
        
    protected:
        string _path, _indexPath;
        size_t _size, _sheetCount;
        int _sheetSize;
        FILE *_fp;
        string _managedIndex;
        byte *_index;
        
        void lock() {}
        void unlock() {}
        
        //bool getSheetState(long long index);
        //void setSheetState(long long index, bool state);
        void unpackIndex(const string &data, string &dst);
        void packIndex  (const string &data, string &dst);
    };

}

#endif	/* FILEBUFFER_H */

