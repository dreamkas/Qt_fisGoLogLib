/*
 * EncodeConvertor.h
 */

#include "EncodeConvertor.h"
#include <string.h>

//-----------------------------------------------

string EncodeConvertor::UTF8toCP866(const string &val )
{
    QString encodedString(val.c_str());
    QTextCodec *codec = QTextCodec::codecForName("IBM 866");
    return (val.length() > 0) ?  codec->fromUnicode(encodedString).toStdString() : "";
}

string EncodeConvertor::CP866toUTF8(const string &val)
{
    QByteArray encodedString(val.c_str());
    QTextCodec *codec = QTextCodec::codecForName("IBM 866");
    return (val.length() > 0) ? codec->toUnicode(encodedString).toStdString() : "";

}
