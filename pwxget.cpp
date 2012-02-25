//============================================================================
// Name        : pwxget.cpp
// Author      : PWX
// Version     :
// Copyright   : Copy PWX's Workshop 2012
// Description : Hello World in C++, Ansi-style
//============================================================================
#include <string>
#include <list>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include "webctl.h"

#ifdef WINNT
#include <getopt.h>
#endif

using namespace std;
using namespace PwxGet;

namespace fs = boost::filesystem;

// parse arguments
struct Arguments {
public:
	size_t threadPerProxy;
	string url;
	string url2;
	string savePath;
	string cookies;
	bool direct;
	list<string> proxies;
	bool useRedirectedUrl;
	SpeedProfile speedProfile;

	inline Arguments() : threadPerProxy(1), url(), url2(), savePath(), cookies(),
			direct(false), proxies(), useRedirectedUrl(false), speedProfile() {
	}
	inline ~Arguments() {}

/*	inline void dump() {
		cout 	<< "ThreadPerProxy=" << threadPerProxy << endl
				<< "Url=" << url << endl
				<< "SavePath=" << savePath << endl
				<< "Cookies=" << cookies << endl;
		for (list<string>::const_iterator it=proxies.begin(); it!=proxies.end(); it++) {
			cout << "Proxies=" << *it << endl;
		}
		cout	<< "UseRedirectedUrl=" << (useRedirectedUrl? "on": "off") << endl
				<< "SpeedProfile=" << speedProfile.name << endl;
	}*/
} arguments;
static const char *optFormat = "n:c:p:drs:h?";
int retCode = 0;

void usage() {
	printf(	"pwxget - Download from multi proxy servers.\n"
			"Usage: pwxget [options] ... target-url output-path\n"
			"\n"
			"  -n [count]       Downloading threads for each proxy.\n"
			"  -c [cookies]     HTTP Cookies.\n"
			"  -p [proxy]       Add a proxy server in protocol://server[:port]/.\n"
			"                   Protocols may be http, socks4, socks4a, socks5 or socks5h.\n"
			"  -d               Download file through direct connection as well.\n"
			"  -r               If request got an HTTP redirection, new threads should use\n"
			"                   the redirected url instead of the origin one.\n"
			"  -s [profile]     Speed profile. Control the file sheet and memory cache size.\n"
			"                   Profile may be extreme, high, medium and low.\n"
			"  -h, -?           Show usage.\n"
			"\n"
			"Target url will be downloaded and saved to output path.\n");
}

bool parseArguments(int argc, char **argv) {
	arguments.speedProfile = SPD_MEDIUM;

	int opt = getopt(argc, argv, optFormat);
	while (opt != -1) {
		switch (opt) {
		case 'n':
			try {
				arguments.threadPerProxy = boost::lexical_cast<size_t>(optarg);
			} catch (boost::bad_lexical_cast) {
				retCode = 1;
				return false;
			}
			break;
		/*case 'o':
			arguments.savePath = string(optarg);
			break;*/
		case 'c':
			arguments.cookies = string(optarg);
			break;
		case 'p':
			arguments.proxies.push_back(string(optarg));
			break;
		case 'd':
			arguments.proxies.push_back(string());
			break;
		case 'r':
			arguments.useRedirectedUrl = true;
			break;
		case 's':
			if (strcmp(optarg, "extreme") == 0)
				arguments.speedProfile = SPD_EXTREME;
			else if (strcmp(optarg, "high") == 0 || strcmp(optarg, "fast") == 0)
				arguments.speedProfile = SPD_HIGH;
			else if (strcmp(optarg, "medium") == 0 || strcmp(optarg, "normal") == 0)
				arguments.speedProfile = SPD_MEDIUM;
			else if (strcmp(optarg, "low") == 0 || strcmp(optarg, "slow") == 0)
				arguments.speedProfile = SPD_LOW;
			else {
				retCode = 2;
				return false;
			}
			break;
		case 'h':
		case '?':
			return false;
		default:
			break;
		}

		opt = getopt(argc, argv, optFormat);
	}
	if (optind != argc - 2) {
		retCode = 3;
		return false;
	}

	arguments.url = string(argv[optind]);
	arguments.savePath = string(argv[optind+1]);
	return true;
}

// format size humanly
string humanSize(double size) {
	double gb = GB*0.9,
			mb = MB*0.9,
			kb = KB*0.9;
	string suffix = "B";
	double num = size;
	char buffer[32] = {0};

	if (size >= gb) {
		suffix = "GB";
		num = num / GB;
	} else if (size >= mb) {
		suffix = "MB";
		num = num / MB;
	} else if (size >= kb) {
		suffix = "KB";
		num = num / KB;
	}

	sprintf(buffer, "%.2f %s", num, suffix.c_str());
	return string(buffer);
}

string humanTime(time_t time) {
	list<string> ret;
	time_t d, h, m, s;

	s = time;
	d = s / 86400; s = s - d * 86400;
	h = s / 3600;  s = s - h * 3600;
	m = s / 60;    s = s - m * 60;

	if (d) ret.push_back(boost::lexical_cast<string>(d) + "d");
	if (h) ret.push_back(boost::lexical_cast<string>(h) + "h");
	if (m) ret.push_back(boost::lexical_cast<string>(m) + "m");
	if (s) ret.push_back(boost::lexical_cast<string>(s) + "s");

	return boost::join(ret, string(" "));
}

// Instances
JobFile *globalJobFile = NULL;
WebCtl *globalWebCtl = NULL;
time_t beginTime = time(NULL);

// Exit signal handling
#include <signal.h>
void signal_callback_handler(int signum) {
	printf("\n");
	if (globalWebCtl) {
		if (globalWebCtl->activeWorker() > 0)
			globalWebCtl->terminate();
		globalWebCtl->flush();
		globalWebCtl->fileBuffer().close();
		if (globalJobFile) {
			globalJobFile->close();
		}
	}
	string duration = humanTime(time(NULL) - beginTime);
	printf("Download terminated, %s elapsed.\n", duration.c_str());
	exit(20);
}

void register_signals() {
#ifndef WIN32
	signal(SIGHUP, signal_callback_handler);
	signal(SIGQUIT, signal_callback_handler);
	signal(SIGKILL, signal_callback_handler);
#endif
	signal(SIGINT, signal_callback_handler);
	signal(SIGILL, signal_callback_handler);
	signal(SIGABRT, signal_callback_handler);
}

// Main Program
int main(int argc, char **argv) {
	// register signals
	register_signals();

	// parse arguments
	if (!parseArguments(argc, argv)) {
		usage();
		return retCode;
	}

	// check proxies
	if (arguments.proxies.size() == 0) arguments.direct = true;
	printf("Checking proxies ... ");
	printf("%llu proxies found.\n", (unsigned long long)WebCtl::checkProxies(arguments.proxies));
	if (arguments.direct) arguments.proxies.push_back(string());
	if (arguments.proxies.size() == 0) {
		printf("Direct connection is disabled, while no available proxy found.\n");
		return 10;
	}

	// getting target status
	long long tmpSize = -1;
	size_t fileSize;
	printf("Preparing for download ...\n");
	if (!WebCtl::checkDownload(arguments.url, arguments.cookies, arguments.proxies.front(),
			tmpSize, arguments.url2)) {
		printf("Target url cannot be reached.\n");
		return 11;
	}
	if (tmpSize <= 0) {
		printf("Cannot download target partially.\n");
		return 12;
	}
	fileSize = size_t(tmpSize);
	string url = arguments.useRedirectedUrl? arguments.url: arguments.url2;

	// open / create job file
	JobFile jobfile;
	try {
		if (fs::exists(arguments.savePath)) {
			try {
				jobfile.open(arguments.savePath);
			} catch (const JobNotExists& ex) {
				printf("Output path already exists.\n");
				return 15;
			}
		} else {
			jobfile.create(url, arguments.cookies, arguments.savePath, arguments.useRedirectedUrl,
					fileSize, arguments.speedProfile.sheetSize);
		}
	} catch (const Exception &ex) {
		string errmsg = ex.message();
		printf("Cannot open job file. %s\n", errmsg.c_str());
		return 13;
	}
	globalJobFile = &jobfile;

	// creating web controller
	WebCtl *webctl = NULL;
	try {
		webctl = new WebCtl(jobfile, arguments.speedProfile, arguments.threadPerProxy);
	} catch (const Exception &ex) {
		string errmsg = ex.message();
		printf("Initializing thread engine failed. %s\n", errmsg.c_str());
		if (webctl != NULL) delete webctl;
		return 14;
	}
	webctl->addProxies(arguments.proxies);
	webctl->reportLevel() = 9999; // disable webctl report
	globalWebCtl = webctl;

	// emiting download
	webctl->perform();
	size_t sheetCount = webctl->sheetCtl().sheetCount();
	string totalSize = humanSize(fileSize);

	int sleepMs = 2000;
	//size_t lastDoneBytes = min(webctl->sheetCtl().doneSheet() * jobfile.sheetSize(), fileSize);
	size_t pageAbstractSize = webctl->sheetCtl().cache().pageSize() * jobfile.sheetSize();
	boost::this_thread::sleep(boost::posix_time::milliseconds(sleepMs));
	char outputBuffer[1024] = {0};
	size_t lastOutputLength = 0, curOutputLength = 0;

	while (true) {
		// generate vars
		size_t doneSheet = webctl->sheetCtl().doneSheet();
		size_t doneBytes = min(doneSheet * jobfile.sheetSize(), fileSize);
		string percent = boost::lexical_cast<string>(doneSheet * 100 / sheetCount),
				doneSize = humanSize(doneBytes);
		//string speed = humanSize((doneBytes - lastDoneBytes) * 1000.0 / sleepMs);
		string speed = humanSize(webctl->getSpeed());
		string workPageSize = humanSize(webctl->sheetCtl().workPageCount() * pageAbstractSize),
				allPageSize = humanSize(webctl->sheetCtl().pageCount() * pageAbstractSize);
		//lastDoneBytes = doneBytes;
		// clear last line
		// make current line
		sprintf(outputBuffer, "Progress: %s%%, %s/%s. Speed: %s/s. Cache: %s/%s.", percent.c_str(),
				doneSize.c_str(), totalSize.c_str(), speed.c_str(), workPageSize.c_str(),
				allPageSize.c_str() );
		curOutputLength = strlen(outputBuffer);
		printf("\r%s", outputBuffer);
		int lenOffset = lastOutputLength-curOutputLength;
		if (lenOffset > 0) {
			for (int i=0; i<lenOffset; i++)
				outputBuffer[i] = ' ';
			outputBuffer[lenOffset] = 0;
			printf("%s", outputBuffer);
		}
		fflush(stdout);
		lastOutputLength = curOutputLength;
		// wait and sleep
		if (webctl->activeWorker() == 0) break;
		for (int i=0; i<sleepMs/200; i++) {
			boost::this_thread::sleep(boost::posix_time::milliseconds(200));
		}
	}
	printf("\n");
	string duration = humanTime(time(NULL) - beginTime);

	// remove progress file
	webctl->flush();
	webctl->fileBuffer().close();
	jobfile.close();
	if (webctl->sheetCtl().allDone()) {
		fs::remove(jobfile.jobPath());
		printf("Download complete, %s elapsed.\n", duration.c_str());
	} else {
		printf("Download unfinished, %s elapsed.\n", duration.c_str());
	}

	return 0;
}
