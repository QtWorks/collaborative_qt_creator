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

#include "debuggerdialogs.h"

#include "debuggerconstants.h"
#include "cdb/cdbengine.h"
#include "shared/hostutils.h"

#include "ui_attachcoredialog.h"
#include "ui_attachexternaldialog.h"
#include "ui_startexternaldialog.h"
#include "ui_startremotedialog.h"
#include "ui_startremoteenginedialog.h"
#include "ui_attachtoqmlportdialog.h"

#include <coreplugin/icore.h>
#include <projectexplorer/abi.h>
#include <utils/historycompleter.h>
#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QRegExp>

#include <QtGui/QButtonGroup>
#include <QtGui/QFileDialog>
#include <QtGui/QGroupBox>
#include <QtGui/QHeaderView>
#include <QtGui/QMessageBox>
#include <QtGui/QProxyModel>
#include <QtGui/QPushButton>
#include <QtGui/QRadioButton>
#include <QtGui/QScrollArea>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QStandardItemModel>
#include <QtGui/QGridLayout>

using namespace Utils;

namespace Debugger {
namespace Internal {

bool operator<(const ProcData &p1, const ProcData &p2)
{
    return p1.name < p2.name;
}

// A filterable process list model
class ProcessListFilterModel : public QSortFilterProxyModel
{
public:
    explicit ProcessListFilterModel(QObject *parent);
    QString processIdAt(const QModelIndex &index) const;
    QString executableForPid(const QString& pid) const;

    void populate(QList<ProcData> processes, const QString &excludePid);

private:
    enum { ProcessImageRole = Qt::UserRole, ProcessNameRole };

    bool lessThan(const QModelIndex &left, const QModelIndex &right) const;

    QStandardItemModel *m_model;
};

ProcessListFilterModel::ProcessListFilterModel(QObject *parent)
  : QSortFilterProxyModel(parent),
    m_model(new QStandardItemModel(this))
{
    QStringList columns;
    columns << AttachExternalDialog::tr("Process ID")
            << AttachExternalDialog::tr("Name")
            << AttachExternalDialog::tr("State");
    m_model->setHorizontalHeaderLabels(columns);
    setSourceModel(m_model);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setFilterKeyColumn(1);
}

bool ProcessListFilterModel::lessThan(const QModelIndex &left,
    const QModelIndex &right) const
{
    const QString l = sourceModel()->data(left).toString();
    const QString r = sourceModel()->data(right).toString();
    if (left.column() == 0)
        return l.toInt() < r.toInt();
    return l < r;
}

QString ProcessListFilterModel::processIdAt(const QModelIndex &index) const
{
    if (index.isValid()) {
        const QModelIndex index0 = mapToSource(index);
        QModelIndex siblingIndex = index0.sibling(index0.row(), 0);
        if (const QStandardItem *item = m_model->itemFromIndex(siblingIndex))
            return item->text();
    }
    return QString();
}

QString ProcessListFilterModel::executableForPid(const QString &pid) const
{
    const int rowCount = m_model->rowCount();
    for (int r = 0; r < rowCount; r++) {
        const QStandardItem *item = m_model->item(r, 0);
        if (item->text() == pid) {
            QString name = item->data(ProcessImageRole).toString();
            if (name.isEmpty())
                name = item->data(ProcessNameRole).toString();
            return name;
        }
    }
    return QString();
}

void ProcessListFilterModel::populate
    (QList<ProcData> processes, const QString &excludePid)
{
    qStableSort(processes);

    if (const int rowCount = m_model->rowCount())
        m_model->removeRows(0, rowCount);

    QStandardItem *root  = m_model->invisibleRootItem();
    foreach (const ProcData &proc, processes) {
        QList<QStandardItem *> row;
        row.append(new QStandardItem(proc.ppid));
        QString name = proc.image.isEmpty() ? proc.name : proc.image;
        row.back()->setData(name, ProcessImageRole);
        row.append(new QStandardItem(proc.name));
        row.back()->setToolTip(proc.image);
        row.append(new QStandardItem(proc.state));

        if (proc.ppid == excludePid)
            foreach (QStandardItem *item, row)
                item->setEnabled(false);
        root->appendRow(row);
    }
}


///////////////////////////////////////////////////////////////////////
//
// AttachCoreDialog
//
///////////////////////////////////////////////////////////////////////

AttachCoreDialog::AttachCoreDialog(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::AttachCoreDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->toolchainComboBox->init(false);

    m_ui->execFileName->setExpectedKind(PathChooser::File);
    m_ui->execFileName->setPromptDialogTitle(tr("Select Executable"));

    m_ui->coreFileName->setExpectedKind(PathChooser::File);
    m_ui->coreFileName->setPromptDialogTitle(tr("Select Core File"));

    m_ui->sysrootPathChooser->setExpectedKind(PathChooser::Directory);
    m_ui->sysrootPathChooser->setPromptDialogTitle(tr("Select Sysroot"));

    m_ui->overrideStartScriptFileName->setExpectedKind(PathChooser::File);
    m_ui->overrideStartScriptFileName->setPromptDialogTitle(tr("Select Startup Script"));

    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_ui->coreFileName, SIGNAL(changed(QString)), this, SLOT(changed()));
    changed();
}

AttachCoreDialog::~AttachCoreDialog()
{
    delete m_ui;
}

QString AttachCoreDialog::executableFile() const
{
    return m_ui->execFileName->path();
}

void AttachCoreDialog::setExecutableFile(const QString &fileName)
{
    m_ui->execFileName->setPath(fileName);
    changed();
}

QString AttachCoreDialog::coreFile() const
{
    return m_ui->coreFileName->path();
}

void AttachCoreDialog::setCoreFile(const QString &fileName)
{
    m_ui->coreFileName->setPath(fileName);
    changed();
}

ProjectExplorer::Abi AttachCoreDialog::abi() const
{
    return m_ui->toolchainComboBox->abi();
}

void AttachCoreDialog::setAbiIndex(int i)
{
    if (i >= 0 && i < m_ui->toolchainComboBox->count())
        m_ui->toolchainComboBox->setCurrentIndex(i);
}

int AttachCoreDialog::abiIndex() const
{
    return m_ui->toolchainComboBox->currentIndex();
}

QString AttachCoreDialog::debuggerCommand()
{
    return m_ui->toolchainComboBox->debuggerCommand();
}

QString AttachCoreDialog::sysroot() const
{
    return m_ui->sysrootPathChooser->path();
}

void AttachCoreDialog::setSysroot(const QString &sysroot)
{
    m_ui->sysrootPathChooser->setPath(sysroot);
}

QString AttachCoreDialog::overrideStartScript() const
{
    return m_ui->overrideStartScriptFileName->path();
}

void AttachCoreDialog::setOverrideStartScript(const QString &scriptName)
{
    m_ui->overrideStartScriptFileName->setPath(scriptName);
}

bool AttachCoreDialog::isValid() const
{
    return m_ui->toolchainComboBox->currentIndex() >= 0 &&
           !coreFile().isEmpty();
}

void AttachCoreDialog::changed()
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid());
}

///////////////////////////////////////////////////////////////////////
//
// AttachExternalDialog
//
///////////////////////////////////////////////////////////////////////

AttachExternalDialog::AttachExternalDialog(QWidget *parent)
  : QDialog(parent),
    m_selfPid(QString::number(QCoreApplication::applicationPid())),
    m_ui(new Ui::AttachExternalDialog),
    m_model(new ProcessListFilterModel(this))
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->toolchainComboBox->init(true);
    okButton()->setDefault(true);
    okButton()->setEnabled(false);

    m_ui->procView->setModel(m_model);
    m_ui->procView->setSortingEnabled(true);
    m_ui->procView->sortByColumn(1, Qt::AscendingOrder);

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QPushButton *refreshButton = new QPushButton(tr("Refresh"));
    connect(refreshButton, SIGNAL(clicked()), this, SLOT(rebuildProcessList()));
    m_ui->buttonBox->addButton(refreshButton, QDialogButtonBox::ActionRole);
    m_ui->filterWidget->setFocus(Qt::TabFocusReason);

    m_ui->procView->setAlternatingRowColors(true);
    m_ui->procView->setRootIsDecorated(false);
    m_ui->procView->setUniformRowHeights(true);

    // Do not use activated, will be single click in Oxygen
    connect(m_ui->procView, SIGNAL(doubleClicked(QModelIndex)),
        this, SLOT(procSelected(QModelIndex)));
    connect(m_ui->procView, SIGNAL(clicked(QModelIndex)),
        this, SLOT(procClicked(QModelIndex)));
    connect(m_ui->pidLineEdit, SIGNAL(textChanged(QString)),
            this, SLOT(pidChanged(QString)));
    connect(m_ui->filterWidget, SIGNAL(filterChanged(QString)),
            this, SLOT(setFilterString(QString)));


    setMinimumHeight(500);
    rebuildProcessList();
}

AttachExternalDialog::~AttachExternalDialog()
{
    delete m_ui;
}

void AttachExternalDialog::setFilterString(const QString &filter)
{
    m_model->setFilterFixedString(filter);
    // Activate the line edit if there's a unique filtered process.
    QString processId;
    if (m_model->rowCount(QModelIndex()) == 1)
        processId = m_model->processIdAt(m_model->index(0, 0, QModelIndex()));
    m_ui->pidLineEdit->setText(processId);
    pidChanged(processId);
}

QPushButton *AttachExternalDialog::okButton() const
{
    return m_ui->buttonBox->button(QDialogButtonBox::Ok);
}

void AttachExternalDialog::rebuildProcessList()
{
    m_model->populate(hostProcessList(), m_selfPid);
    m_ui->procView->expandAll();
    m_ui->procView->resizeColumnToContents(0);
    m_ui->procView->resizeColumnToContents(1);
}

void AttachExternalDialog::procSelected(const QModelIndex &proxyIndex)
{
    const QString processId = m_model->processIdAt(proxyIndex);
    if (!processId.isEmpty()) {
        m_ui->pidLineEdit->setText(processId);
        if (okButton()->isEnabled())
            okButton()->animateClick();
    }
}

void AttachExternalDialog::procClicked(const QModelIndex &proxyIndex)
{
    const QString processId = m_model->processIdAt(proxyIndex);
    if (!processId.isEmpty())
        m_ui->pidLineEdit->setText(processId);
}

QString AttachExternalDialog::attachPIDText() const
{
    return m_ui->pidLineEdit->text().trimmed();
}

qint64 AttachExternalDialog::attachPID() const
{
    return attachPIDText().toLongLong();
}

QString AttachExternalDialog::executable() const
{
    // Search pid in model in case the user typed in the PID.
    return m_model->executableForPid(attachPIDText());
}

ProjectExplorer::Abi AttachExternalDialog::abi() const
{
    return m_ui->toolchainComboBox->abi();
}

void AttachExternalDialog::setAbiIndex(int i)
{
    if (i >= 0 && i < m_ui->toolchainComboBox->count())
        m_ui->toolchainComboBox->setCurrentIndex(i);
}

int AttachExternalDialog::abiIndex() const
{
    return m_ui->toolchainComboBox->currentIndex();
}

QString AttachExternalDialog::debuggerCommand()
{
    return m_ui->toolchainComboBox->debuggerCommand();
}

void AttachExternalDialog::pidChanged(const QString &pid)
{
    const bool enabled = !pid.isEmpty() && pid != QLatin1String("0") && pid != m_selfPid
            && m_ui->toolchainComboBox->currentIndex() >= 0;
    okButton()->setEnabled(enabled);
}

void AttachExternalDialog::accept()
{
#ifdef Q_OS_WIN
    const qint64 pid = attachPID();
    if (pid && isWinProcessBeingDebugged(pid)) {
        QMessageBox::warning(this, tr("Process Already Under Debugger Control"),
                             tr("The process %1 is already under the control of a debugger.\n"
                                "Qt Creator cannot attach to it.").arg(pid));
        return;
    }
#endif
    QDialog::accept();
}


///////////////////////////////////////////////////////////////////////
//
// StartExternalDialog
//
///////////////////////////////////////////////////////////////////////

StartExternalDialog::StartExternalDialog(QWidget *parent)
  : QDialog(parent), m_ui(new Ui::StartExternalDialog)
{
    QSettings *settings = Core::ICore::instance()->settings();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->toolChainComboBox->init(true);
    m_ui->execFile->setExpectedKind(PathChooser::File);
    m_ui->execFile->setPromptDialogTitle(tr("Select Executable"));
    m_ui->execFile->lineEdit()->setCompleter(
        new HistoryCompleter(settings, m_ui->execFile->lineEdit()));
    connect(m_ui->execFile, SIGNAL(changed(QString)), this, SLOT(changed()));
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_ui->workingDirectory->setExpectedKind(PathChooser::ExistingDirectory);
    m_ui->workingDirectory->setPromptDialogTitle(tr("Select Working Directory"));
    m_ui->workingDirectory->lineEdit()->setCompleter(
        new HistoryCompleter(settings, m_ui->workingDirectory->lineEdit()));

    m_ui->argsEdit->setCompleter(new HistoryCompleter(settings, m_ui->argsEdit));

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    changed();
}

StartExternalDialog::~StartExternalDialog()
{
    delete m_ui;
}

void StartExternalDialog::setExecutableFile(const QString &str)
{
    m_ui->execFile->setPath(str);
    changed();
}

QString StartExternalDialog::executableFile() const
{
    return m_ui->execFile->path();
}

void StartExternalDialog::setWorkingDirectory(const QString &str)
{
    m_ui->workingDirectory->setPath(str);
}

QString StartExternalDialog::workingDirectory() const
{
    return m_ui->workingDirectory->path();
}

void StartExternalDialog::setExecutableArguments(const QString &str)
{
    m_ui->argsEdit->setText(str);
}

QString StartExternalDialog::executableArguments() const
{
    return m_ui->argsEdit->text();
}

bool StartExternalDialog::breakAtMain() const
{
    return m_ui->checkBoxBreakAtMain->isChecked();
}

bool StartExternalDialog::runInTerminal() const
{
    return m_ui->checkBoxRunInTerminal->isChecked();
}

void StartExternalDialog::setRunInTerminal(bool v)
{
    m_ui->checkBoxRunInTerminal->setChecked(v);
}

ProjectExplorer::Abi StartExternalDialog::abi() const
{
    return m_ui->toolChainComboBox->abi();
}

void StartExternalDialog::setAbiIndex(int i)
{
    if (i >= 0 && i < m_ui->toolChainComboBox->count())
        m_ui->toolChainComboBox->setCurrentIndex(i);
}

int StartExternalDialog::abiIndex() const
{
    return m_ui->toolChainComboBox->currentIndex();
}

QString StartExternalDialog::debuggerCommand()
{
    return m_ui->toolChainComboBox->debuggerCommand();
}

bool StartExternalDialog::isValid() const
{
    return m_ui->toolChainComboBox->currentIndex() >= 0
           && !executableFile().isEmpty();
}

void StartExternalDialog::changed()
{
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(isValid());
}

///////////////////////////////////////////////////////////////////////
//
// StartRemoteDialog
//
///////////////////////////////////////////////////////////////////////

StartRemoteDialog::StartRemoteDialog(QWidget *parent, bool enableStartScript)
  : QDialog(parent),
    m_ui(new Ui::StartRemoteDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);
    m_ui->debuggerPathChooser->setExpectedKind(PathChooser::File);
    m_ui->debuggerPathChooser->setPromptDialogTitle(tr("Select Debugger"));
    m_ui->debuginfoPathChooser->setExpectedKind(PathChooser::File);
    m_ui->debuginfoPathChooser->setPromptDialogTitle(tr("Select Location of Debugging Information"));
    m_ui->executablePathChooser->setExpectedKind(PathChooser::File);
    m_ui->executablePathChooser->setPromptDialogTitle(tr("Select Executable"));
    m_ui->sysrootPathChooser->setPromptDialogTitle(tr("Select Sysroot"));
    m_ui->overrideStartScriptPathChooser->setExpectedKind(PathChooser::File);
    m_ui->overrideStartScriptPathChooser->setPromptDialogTitle(tr("Select GDB Start Script"));
    m_ui->serverStartScriptPathChooser->setExpectedKind(PathChooser::File);
    m_ui->serverStartScriptPathChooser->setPromptDialogTitle(tr("Select Server Start Script"));
    m_ui->serverStartScriptPathChooser->setVisible(enableStartScript);
    m_ui->serverStartScriptLabel->setVisible(enableStartScript);
    m_ui->useServerStartScriptCheckBox->setVisible(enableStartScript);
    m_ui->useServerStartScriptLabel->setVisible(enableStartScript);

    connect(m_ui->useServerStartScriptCheckBox, SIGNAL(toggled(bool)),
        SLOT(updateState()));
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));

    updateState();
}

StartRemoteDialog::~StartRemoteDialog()
{
    delete m_ui;
}

void StartRemoteDialog::setRemoteChannel(const QString &channel)
{
    m_ui->channelLineEdit->setText(channel);
}

QString StartRemoteDialog::remoteChannel() const
{
    return m_ui->channelLineEdit->text();
}

void StartRemoteDialog::setLocalExecutable(const QString &executable)
{
    m_ui->executablePathChooser->setPath(executable);
}

QString StartRemoteDialog::localExecutable() const
{
    return m_ui->executablePathChooser->path();
}

void StartRemoteDialog::setDebugger(const QString &debugger)
{
    m_ui->debuggerPathChooser->setPath(debugger);
}

QString StartRemoteDialog::debugger() const
{
    return m_ui->debuggerPathChooser->path();
}

void StartRemoteDialog::setDebugInfoLocation(const QString &location)
{
    m_ui->debuginfoPathChooser->setPath(location);
}

QString StartRemoteDialog::debugInfoLocation() const
{
    return m_ui->debuginfoPathChooser->path();
}

void StartRemoteDialog::setRemoteArchitectures(const QStringList &list)
{
    m_ui->architectureComboBox->clear();
    if (!list.isEmpty()) {
        m_ui->architectureComboBox->insertItems(0, list);
        m_ui->architectureComboBox->setCurrentIndex(0);
    }
}

void StartRemoteDialog::setRemoteArchitecture(const QString &arch)
{
    int index = m_ui->architectureComboBox->findText(arch);
    if (index != -1)
        m_ui->architectureComboBox->setCurrentIndex(index);
}

QString StartRemoteDialog::remoteArchitecture() const
{
    return m_ui->architectureComboBox->currentText();
}

QString StartRemoteDialog::gnuTarget() const
{
    return m_ui->gnuTargetComboBox->currentText();
}

void StartRemoteDialog::setGnuTargets(const QStringList &gnuTargets)
{
    m_ui->gnuTargetComboBox->clear();
    if (!gnuTargets.isEmpty()) {
        m_ui->gnuTargetComboBox->insertItems(0, gnuTargets);
        m_ui->gnuTargetComboBox->setCurrentIndex(0);
    }
}

void StartRemoteDialog::setGnuTarget(const QString &gnuTarget)
{
    const int index = m_ui->gnuTargetComboBox->findText(gnuTarget);
    if (index != -1)
        m_ui->gnuTargetComboBox->setCurrentIndex(index);
}

QString StartRemoteDialog::overrideStartScript() const
{
    return m_ui->overrideStartScriptPathChooser->path();
}

void StartRemoteDialog::setOverrideStartScript(const QString &scriptName)
{
    m_ui->overrideStartScriptPathChooser->setPath(scriptName);
}

void StartRemoteDialog::setServerStartScript(const QString &scriptName)
{
    m_ui->serverStartScriptPathChooser->setPath(scriptName);
}

QString StartRemoteDialog::serverStartScript() const
{
    return m_ui->serverStartScriptPathChooser->path();
}

void StartRemoteDialog::setUseServerStartScript(bool on)
{
    m_ui->useServerStartScriptCheckBox->setChecked(on);
}

bool StartRemoteDialog::useServerStartScript() const
{
    return m_ui->useServerStartScriptCheckBox->isChecked();
}

void StartRemoteDialog::setSysroot(const QString &sysroot)
{
    m_ui->sysrootPathChooser->setPath(sysroot);
}

QString StartRemoteDialog::sysroot() const
{
    return m_ui->sysrootPathChooser->path();
}

void StartRemoteDialog::updateState()
{
    bool enabled = m_ui->useServerStartScriptCheckBox->isChecked();
    m_ui->serverStartScriptLabel->setEnabled(enabled);
    m_ui->serverStartScriptPathChooser->setEnabled(enabled);
}

///////////////////////////////////////////////////////////////////////
//
// AttachToQmlPortDialog
//
///////////////////////////////////////////////////////////////////////

AttachToQmlPortDialog::AttachToQmlPortDialog(QWidget *parent)
  : QDialog(parent),
    m_ui(new Ui::AttachToQmlPortDialog)
{
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setDefault(true);

    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

AttachToQmlPortDialog::~AttachToQmlPortDialog()
{
    delete m_ui;
}

void AttachToQmlPortDialog::setHost(const QString &host)
{
    m_ui->hostLineEdit->setText(host);
}

QString AttachToQmlPortDialog::host() const
{
    return m_ui->hostLineEdit->text();
}

void AttachToQmlPortDialog::setPort(const int port)
{
    m_ui->portSpinBox->setValue(port);
}

int AttachToQmlPortDialog::port() const
{
    return m_ui->portSpinBox->value();
}

QString AttachToQmlPortDialog::sysroot() const
{
    return m_ui->sysRootChooser->path();
}

void AttachToQmlPortDialog::setSysroot(const QString &sysroot)
{
    m_ui->sysRootChooser->setPath(sysroot);
}

// --------- StartRemoteCdbDialog
static inline QString cdbRemoteHelp()
{
    const char *cdbConnectionSyntax =
            "Server:Port<br>"
            "tcp:server=Server,port=Port[,password=Password][,ipversion=6]\n"
            "tcp:clicon=Server,port=Port[,password=Password][,ipversion=6]\n"
            "npipe:server=Server,pipe=PipeName[,password=Password]\n"
            "com:port=COMPort,baud=BaudRate,channel=COMChannel[,password=Password]\n"
            "spipe:proto=Protocol,{certuser=Cert|machuser=Cert},server=Server,pipe=PipeName[,password=Password]\n"
            "ssl:proto=Protocol,{certuser=Cert|machuser=Cert},server=Server,port=Socket[,password=Password]\n"
            "ssl:proto=Protocol,{certuser=Cert|machuser=Cert},clicon=Server,port=Socket[,password=Password]";

    const QString ext32 = QDir::toNativeSeparators(CdbEngine::extensionLibraryName(false));
    const QString ext64 = QDir::toNativeSeparators(CdbEngine::extensionLibraryName(true));
    return  StartRemoteCdbDialog::tr(
                "<html><body><p>The remote CDB needs to load the matching Qt Creator CDB extension "
                "(<code>%1</code> or <code>%2</code>, respectively).</p><p>Copy it onto the remote machine and set the "
                "environment variable <code>%3</code> to point to its folder.</p><p>"
                "Launch the remote CDB as <code>%4 &lt;executable&gt;</code> "
                " to use TCP/IP as communication protocol.</p><p>Enter the connection parameters as:</p>"
                "<pre>%5</pre></body></html>").
            arg(ext32, ext64, QLatin1String("_NT_DEBUGGER_EXTENSION_PATH"),
                QLatin1String("cdb.exe -server tcp:port=1234"),
                QLatin1String(cdbConnectionSyntax));
}

StartRemoteCdbDialog::StartRemoteCdbDialog(QWidget *parent) :
    QDialog(parent), m_okButton(0), m_lineEdit(new QLineEdit)
{
    setWindowTitle(tr("Start a CDB Remote Session"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);

    QGroupBox *groupBox = new QGroupBox;
    QFormLayout *formLayout = new QFormLayout;
    QLabel *helpLabel = new QLabel(cdbRemoteHelp());
    helpLabel->setWordWrap(true);
    helpLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    formLayout->addRow(helpLabel);
    QLabel *label = new QLabel(tr("&Connection:"));
    label->setBuddy(m_lineEdit);
    m_lineEdit->setMinimumWidth(400);
    connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));
    formLayout->addRow(label, m_lineEdit);
    groupBox->setLayout(formLayout);

    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addWidget(groupBox);
    QDialogButtonBox *box = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    vLayout->addWidget(box);
    m_okButton = box->button(QDialogButtonBox::Ok);
    connect(m_lineEdit, SIGNAL(returnPressed()), m_okButton, SLOT(animateClick()));
    m_okButton->setEnabled(false);
    connect(box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(box, SIGNAL(rejected()), this, SLOT(reject()));

    setLayout(vLayout);
}

void StartRemoteCdbDialog::accept()
{
    if (!m_lineEdit->text().isEmpty())
        QDialog::accept();
}

StartRemoteCdbDialog::~StartRemoteCdbDialog()
{
}

void StartRemoteCdbDialog::textChanged(const QString &t)
{
    m_okButton->setEnabled(!t.isEmpty());
}

QString StartRemoteCdbDialog::connection() const
{
    const QString rc = m_lineEdit->text();
    // Transform an IP:POrt ('localhost:1234') specification into full spec
    QRegExp ipRegexp(QLatin1String("([\\w\\.\\-_]+):([0-9]{1,4})"));
    QTC_ASSERT(ipRegexp.isValid(), return QString());
    if (ipRegexp.exactMatch(rc))
        return QString::fromAscii("tcp:server=%1,port=%2").arg(ipRegexp.cap(1), ipRegexp.cap(2));
    return rc;
}

void StartRemoteCdbDialog::setConnection(const QString &c)
{
    m_lineEdit->setText(c);
    m_okButton->setEnabled(!c.isEmpty());
}

AddressDialog::AddressDialog(QWidget *parent) :
        QDialog(parent),
        m_lineEdit(new QLineEdit),
        m_box(new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel))
{
    setWindowTitle(tr("Select Start Address"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->addWidget(new QLabel(tr("Enter an address: ")));
    hLayout->addWidget(m_lineEdit);
    QVBoxLayout *vLayout = new QVBoxLayout;
    vLayout->addLayout(hLayout);
    vLayout->addWidget(m_box);
    setLayout(vLayout);

    connect(m_box, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_box, SIGNAL(rejected()), this, SLOT(reject()));
    connect(m_lineEdit, SIGNAL(returnPressed()), this, SLOT(accept()));
    connect(m_lineEdit, SIGNAL(textChanged(QString)), this, SLOT(textChanged()));

    setOkButtonEnabled(false);
}

void AddressDialog::setOkButtonEnabled(bool v)
{
    m_box->button(QDialogButtonBox::Ok)->setEnabled(v);
}

bool AddressDialog::isOkButtonEnabled() const
{
    return m_box->button(QDialogButtonBox::Ok)->isEnabled();
}

void AddressDialog::setAddress(quint64 a)
{
    m_lineEdit->setText(QLatin1String("0x")
                        + QString::number(a, 16));
}

quint64 AddressDialog::address() const
{
    return m_lineEdit->text().toULongLong(0, 16);
}

void AddressDialog::accept()
{
    if (isOkButtonEnabled())
        QDialog::accept();
}

void AddressDialog::textChanged()
{
    setOkButtonEnabled(isValid());
}

bool AddressDialog::isValid() const
{
    const QString text = m_lineEdit->text();
    bool ok = false;
    text.toULongLong(&ok, 16);
    return ok;
}

///////////////////////////////////////////////////////////////////////
//
// StartRemoteEngineDialog
//
///////////////////////////////////////////////////////////////////////

StartRemoteEngineDialog::StartRemoteEngineDialog(QWidget *parent) :
    QDialog(parent) ,
    m_ui(new Ui::StartRemoteEngineDialog)
{
    QSettings *settings = Core::ICore::instance()->settings();
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->setupUi(this);
    m_ui->host->setCompleter(new HistoryCompleter(settings, m_ui->host));
    m_ui->username->setCompleter(new HistoryCompleter(settings, m_ui->username));
    m_ui->enginepath->setCompleter(new HistoryCompleter(settings, m_ui->enginepath));
    m_ui->inferiorpath->setCompleter(new HistoryCompleter(settings, m_ui->inferiorpath));
    connect(m_ui->buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

StartRemoteEngineDialog::~StartRemoteEngineDialog()
{
    delete m_ui;
}

QString StartRemoteEngineDialog::host() const
{
    return m_ui->host->text();
}

QString StartRemoteEngineDialog::username() const
{
    return m_ui->username->text();
}

QString StartRemoteEngineDialog::password() const
{
    return m_ui->password->text();
}

QString StartRemoteEngineDialog::inferiorPath() const
{
    return m_ui->inferiorpath->text();
}

QString StartRemoteEngineDialog::enginePath() const
{
    return m_ui->enginepath->text();
}

///////////////////////////////////////////////////////////////////////
//
// TypeFormatsDialogUi
//
///////////////////////////////////////////////////////////////////////

class TypeFormatsDialogPage : public QWidget
{
public:
    TypeFormatsDialogPage()
    {
        m_layout = new QGridLayout;
        m_layout->setColumnStretch(0, 2);
        QVBoxLayout *vboxLayout = new QVBoxLayout;
        vboxLayout->addLayout(m_layout);
        vboxLayout->addItem(new QSpacerItem(1, 1, QSizePolicy::Ignored,
                                            QSizePolicy::MinimumExpanding));
        setLayout(vboxLayout);
    }

    void addTypeFormats(const QString &type,
        const QStringList &typeFormats, int current)
    {
        const int row = m_layout->rowCount();
        int column = 0;
        QButtonGroup *group = new QButtonGroup(this);
        m_layout->addWidget(new QLabel(type), row, column++);
        for (int i = -1; i != typeFormats.size(); ++i) {
            QRadioButton *choice = new QRadioButton(this);
            choice->setText(i == -1 ? TypeFormatsDialog::tr("Reset") : typeFormats.at(i));
            m_layout->addWidget(choice, row, column++);
            if (i == current)
                choice->setChecked(true);
            group->addButton(choice, i);
        }
    }
private:
    QGridLayout *m_layout;
};

class TypeFormatsDialogUi
{
public:
    TypeFormatsDialogUi(TypeFormatsDialog *q)
    {
        QVBoxLayout *layout = new QVBoxLayout(q);
        tabs = new QTabWidget(q);

        buttonBox = new QDialogButtonBox(q);
        buttonBox->setOrientation(Qt::Horizontal);
        buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
        layout->addWidget(tabs);
        layout->addWidget(buttonBox);
        q->setLayout(layout);
    }

    void addPage(const QString &name)
    {
        TypeFormatsDialogPage *page = new TypeFormatsDialogPage;
        pages.append(page);
        QScrollArea *scroller = new QScrollArea;
        scroller->setWidgetResizable(true);
        scroller->setWidget(page);
        scroller->setFrameStyle(QFrame::NoFrame);
        tabs->addTab(scroller, name);
    }

public:
    QDialogButtonBox *buttonBox;
    QList<TypeFormatsDialogPage *> pages;

private:
    QTabWidget *tabs;
};


///////////////////////////////////////////////////////////////////////
//
// TypeFormatsDialog
//
///////////////////////////////////////////////////////////////////////


TypeFormatsDialog::TypeFormatsDialog(QWidget *parent)
   : QDialog(parent), m_ui(new TypeFormatsDialogUi(this))
{
    setWindowTitle(tr("Type Formats"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    m_ui->addPage(tr("Qt Types"));
    m_ui->addPage(tr("Standard Types"));
    m_ui->addPage(tr("Misc Types"));

    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(accept()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

TypeFormatsDialog::~TypeFormatsDialog()
{
    delete m_ui;
}

void TypeFormatsDialog::addTypeFormats(const QString &type0,
    const QStringList &typeFormats, int current)
{
    QString type = type0;
    type.replace("__", "::");
    int pos = 2;
    if (type.startsWith(QLatin1Char('Q')))
        pos = 0;
    else if (type.startsWith(QLatin1String("std::")))
        pos = 1;
    m_ui->pages[pos]->addTypeFormats(type, typeFormats, current);
}

TypeFormats TypeFormatsDialog::typeFormats() const
{
    return TypeFormats();
}

} // namespace Internal
} // namespace Debugger
