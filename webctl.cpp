/*
 * threadctl.cpp
 *
 *  Created on: 2012-2-21
 *      Author: pwx
 */

#include "webctl.h"
#include <boost/filesystem.hpp>
namespace fs = boost::filesystem;

namespace PwxGet {
	const unsigned int JobFile::MAGIC_FLAG = 0x62874517;

	/* JobFile */
	JobFile::JobFile() : _url(), _url2(), _cookies(), _savePath(), _jobPath(),
			_useRedirectedUrl(), _fileSize(0), _sheetSize(0), _index(), _jobFile() {
	}

	JobFile::~JobFile() throw() {
		close();
	}

	void JobFile::open(const string &savePath) {
		string jobPath = savePath + ".pg!";
		if (!fs::exists(jobPath)) throw JobNotExists(jobPath);
		//if (!fs::exists(savePath)) throw JobNotExists(savePath);
		// open job file
		_jobFile.open(jobPath.c_str(), (ios::binary | ios::out | ios::in) & ~ios::trunc);
		if (!_jobFile.is_open()) throw BadJobFile(jobPath);
		_jobPath = jobPath;
		// read information
		read(savePath);
	}

	void JobFile::create(const string &url, /*const string &url2, */const string &cookies,
			const string &savePath, bool useRedirectedUrl, size_t fileSize, size_t sheetSize) {
		string jobPath = savePath + ".pg!";
		if (fs::exists(jobPath)) throw JobExists(jobPath);
		//if (fs::exists(savePath)) throw JobExists(savePath);
		// create job file
		_jobFile.open(jobPath.c_str(), (ios::binary | ios::out | ios::in | ios::trunc));
		if (!_jobFile.is_open()) throw BadJobFile(jobPath);
		_jobPath = jobPath;
		// set information
		_url = url;
		//_url2 = url2;
		_cookies = cookies;
		_savePath = savePath;
		_jobPath = jobPath;
		_useRedirectedUrl = useRedirectedUrl;
		_fileSize = fileSize;
		_sheetSize = sheetSize;
		_index.resize(indexSize());
		// flush into job file
		flush();
	}

	size_t JobFile::indexSize() const throw() {
		size_t sheetCount = _fileSize / _sheetSize;
		if (sheetCount * _sheetSize != _fileSize) ++sheetCount;
		size_t ret = sheetCount / 8;
		if (ret * 8 != sheetCount) ++ret;
		return ret;
	}

	size_t JobFile::headerSize() const throw() {
		return sizeof(unsigned int) 							// Magic Flag
				+ sizeof(unsigned int) + _url.size() 			// url
				+ sizeof(unsigned int) + _url2.size() 			// url2
				+ sizeof(unsigned int) + _cookies.size()		// cookies
				+ sizeof(unsigned int) + _savePath.size() 		// savePath
				+ sizeof(char)									// useRedirectedUrl
				+ sizeof(unsigned long long)					// fileSize
				+ sizeof(unsigned long long);					// sheetSize
	}

	void JobFile::writeBytes(const char *data, size_t n) {
		if (!_jobFile.write(data, n))
			throw IOException(_jobPath, "Write job information to file failed.");
	}

	void JobFile::flush() {
		if (!_jobFile.is_open()) return;
		// generate file header
		size_t headerSize = this->headerSize();
		WebClient::DataBuffer db(headerSize);
		db.appendValue(MAGIC_FLAG);
		db.appendValue((unsigned int)_url.size());
		db.append(_url.data(), _url.size());
		db.appendValue((unsigned int)_url2.size());
		db.append(_url2.data(), _url2.size());
		db.appendValue((unsigned int)_cookies.size());
		db.append(_cookies.data(), _cookies.size());
		db.appendValue((unsigned int)_savePath.size());
		db.append(_savePath.data(), _savePath.size());
		db.appendValue(char(_useRedirectedUrl? 1: 0));
		db.appendValue((unsigned long long)_fileSize);
		db.appendValue((unsigned long long)_sheetSize);
		// write header & index to file
		_jobFile.seekg(0, ios::beg);
		writeBytes(db.data(), headerSize);
		writeBytes(_index.data(), indexSize());
		_jobFile.flush();
	}

	void JobFile::read(const string &checkSavePath) {
		if (!_jobFile.is_open()) return;

		// read file content
		WebClient::DataBuffer db;
		_jobFile.seekg(0, ios::end);
		size_t len2read = _jobFile.tellg();
		_jobFile.seekg(0, ios::beg);
		db.resize(len2read);

		size_t done = 0;
		while (done < len2read) {
			if (!_jobFile.read(db.data()+done, len2read-done))
				throw BadJobFile(_jobPath);
			done += _jobFile.gcount();
		}

		// pick out headers & index
		size_t pos = 0;
		unsigned int n = 0;
		char ch;
		unsigned long long ldd;
		unsigned int magic_flag;
		try {
			// magic flag
			db.safeGetValue(pos, magic_flag, &pos);
			if (magic_flag != MAGIC_FLAG) throw BadJobFile(_jobPath);
			// other headers
			db.safeGetValue(pos, n, &pos);
			db.safeGetString(pos, n, _url, &pos);
			db.safeGetValue(pos, n, &pos);
			db.safeGetString(pos, n, _url2, &pos);
			db.safeGetValue(pos, n, &pos);
			db.safeGetString(pos, n, _cookies, &pos);
			db.safeGetValue(pos, n, &pos);
			db.safeGetString(pos, n, _savePath, &pos);
			db.safeGetValue(pos, ch, &pos);
			_useRedirectedUrl = bool(ch);
			db.safeGetValue(pos, ldd, &pos);
			_fileSize = size_t(ldd);
			db.safeGetValue(pos, ldd, &pos);
			_sheetSize = size_t(ldd);
			// read index
			db.safeGetString(pos, indexSize(), _index, &pos);
		} catch (OutOfRange) {
			throw BadJobFile(_jobPath);
		}

		// check file
		if (checkSavePath != _savePath) {
			throw BadJobFile(_jobPath);
		}
	}

	void JobFile::close() throw() {
		try {
			flush();
		} catch (IOException) {}
		_jobFile.close();
	}

	const string &JobFile::getData() const {
		return _index;
	}
	void JobFile::setData(const string &data) {
		if (_index.capacity() != data.size())
			throw BadIndex("Index of " + _savePath + " does not have a valid length.");
		memcpy((char*)_index.data(), data.data(), _index.capacity());
		flush();
	}
	bool JobFile::isValid() const throw() {
		return _jobFile.is_open();
	}
	const string JobFile::identifier() const throw() {
		return _jobPath;
	}
}
