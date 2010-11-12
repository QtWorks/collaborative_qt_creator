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
#ifndef GLSLCODECOMPLETION_H
#define GLSLCODECOMPLETION_H

#include <texteditor/icompletioncollector.h>

namespace GLSLEditor {

class CodeCompletion: public TextEditor::ICompletionCollector
{
    Q_OBJECT

public:
    CodeCompletion(QObject *parent = 0);
    virtual ~CodeCompletion();

    /* Returns the current active ITextEditable */
    virtual TextEditor::ITextEditable *editor() const;
    virtual int startPosition() const;

    /*
     * Returns true if this completion collector can be used with the given editor.
     */
    virtual bool supportsEditor(TextEditor::ITextEditable *editor);

    /* This method should return whether the cursor is at a position which could
     * trigger an autocomplete. It will be called each time a character is typed in
     * the text editor.
     */
    virtual bool triggersCompletion(TextEditor::ITextEditable *editor);

    // returns starting position
    virtual int startCompletion(TextEditor::ITextEditable *editor);

    /* This method should add all the completions it wants to show into the list,
     * based on the given cursor position.
     */
    virtual void completions(QList<TextEditor::CompletionItem> *completions);

    /**
     * This method should return true when the given typed character should cause
     * the selected completion item to be completed.
     */
    virtual bool typedCharCompletes(const TextEditor::CompletionItem &item, QChar typedChar);

    /**
     * This method should complete the given completion item.
     *
     * \param typedChar Non-null when completion was triggered by typing a
     *                  character. Possible values depend on typedCharCompletes()
     */
    virtual void complete(const TextEditor::CompletionItem &item, QChar typedChar);

    /* This method gives the completion collector a chance to partially complete
     * based on a set of items. The general use case is to complete the common
     * prefix shared by all possible completion items.
     *
     * Returns whether the completion popup should be closed.
     */
    virtual bool partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems);

    /* Called when it's safe to clean up the completion items.
     */
    virtual void cleanup();

private:
    QList<TextEditor::CompletionItem> m_completions;
    QList<TextEditor::CompletionItem> m_keywordCompletions;
    TextEditor::ITextEditable *m_editor;
    int m_startPosition;
    bool m_restartCompletion;
};

} // namespace GLSLEditor

#endif // GLSLCODECOMPLETION_H