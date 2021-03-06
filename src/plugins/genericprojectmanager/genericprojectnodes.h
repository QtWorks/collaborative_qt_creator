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

#ifndef GENERICPROJECTNODE_H
#define GENERICPROJECTNODE_H

#include <projectexplorer/projectnodes.h>

#include <QtCore/QStringList>
#include <QtCore/QHash>

namespace Core {
class IFile;
}

namespace GenericProjectManager {
namespace Internal {

class GenericProject;

class GenericProjectNode : public ProjectExplorer::ProjectNode
{
public:
    GenericProjectNode(GenericProject *project, Core::IFile *projectFile);

    Core::IFile *projectFile() const;
    QString projectFilePath() const;

    virtual bool hasBuildTargets() const;

    virtual QList<ProjectExplorer::ProjectNode::ProjectAction> supportedActions(Node *node) const;

    virtual bool canAddSubProject(const QString &proFilePath) const;

    virtual bool addSubProjects(const QStringList &proFilePaths);
    virtual bool removeSubProjects(const QStringList &proFilePaths);

    virtual bool addFiles(const ProjectExplorer::FileType fileType,
                          const QStringList &filePaths,
                          QStringList *notAdded = 0);

    virtual bool removeFiles(const ProjectExplorer::FileType fileType,
                             const QStringList &filePaths,
                             QStringList *notRemoved = 0);
    virtual bool deleteFiles(const ProjectExplorer::FileType fileType,
                             const QStringList &filePaths);

    virtual bool renameFile(const ProjectExplorer::FileType fileType,
                             const QString &filePath,
                             const QString &newFilePath);

    virtual QList<ProjectExplorer::RunConfiguration *> runConfigurationsFor(Node *node);

    void refresh();

private:
    typedef QHash<QString, FolderNode *> FolderByName;
    FolderNode *findOrCreateFolderByName(FolderByName *folderByName,
                                         const QStringList &components, int end);

private:
    GenericProject *m_project;
    Core::IFile *m_projectFile;
};

} // namespace Internal
} // namespace GenericProjectManager

#endif // GENERICPROJECTNODE_H
