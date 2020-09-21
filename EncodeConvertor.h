/*
 * EncodeConvertor.h
 *
 *  Created on: 21 апреля 2017.
 *      Author: Denis Kashitsin
 */

#ifndef SRC_APPL_ENCODECONVERTOR_H_
#define SRC_APPL_ENCODECONVERTOR_H_

#include "iconv.h"
#include <string>
#include <QTextCodec>
#include <QString>

using namespace std;

class EncodeConvertor
{
public:
    string UTF8toCP866(const string &val);
    string CP866toUTF8(const string &val);
};

#endif /* SRC_APPL_ENCODECONVERTOR_H_ */
