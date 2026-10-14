#include "MfErrNo.h"
#include "Core.h"
#include "Sw_Device.h"
#include "Sw_ISO14443A-3.h"
#include "Sw_Mf_Classic.h"
#include "Tools.h"

#include "odalidbackend.h"
#include <QThread>
#include <QDebug>
#include <cstring>

static constexpr uint8_t BLK_PRENOM   = 9;
static constexpr uint8_t BLK_NOM      = 10;
static constexpr uint8_t KEY_ID       = 2;

static constexpr uint8_t BLK_BACKUP   = 13;
static constexpr uint8_t BLK_COUNTER  = 14;
static constexpr uint8_t KEY_WALLET   = 3;

struct OdalidBackend::Impl { ReaderName reader; };

OdalidBackend::OdalidBackend() : d(new Impl) { std::memset(&d->reader, 0, sizeof(ReaderName)); }
OdalidBackend::~OdalidBackend() { if (connected) disconnectReader(); delete d; }

bool OdalidBackend::fail(int16_t status, const char *what)
{
    m_error = QString("%1: %2 (%3)").arg(what, QString::fromLatin1(GetErrorMessage(status))).arg(status);
    uint8_t atq[2], sak[1], uid[12]; uint16_t len = 12;
    ISO14443_3_A_PollCard(&d->reader, atq, sak, uid, &len);
    return false;
}

bool OdalidBackend::connectReader()
{
    qDebug() << "1 before GetLibraryExtension";
    LIB_VERSION ext;
    GetLibraryExtension(&ext);
    qDebug() << "2 after GetLibraryExtension";

    std::memset(&d->reader, 0, sizeof(ReaderName));
    d->reader.Type   = ReaderCDC;
    d->reader.device = 10;

    qDebug() << "3 before OpenCOM";
    int16_t st = OpenCOM(&d->reader);
    qDebug() << "4 after OpenCOM, status =" << st;
    if (st != MI_OK) { m_error = QString("Reader not found: %1").arg(GetErrorMessage(st)); return false; }
    connected = true;

    qDebug() << "5 before Version";
    st = Version(&d->reader);
    qDebug() << "6 after Version, status =" << st;
    if (st != MI_OK) return fail(st, "Version");

    qDebug() << "7 before RF_Power_Control";
    st = RF_Power_Control(&d->reader, TRUE, 0);
    qDebug() << "8 after RF_Power_Control, status =" << st;
    if (st != MI_OK) return fail(st, "RF_Power_Control");

    LEDBuzzer(&d->reader, LED_GREEN_ON);
    return true;
}

bool OdalidBackend::disconnectReader()
{
    if (!connected) return true;
    LEDBuzzer(&d->reader, LED_OFF);
    int16_t st = RF_Power_Control(&d->reader, FALSE, 0);
    int16_t st2 = CloseCOM(&d->reader);
    connected = false;
    if (st != MI_OK)  { m_error = QString("RF_Power_Control: %1").arg(GetErrorMessage(st));  return false; }
    if (st2 != MI_OK) { m_error = QString("CloseCOM: %1").arg(GetErrorMessage(st2)); return false; }
    return true;
}

bool OdalidBackend::selectCard(QString &uidHex)
{
    uint8_t atq[2], sak[1], uid[12]; uint16_t len = 12;
    int16_t st = ISO14443_3_A_PollCard(&d->reader, atq, sak, uid, &len);
    if (st != MI_OK) { m_error = "No card in the RF field"; return false; }
    uidHex.clear();
    for (int i = 0; i < len; ++i) uidHex += QString("%1").arg(uid[i], 2, 16, QChar('0')).toUpper();
    return true;
}

bool OdalidBackend::readBlock(uint8_t block, bool keyA, uint8_t keyIndex, QString &out)
{
    uint8_t data[17] = {0};
    int16_t st = Mf_Classic_Read_Block(&d->reader, TRUE, block, data, keyA ? AuthKeyA : AuthKeyB, keyIndex);
    if (st != MI_OK) return fail(st, "Mf_Classic_Read_Block");
    out = QString::fromLatin1(reinterpret_cast<char*>(data));
    return true;
}

bool OdalidBackend::writeBlock(uint8_t block, bool keyA, uint8_t keyIndex, const QString &in)
{
    uint8_t data[16] = {0};
    const QByteArray b = in.toLatin1();
    std::memcpy(data, b.constData(), qMin<int>(16, b.size()));
    int16_t st = Mf_Classic_Write_Block(&d->reader, TRUE, block, data, keyA ? AuthKeyA : AuthKeyB, keyIndex);
    return st == MI_OK ? true : fail(st, "Mf_Classic_Write_Block");
}

bool OdalidBackend::readIdentity(QString &nom, QString &prenom)
{
    return readBlock(BLK_NOM,    true, KEY_ID, nom)
        && readBlock(BLK_PRENOM, true, KEY_ID, prenom);
}

bool OdalidBackend::writeIdentity(const QString &nom, const QString &prenom)
{
    return writeBlock(BLK_NOM,    false, KEY_ID, nom)
        && writeBlock(BLK_PRENOM, false, KEY_ID, prenom);
}

bool OdalidBackend::readCounter(uint32_t &units)
{
    int16_t st = Mf_Classic_Read_Value(&d->reader, TRUE, BLK_COUNTER, &units, AuthKeyA, KEY_WALLET);
    return st == MI_OK ? true : fail(st, "Mf_Classic_Read_Value");
}

bool OdalidBackend::decrementCounter(uint32_t n)
{
    int16_t st = Mf_Classic_Decrement_Value(&d->reader, TRUE, BLK_COUNTER, n, BLK_BACKUP, AuthKeyA, KEY_WALLET);
    if (st != MI_OK) return fail(st, "Mf_Classic_Decrement_Value");
    st = Mf_Classic_Restore_Value(&d->reader, TRUE, BLK_BACKUP, BLK_COUNTER, AuthKeyA, KEY_WALLET);
    return st == MI_OK ? true : fail(st, "Mf_Classic_Restore_Value");
}

bool OdalidBackend::incrementCounter(uint32_t n)
{
    int16_t st = Mf_Classic_Increment_Value(&d->reader, TRUE, BLK_COUNTER, n, BLK_BACKUP, AuthKeyB, KEY_WALLET);
    if (st != MI_OK) return fail(st, "Mf_Classic_Increment_Value");
    st = Mf_Classic_Restore_Value(&d->reader, TRUE, BLK_BACKUP, BLK_COUNTER, AuthKeyB, KEY_WALLET);
    return st == MI_OK ? true : fail(st, "Mf_Classic_Restore_Value");
}

void OdalidBackend::signal(bool success)
{
    if (!connected) return;
    if (success) { LEDBuzzer(&d->reader, LED_GREEN_ON | BUZZER_ON); QThread::msleep(100); }
    else         { LEDBuzzer(&d->reader, LED_RED_ON   | BUZZER_ON); QThread::msleep(400); }
    LEDBuzzer(&d->reader, LED_GREEN_ON);
}
