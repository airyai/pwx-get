#include "filebuffer.h"
#include "webclient.h"
#include "controller.h"
#include "exceptions.h"
#include "test.h"
#include <boost/lexical_cast.hpp>

using namespace PwxGet;
namespace fs2 = boost::filesystem;

template <typename T>
string toString(const T& v) {
    return boost::lexical_cast<string>(v);
}

#define assertTrue(v, message) _assertTrue(v, message, __FILE__, __LINE__)
void _assertTrue(bool v, const string& message, const char* file, size_t lineno) {
    if (!v) throw AssertionError("[" + string(file) + ":" + toString(lineno) + "] " + message);
}

void test_all() {
    cout << "Testing file buffer ..." << endl;
    test_file_buffer();
    //cout <<(byte*) "Testing web client ..." << endl;
    //test_web_client();
    cout << "Testing paged memory cache ..." << endl;
    test_paged_memory_cache();
    cout << "All tests passed!" << endl;
}

void test_file_buffer() {
    const string path = "/tmp/dat";
    const string indexPath = "/tmp/dat.index";
    
    if (fs2::exists(path)) fs2::remove(path);
    if (fs2::exists(indexPath)) fs2::remove(indexPath);
    assertTrue(!fs2::exists(path) && !fs2::exists(indexPath), "Target files should not exist.");
    
    {
        FileBuffer::PackedIndexFile packedIndex(indexPath);
        FileBuffer fb(path, 12345, packedIndex, 2);
        assertTrue(fb.sheetCount() == 12345 / 2 + 1, "SheetCount does not agree.");
        byte data[] = {1,2,3,4,5,6};
        assertTrue(fb.write(data, 2, 3) == (3), "Write failed.");
        assertTrue(fb.write(data, 6172, 1) == (1), "Write failed.");
    }
    
    {
        assertTrue(fs2::file_size(path) == 12345, "File total size does not agree: "
                + toString(fs2::file_size(path)));
        FileBuffer::PackedIndexFile packedIndex(indexPath);
        FileBuffer fb(path, 12345, packedIndex, 2);
        byte data[6] = {0};
        assertTrue(fb.read(data, 6172, 1) == 1, "Read last sheet failed.");
        assertTrue(data[0] == 1 && data[1] == 0, "Last sheet does not agree.");
        assertTrue(fb.read(data, 2, 3) == 3, "Read failed.");
        assertTrue(data[0] == 1 && data[1] == 2 && data[2] == 3 && data[3] == 4
                && data[4] == 5 && data[5] == 6, "Data error after read.");
        assertTrue(fb.index()[2] == 1 && fb.index()[3] == 1 && fb.index()[4] == 1,
                "Sheet data does not agree.");
        assertTrue(fb.doneSheet() == 3, "Done sheet counter does not agree.");
        
        for (int i=0; i<6; i++) { data[i] = data[i] + 'a'; }
        assertTrue(fb.write(data, 3, 3) == (3), "Write failed.");
        assertTrue(fb.doneSheet() == 4, "Done sheet counter does not agree after write.");
        
        fb.erase(4, 3);
        assertTrue(fb.doneSheet() == 2, "Done sheet counter does not agree after erase.");
    }
}

void test_web_client() {
    WebClient::BufferDataWriter dw;
    WebClient wc(dw);
    //wc.setVerbose(true);
    double vd = 0;
    
    // test ordinary get
    wc.reset(); wc.setUrl("http://www.baidu.com/");
    assertTrue(wc.perform(), "Cannot perform http request.");
    assertTrue(dw.data().find("030173") != string::npos, 
            "Cannot fetch valid content of http://www.baidu.com/");
    curl_easy_getinfo(wc.handle(), CURLINFO_CONTENT_LENGTH_DOWNLOAD, &vd);
    assertTrue((long)vd == wc.getResponseLength(),  "Content-Length does not agree: " + 
            toString(wc.getResponseLength()) + " != " + toString((size_t)vd));
    assertTrue(wc.getHttpCode() == 200, "Response code does not agree: " + toString(wc.getHttpCode()));
    assertTrue(wc.getResponseUrl() == "http://www.baidu.com/", 
            "Response url does not agree: " + wc.getResponseUrl() + " != http://www.baidu.com/");
    
    // test redirect get
    wc.reset(); dw.clear();
    wc.setUrl("http://www.google.com/ncr");
    assertTrue(wc.perform(), "Cannot perform http request.");
    assertTrue(dw.data().find("google") != string::npos,
            "Cannot fetch valid content of http://www.google.com/ncr");
    assertTrue(wc.getHttpCode() == 200, "Response code does not agree: " + 
            toString(wc.getHttpCode()));
    assertTrue(wc.getResponseUrl() == "http://www.google.com/", 
            "Response url does not agree: " + wc.getResponseUrl() + " != http://www.google.com/");
    assertTrue(!wc.supportRange(), "Response should not accept ranges.");
    
    // test head support
    wc.reset(); dw.clear();
    wc.setUrl("http://korepwx.com/10M");
    wc.setHeaderOnly(true);
    assertTrue(wc.perform(), "Cannot perform http request.");
    assertTrue(dw.data()[0] == 0, "HEAD method should not receive any content.");
    assertTrue(wc.getHttpCode() == 200, "Response code does not agree: " + toString(wc.getHttpCode()));
    assertTrue(wc.getResponseLength() == 10485760, "Content-Length does not agree: " + 
            toString(wc.getResponseLength()) +" != 10485760");
    assertTrue(wc.supportRange(), "Response should accept ranges.");
    
    // test cookies
    wc.reset(); dw.clear();
    wc.setCookies("value=hello");
    wc.setUrl("http://sola.korepwx.com/echo/cookies");
    assertTrue(wc.perform(), "Cannot perform http request.");
    assertTrue(dw.data().find("{'value': 'hello'}") != string::npos,
            "Cookies have not been uploaded: expect value=hello.");
    assertTrue(wc.perform(), "Cannot perform http request.");
    assertTrue(dw.data().find("'mark'") != string::npos,
            "Cannot preserve server-side new cookies.");
    
    // test range
    wc.reset(); dw.clear();
    wc.setUrl("http://korepwx.com/128K");
    wc.setRange("10-225");
    assertTrue(wc.perform(), "Cannot perform http request.");
    assertTrue(dw.data().size() == 216, "Ranged data length does not agree: " + 
            toString(dw.data().size()));
    for (size_t i=0; i<108; i++) {
        assertTrue(dw.data().at(i*2) == (i+5) && dw.data().at(i*2+1) == 0,
                "Partial data content does not agree.");
    }
    
    // test 404
    wc.reset(); dw.clear();
    wc.setUrl("http://www.google.com/nothing");
    assertTrue(wc.perform(), "Cannot perform http request.");
    assertTrue(wc.getHttpCode() == 404, "Http code should be 404.");
    
    // test malformed url
    wc.reset(); dw.clear();
    wc.setUrl("这不是一个合法的URL");
    assertTrue(!wc.perform(), "Malformed url should not be executed successfully.");
    assertTrue(!wc.errorMessage().empty(), "No error message for malformed url.");
    
    // test https
    wc.reset(); dw.clear();
    wc.setUrl("https://www.google.com/");
    assertTrue(wc.perform(), "Cannot perform http request.");
    assertTrue(dw.data().find("google") != string::npos,
            "Cannot fetch valid content of https://www.google.com/.");
    assertTrue(wc.getHttpCode() == 200, "Response code does not agree: " + toString(wc.getHttpCode()));
    
    // test proxy & http
    wc.reset(); dw.clear();
    wc.setProxy("socks5h://127.0.0.1:3127/");
    wc.setUrl("http://www.facebook.com/");
    assertTrue(wc.perform(), "Cannot perform http request.");
    assertTrue(dw.data().find("facebook") != string::npos,
            "Cannot fetch valid content of http://www.facebook.com/ via proxy.");
    assertTrue(wc.getHttpCode() == 200, "Response code does not agree: " + toString(wc.getHttpCode()));
    
    // test clear proxy
    wc.reset(); dw.clear();
    wc.setUrl("http://www.google.com/");
    assertTrue(wc.perform(), "Cannot perform http request.");
    assertTrue(dw.data().find("google") != string::npos,
            "Cannot fetch valid content of http://www.google.com/.");
    assertTrue(wc.getHttpCode() == 200, "Response code does not agree: " + toString(wc.getHttpCode()));
}

bool test_paged_memory_cache_check_file(FileBuffer &fb, char *expect) {
    WebClient::DataBuffer db(fb.size());
    fb.read((byte*)db.data(), 0, fb.sheetCount());
    for (size_t i=0; i<fb.size(); i++) {
        if (db.data()[i] != expect[i]) return false;
    }
    return true;
}

void test_paged_memory_cache() {
    const string path = "/tmp/dat";
    const string indexPath = "/tmp/dat.index";
    
    if (fs2::exists(path)) fs2::remove(path);
    if (fs2::exists(indexPath)) fs2::remove(indexPath);
    assertTrue(!fs2::exists(path) && !fs2::exists(indexPath), "Target files should not exist.");
    
    FileBuffer::PackedIndexFile ind(indexPath);
    FileBuffer fb(path, 31, ind, 2);
    PagedMemoryCache pmc(fb, 4, 3);
    
    // [( 0,  1), ( 2,  3), ( 4,  5), ( 6,  7)] [( 8,  9), (10, 11), (12, 13), (14, 15)]
    // [(13, 14), (15, 16), (17, 18), (19, 20)] [(21, 22), (23, 24), (25, 26), (27, 28)]
    char sheetData[][2] = {
        {1, 2}, {3, 4}, {5, 6}, {7, 8}, {9, 10}, {11, 12},
        {13, 14}, {15, 16}, {17, 18}, {19, 20}, {21, 22},
        {23, 24}, {25, 26}, {27, 28}, {29, 30}, {31, 0} 
    };
    char expectData[31] = {0};
    
    // test cache
    pmc.commit(0, sheetData[0]); fb.flush();
    assertTrue(test_paged_memory_cache_check_file(fb, expectData),
            "Cached data shoult not flush into file so early.");
    
    // test finish sheet
    pmc.commit(1, sheetData[1]); 
    pmc.commit(2, sheetData[2]);
    pmc.commit(3, sheetData[3]); fb.flush();
    memcpy(expectData, sheetData, 8);
    assertTrue(test_paged_memory_cache_check_file(fb, expectData),
            "Complete sheet should be flushed into file.");
}
