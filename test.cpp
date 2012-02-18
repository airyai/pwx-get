#include "filebuffer.h"
#include "exceptions.h"

using namespace PwxGet;
namespace fs2 = boost::filesystem;

void assertTrue(bool v, const string& message) {
    if (!v) throw RuntimeError(message);
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
