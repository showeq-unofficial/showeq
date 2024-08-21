/*
 *  filtermgr.h
 *  Copyright 2001-2005, 2019 by the respective ShowEQ Developers
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


//
// NOTE: Trying to keep this file ShowEQ/Everquest independent to allow it
// to be reused for other Show{} style projects.  Any existing ShowEQ/EQ
// dependencies will be migrated out.
//

#ifndef FILTERMGR_H
#define FILTERMGR_H

#ifdef __FreeBSD__
#include <sys/types.h>
#else
#include <cstdint>
#endif

#include <map>

#include <QObject>
#include <QDialog>
#include <QString>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>

#include "everquest.h"

//----------------------------------------------------------------------
// forward declarations
class Filter;
class Filters;
class FilterTypes;
class DataLocationMgr;

//
// ZBTEMP: predefined filters and filter mask will be migrated out
// so that ShowEQ code can register the file based filters and there mask
// at runtime ala the runtime Filter stuff
//

//----------------------------------------------------------------------
// Macro defintions
//Filter Flags
#define HUNT_FILTER 0
#define CAUTION_FILTER 1
#define DANGER_FILTER 2
#define LOCATE_FILTER 3
#define ALERT_FILTER 4
#define FILTERED_FILTER 5
#define TRACER_FILTER 6
#define SIZEOF_FILTERS 7

// max of 32 flags

#define FILTER_FLAG_HUNT		(1 << HUNT_FILTER)
#define FILTER_FLAG_CAUTION		(1 << CAUTION_FILTER)
#define FILTER_FLAG_DANGER		(1 << DANGER_FILTER)
#define FILTER_FLAG_LOCATE		(1 << LOCATE_FILTER)
#define FILTER_FLAG_ALERT		(1 << ALERT_FILTER)
#define FILTER_FLAG_FILTERED	(1 << FILTERED_FILTER)
#define FILTER_FLAG_TRACER		(1 << TRACER_FILTER)


#define FILTERSTRINGFIELD_TABLE  \
    X(FSF_Name, "Name")          \
    X(FSF_Level, "Level")        \
    X(FSF_Race, "Race")          \
    X(FSF_Class, "Class")        \
    X(FSF_NPC, "NPC")            \
    X(FSF_X, "X")                \
    X(FSF_Y, "Y")                \
    X(FSF_Z, "Z")                \
    X(FSF_Light, "Light")        \
    X(FSF_Deity, "Deity")        \
    X(FSF_RTeam, "RTeam")        \
    X(FSF_DTeam, "DTeam")        \
    X(FSF_Type, "Type")          \
    X(FSF_LastName, "LastName")  \
    X(FSF_Guild, "Guild")        \
    X(FSF_Spawn, "Spawn")        \
    X(FSF_Info, "Info")          \
    X(FSF_GM, "GM")

#define INFOFILTERSTRINGFIELD_TABLE \
    X(IFSF_Light, "Light") \
    X(IFSF_Head, "H")      \
    X(IFSF_Chest, "C")     \
    X(IFSF_Arms, "A")      \
    X(IFSF_Waist, "W")     \
    X(IFSF_Gloves, "G")    \
    X(IFSF_Legs, "L")      \
    X(IFSF_Feet, "F")      \
    X(IFSF_Primary, "1")   \
    X(IFSF_Secondary, "2")


#define X(a, b) a,
enum FilterStringField
{
    FILTERSTRINGFIELD_TABLE
    FSF_Max
};
#undef X

#define X(a, b) a,
enum InfoFilterStringField
{
    INFOFILTERSTRINGFIELD_TABLE
    IFSF_Max
};
#undef X


// special handling for min/max level, which aren't part of regex filter string
#define FSF_MINLEVEL_NAME "MinLevel"
#define FSF_MINLEVEL_LABEL "Min Level"
#define FSF_MAXLEVEL_NAME "MaxLevel"
#define FSF_MAXLEVEL_LABEL "Max Level"

typedef QHash<QString, QString> FilterFieldMap;

//----------------------------------------------------------------------
// FilterMgr
class FilterMgr : public QObject
{
  Q_OBJECT

 public:
  FilterMgr(const DataLocationMgr* dataLocMgr, 
	    const QString filterFile, bool spawnfilter_case);
  ~FilterMgr();

  const QString& filterFile(void) { return m_filterFile; }
  const QString& zoneFilterFile(void) { return m_zoneFilterFile; }
  bool caseSensitive(void) { return m_caseSensitive; }
  void setCaseSensitive(bool caseSensitive);

  uint32_t filterMask(const QString& filterString, uint8_t level) const;
  QString filterString(uint32_t mask) const;
  QString filterName(uint8_t filter) const;
  bool addFilter(uint8_t filter, const QString& filterString);
  void remFilter(uint8_t filter, const QString& filterString);
  bool addZoneFilter(uint8_t filter, const QString& filterString);
  void remZoneFilter(uint8_t filter, const QString& filterString);

  bool registerRuntimeFilter(const QString& name, 
			     uint8_t& flag,
			     uint32_t& flagMask);
  void unregisterRuntimeFilter(uint8_t flag);
  uint32_t runtimeFilterMask(const QString& filterString, uint8_t level) const;
  QString runtimeFilterString(uint32_t filterMask) const;
  bool runtimeFilterAddFilter(uint8_t flag, const QString& filter);
  void runtimeFilterRemFilter(uint8_t flag, const QString& filter);
  void runtimeFilterCommit(uint8_t flag);

 public slots:
  void loadFilters(void);
  void loadFilters(const QString& filterFile);
  void saveFilters(void);
  void listFilters(void);
  void loadZone(const QString& zoneShortName);
  void loadZoneFilters(void);
  void listZoneFilters(void);
  void saveZoneFilters(void);

 signals:
  void filtersChanged();
  void runtimeFiltersChanged(uint8_t flag);


 private:
  const DataLocationMgr* m_dataLocMgr;
  FilterTypes* m_types;
  QString m_filterFile;
  Filters* m_filters;
  QString m_zoneFilterFile;
  Filters* m_zoneFilters;

  FilterTypes* m_runtimeTypes;
  Filters* m_runtimeFilters;

  bool m_caseSensitive;
};


class FilterFormField : public QWidget
{
    Q_OBJECT

    public:
        FilterFormField(QString name, QString labeltext = QString(), QWidget* parent=nullptr);

        QString m_name;
        QString m_labeltext;
        QCheckBox* m_check;
        QLabel* m_label;
        QLineEdit* m_edit;

    public slots:
        void stateChanged(int state);
};

//SubClassing QCheckBox so we can control the sequence of check/uncheck/partial when
//clicking "Toggle All"
class ToggleAllCheckBox : public QCheckBox
{
    protected:
        virtual void nextCheckState() override;

};


class FilterDialog : public QDialog
{
    Q_OBJECT

    public:

        static QString getFilter(QWidget* parent, const QString& title,
                const QString& filterString, bool* ok=nullptr,
                Qt::WindowFlags flags = Qt::WindowFlags(),
                Qt::InputMethodHints inputMethodHints = Qt::ImhNone);

    protected:
        FilterDialog(QWidget* parent=nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
        ~FilterDialog();

        void setData(const QString filterString);
        void createForm();

        QHash<QString, FilterFormField*> m_filterFields;
        ToggleAllCheckBox* m_toggleAll;

        QString m_spawnFilterString;
        FilterFieldMap m_spawnFilterMap;

        QString m_filterString;
        int m_fieldCount;
        int m_fieldsCheckedCount;
        bool m_hasTrailingColon;

    signals:
        void stateChanged(int state);

    protected slots:
        void resetForm();
        void acceptDialog();
        void fieldToggled(bool checked);
        void toggleAllToggled(int state);

};

// helper functions
void FilterString2FilterFieldMap(const QString filterString, FilterFieldMap* map);
QString FilterFieldMap2FilterString(FilterFieldMap* map);

#endif // FILTERMGR_H
