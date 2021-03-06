/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "currentprojectfind.h"

#include "projectexplorer.h"
#include "project.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QSettings>
#include <QtGui/QLabel>
#include <QtGui/QHBoxLayout>

using namespace Find;
using namespace ProjectExplorer;
using namespace ProjectExplorer::Internal;
using namespace TextEditor;

CurrentProjectFind::CurrentProjectFind(ProjectExplorerPlugin *plugin)
  : AllProjectsFind(plugin),
    m_plugin(plugin)
{
    connect(m_plugin, SIGNAL(currentProjectChanged(ProjectExplorer::Project*)),
            this, SIGNAL(changed()));
}

QString CurrentProjectFind::id() const
{
    return QLatin1String("Current Project");
}

QString CurrentProjectFind::displayName() const
{
    return tr("Current Project");
}

bool CurrentProjectFind::isEnabled() const
{
    return m_plugin->currentProject() != 0 && BaseFileFind::isEnabled();
}

QList<Project *> CurrentProjectFind::projects() const
{
    return QList<Project *>() << m_plugin->currentProject();
}

QString CurrentProjectFind::label() const
{
    return tr("Project '%1':").arg(projects().first()->displayName());
}

void CurrentProjectFind::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("CurrentProjectFind"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void CurrentProjectFind::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("CurrentProjectFind"));
    readCommonSettings(settings, QString(QLatin1Char('*')));
    settings->endGroup();
}
