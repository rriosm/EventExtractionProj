#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QSettings>
#include <QString>
#include "inc/Message.h"

class SettingsManager : public QObject
{
        Q_OBJECT

        Q_CLASSINFO("Author", "author")
        Q_CLASSINFO("Version", "1.0.1")
        Q_CLASSINFO("Status", "Active")
        Q_CLASSINFO("Description", "")

    public:

        /**
         * @brief SettingsManager
         * @param parent
         */
        explicit SettingsManager(QObject *parent = nullptr);

        /**
         * @brief ~SettingsManager
         */
        virtual ~SettingsManager();

        Q_DISABLE_COPY(SettingsManager)

        /**
         * @brief operator ==
         * @param ptr
         * @return
         */
        bool operator==(const SettingsManager *ptr) const;

        /**
         * @brief operator !=
         * @param ptr
         * @return
         */
        bool operator!=(const SettingsManager *ptr) const;

        // Provider Accessors

        /**
         * @brief setApiKey
         * @param schema
         * @param key
         */
        void setApiKey(Message::Schema schema, const QString &key);

        /**
         * @brief apiKey
         * @param schema
         * @return
         */
        QString apiKey(Message::Schema schema) const;

        /**
         * @brief setModel
         * @param schema
         * @param modelName
         */
        void setModel(Message::Schema schema, const QString &modelName);

        /**
         * @brief model
         * @param schema
         * @return
         */
        QString model(Message::Schema schema) const;

        // Global Preferences

        /**
         * @brief setDefaultSchema
         * @param schema
         */
        void setDefaultSchema(Message::Schema schema);

        /**
         * @brief defaultSchema
         * @return
         */
        Message::Schema defaultSchema() const;

        // Persistence
        void sync(); // Force write to disk

    signals:

        // TODO

    private:

        /**
         * @brief schemaToString
         * @param schema
         * @return
         */
        QString schemaToString(Message::Schema schema) const;

    private:

        QSettings m_settings;


};

#endif // SETTINGSMANAGER_H
