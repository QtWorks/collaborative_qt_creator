/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "glsleditoractionhandler.h"
#include "glsleditorconstants.h"
#include "glsleditor.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>

#include <QtCore/QDebug>
#include <QtGui/QAction>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>

namespace GLSLEditor {
namespace Internal {

GLSLEditorActionHandler::GLSLEditorActionHandler()
  : TextEditor::TextEditorActionHandler(GLSLEditor::Constants::C_GLSLEDITOR_ID, Format)
{
}

void GLSLEditorActionHandler::createActions()
{
    TextEditor::TextEditorActionHandler::createActions();
}


} // namespace Internal
} // namespace GLSLEditor