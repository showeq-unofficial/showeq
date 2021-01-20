/*
 *  guild.cpp
 *  Copyright 2001 Fee (fee@users.sourceforge.net). All Rights Reserved.
 *  Portions Copyright 2001-2007, 2009, 2016, 2019 by the respective ShowEQ Developers
 *
 *  Contributed to ShowEQ by fee (fee@users.sourceforge.net)
 *  for use under the terms of the GNU General Public License,
 *  incorporated herein by reference.
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

#include "guild.h"
#include "packet.h"
#include "diagnosticmessages.h"
#include "netstream.h"

#include <QFile>
#include <QDataStream>
#include <QTextStream>

GuildMgr::GuildMgr(QString fn, QObject* parent, const char* name)
  : QObject(parent)
{
  setObjectName(name);
  guildsFileName = fn;

  readGuildList();
}

GuildMgr::~GuildMgr()
{
}

QString GuildMgr::guildIdToName(uint16_t guildID)
{
  if (guildID >= m_guildMap.size())
    return "";
  return m_guildMap[guildID];
}

void GuildMgr::worldGuildList(const uint8_t* data, size_t len)
{
  writeGuildList(data, len);
  readGuildList();
}

void GuildMgr::writeGuildList(const uint8_t* data, size_t len)
{
  QFile guildsfile(guildsFileName);

  if (guildsfile.exists()) {
     if (!guildsfile.remove()) {
       seqWarn("GuildMgr: Could not remove old %s, unable to replace with server data!",
                guildsFileName.toLatin1().data());
        return;
     }
  }

  if(!guildsfile.open(QIODevice::WriteOnly))
    seqWarn("GuildMgr: Could not open %s for writing, unable to replace with server data!",
             guildsFileName.toLatin1().data());

  QDataStream guildDataStream(&guildsfile);

  NetStream netStream(data,len);
  QString guildName;
  QString emptyName = "";
  uint32_t size = 0; // to keep track of how much we're reading from the packet
  uint32_t guildId = 0;

  for (guildId = 0; guildId < 20000; guildId++)
    m_guildList[guildId] = emptyName;
  guildId = 0;

  /*
   0x48 in the packet starts the serialized list.  See guildListStruct
   and worldGuildListStruct in everquest.h
  */

  // skip to the first guild in the list
  netStream.skipBytes(0x44);
  size += 0x44;

  while(!netStream.end())
  {
     guildId = netStream.readUInt32NC();
     size += 4; // four bytes for the guild ID
     netStream.skipBytes(4);
     size += 4; // four bytes added 11/16/2016
     guildName = netStream.readText();

     if(guildName.length())
     {
        m_guildList[guildId] = guildName;

        // add guild name length, plus one for the null character
        size += guildName.length() + 1;
     }

     if(size + 1 == len)
        break; // the end
  }

  std::map<uint32_t, QString>::iterator it;

  for(it = m_guildList.begin(); it != m_guildList.end(); it++)
  {
     char szGuildName[64] = {0};

     strcpy(szGuildName, it->second.toLatin1().data());
     //seqDebug("GuildMgr::writeGuildList - add guild '%s' (%d)", szGuildName, it->first);
     guildDataStream.writeRawData(szGuildName, sizeof(szGuildName));
  }

  guildsfile.close();
  seqInfo("GuildMgr: New guildsfile written");
}

void GuildMgr::readGuildList()
{
  QFile guildsfile(guildsFileName);

  m_guildMap.clear();
  if (guildsfile.open(QIODevice::ReadOnly))
  {
     while (!guildsfile.atEnd())
     {
        char szGuildName[64] = {0};

        guildsfile.read(szGuildName, sizeof(szGuildName));
        //seqDebug("GuildMgr::readGuildList - read guild '%s'", szGuildName);
        m_guildMap.push_back(QString::fromUtf8(szGuildName));
     }

    guildsfile.close();
    seqInfo("GuildMgr: Guildsfile loaded");
  }
  else
    seqWarn("GuildMgr: Could not load guildsfile, %s", guildsFileName.toAscii().data());
}

void GuildMgr::guildList2text(QString fn)
{
  QFile guildsfile(fn);
  QTextStream guildtext(&guildsfile);

    if (guildsfile.exists()) {
         if (!guildsfile.remove()) {
             seqWarn("GuildMgr: Could not remove old %s, unable to process request!",
                   fn.toLatin1().data());
           return;
        }
   }

   if (!guildsfile.open(QIODevice::WriteOnly)) {
     seqWarn("GuildMgr: Could not open %s for writing, unable to process request!",
              fn.toLatin1().data());
      return;
   }

   for (unsigned int i =0 ; i < m_guildMap.size(); i++) 
   {
       if (!m_guildMap[i].isNull())
          guildtext << i << "\t" << m_guildMap[i] << endl;
   }

   guildsfile.close();

   return;
}


void GuildMgr::listGuildInfo()
{
   for (unsigned int i = 0; i < m_guildMap.size(); i++)
   {
     if (!m_guildMap[i].isNull())
       seqInfo("%d\t%s", i, m_guildMap[i].toAscii().data());
   }
}

#ifndef QMAKEBUILD
#include "guild.moc"
#endif

