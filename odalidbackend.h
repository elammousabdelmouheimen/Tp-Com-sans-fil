#pragma once
#include "cardbackend.h"

class OdalidBackend : public CardBackend
{
public:
    OdalidBackend();
    ~OdalidBackend() override;

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
    struct Impl;
    Impl *d;

    bool fail(int16_t status, const char *what);
    bool readBlock(uint8_t block, bool keyA, uint8_t keyIndex, QString &out);
    bool writeBlock(uint8_t block, bool keyA, uint8_t keyIndex, const QString &in);
    bool connected = false;
};
