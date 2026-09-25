#include "SettingsManager.h"

SettingsManager::SettingsManager(QObject *parent) :
    QObject(parent),
    m_settings("Rios' Coorp.", "EventExtractor")
{
    // TODO
}

SettingsManager::~SettingsManager()
{
    // TODO
}

bool SettingsManager::operator==(const SettingsManager *ptr) const
{
    return (this == ptr);
}

bool SettingsManager::operator!=(const SettingsManager *ptr) const
{
    return (this != ptr);
}

void SettingsManager::setApiKey(Message::Schema schema, const QString &key)
{
    this->m_settings.setValue(QString("%1/ApiKey").arg(schemaToString(schema)), key);
}

QString SettingsManager::apiKey(Message::Schema schema) const
{
    return this->m_settings.value(QString("%1/ApiKey").arg(schemaToString(schema))).toString();
}

void SettingsManager::setModel(Message::Schema schema, const QString &modelName)
{
    this->m_settings.setValue(QString("%1/Model").arg(schemaToString(schema)), modelName);
}

QString SettingsManager::model(Message::Schema schema) const // TODO Refactor to consider other models!
{
    QString defaultModel = (schema == Message::Schema::Gemini) ? "gemini-1.5-flash" : "openai/gpt-4o-mini";

    return this->m_settings.value(QString("%1/Model").arg(schemaToString(schema)), defaultModel).toString();
}

void SettingsManager::setDefaultSchema(Message::Schema schema)
{
    this->m_settings.setValue("Global/DefaultSchema", static_cast<int>(schema));
}

Message::Schema SettingsManager::defaultSchema() const
{
    return static_cast<Message::Schema>(this->m_settings.value("Global/DefaultSchema", static_cast<int>(Message::Schema::OpenRouter)).toInt());
}

void SettingsManager::sync()
{
    this->m_settings.sync();
}

QString SettingsManager::schemaToString(Message::Schema schema) const // TODO Refactor to consider other models!
{
    return (schema == Message::Schema::Gemini) ? "Gemini" : "OpenRouter";
}
