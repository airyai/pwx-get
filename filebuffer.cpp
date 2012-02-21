/* 
 * File:   filebuffer.cpp
 * Author: pwx
 * 
 * Created on 2012年2月18日, 下午2:32
 */

#include "filebuffer.h"

namespace PwxGet {
    
    string readfile(const string& path) {
        ifstream fin(path.c_str(), ios::binary);
        if (!fin)
            throw IOException(path, "Cannot open file " + path + " for read.");
        
        string ret;
        size_t len2read = 0;
        
        fin.seekg(0, ios::end);
        len2read = fin.tellg();
        fin.seekg(0, ios::beg);
        ret.resize(len2read);
        
        size_t done = 0;
        char* p = (char*)ret.data();
        while (done < len2read) {
            if (!fin.read(p+done, len2read-done)) break;
            done += fin.gcount();
        }
        if (done < len2read)
            throw IOException(path, "Read file " + path +
                        " failed: unexpected end of file.");
        
        return ret;
    }
    
    void writefile(const string &path, const string &cnt) {
        ofstream fout(path.c_str(), ios::binary | ios::trunc);
        if (!fout)
            throw IOException(path, "Cannot open file " + path + " for write.");
       
        size_t len2write = cnt.size();
        const char* p = cnt.data();
        
        if (!fout.write(p, len2write))
            throw IOException(path, "Write file " + path + " failed.");
    }
    
    FileBuffer::PackedIndexFile::PackedIndexFile(const string &indexPath) : FileBuffer::PackedIndex(), 
            _data(), _indexPath(indexPath), _valid(false) {
        if (fs::is_regular_file(indexPath)) {
            _data = readfile(indexPath);
            _valid = true;
        }
    }
    
    void FileBuffer::PackedIndexFile::setData(const string &data) {
        _data = data;
        writefile(this->_indexPath, data);
        _valid = true;
    }
    
    FileBuffer::PackedIndexFile::~PackedIndexFile() {
        _valid = false;
    }

    FileBuffer::FileBuffer(const string &path, size_t size, PackedIndex &packedIndex, 
            size_t sheetSize) : _valid(false), _path(path), _size(size), 
            _packedIndex(packedIndex), _sheetSize(sheetSize), _doneSheet(0) {
        // read file index
        this->_sheetCount = this->_size / this->_sheetSize;
        if (this->_sheetSize * this->_sheetCount != this->_size) ++this->_sheetCount;

        if (packedIndex.isValid()) {
            size_t packedSheetCount = this->_sheetCount / 8;
            if (packedSheetCount * 8 != this->_sheetCount) ++packedSheetCount;
            const string &data = packedIndex.getData();
            if (packedSheetCount != data.size()) {
                throw BadIndex("Bad sheet index " + packedIndex.identifier() + ".");
            }
            this->unpackIndex(data, this->_managedIndex, this->_doneSheet);
        } else {
            this->_managedIndex.resize(this->_sheetCount);
            memset((char*)this->_managedIndex.data(), 0, this->_sheetCount);
        }
        this->_index = (byte*)(this->_managedIndex.data());
        
        // resize file
        if (!fs::is_regular_file(path)) {
            ofstream tmpfout(path.c_str(), ios::binary);
            tmpfout.close();
        }
        if (fs::file_size(path) != size) {
            try {
                fs::resize_file(path, size);
            } catch (fs::filesystem_error& ex) {
                throw IOException("Cannot allocate enough space for file " + path + ".");
            }
        }
        
        // open file handler
        _f.open(path.c_str(), (ios::binary | ios::out | ios::in) & ~ios::trunc);
        if (!_f)
            throw IOException("Cannot open data file " + path + ".");
        _valid = true;
    }

    FileBuffer::~FileBuffer() throw() {
        try {
            this->close();
        } catch (const IOException&) {}
    }
    
    void FileBuffer::flush() {
        this->lock();
        try {
            if (_valid) {
                // flush data
                _f.flush();
                // write index
                string data;
                this->packIndex(this->_managedIndex, data);
                this->_packedIndex.setData(data);
            }
        } catch (...) {
            this->unlock();
            throw;
        }
        this->unlock();
    }
    
    void FileBuffer::close() {
        this->lock();
        if (_valid) {
            this->flush();
            _f.close();
            _valid = false;
        }
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
    
    void FileBuffer::unpackIndex(const string& data, string& dst, size_t &doneSheet) {
        dst.resize(this->_sheetCount);
        byte *b = (byte*)data.data();
        byte *d = (byte*)dst.data();
        byte *dend = d + this->_sheetCount;
        
        while (d < dend) {
            byte bit = *b;
            for (int j=7; j>=0 && d < dend; --j) {
                *d = (bit >> j) & 0x1;
                if (*d) ++doneSheet;
                d++;
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
            if (this->_index[i]) {
                --this->_doneSheet;
                this->_index[i] = 0;
            }
        }
        this->unlock();
    }
    
    size_t FileBuffer::write(const byte *buffer, size_t startSheet, size_t sheetCount) {
        this->lock();
        if (startSheet < 0 || startSheet >= _sheetCount) {
            this->unlock();
            throw OutOfRange("startSheet");
        }
        if (!_f.seekg(startSheet * _sheetSize, ios::beg)) {
            this->unlock();
            throw SeekError(_path, startSheet * _sheetSize);
        }
        size_t total = min(sheetCount * _sheetSize, _size-startSheet*_sheetSize);
        // "min" to fix the last sheet 
        
        if (!_f.write((const char*)buffer, total)) {
            this->unlock();
            return 0;
        }

        for (size_t i=0; i<sheetCount; i++) {
            size_t j = startSheet+i;
            if (!this->_index[j]) {
                ++this->_doneSheet;
                this->_index[j] = 1;
            }
        }
        this->unlock();
        return sheetCount;
    }
    
    size_t FileBuffer::read(byte *buffer, size_t startSheet, size_t sheetCount) {
        this->lock();
        if (startSheet < 0 || startSheet >= _sheetCount) {
            this->unlock();
            throw OutOfRange("startSheet");
        }
        if (!_f.seekg(startSheet * _sheetSize, ios::beg)) {
            this->unlock();
            throw SeekError(_path, startSheet * _sheetSize);
        }
        size_t done = 0;
        size_t total = min(sheetCount*_sheetSize, _size-startSheet*_sheetSize);
        // "min" to fix the last sheet 
        
        while (done < total) {
            if (!_f.read((char*)buffer+done, total-done)) break;
            done += _f.gcount();
        }

        this->unlock();
        size_t ret = done / _sheetSize;
        if (ret * _sheetSize != done) ++ret; // fix the last sheet
        return ret;
    }
}
