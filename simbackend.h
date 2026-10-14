#pragma once
#include "cardbackend.h"

class SimBackend : public CardBackend
{
public:
    bool connectReader() override;
    bool disconnectReader() override;
    bool selectCard(QString &uidHex) override;
    bool readIdentity(QString &nom, QString &prenom) override;
    bool writeIdentity(const QString &nom, const QString &prenom) override;
    bool readCounter(uint32_t &units) override;
    bool decrementCounter(uint32_t n) override;
    bool incrementCounter(uint32_t n) override;
    void signal(bool success) override;

private:
    bool m_connected = false;
    bool m_selected  = false;
    QString m_nom    = "Thivent";
    QString m_prenom = "Vincent";
    uint32_t m_counter = 12;
    uint32_t m_backup  = 12;
};
