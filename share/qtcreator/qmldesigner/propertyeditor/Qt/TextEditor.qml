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

import Qt 4.7
import Bauhaus 1.0

QWidget {
    id: textEdit

    function escapeString(string) {
        var str  = string;
        str = str.replace(/\\/g, "\\\\");
        str.replace(/\"/g, "\\\"");
        str = str.replace(/\t/g, "\\t");
        str = str.replace(/\r/g, "\\r");
        str = str.replace(/\n/g, '\\n');
        return str;
    }

    property variant backendValue
    property alias enabled: textEdit.enabled
    property variant baseStateFlag
    property alias text: textEditWidget.plainText
    property alias readOnly: textEditWidget.readOnly
    property alias translation: trCheckbox.visible

    minimumHeight: 72;

    onBaseStateFlagChanged: {
        evaluate();
    }

    property variant isEnabled: textEdit.enabled
    onIsEnabledChanged: {
        evaluate();
    }


    property bool isInModel: backendValue.isInModel;
    onIsInModelChanged: {
        evaluate();
    }
    property bool isInSubState: backendValue.isInSubState;
    onIsInSubStateChanged: {
        evaluate();
    }

    function evaluate() {
        if (!enabled) {
            textEditWidget.setStyleSheet("color: "+scheme.disabledColor);
        } else {
            if (baseStateFlag) {
                if (backendValue != null && backendValue.isInModel)
                    textEditWidget.setStyleSheet("color: "+scheme.changedBaseColor);
                else
                    textEditWidget.setStyleSheet("color: "+scheme.defaultColor);
            } else {
                if (backendValue != null && backendValue.isInSubState)
                    textEditWidget.setStyleSheet("color: "+scheme.changedStateColor);
                else
                    textEditWidget.setStyleSheet("color: "+scheme.defaultColor);
            }
        }
    }

    ColorScheme { id:scheme; }

    QTextEdit {
        acceptRichText: false
        verticalScrollBarPolicy: "Qt::ScrollBarAlwaysOff"
        y: 2
        id: textEditWidget
        styleSheet: "QLineEdit { padding-left: 32; }"
        width: textEdit.width
        height: textEdit.height - 2
        toolTip: backendValue.isBound ? backendValue.expression : ""

        property string valueFromBackend: (backendValue === undefined || backendValue.value === undefined) ? "" : backendValue.value;

        onValueFromBackendChanged: {
            if (backendValue.value === undefined)
                return;
            if (plainText === backendValue.value)
                return;
            plainText = backendValue.value;
        }

        onTextChanged: {
            if (backendValue.isTranslated) {
                backendValue.expression = "qsTr(\"" + escapeString(plainText) + "\")"
            } else {
                backendValue.value = plainText
            }
            evaluate();
        }

        onFocusChanged: {
            if (focus)
                backendValue.lock();
            else
                backendValue.unlock();
        }


    }
    ExtendedFunctionButton {
        backendValue: textEdit.backendValue
        y: 6
        x: 0
        visible: textEdit.enabled
    }

    QCheckBox {
        id: trCheckbox
        y: 2
        styleSheetFile: "checkbox_tr.css";
        toolTip: qsTr("Translate this string")
        x: textEditWidget.width - 22
        height: 24;
        width: 24
        visible: false
        checked: backendValue.isTranslated
        onToggled: {
            if (trCheckbox.checked) {
                backendValue.expression = "qsTr(\"" + escapeString(textEditWidget.plainText) + "\")"
            } else {
                backendValue.value = textEditWidget.plainText
            }
            evaluate();
        }

        onVisibleChanged: {
            if (trCheckbox.visible) {
                trCheckbox.raise();
                textEditWidget.styleSheet =  "QLineEdit { padding-left: 32; padding-right: 62;}"
            }
        }
    }
}
