/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "maemoruncontrol.h"

#include "maemoglobal.h"
#include "maemorunconfiguration.h"
#include "maemosshrunner.h"

namespace Madde {
namespace Internal {

using namespace RemoteLinux;
using ProjectExplorer::RunConfiguration;

MaemoRunControl::MaemoRunControl(RunConfiguration *rc)
    : AbstractRemoteLinuxRunControl(rc)
    , m_runner(new MaemoSshRunner(this, qobject_cast<MaemoRunConfiguration *>(rc)))
{
}

MaemoRunControl::~MaemoRunControl()
{
}

void MaemoRunControl::start()
{
    AbstractRemoteLinuxRunControl::start();
    connect(m_runner, SIGNAL(mountDebugOutput(QString)), SLOT(handleMountDebugOutput(QString)));
}

void MaemoRunControl::handleMountDebugOutput(const QString &output)
{
    appendMessage(output, Utils::StdErrFormatSameLine);
}

AbstractRemoteLinuxApplicationRunner *MaemoRunControl::runner() const { return m_runner; }

} // namespace Internal
} // namespace Madde
