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

#ifndef S60DEVICERUNCONFIGURATION_H
#define S60DEVICERUNCONFIGURATION_H

#include <projectexplorer/runconfiguration.h>
#include <qt4projectmanager/qt4projectmanager_global.h>

#include <QtCore/QFutureInterface>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
class Qt4BaseTarget;
class Qt4ProFileNode;

namespace Internal {
class SymbianQtVersion;
class Qt4SymbianTarget;
}

class S60DeviceRunConfigurationFactory;

class QT4PROJECTMANAGER_EXPORT S60DeviceRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
    friend class S60DeviceRunConfigurationFactory;

public:
    S60DeviceRunConfiguration(Qt4ProjectManager::Qt4BaseTarget *parent, const QString &proFilePath);
    virtual ~S60DeviceRunConfiguration();

    bool isEnabled() const;
    QString disabledReason() const;
    QWidget *createConfigurationWidget();

    Utils::OutputFormatter *createOutputFormatter() const;

    QString commandLineArguments() const;
    void setCommandLineArguments(const QString &args);
    QString qmlCommandLineArguments() const;

    QString projectFilePath() const;

    QString targetName() const;
    QString localExecutableFileName() const;
    quint32 executableUid() const;

    bool isDebug() const;
    QString symbianTarget() const;

    QVariantMap toMap() const;

    QString proFilePath() const;
signals:
    void targetInformationChanged();

protected:
    S60DeviceRunConfiguration(Qt4ProjectManager::Qt4BaseTarget *parent, S60DeviceRunConfiguration *source);
    QString defaultDisplayName() const;
    virtual bool fromMap(const QVariantMap &map);

private slots:
    void proFileUpdate(Qt4ProjectManager::Qt4ProFileNode *pro, bool success, bool parseInProgress);

private:
    void ctor();
    Internal::Qt4SymbianTarget *qt4Target() const;
    Internal::SymbianQtVersion *qtVersion() const;

    QString m_proFilePath;
    QString m_commandLineArguments;
    bool m_validParse;
    bool m_parseInProgress;
};

class S60DeviceRunConfigurationFactory : public ProjectExplorer::IRunConfigurationFactory
{
    Q_OBJECT

public:
    explicit S60DeviceRunConfigurationFactory(QObject *parent = 0);
    ~S60DeviceRunConfigurationFactory();

    bool canCreate(ProjectExplorer::Target *parent, const QString &id) const;
    ProjectExplorer::RunConfiguration *create(ProjectExplorer::Target *parent, const QString &id);
    bool canRestore(ProjectExplorer::Target *parent, const QVariantMap &map) const;
    ProjectExplorer::RunConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map);
    bool canClone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source) const;
    ProjectExplorer::RunConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::RunConfiguration *source);

    QStringList availableCreationIds(ProjectExplorer::Target *parent) const;
    // used to translate the ids to names to display to the user
    QString displayNameForId(const QString &id) const;
};

} // namespace Qt4ProjectManager

#endif // S60DEVICERUNCONFIGURATION_H
