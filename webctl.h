/*
 * threadctl.h
 *
 *  Created on: 2012-2-21
 *      Author: pwx
 */

#ifndef THREADCTL_H_
#define THREADCTL_H_

#include <string>
#include <list>
#include "filebuffer.h"
#include "webclient.h"
#include "sheetctl.h"

namespace PwxGet {
	using namespace std;

	const size_t DEFAULT_THREAD_COUNT = 5;
	const size_t NOSIZE = (size_t)-1;
	const size_t WAIT_SECONDS_BEFORE_TERMINATE = 10000;
	const int MAX_WEBCLIENT_CONTINOUS_ERROR = 100;

	class JobFile : public FileBuffer::PackedIndex {
	public:
		static const unsigned int MAGIC_FLAG;

		// construct & destruct
		JobFile();
		JobFile(const string &savePath);
		virtual ~JobFile() throw();

		// get & set props
		const string &url() const throw() { return _url; }
		// const string &url2() const throw() { return _url2; }
		const string &cookies() const throw() { return _cookies; }
		const string &savePath() const throw() { return _savePath; }
		const string &jobPath() const throw() { return _jobPath; }
		bool useRedirectedUrl() const throw() { return _useRedirectedUrl; }
		size_t fileSize() const throw() { return _fileSize; }
		size_t sheetSize() const throw () { return _sheetSize; }

		void open(const string &savePath);
		void create(const string &url, /*const string &url2, */const string &cookies,
				const string &savePath, bool useRedirectedUrl, size_t fileSize, size_t sheetSize);
		void flush();
		void close() throw();

		// implement packedIndex
		virtual const string &getData() const;
		virtual void setData(const string &data);
		virtual bool isValid() const throw();
		virtual const string identifier() const throw();

	protected:
		string _url, _url2, _cookies;
		string _savePath, _jobPath;
		bool _useRedirectedUrl;
		size_t _fileSize, _sheetSize;
		string _index;
		fstream _jobFile;
		size_t indexSize() const throw();
		size_t headerSize() const throw();
		void writeBytes(const char *data, size_t n);
		void read(const string &checkSavePath);
	};

	// control the download sheet size and page size
	struct SpeedProfile {
	public:
		inline SpeedProfile(size_t sheetSize=DEFAULT_SHEET_SIZE,
				size_t pageSize=DEFAULT_PAGE_SIZE, size_t pageCount=DEFAULT_PAGE_COUNT,
				size_t scanCount=DEFAULT_SCAN_COUNT, const string &name=string()) :
				sheetSize(sheetSize), pageSize(pageSize), pageCount(pageCount),
				scanCount(scanCount), name(name) {}
		inline SpeedProfile(const SpeedProfile &other) : sheetSize(other.sheetSize),
				pageSize(other.pageSize), pageCount(other.pageCount), scanCount(other.scanCount),
				name(other.name) {}
		inline ~SpeedProfile() {}
		size_t sheetSize, pageSize, pageCount, scanCount;
		string name;
	};

	extern size_t KB, MB, GB;
	extern SpeedProfile SPD_EXTREME, SPD_HIGH, SPD_MEDIUM, SPD_LOW;

	class WebCtl {
	public:
		WebCtl(JobFile &jobFile, const SpeedProfile &speedProfile, size_t threadPerProxy=1);
		virtual ~WebCtl();

		// get & set props
		inline const SpeedProfile &speedProfile() const throw() { return _speedProfile; }
		inline const list<string> &proxies() const throw() { return _proxies; }
		inline JobFile &jobFile() throw() { return _jobFile; }
		inline FileBuffer &fileBuffer() throw() { return _fileBuffer; }
		inline SheetCtl &sheetCtl() throw() { return _sheetCtl; }
		inline int &reportLevel() throw() { return _reportLevel; }
		inline size_t activeWorker() const throw() { return _activeWorker; }

		// set proxies
		void clearProxies();
		void addProxies(const list<string> &proxies);

		// console output
		static const int DEBUG = 10, INFO = 20, WARNING = 30, CRITICAL = 40;
		virtual const string levelName(int level);
		virtual void report(int level, const string &message);

		// do perform
		// bool started() const throw();
		bool isRunning() throw();
		void perform();
		void terminate(size_t waitWebTimeout = WAIT_SECONDS_BEFORE_TERMINATE);
		double getSpeed();

		// flush data
		void flush();

		// before perform; utilities
		static size_t checkProxies(list<string> &proxies);
		static bool checkDownload(const string &url, const string &cookies,
				const string &proxy, long long &fileSize, string &redirected);

	protected:
		// The worker to execute the requests
		class Worker {
		public:
			Worker(WebCtl &ctl, const string& proxy);
			~Worker();
			void operator()();
			void terminate();
			inline bool isRunning() const throw() { return _isRunning; }
			inline WebClient &client() { return _wc; }
		protected:
			WebCtl &_ctl;
			string _proxy;
			WebClient::BufferDataWriter _dw;
			WebClient _wc;
			bool _isRunning;
			const string getRange(size_t sheet) const throw();
		};

		typedef boost::recursive_mutex Mutex;
		typedef list<Worker*> WorkerList;
		typedef list<boost::thread*> ThreadList;

		int _reportLevel;
		SpeedProfile _speedProfile;
		list<string> _proxies;
		size_t _threadPerProxy;
		JobFile &_jobFile;
		FileBuffer _fileBuffer;
		SheetCtl _sheetCtl;

		bool _running;
		WorkerList _workers;
		size_t _activeWorker;
		ThreadList _threads;
		Mutex _threadMutex, _reportMutex;

		void setRunning(bool running) throw();
		void increaseActive() throw();
		void decreaseActive() throw();
	};


}

#endif /* THREADCTL_H_ */
