/* 
 * File:   filebuffer.h
 * Author: pwx
 *
 * Created on 2012年2月18日, 下午2:32
 */

#ifndef FILEBUFFER_H
#define	FILEBUFFER_H

#include <string>
#include <fstream>
#include <boost/filesystem.hpp>
#include "exceptions.h"

namespace PwxGet {
    using namespace std;
    namespace fs = boost::filesystem;
    
    const size_t DEFAULT_SHEET_SIZE = 81920;
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
        class PackedIndex {
        public:
            virtual const string &getData() const = 0;
            virtual void setData(const string &data) = 0;
            virtual bool isValid() const throw() = 0;
            virtual const string identifier() const throw() = 0;
        };
        
        class PackedIndexFile : public PackedIndex {
        public:
            PackedIndexFile(const string& indexPath);
            virtual bool isValid() const throw() { return _valid; }
            virtual const string identifier() const throw() { return _indexPath; }
            virtual const string &getData() const { return _data; }
            virtual void setData(const string &data);
            virtual ~PackedIndexFile();
        protected:
            bool _valid;
            string _data, _indexPath;
        };
        
        /**
         * Create a filebuffer instance.
         * @param path: The destination file path.
         * @param size: The destination file size.
         * @param packedIndex: Sheet index object.
         * @param sheetSize: Sheet size for the destination file.
         */
        FileBuffer(const string &path, size_t size, PackedIndex &packedIndex, 
                size_t sheetSize = DEFAULT_SHEET_SIZE);
        FileBuffer(const FileBuffer& orig);
        virtual ~FileBuffer() throw();
        
        const byte *index() const throw ();
        const string &path() const throw () { return _path; }
        size_t size() const throw () { return _size; }
        size_t sheetCount() const throw () { return _sheetCount; }
        size_t sheetSize() const throw () { return _sheetSize; }
        size_t doneSheet() const throw() { return _doneSheet; }
        PackedIndex &packedIndex() const throw () { return _packedIndex; }
        
        void close();
        void flush();
        
        size_t write(const byte *buffer, size_t startSheet, size_t sheetCount);
        size_t read(byte *buffer, size_t startSheet, size_t sheetCount);
        void erase(size_t startSheet, size_t sheetCount);
        
    protected:
        fstream _f;
        bool _valid;
        string _path;
        size_t _size, _sheetCount, _sheetSize;
        string _managedIndex; byte *_index;
        size_t _doneSheet;
        PackedIndex &_packedIndex;
        
        void lock() {}
        void unlock() {}
        
        //bool getSheetState(long long index);
        //void setSheetState(long long index, bool state);
        void unpackIndex(const string &data, string &dst, size_t &doneSheet);
        void packIndex  (const string &data, string &dst);
    };

}

#endif	/* FILEBUFFER_H */

