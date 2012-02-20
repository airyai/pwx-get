/* 
 * File:   test.h
 * Author: pwx
 *
 * Created on 2012年2月18日, 下午4:38
 */

#ifndef TEST_H
#define	TEST_H

#include <string>
using namespace std;

void assertTrue(bool v, const string& message);
void test_all();
void test_file_buffer();
void test_web_client();

#endif	/* TEST_H */

