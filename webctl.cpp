/*
 * threadctl.cpp
 *
 *  Created on: 2012-2-21
 *      Author: pwx
 */

#include "webctl.h"
#include <stdio.h>
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
namespace fs = boost::filesystem;
using namespace std;

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
		// create job file directory
		string parent_dir = fs::path(jobPath).parent_path().generic_string();
		if (!fs::exists(parent_dir))
			try {
				fs::create_directories(parent_dir);
			} catch (fs::filesystem_error) {
				throw IOException("Cannot create job file directory.");
			}
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

	// Speed Profile
	size_t KB(1024), MB(1024 * 1024), GB(1024 * 1024 * 1024);
	SpeedProfile SPD_EXTREME(8*MB, 		1, 	16, 128, "extreme"),	// 8 MB/sheet, 	1 sheet/page
				SPD_HIGH	(4*MB, 		2, 	16, 128, "high"),		// 4 MB/sheet, 	2 sheet/page
				SPD_MEDIUM	(1*MB, 		8, 	16, 128, "medium"),		// 1 MB/sheet, 	8 sheet/page
				SPD_LOW		(256*KB, 	32, 16, 128, "low");		// 256 MB/sheet,32 sheet/page

	// WebCtl Utilities
	size_t WebCtl::checkProxies(list<string> &proxies) {
		WebClient::BufferDataWriter db;
		WebClient wc(db);
		list<string> ret;

		wc.setConnectTimeout(10);
		wc.setTimeout(30);
		for(list<string>::const_iterator it=proxies.begin(); it!=proxies.end(); it++) {
			try {
				wc.reset(); db.clear();
				wc.setUrl("http://www.google.com/");
				wc.setProxy(*it);
				if (wc.perform() && db.data().find("google") != string::npos) {
					ret.push_back(*it);
				}
			} catch (Exception) {
				// simply give up proxy
			}
		}

		proxies = ret;
		return ret.size();
	}

	bool WebCtl::checkDownload(const string &url, const string &cookies,
					const string &proxy, long long &fileSize, string &redirected) {
		WebClient::DummyDataWriter db;
		WebClient wc(db);

		wc.setConnectTimeout(10);
		wc.setTimeout(30);
		wc.setUrl(url);
		wc.setProxy(proxy);
		wc.setCookies(cookies);
		wc.setRange("0-1");
		//wc.setHeaderOnly(true);

		if (!wc.perform()) return false;
		if (!wc.supportRange())
			fileSize = -1;
		else
			fileSize = wc.getFileSize();
		redirected = wc.getResponseUrl();

		return true;
	}

	// WebCtl
	WebCtl::WebCtl(JobFile &jobFile, const SpeedProfile &speedProfile, size_t threadPerProxy) :
		_reportLevel(INFO), _speedProfile(speedProfile), _proxies(),
		_threadPerProxy(threadPerProxy),_jobFile(jobFile),
		_fileBuffer(jobFile.savePath(), jobFile.fileSize(), jobFile, jobFile.sheetSize()),
		_sheetCtl(_fileBuffer, speedProfile.pageSize, speedProfile.pageCount, speedProfile.scanCount),
		_running(false), _workers(), _activeWorker(0), _threads(), _threadMutex(), _reportMutex() {
	}

	WebCtl::~WebCtl() {
		WorkerList::iterator it = _workers.begin();
		while (it != _workers.end()) {
			delete *it;
			it++;
		}
		_workers.clear();
	}

	void WebCtl::clearProxies() {
		_proxies.clear();
	}

	void WebCtl::addProxies(const list<string> &proxies) {
		for (list<string>::const_iterator it=proxies.begin(); it!=proxies.end(); it++) {
			_proxies.push_back(*it);
		}
	}

	const string WebCtl::levelName(int level) {
		static string knownLevelNames[] = {"", "DEBUG", "INFO", "WARNING", "CRITICAL"};
		int id = level / 10;
		if (id * 10 != level || id < 1 || id > 4)
			return "Lv" + boost::lexical_cast<string>(level);
		return knownLevelNames[id];
	}

	void WebCtl::report(int level, const string &message) {
		if (level < _reportLevel) return;
		Mutex::scoped_lock lock(_reportMutex);
		string lv = levelName(level);
		printf("[%s] %s\n", lv.c_str(), message.c_str());
	}

	void WebCtl::increaseActive() throw() {
		Mutex::scoped_lock lock(_reportMutex);
		++_activeWorker;
	}
	void WebCtl::decreaseActive() throw() {
		Mutex::scoped_lock lock(_reportMutex);
		--_activeWorker;
	}

	// Thread workers
	WebCtl::Worker::Worker(WebCtl &ctl, const string& proxy) : _ctl(ctl), _proxy(proxy),
			_dw(ctl.speedProfile().sheetSize), _wc(_dw, ctl.speedProfile().sheetSize),
			_isRunning(false) {
		_wc.setProxy(_proxy);
		_wc.setCookies(_ctl.jobFile().cookies());
		_wc.setUrl(_ctl.jobFile().url());
	}

	WebCtl::Worker::~Worker() {}

	const string WebCtl::Worker::getRange(size_t sheet) const throw() {
		size_t start = sheet * _ctl.jobFile().sheetSize(),
				end = (sheet+1) * _ctl.jobFile().sheetSize() - 1;
		if (end >= _ctl.jobFile().fileSize()) end = _ctl.jobFile().fileSize() - 1;
		return boost::lexical_cast<string>(start) + "-" + boost::lexical_cast<string>(end);
	}

	void WebCtl::Worker::terminate() {
		_wc.terminate();
	}

	void WebCtl::Worker::operator()() {
		size_t sheet, token;
		string viaProxy;
		string url = _ctl.jobFile().url();
		string cookies = _ctl.jobFile().cookies();
		if (!_proxy.empty())
			viaProxy = " via proxy " + _proxy;
		// loop and do job
		_isRunning = true;
		_ctl.increaseActive();
		int errorCount = 0, continousError = 0;
		try {
			_ctl.report(DEBUG, "Enter download mode.");
			while (_ctl.isRunning() && _ctl.sheetCtl().fetch(sheet, token)) {
				string range = getRange(sheet);
				_ctl.report(DEBUG, "Download range " + range + " ...");
				_wc.reset(); _dw.clear();
				_wc.setUrl(url);
				_wc.setRange(range);
				_wc.setCookies(cookies);
				_wc.setProxy(_proxy);
				if (!_wc.perform()) {
					++errorCount; ++continousError;
					_ctl.sheetCtl().rollback(sheet, token);
					_ctl.report(ERROR, "Download range " + range + " failed, http code " +
							boost::lexical_cast<string>(_wc.getHttpCode()) + ".");
					if (continousError > MAX_WEBCLIENT_CONTINOUS_ERROR) {
						break; // maximum retry
					}
				} else {
					continousError = 0;
					_ctl.sheetCtl().commit(sheet, token, _dw.data().data());
				}
			}
			_ctl.report(DEBUG, "Leave download mode.");
		} catch (const Exception& ex) {
			_ctl.report(ERROR, "Web client" + viaProxy + " terminated. " + ex.message());
		}
		_ctl.decreaseActive();
		_isRunning = false;
	}

	// create workers & run
	bool WebCtl::isRunning() throw() {
		Mutex::scoped_lock lock(_threadMutex);
		return _running;
	}

	void WebCtl::setRunning(bool running) throw() {
		Mutex::scoped_lock lock(_threadMutex);
		_running = running;
	}

	double WebCtl::getSpeed() {
		Mutex::scoped_lock lock(_threadMutex);
		double ret = 0.0;
		for (WorkerList::iterator it=_workers.begin(); it!=_workers.end(); it++) {
			ret += (*it)->client().getDownloadSpeed();
		}
		return ret;
	}

	void WebCtl::perform() {
		Mutex::scoped_lock lock(_threadMutex);
		if (isRunning())
			throw OperationCannotEmit("Workers are running.");
		setRunning(true);
		// create workers
		list<string>::const_iterator it = _proxies.begin();
		while (it != _proxies.end()) {
			for (size_t i=0; i<_threadPerProxy; i++) {
				Worker *worker = new Worker(*this, *it);
				boost::thread *thread = new boost::thread(boost::ref(*worker));
				_workers.push_back(worker);
				_threads.push_back(thread);
			}
			it++;
		}
	}

	void WebCtl::terminate(size_t waitWebTimeout) {
		{
			Mutex::scoped_lock lock(_threadMutex);
			if (!isRunning()) return;
			report(WARNING, "Terminate workers because of user interrupt.");
			// set stop flag
			setRunning(false);
		}
		{
			// sleep for a short time before kill web clients
			report(INFO, "Wait a short time for net operations.");
			size_t waited = 0;
			while (waited < waitWebTimeout && activeWorker() > 0) {
				waited += 50;
				boost::this_thread::sleep(boost::posix_time::milliseconds(50));
			}
		}
		WorkerList workers;
		ThreadList threads;
		{
			Mutex::scoped_lock lock(_threadMutex);
			workers = _workers;
			threads = _threads;
		}

		// kill objects
		report(INFO, "Kill still alive web clients.");
		// kill web clients
		for (WorkerList::iterator it=workers.begin(); it!=workers.end(); it++) {
			if ((*it)->isRunning()) (*it)->terminate();
		}
		// kill threads
		for (ThreadList::iterator it=threads.begin(); it!=threads.end(); it++) {
			(*it)->interrupt();
		}
		boost::this_thread::sleep(boost::posix_time::milliseconds(50)); // Wait for file flush.
		// dispose objects
		for (WorkerList::iterator it=workers.begin(); it!=workers.end(); it++) {
			delete *it;
		}
		for (ThreadList::iterator it=threads.begin(); it!=threads.end(); it++) {
			delete *it;
		}
		// clear list
		{
			Mutex::scoped_lock lock(_threadMutex);
			_workers.clear();
			_threads.clear();
		}
	}

	void WebCtl::flush() {
		Mutex::scoped_lock lock(_threadMutex);
		_sheetCtl.flush();
		_fileBuffer.flush();
		// _jobFile.flush();
	}

}
