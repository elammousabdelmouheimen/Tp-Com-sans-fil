#pragma once
#include <QMainWindow>
#include <memory>
#include "cardbackend.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onConnect();
    void onConnectFinished(bool ok);
    void onDisconnect();
    void onSelectCard();
    void onUpdateIdentity();
    void onPay();
    void onCharge();

private:
    void setState(bool connected, bool cardSelected);
    void refreshCounter();
    bool report(bool ok, const QString &okMsg);

    Ui::MainWindow *ui;
    std::unique_ptr<CardBackend> m_card;
    bool m_connecting = false;
};
