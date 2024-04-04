#ifndef NET_LOGIN_H
#define NET_LOGIN_H

#include <Arduino.h>

bool buptLogin(const String& url, const String& ref);
bool testPage(const String& url, String& retLoc);
bool testPage(String url);

#endif