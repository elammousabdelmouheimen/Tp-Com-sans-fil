#include "simbackend.h"
#include <QDebug>

bool SimBackend::connectReader()    { m_connected = true;  return true; }
bool SimBackend::disconnectReader() { m_connected = false; m_selected = false; return true; }

bool SimBackend::selectCard(QString &uidHex)
{
    if (!m_connected) { m_error = "Reader not connected"; return false; }
    m_selected = true;
    uidHex = "04A1B2C3";
    return true;
}
bool SimBackend::readIdentity(QString &nom, QString &prenom)
{
    if (!m_selected) { m_error = "No card selected"; return false; }
    nom = m_nom; prenom = m_prenom; return true;
}
bool SimBackend::writeIdentity(const QString &nom, const QString &prenom)
{
    if (!m_selected) { m_error = "No card selected"; return false; }
    m_nom = nom; m_prenom = prenom; return true;
}
bool SimBackend::readCounter(uint32_t &units)
{
    if (!m_selected) { m_error = "No card selected"; return false; }
    units = m_counter; return true;
}
bool SimBackend::decrementCounter(uint32_t n)
{
    if (!m_selected) { m_error = "No card selected"; return false; }
    if (n > m_counter) { m_error = "Insufficient balance"; return false; }
    m_backup = m_counter - n;
    m_counter = m_backup;
    return true;
}
bool SimBackend::incrementCounter(uint32_t n)
{
    if (!m_selected) { m_error = "No card selected"; return false; }
    m_backup = m_counter + n;
    m_counter = m_backup;
    return true;
}
void SimBackend::signal(bool success) { qDebug() << "[SIM] LED/buzzer:" << (success ? "green + beep" : "red + long beep"); }
