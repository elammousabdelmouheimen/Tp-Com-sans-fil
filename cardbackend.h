#pragma once
#include <QString>
#include <cstdint>

class CardBackend
{
public:
    virtual ~CardBackend() = default;

    virtual bool connectReader()    = 0;
    virtual bool disconnectReader() = 0;
    virtual bool selectCard(QString &uidHex) = 0;

    virtual bool readIdentity(QString &nom, QString &prenom) = 0;
    virtual bool writeIdentity(const QString &nom, const QString &prenom) = 0;

    virtual bool readCounter(uint32_t &units) = 0;
    virtual bool decrementCounter(uint32_t n) = 0;
    virtual bool incrementCounter(uint32_t n) = 0;

    virtual void signal(bool success) = 0;

    QString lastError() const { return m_error; }

protected:
    QString m_error;
};
