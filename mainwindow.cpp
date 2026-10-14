#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "simbackend.h"
#ifdef USE_ODALID
#include "odalidbackend.h"
#endif

#include <QMessageBox>
#include <QStatusBar>
#include <QThread>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
#ifdef USE_ODALID
    m_card = std::make_unique<OdalidBackend>();
#else
    m_card = std::make_unique<SimBackend>();
#endif
    statusBar()->showMessage("Ready");

    connect(ui->btnConnect,    &QPushButton::clicked, this, &MainWindow::onConnect);
    connect(ui->btnDisconnect, &QPushButton::clicked, this, &MainWindow::onDisconnect);
    connect(ui->btnSelect,     &QPushButton::clicked, this, &MainWindow::onSelectCard);
    connect(ui->btnUpdate,     &QPushButton::clicked, this, &MainWindow::onUpdateIdentity);
    connect(ui->btnPay,        &QPushButton::clicked, this, &MainWindow::onPay);
    connect(ui->btnCharge,     &QPushButton::clicked, this, &MainWindow::onCharge);
    connect(ui->btnQuit,       &QPushButton::clicked, this, &QWidget::close);
    setState(false, false);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::setState(bool connected, bool cardSelected)
{
    ui->btnConnect->setEnabled(!connected);
    ui->btnDisconnect->setEnabled(connected);
    ui->btnSelect->setEnabled(connected);
    ui->groupId->setEnabled(cardSelected);
    ui->groupDec->setEnabled(cardSelected);
    ui->groupInc->setEnabled(cardSelected);
}

bool MainWindow::report(bool ok, const QString &okMsg)
{
    m_card->signal(ok);
    if (ok) {
        statusBar()->showMessage(okMsg, 5000);
    } else {
        statusBar()->showMessage("Error: " + m_card->lastError());
        QMessageBox::warning(this, "ODALID", m_card->lastError());
    }
    return ok;
}

void MainWindow::refreshCounter()
{
    uint32_t v = 0;
    if (m_card->readCounter(v)) ui->labelUnits->setText(QString::number(v));
    else report(false, QString());
}

void MainWindow::onConnect()
{
    if (m_connecting) return;
    m_connecting = true;
    ui->btnConnect->setEnabled(false);
    statusBar()->showMessage("Connecting to the reader... (can take a few seconds)");

    QThread *t = QThread::create([this] {
        const bool ok = m_card->connectReader();
        QMetaObject::invokeMethod(this, [this, ok] { onConnectFinished(ok); }, Qt::QueuedConnection);
    });
    connect(t, &QThread::finished, t, &QObject::deleteLater);
    t->start();

    QTimer::singleShot(10000, this, [this] {
        if (!m_connecting) return;
        statusBar()->showMessage("The reader is not answering...");
        QMessageBox::warning(this, "ODALID",
            "The reader does not answer (OpenCOM is blocked).\n\n"
            "- Check it appears under Ports (COM & LPT) in Device Manager\n"
            "- Close any other program using the reader\n"
            "- Unplug / replug the reader, then restart this application");
    });
}

void MainWindow::onConnectFinished(bool ok)
{
    m_connecting = false;
    if (report(ok, "Reader connected")) setState(true, false);
    else ui->btnConnect->setEnabled(true);
}

void MainWindow::onDisconnect()
{
    report(m_card->disconnectReader(), "Reader disconnected");
    setState(false, false);
    ui->lineNom->clear(); ui->linePrenom->clear(); ui->labelUnits->setText("0");
}

void MainWindow::onSelectCard()
{
    QString uid, nom, prenom;
    if (!report(m_card->selectCard(uid), QString()) ) { setState(true, false); return; }
    if (!m_card->readIdentity(nom, prenom))           { report(false, QString()); setState(true, false); return; }
    ui->lineNom->setText(nom);
    ui->linePrenom->setText(prenom);
    setState(true, true);
    refreshCounter();
    statusBar()->showMessage("Card selected, UID = " + uid, 5000);
}

void MainWindow::onUpdateIdentity()
{
    if (!report(m_card->writeIdentity(ui->lineNom->text(), ui->linePrenom->text()), "Identity updated")) return;
    QString nom, prenom;
    if (m_card->readIdentity(nom, prenom)) { ui->lineNom->setText(nom); ui->linePrenom->setText(prenom); }
}

void MainWindow::onPay()
{
    uint32_t n = static_cast<uint32_t>(ui->spinDec->value());
    if (n == 0) { statusBar()->showMessage("Choose a number of units to debit", 4000); return; }
    uint32_t cur = ui->labelUnits->text().toUInt();
    if (n > cur) {
        QMessageBox::warning(this, "ODALID", "Insufficient balance");
        m_card->signal(false);
        return;
    }
    if (report(m_card->decrementCounter(n), QString("%1 unit(s) debited").arg(n))) {
        refreshCounter();
        ui->spinDec->setValue(0);
    }
}

void MainWindow::onCharge()
{
    uint32_t n = static_cast<uint32_t>(ui->spinInc->value());
    if (n == 0) { statusBar()->showMessage("Choose a number of units to credit", 4000); return; }
    if (report(m_card->incrementCounter(n), QString("%1 unit(s) credited").arg(n))) {
        refreshCounter();
        ui->spinInc->setValue(0);
    }
}
