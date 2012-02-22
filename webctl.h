/*
 * threadctl.h
 *
 *  Created on: 2012-2-21
 *      Author: pwx
 */

#ifndef THREADCTL_H_
#define THREADCTL_H_

#include <string>
#include "filebuffer.h"
#include "webclient.h"
#include "sheetctl.h"

namespace PwxGet {
	using namespace std;

	const size_t DEFAULT_THREAD_COUNT = 5;
	const size_t NOSIZE = (size_t)-1;

	class JobFile : public FileBuffer::PackedIndex {
	public:
		static const unsigned int MAGIC_FLAG;

		// construct & destruct
		JobFile();
		/* JobFile(const string &url,
				const string &savePath,
				bool useRedirectedUrl,
				size_t fileSize=0,
				size_t sheetSize=DEFAULT_SHEET_SIZE);*/
		JobFile(const string &savePath);
		virtual ~JobFile() throw();

		// get & set props
		const string &url() const throw() { return _url; }
		// const string &url2() const throw() { return _url2; }
		const string &cookies() const throw() { return _cookies; }
		const string &savePath() const throw() { return _savePath; }
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

	class WebCtl {
	public:
		WebCtl(const string &url,
				const string &savePath,
				size_t threadCount=DEFAULT_THREAD_COUNT,
				size_t sheetSize=DEFAULT_SHEET_SIZE,
				size_t pageSize=DEFAULT_PAGE_SIZE,
				size_t pageCount=DEFAULT_PAGE_COUNT,
				size_t scanCount=DEFAULT_SCAN_COUNT);
		virtual ~WebCtl();
	protected:
		// The worker to execute the requests
		class Worker {
		public:
			Worker(WebCtl &ctl);
			~Worker();

			void operator()();
		protected:
			WebCtl &_ctl;
		};

		string _url, _savePath;
		size_t _threadCount;
		JobFile _jobFile;
		FileBuffer _fileBuffer;
		SheetCtl _sheetCtl;
	};

}

#endif /* THREADCTL_H_ */
