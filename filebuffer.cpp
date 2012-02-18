/* 
 * File:   filebuffer.cpp
 * Author: pwx
 * 
 * Created on 2012年2月18日, 下午2:32
 */

#include "filebuffer.h"

namespace PwxGet {
    
    string readfile(const string& path) {
        FILE *fp = fopen(path.c_str(), "rb");
        if (fp == NULL)
            throw IOException(path, "Cannot open file " + path + " for read.");
        
        char buffer[10240];
        string ret; size_t buflen;
        
        while ((buflen = fread(buffer, 10240, 1, fp))) {
            ret.append(buffer, buflen);
        }
        
        fclose(fp);
        return ret;
    }
    
    void writefile(const string &path, const string &cnt) {
        FILE *fp = fopen(path.c_str(), "wb");
        if (fp == NULL) 
            throw IOException(path, "Cannot open file " + path + " for write.");
        
        /*if (fwrite(cnt.data(), cnt.size(), 1, fp) != cnt.size()) {
            throw IOException(path, "File content truncated.");
        }*/
        size_t current, next, total;
        const char *buffer = cnt.data();
        total = cnt.size();
        while (next < total) {
            current = fwrite(buffer+next, total-next, 1, fp);
            if (!current) {
                fclose(fp);
                throw IOException(path, "Write content to file " + path + " failed.");
            }
            next += current;
        }
        fclose(fp);
    }

    FileBuffer::FileBuffer(const string &path, size_t size, const string &indexPath, 
            int sheetSize) : _path(path), _size(size),  _indexPath(indexPath), _sheetSize(sheetSize) {
        // read file index
        this->_sheetCount = this->_size / this->_sheetSize;
        if (this->_sheetSize * this->_sheetCount != this->_size) ++this->_sheetCount;
        
        size_t packedSheetCount = this->_sheetCount / 8;
        if (packedSheetCount * 8 != this->_sheetCount) ++packedSheetCount;
        string packedIndex = readfile(path);
        if (packedSheetCount != packedIndex.size()) {
            throw BadIndexFile(indexPath);
        }
        this->unpackIndex(packedIndex, this->_managedIndex);
        this->_index = (byte*)(this->_managedIndex.data());
        
        // open file handler
        _fp = fopen(path.c_str(), "ab+");
        if (_fp == NULL) {
            throw IOException(path, "Cannot open data file " + path + ".");
        }
        
        // 
    }

    FileBuffer::~FileBuffer() throw() {
        try {
            this->close();
        } catch (const IOException&) {}
    }
    
    void FileBuffer::flush() {
        this->lock();
        try {
            if (this->_fp) {
                // flush data
                // write index
                string packedIndex;
                this->packIndex(this->_managedIndex, packedIndex);
                writefile(this->_indexPath, packedIndex);
            }
        } catch (...) {
            this->unlock();
            throw;
        }
        this->unlock();
    }
    
    void FileBuffer::close() {
        this->lock();
        if (this->_fp) {
            this->flush();
            fclose(this->_fp);
        }
        this->_fp = NULL;
        this->unlock();
    }

    /*bool FileBuffer::getSheetState(long long index) {
        //if (index < 0 || index >= this->_size) throw OutOfRange("index");
        return ((this->_index[index >> 8]) >> (7 - index & 0x8)) & 0x1;
    }
    
    void FileBuffer::setSheetState(long long index, bool state) {
        //if (index < 0 || index >= this->_size) throw OutOfRange("index");
        long long i = index >> 8;
        long long b = 7 - index & 0x8;
        if (state)
            this->_index[i] |= 0x1 << b;
        else
            this->_index[i] &= ~(0x1 << b);
    }*/
    
    void FileBuffer::unpackIndex(const string& data, string& dst) {
        dst.resize(this->_sheetCount);
        byte *b = (byte*)data.data();
        byte *d = (byte*)dst.data();
        byte *dend = d + this->_sheetCount;
        
        while (d < dend) {
            byte bit = *b;
            for (int j=7; j>=0 && d < dend; --j) {
                *d++ = (bit >> j) & 0x1;
            }
            b++;
        }
    }
    
    void FileBuffer::packIndex(const string& data, string& dst) {
        size_t dstlen = this->_sheetCount / 8;
        if (dstlen * 8 != this->_sheetCount) ++dstlen;
        dst.resize(dstlen);
        
        byte *b = (byte*)data.data();
        byte *d = (byte*)dst.data();
        byte *bend = b + this->_sheetCount;
        
        while (b < bend) {
            byte bit = 0;
            for (size_t j=7; j>=0 && b < bend; --j) {
                bit |= (*b++ & 0x1) << j;
            }
            *d++ = bit;
        }
    }
    
    const byte *FileBuffer::index() const throw () {
        return this->_index;
    }
    
    void FileBuffer::erase(size_t startSheet, size_t sheetCount) {
        this->lock();
        size_t endSheet = startSheet+sheetCount;
        for (size_t i=startSheet; i<endSheet; i++) {
            this->_index[i] = 0;
        }
        this->unlock();
    }
    
    size_t FileBuffer::write(byte *buffer, size_t startSheet, size_t sheetCount) {
        if (startSheet < 0 || startSheet >= this->_sheetCount)
                throw OutOfRange("startSheet");
        if (!fseek(this->_fp, startSheet * this->_sheetSize, SEEK_SET)) {
            throw SeekError(this->_path, startSheet * this->_sheetSize);
        }
        size_t done = 0, current = 0, total = sheetCount * this->_sheetSize;
        while (done < total) {
            current = fwrite(buffer+done, total-done, 1, this->_fp);
            if (!current)
                break;
            done += current;
        }
        
        sheetCount = done / this->_sheetSize;
        for (size_t i=0; i<sheetCount; i++) {
            this->_index[startSheet+i] = 1;
        }
        return done;
    }
    
    size_t FileBuffer::read(byte *buffer, size_t startSheet, size_t sheetCount) {
        if (startSheet < 0 || startSheet >= this->_sheetCount)
                throw OutOfRange("startSheet");
        if (!fseek(this->_fp, startSheet * this->_sheetSize, SEEK_SET)) {
            throw SeekError(this->_path, startSheet * this->_sheetSize);
        }
        size_t done = 0, current = 0, total = sheetCount * this->_sheetSize;
        while (done < total) {
            current = fread(buffer+done, total-done, 1, this->_fp);
            if (!current)
                break;
            done += current;
        }
        
        return done;
    }
}
