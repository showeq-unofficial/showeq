/*
 *  filtermgr.cpp
 *  Copyright 2001-2007, 2019 by the respective ShowEQ Developers
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
#include "filtermgr.h"
#include "filter.h"
#include "datalocationmgr.h"
#include "diagnosticmessages.h"

#include <cerrno>

#include <QRegExp>
#include <QString>
#include <QFileInfo>
#include <QLineEdit>
#include <QLabel>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>

//
// ZBTEMP: predefined filters and filter mask will be migrated out
// so that ShowEQ code can register the file based filters and there mask
// at runtime ala the runtime Filter stuff
//

#define X(a, b) b,
const QString FilterStringFieldName[] = {
    FILTERSTRINGFIELD_TABLE
};
#undef X

//----------------------------------------------------------------------
// FilterMgr
FilterMgr::FilterMgr(const DataLocationMgr* dataLocMgr,
		     const QString filterFile, bool spawnfilter_case)
  : QObject(NULL),
    m_dataLocMgr(dataLocMgr),
    m_caseSensitive(spawnfilter_case)
{
  setObjectName("filtermgr");
  // Initialize filters

  // allocate general filter types object
  m_types = new FilterTypes;

  // Initialize filter types (start with legacy filter types)
  uint8_t type;
  uint32_t mask;
  m_types->registerType("Hunt", type, mask);
  m_types->registerType("Caution", type, mask);
  m_types->registerType("Danger", type, mask);
  m_types->registerType("Locate", type, mask);
  m_types->registerType("Alert", type, mask);
  m_types->registerType("Filtered", type, mask);
  m_types->registerType("Tracer", type, mask);

  // create global filters object
  m_filters = new Filters(*m_types);

  // load the global filters
  loadFilters(filterFile);

  // create the zone filters object
  m_zoneFilters = new Filters(*m_types);

  // load the zone filters
  loadZone("unknown");

  // allocate runtime filter types object
  m_runtimeTypes = new FilterTypes;

  // create runtime filters object
  m_runtimeFilters = new Filters(*m_runtimeTypes);
}

FilterMgr::~FilterMgr()
{
  if (m_filters)
    delete m_filters;
  if (m_zoneFilters)
    delete m_zoneFilters;
  if (m_types)
    delete m_types;
  if (m_runtimeFilters)
    delete m_runtimeFilters;
  if (m_runtimeTypes)
    delete m_runtimeTypes;
}

void FilterMgr::setCaseSensitive(bool caseSensitive)
{
  m_caseSensitive = caseSensitive;

  m_filters->setCaseSensitive(m_caseSensitive);
  m_zoneFilters->setCaseSensitive(m_caseSensitive);
  m_runtimeFilters->setCaseSensitive(m_caseSensitive);
}

uint32_t FilterMgr::filterMask(const QString& filterString, uint8_t level) const
{
  uint32_t mask = 0;

  mask = m_filters->filterMask(filterString, level);
  mask |= m_zoneFilters->filterMask(filterString, level);

  return mask;
}

QString FilterMgr::filterString(uint32_t mask) const
{
  return m_types->names(mask);
}

QString FilterMgr::filterName(uint8_t filter) const
{
  return m_types->name(filter);
}

void FilterMgr::loadFilters(void)
{
  QFileInfo fileInfo(m_filterFile);

  fileInfo = 
    m_dataLocMgr->findExistingFile("filters", fileInfo.fileName(), false);

  m_filterFile = fileInfo.absoluteFilePath();

  seqInfo("Loading Filters from '%s'", m_filterFile.toLatin1().data());

  m_filters->load(m_filterFile);

  emit filtersChanged();
}

void FilterMgr::loadFilters(const QString& fileName)
{
  QFileInfo fileInfo = 
    m_dataLocMgr->findExistingFile("filters", fileName, false);

  m_filterFile = fileInfo.absoluteFilePath();

  seqInfo("Loading Filters from '%s'", m_filterFile.toLatin1().data());
  
  m_filters->load(m_filterFile);

  emit filtersChanged();
}


void FilterMgr::saveFilters(void)
{
  QFileInfo fileInfo(m_filterFile);
  
  fileInfo = m_dataLocMgr->findWriteFile("filters", fileInfo.fileName(), true);

  m_filterFile = fileInfo.absoluteFilePath();

  seqInfo("Saving filters to %s", m_filterFile.toLatin1().data());

  m_filters->save(m_filterFile);
}

void FilterMgr::listFilters(void)
{
  m_filters->list();
}

bool FilterMgr::addFilter(uint8_t filter, const QString& filterString)
{
  // make sure it's actually a filter
  if (filter >= SIZEOF_FILTERS)
    return false;

  // add the filter
  bool ok = m_filters->addFilter(filter, filterString);

  // signal that the filters have changed
  emit filtersChanged();

  return ok;
}

void FilterMgr::remFilter(uint8_t filter, const QString& filterString)
{
  // validate that it's a valid filter
  if (filter >= SIZEOF_FILTERS)
    return;

  // remove a filter
  m_filters->remFilter(filter, filterString);

  // notify that the filters have changed
  emit filtersChanged();
}

bool FilterMgr::addZoneFilter(uint8_t filter, const QString& filterString)
{
  // make sure it's actually a filter
  if (filter >= SIZEOF_FILTERS)
    return false;

  // add the filter
  bool ok = m_zoneFilters->addFilter(filter, filterString);

  // signal that the filters have changed
  emit filtersChanged();

  return ok;
}

void FilterMgr::remZoneFilter(uint8_t filter, const QString& filterString)
{
  // validate that it's a valid filter
  if (filter >= SIZEOF_FILTERS)
    return;

  // remove a filter
  m_zoneFilters->remFilter(filter, filterString);

  // notify that the filters have changed
  emit filtersChanged();
}

void FilterMgr::loadZone(const QString& shortZoneName)
{
  QString fileName = shortZoneName + ".xml";

  QFileInfo fileInfo = 
    m_dataLocMgr->findExistingFile("filters", fileName, false);

  m_zoneFilterFile = fileInfo.absoluteFilePath();

  seqInfo("Loading Zone Filter File: %s", m_zoneFilterFile.toLatin1().data());

  m_zoneFilters->load(m_zoneFilterFile);

  emit filtersChanged();
}

void FilterMgr::loadZoneFilters(void)
{
  QFileInfo fileInfo(m_zoneFilterFile);
  
  fileInfo = m_dataLocMgr->findExistingFile("filters", fileInfo.fileName(),
					    false);

  m_zoneFilterFile = fileInfo.absoluteFilePath();

  seqInfo("Loading Zone Filter File: %s", m_zoneFilterFile.toLatin1().data());

  m_zoneFilters->load(m_zoneFilterFile);
  
  emit filtersChanged();
}


void FilterMgr::listZoneFilters(void)
{
  m_zoneFilters->list();
}


void FilterMgr::saveZoneFilters(void)
{
  QFileInfo fileInfo(m_zoneFilterFile);
  
  fileInfo = m_dataLocMgr->findWriteFile("filters", fileInfo.fileName(), true);

  m_zoneFilterFile = fileInfo.absoluteFilePath();

  seqInfo("Saving filters to %s", m_zoneFilterFile.toLatin1().data());

  if (! m_zoneFilters->save(m_zoneFilterFile))
  {
    seqWarn("Failed saving filters.");
  }
}

bool FilterMgr::registerRuntimeFilter(const QString& name, 
				      uint8_t& type,
				      uint32_t& mask)
{
  return m_runtimeTypes->registerType(name, type, mask);
}

void FilterMgr::unregisterRuntimeFilter(uint8_t type)
{
  // first, clear any filter associated with the type
  m_runtimeFilters->clearType(type);

  // Then unregister the type
  m_runtimeTypes->unregisterType(type);
}

uint32_t FilterMgr::runtimeFilterMask(const QString& filterString,
				       uint8_t level) const
{
  return m_runtimeFilters->filterMask(filterString, level);
}

QString FilterMgr::runtimeFilterString(uint32_t filterMask) const
{
  return m_runtimeTypes->names(filterMask);
}

bool FilterMgr::runtimeFilterAddFilter(uint8_t type, const QString& filter)
{
  return m_runtimeFilters->addFilter(type, filter);
}

void FilterMgr::runtimeFilterRemFilter(uint8_t type, const QString& filter)
{
  return m_runtimeFilters->remFilter(type, filter);
}

void FilterMgr::runtimeFilterCommit(uint8_t type)
{
  // notify that the runtime filters have changed
  emit runtimeFiltersChanged(type);
}



FilterFormField::FilterFormField(QString name, QString labeltext, QWidget* parent) :
    QWidget(parent),
    m_name(name),
    m_labeltext(labeltext),
    m_check(nullptr),
    m_label(nullptr),
    m_edit(nullptr)
{
    if (m_labeltext.isNull())
        m_labeltext = m_name;

    m_check = new QCheckBox(this);
    m_check->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);

    m_label = new QLabel(m_labeltext, this);
    QSizePolicy tmpPolicy = m_label->sizePolicy();
    tmpPolicy.setVerticalPolicy(QSizePolicy::Fixed);
    m_label->setSizePolicy(tmpPolicy);

    m_edit = new QLineEdit(this);
    tmpPolicy = m_edit->sizePolicy();
    tmpPolicy.setVerticalPolicy(QSizePolicy::Fixed);
    m_edit->setSizePolicy(tmpPolicy);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->addWidget(m_check);
    layout->addWidget(m_label);
    layout->addWidget(m_edit);

    connect(m_check, SIGNAL(toggled(bool)), m_edit, SLOT(setEnabled(bool)));
}

void FilterFormField::stateChanged(int state)
{
    bool old_check_state = m_check->blockSignals(true);
    bool old_edit_state = m_edit->blockSignals(true);
    switch (state)
    {
        case Qt::Unchecked:
            m_check->setChecked(false);
            m_edit->setEnabled(false);
            break;
        case Qt::PartiallyChecked:
            break;
        case Qt::Checked:
            m_check->setChecked(true);
            m_edit->setEnabled(true);
            break;
    }
    m_check->blockSignals(old_check_state);
    m_edit->blockSignals(old_edit_state);
}

void ToggleAllCheckBox::nextCheckState()
{
    switch(checkState())
    {
        case Qt::Unchecked:
            setCheckState(Qt::Checked);
            break;
        case Qt::PartiallyChecked:
            setCheckState(Qt::Unchecked);
            break;
        case Qt::Checked:
            setCheckState(Qt::Unchecked);
            break;
    }

}

FilterDialog::FilterDialog(QWidget* parent, Qt::WindowFlags flags) :
    QDialog(parent, flags),
    m_toggleAll(nullptr),
    m_filterString(QString()),
    m_fieldCount(0),
    m_fieldsCheckedCount(0)
{
    //init m_spawnFilterMap
    for (int field = FSF_Name; field < FSF_Max; ++field)
    {
        QString name = FilterStringFieldName[field];
        m_spawnFilterMap[name] = "";
    }
    m_spawnFilterMap[FSF_MINLEVEL_NAME] = "";
    m_spawnFilterMap[FSF_MAXLEVEL_NAME] = "";

    createForm();
}

FilterDialog::~FilterDialog()
{ }

void FilterDialog::createForm()
{

    const int colspc_x = 20;
    const int colspc_y = 1;

    QVBoxLayout* pageLayout = new QVBoxLayout(this);
    QGridLayout* gridLayout = new QGridLayout();

    // info/instructions
    QLabel* tmpLabel = new QLabel("All fields except '" FSF_MINLEVEL_LABEL "' and '" FSF_MAXLEVEL_LABEL "' accept Regular Expression syntax.", this);
    tmpLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    pageLayout->addWidget(tmpLabel);
    tmpLabel = new QLabel("For an exact level match or matching multiple levels using a RegEx, use the 'Level' field.", this);
    tmpLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    pageLayout->addWidget(tmpLabel);
    tmpLabel = new QLabel("To limit to a simple level range (no RegEx), use the '" FSF_MINLEVEL_LABEL "' and '" FSF_MAXLEVEL_LABEL "' fields.", this);
    tmpLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    pageLayout->addWidget(tmpLabel);
    tmpLabel = new QLabel("Any fields left blank or not checked will not be matched against.", this);
    tmpLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    pageLayout->addWidget(tmpLabel);

    pageLayout->addItem(new QSpacerItem(colspc_x, 25));

    //using an extra widget so the spacing lines up
    QWidget* toggleAllWidget = new QWidget();
    QHBoxLayout* toggleAllLayout = new QHBoxLayout(toggleAllWidget);
    m_toggleAll = new ToggleAllCheckBox();
    m_toggleAll->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);
    m_toggleAll->setTristate(true);
    toggleAllLayout->addWidget(m_toggleAll);
    toggleAllLayout->addWidget(new QLabel("Toggle All", this));
    connect(m_toggleAll, SIGNAL(stateChanged(int)), this, SLOT(toggleAllToggled(int)));

    pageLayout->addLayout(gridLayout);
    gridLayout->addWidget(toggleAllWidget, 0, 0);

    const QString labels[] = {
        "Name", "Level", "Race", "Class", "NPC", "X", "Y", "Z", "Light", "Deity",
        "Race Team", "Deity Team", "Type", "Last Name", "Guild", "Spawn Time", "GM" };

    for (int field = FSF_Name; field < FSF_Max; ++field)
    {
        QString name = FilterStringFieldName[field];
        QString label = labels[field];
        m_filterFields[name] = new FilterFormField(name, label);
        m_fieldCount++;

    }

    //not part of normal regex string, but still part of filter
    m_filterFields[FSF_MINLEVEL_NAME] = new FilterFormField(FSF_MINLEVEL_NAME, FSF_MINLEVEL_LABEL);
    m_filterFields[FSF_MAXLEVEL_NAME] = new FilterFormField(FSF_MAXLEVEL_NAME, FSF_MAXLEVEL_LABEL);
    m_fieldCount += 2;

    const QString formFieldOrder[] = { "Name", "LastName", "Guild", "Race", "Class",
        "Deity", "Level", FSF_MINLEVEL_NAME, FSF_MAXLEVEL_NAME, "X", "Y", "Z", "NPC", "Type",
        "GM", "RTeam", "DTeam", "Spawn", "Light" };

    int row = 1; //toggle all is row 0
    int col = 0;
    for (auto fieldname : formFieldOrder)
    {
        gridLayout->addWidget(m_filterFields[fieldname], row, col++);

        connect(m_toggleAll, SIGNAL(stateChanged(int)), m_filterFields[fieldname], SLOT(stateChanged(int)));
        connect(m_filterFields[fieldname]->m_check, SIGNAL(toggled(bool)), this, SLOT(fieldToggled(bool)));

        if (fieldname == "Guild" || fieldname == "Deity" ||
                fieldname == FSF_MAXLEVEL_NAME || fieldname == "Z" ||
                fieldname == "GM" || fieldname == "Spawn")
        {
            row++;
            col = 0;
        }
    }

    //buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    pageLayout->addItem(new QSpacerItem(colspc_x, 25));
    pageLayout->addLayout(buttonLayout);

    QPushButton* resetButton = new QPushButton("Reset");
    resetButton->setDefault(false);
    resetButton->setAutoDefault(false);
    buttonLayout->addWidget(resetButton);
    connect(resetButton, SIGNAL(clicked()), this, SLOT(resetForm()));

    buttonLayout->addItem(new QSpacerItem(colspc_x, colspc_y));

    QPushButton* okButton = new QPushButton("Ok");
    okButton->setDefault(false);
    okButton->setAutoDefault(false);
    buttonLayout->addWidget(okButton);
    connect(okButton, SIGNAL(clicked()), this, SLOT(acceptDialog()));

    QPushButton* cancelButton = new QPushButton("Cancel");
    cancelButton->setDefault(false);
    cancelButton->setAutoDefault(false);
    buttonLayout->addWidget(cancelButton);
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void FilterDialog::setData(const QString filterString)
{
    m_spawnFilterString = filterString;

    FilterString2FilterFieldMap(filterString, &m_spawnFilterMap);

    resetForm();
}

void FilterDialog::resetForm()
{
    m_fieldsCheckedCount = 0;
    for (int field = FSF_Name; field < FSF_Max; ++field)
    {
        QString name = FilterStringFieldName[field];
        m_filterFields[name]->m_edit->setText(m_spawnFilterMap[name]);

        if (m_filterFields[name]->m_edit->text().length())
        {
            m_filterFields[name]->stateChanged(Qt::Checked);
            m_fieldsCheckedCount++;
        }
        else
        {
            m_filterFields[name]->stateChanged(Qt::Unchecked);
        }
    }
    //not part of normal regex string, but still part of filter
    m_filterFields[FSF_MINLEVEL_NAME]->m_edit->setText(m_spawnFilterMap[FSF_MINLEVEL_NAME]);
    if (m_filterFields[FSF_MINLEVEL_NAME]->m_edit->text().length())
    {
        m_filterFields[FSF_MINLEVEL_NAME]->stateChanged(Qt::Checked);
        m_fieldsCheckedCount++;
    }
    else
    {
        m_filterFields[FSF_MINLEVEL_NAME]->stateChanged(Qt::Unchecked);
    }
    m_filterFields[FSF_MAXLEVEL_NAME]->m_edit->setText(m_spawnFilterMap[FSF_MAXLEVEL_NAME]);
    if (m_filterFields[FSF_MAXLEVEL_NAME]->m_edit->text().length())
    {
        m_filterFields[FSF_MAXLEVEL_NAME]->stateChanged(Qt::Checked);
        m_fieldsCheckedCount++;
    }
    else
    {
        m_filterFields[FSF_MAXLEVEL_NAME]->stateChanged(Qt::Unchecked);
    }

    bool old_state = m_toggleAll->blockSignals(true);
    if (m_fieldsCheckedCount == 0)
        m_toggleAll->setCheckState(Qt::Unchecked);
    else if (m_fieldsCheckedCount == m_fieldCount)
        m_toggleAll->setCheckState(Qt::Checked);
    else
        m_toggleAll->setCheckState(Qt::PartiallyChecked);
    m_toggleAll->blockSignals(old_state);

}

void FilterDialog::acceptDialog()
{
    FilterFieldMap map;

    //if enabled, add to map
    for (int field = FSF_Name; field < FSF_Max; ++field)
    {
        QString name = FilterStringFieldName[field];
        if (m_filterFields[name]->m_edit->isEnabled())
            map[name] = m_filterFields[name]->m_edit->text();
    }
    //not part of normal regex string, but still part of filter
    if (m_filterFields[FSF_MINLEVEL_NAME]->m_edit->isEnabled())
        map[FSF_MINLEVEL_NAME] = m_filterFields[FSF_MINLEVEL_NAME]->m_edit->text();

    if (m_filterFields[FSF_MAXLEVEL_NAME]->m_edit->isEnabled())
        map[FSF_MAXLEVEL_NAME] = m_filterFields[FSF_MAXLEVEL_NAME]->m_edit->text();


    m_filterString = FilterFieldMap2FilterString(&map);

    done(QDialog::Accepted);
}

void FilterDialog::fieldToggled(bool checked)
{
    if (checked)
        m_fieldsCheckedCount++;
    else
        m_fieldsCheckedCount--;

    bool old_state = m_toggleAll->blockSignals(true);
    if (m_fieldsCheckedCount == m_fieldCount)
        m_toggleAll->setCheckState(Qt::Checked);
    else if (m_fieldsCheckedCount == 0)
        m_toggleAll->setCheckState(Qt::Unchecked);
    else
        m_toggleAll->setCheckState(Qt::PartiallyChecked);
    m_toggleAll->blockSignals(old_state);
}

void FilterDialog::toggleAllToggled(int state)
{
    if (sender() != m_toggleAll)
        return;

    switch(state)
    {
        case Qt::Checked:
            m_fieldsCheckedCount = m_fieldCount;
            break;
        case Qt::Unchecked:
            m_fieldsCheckedCount = 0;
            break;
        case Qt::PartiallyChecked:
            m_toggleAll->setCheckState(Qt::Checked);
            m_fieldsCheckedCount = m_fieldCount;
    }
    emit stateChanged(m_toggleAll->checkState());
}

QString FilterDialog::getFilter(QWidget* parent, const QString& title,
        const QString& filterString, bool* ok, Qt::WindowFlags flags,
        Qt::InputMethodHints inputMethodHints)
{
    FilterDialog* dlg = new FilterDialog(parent, flags);
    dlg->setWindowTitle(title);
    dlg->setData(filterString);

    const int ret = dlg->exec();
    if (ok)
        *ok = ret;

    QString result;
    if (ok)
        result = dlg->m_filterString;

    dlg->deleteLater();
    return result;
}


void FilterString2FilterFieldMap(const QString filterString, FilterFieldMap* map)
{
    if (!map || !filterString.length())
        return;

    QString levelSuffix;
    QString regex;
    int minLevel = -1;
    int maxLevel = -1;

    int split = filterString.lastIndexOf(';');
    if (split == -1)
    {
        regex = filterString;
    }
    else
    {
        regex = filterString.left(split);
        levelSuffix = filterString.mid(split+1);
    }


    // parse level range string
    if (levelSuffix.length())
    {
        auto range = levelSuffix.split('-');
        bool ok = false;

        if (range.size() == 1)
        {
            //no dash, only a single level specified - treat as exact match
            int level = range[0].toInt(&ok);
            if (ok)
            {
                minLevel = level;
                maxLevel = level;
            }
            else
            {
                seqWarn("Could not parse level: %s", range[0].toLatin1().data());
            }
        }
        else if (range.size() == 2)
        {
            //one dash, two fields - treat as range
            int level = range[0].toInt(&ok);
            if (ok)
                minLevel = level;
            else
                seqWarn("Could not parse min level: %s", range[0].toLatin1().data());

            ok = false;
            level = range[1].toInt(&ok);
            if (ok)
                maxLevel = level;
            else
                seqWarn("Could not parse max level: %s", range[0].toLatin1().data());


            // if range wasn't fully/correctly specified, use defaults
            minLevel = (minLevel > -1) ? minLevel : 0;
            maxLevel = (maxLevel > -1) ? maxLevel : SHRT_MAX;

        }
        else
        {
            seqWarn("Ignoring malformed level range string.");
        }

        if (maxLevel < minLevel)
        {
            int tmp = maxLevel;
            maxLevel = minLevel;
            minLevel = tmp;
        }
    }


    // parse regex string and set map fields

    //process filter string and set form fields
    QStringList tokens = regex.split(":");

    //fields should be key:value, but split will create an extra item in the
    //list after the last :, so for a well-formed filterString, there should
    //always be an odd number of elements
    if (tokens.length() % 2 != 1)
    {
        seqWarn("Malformed filterString regex: %s", regex.toLatin1().data());
        return;
    }

    QStringList::const_iterator itr = tokens.begin();
    for (;itr < tokens.end(); ++itr)
    {
        QString name = *itr;
        if (!map->contains(name))
        {
            if (!name.length() && itr == tokens.end() - 1)
            {
                //filter string has an ending : that we can ignore
                continue;
            }

            seqWarn("Ignoring unknown filter string field: %s", name.toLatin1().data());
            ++itr; // skip this field's data
            continue;
        }
        if (++itr == tokens.end())
            continue;
        QString value = *itr;

        if (name == "Name" && (value == "Door" || value == "Drop"))
        {
            //infuriatingly, we add a colon to door and drop names.
            //TODO try to find out how many people's filters this will break
            //if we remove the : from door and drop names (maybe replace it with
            //a - or something.  Or save it for 7.x and do it anyway.
            //TODO also, check on adding trailing : to Item spawn filterstring
            //to make it consistent with the Spawn filterstring
            value += ":";
            if (++itr == tokens.end())
                continue;
            value += *itr;
        }

        (*map)[name] = value.trimmed();
    }

    if (minLevel > -1)
        (*map)[FSF_MINLEVEL_NAME] = QString::number(minLevel);

    if (maxLevel > -1)
        (*map)[FSF_MAXLEVEL_NAME] = QString::number(maxLevel);

}

QString FilterFieldMap2FilterString(FilterFieldMap* map)
{
    if (!map)
        return QString();

    QString filterString;
    bool wildcard = false;
    bool has_first_match = false;

    for (int field = FSF_Name; field < FSF_Max; ++field)
    {
        QString name = FilterStringFieldName[field];

        if (!map->contains(name) || !(*map)[name].trimmed().length())
        {
            if (has_first_match && !wildcard)
            {
                wildcard = true;
            }
            continue;
        }

        QString value = (*map)[name];
        value = value.trimmed();

        has_first_match = true;

        if (wildcard)
        {
            wildcard = false;
            filterString += ".*:";
        }
        filterString += name;
        filterString += ":";
        filterString += value;
        filterString += ":";

    }

    //min/max level are not part of normal regex string, but still part of filter
    int minLevel = -1;
    int maxLevel = -1;

    if (map->contains(FSF_MINLEVEL_NAME))
    {
        QString value = (*map)[FSF_MINLEVEL_NAME];
        value = value.trimmed();
        bool ok = false;
        int level = value.toInt(&ok);
        if (ok)
            minLevel = level;
    }

    if (map->contains(FSF_MAXLEVEL_NAME))
    {
        QString value = (*map)[FSF_MAXLEVEL_NAME];
        value = value.trimmed();
        bool ok = false;
        int level = value.toInt(&ok);
        if (ok)
            maxLevel = level;
    }

    if (minLevel >= 0 || maxLevel >= 0)
    {
        minLevel = (minLevel >= 0) ? minLevel : 0;
        maxLevel = (maxLevel >= 0) ? maxLevel : SHRT_MAX;

        if (maxLevel < minLevel)
        {
            int tmp = maxLevel;
            maxLevel = minLevel;
            minLevel = tmp;
        }

        filterString += ";";
        filterString += QString::number(minLevel);
        filterString += "-";
        filterString += QString::number(maxLevel);
    }

    return filterString;
}

#ifndef QMAKEBUILD
#include "filtermgr.moc"
#endif

