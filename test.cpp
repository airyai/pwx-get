#include "filebuffer.h"
#include "webclient.h"
#include "exceptions.h"
#include "test.h"
#include <boost/lexical_cast.hpp>

using namespace PwxGet;
namespace fs2 = boost::filesystem;

void assertTrue(bool v, const string& message) {
    if (!v) throw AssertionError(message);
}

template <typename T>
string toString(const T& v) {
    return toString(v);
}

void test_all() {
    cout << "Testing file buffer ..." << endl;
    test_file_buffer();
    cout << "Testing web client ..." << endl;
    test_web_client();
    cout << "All tests passed!" << endl;
}

void test_file_buffer() {
    const string path = "/tmp/dat";
    const string indexPath = "/tmp/dat.index";
    
    if (fs2::exists(path)) fs2::remove(path);
    if (fs2::exists(indexPath)) fs2::remove(indexPath);
    assertTrue(!fs2::exists(path) && !fs2::exists(indexPath), "Target files should not exist.");
    
    {
        FileBuffer fb(path, 12345, indexPath, 2);
        assertTrue(fb.sheetCount() == 12345 / 2 + 1, "SheetCount does not agree.");
        byte data[] = {1,2,3,4,5,6};
        assertTrue(fb.write(data, 2, 3) == (3), "Write failed.");
    }
    
    {
        FileBuffer fb(path, 12345, indexPath, 2);
        byte data[6] = {0};
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
    wc.setVerbose(true);
    double vd = 0;
    
    // test ordinary get
    wc.reset(); wc.setUrl("http://www.baidu.com/");
    assertTrue(wc.perform(), "Cannot perform http request.");
    assertTrue(dw.data().find("030173") != string::npos, 
            "Cannot fetch valid content of http://www.baidu.com/");
    curl_easy_getinfo(wc.handle(), CURLINFO_CONTENT_LENGTH_DOWNLOAD, &vd);
    assertTrue((size_t)vd == wc.getResponseLength(),  "Content-Length does not agree: " + 
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
    assertTrue(dw.data().find("google") != string::npos,
            "Cannot fetch valid content of https://www.google.com/.");
    assertTrue(wc.getHttpCode() == 200, "Response code does not agree: " + toString(wc.getHttpCode()));
}
