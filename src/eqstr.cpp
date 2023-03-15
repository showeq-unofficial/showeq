/*
 *  eqstr.cpp
 *  Copyright 2002-2003 Zaphod (dohpaz@users.sourceforge.net)
 *  Copyright 2003-2004, 2016, 2019 by the respective ShowEQ Developers
 *
 *  This file is part of ShowEQ.
 *  http://www.sourceforge.net/projects/seq
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "eqstr.h"
#include "diagnosticmessages.h"
#include "packetcommon.h"

#include <cstdio>

#include <QRegExp>
#include <QFile>
#include <QStringList>
#include <QVector>
#include <QString>

EQStr::EQStr()
  : m_messageStrings(),
    m_loaded(false)
{
}

EQStr::~EQStr()
{
  m_messageStrings.clear();
}

bool EQStr::load(const QString& fileName)
{
  // clear out any existing contents
  m_messageStrings.clear();

  // create a QFile on the file
  QFile formatFile(fileName);

  // open the file read only
  if (!formatFile.open(QIODevice::ReadOnly))
  {
    seqWarn("EQStr: Failed to open '%s'", fileName.toLatin1().data());
    return false;
  }

  // allocate a QByteArray large enough to hold the entire file
  QByteArray textData(formatFile.size() + 1, '\0');

  // read in the entire file
  formatFile.read(textData.data(), textData.size());
  
  // construct a regex to deal with either style line termination
  QRegExp lineTerm("[\r\n]{1,2}");

  // split the data into lines at the line termination
  QStringList lines = QString::fromUtf8(textData).split(lineTerm, QString::SkipEmptyParts);

  // start iterating over the lines
  QStringList::Iterator it = lines.begin();
  
  // first is the magic id string
  QString magicString = (*it++);
  int spc;
  uint32_t formatId;
  QString formatString;
  uint32_t maxFormatId = 0;
  
  // next skip over the count, etc...
  it++;
  
  // now iterate over the format lines
  for (; it != lines.end(); ++it)
  {
    // find the beginning space
    spc = (*it).indexOf(' ');

    // convert the beginnign of the string to a ULong
    formatId = (*it).left(spc).toULong();
    
    if (formatId > maxFormatId) 
      maxFormatId = formatId;
    
    // insert the format string into the dictionary.
    m_messageStrings.insert(formatId, QString((*it).mid(spc+1)));    
  }

  // note that strings are loaded
  m_loaded = true;

  seqInfo("Loaded %d message strings from '%s' maxFormat=%d",
          m_messageStrings.count(), fileName.toLatin1().data(),
          maxFormatId);

  return true;
}

QString EQStr::find(uint32_t formatid) const
{
  // attempt to find the message string
  QString res = m_messageStrings.value(formatid, QString());

  return res;
}

QString EQStr::message(uint32_t formatid) const
{
  // attempt to find the message string
  QString res = m_messageStrings.value(formatid, QString());

  // if the message string was found, return it
  if (!res.isEmpty())
    return res;

  // otherwise return a fabricated string
  return QString("Unknown: ") + QString::number(formatid, 16);
}

QString EQStr::formatMessage(uint32_t formatid, 
			     const char* arguments, size_t argsLen) const
{
  QString formatStringRes = m_messageStrings.value(formatid, QString());

  QString tempStr;

    if (formatStringRes.isEmpty())
    {
	uint32_t arg_len;
	unsigned char *cp;
#if (QT_VERSION >= QT_VERSION_CHECK(5,5,0))
	tempStr.asprintf( "Unknown: %04x:", formatid);
#else
	tempStr.sprintf( "Unknown: %04x:", formatid);
#endif
	cp = (unsigned char *) arguments;
	while (cp < ((unsigned char *) &arguments[argsLen] - sizeof(uint32_t)*sizeof(unsigned char))) {
	    arg_len = (cp[0] << 0) | (cp[1] << 8) | (cp[2] << 16) | (cp[3] << 24);
	    cp += 4;
	    if (arg_len == 0 || arg_len > argsLen)
		break;
	    tempStr += " ";
	    tempStr += QString::fromUtf8((const char *) cp, arg_len);
	    cp += arg_len;
	}
	return tempStr;
    }
    else
    {
	QVector<QString> argList;
	argList.reserve(5); // reserve space for 5 elements to handle most common sizes

	//Adjusted to handle prepended string length 05/28/2019
	size_t totalArgsLen = 0;
	const char* curArg;
        uint32_t curSize = 0;
	while (totalArgsLen < argsLen)
	{
	    curArg = arguments + totalArgsLen;
            curSize = eqtohuint32((const uint8_t*) curArg);
            curArg += 4;

            if (curSize > 0) {
	        // insert argument into the argument list
	        argList.push_back(QString::fromUtf8(curArg, curSize));
            }

	    totalArgsLen += curSize + 4;
	}

	bool ok;
	int curPos;
	int substArg;
	int substArgValue;
	QString substFormatStringRes;
	QString substFormatString;

	////////////////////////////
	// replace template (%T) arguments in formatted string
	QString formatString = formatStringRes;
	QRegExp rxt("%T(\\d{1,3})");

	// find first template substitution
	curPos = rxt.indexIn(formatString, 0);

	while (curPos != -1)
	{
	    substFormatStringRes = QString();
	    substArg = rxt.cap(1).toInt(&ok);
	    if (ok && (substArg <= argList.size()))
	    {
		substArgValue = argList[substArg-1].toInt(&ok);

		if (ok)
		    substFormatStringRes = m_messageStrings.value(substArgValue, QString());
	    }

	    // replace template argument with subst string
	    if (!substFormatStringRes.isEmpty())
		formatString.replace(curPos, rxt.matchedLength(), substFormatStringRes);
	    else
		curPos += rxt.matchedLength(); // if no replacement string, skip over

	    // find next substitution
	    curPos = rxt.indexIn(formatString, curPos);
	}

	////////////////////////////
	// now replace substitution arguments in formatted string
	// NOTE: not using QString::arg() because not all arguments are always used
	//       and it will do screwy stuff in this situation
	QRegExp rx("%(\\d{1,3})");

	// find first template substitution
	curPos = rx.indexIn(formatString, 0);

	while (curPos != -1)
	{
	    substArg = rx.cap(1).toInt(&ok);

	    // replace substitution argument with argument from list
	    if (ok && (substArg <= argList.size()))
		formatString.replace(curPos, rx.matchedLength(), argList[substArg-1]);
	    else
		curPos += rx.matchedLength(); // if no such argument, skip over

	    // find next substitution
	    curPos = rx.indexIn(formatString, curPos);
	}

	return formatString;
    }

}
