#ifndef PROMPTMANAGER_H
#define PROMPTMANAGER_H

#include <QString>
#include <QMap>
#include <QFile>
#include <QTextStream>

#include "inc/Defs.h"

class PromptManager
{
    public:

        /**
         * @brief instance
         * @return
         */
        static PromptManager& instance();

        /**
         * @brief getSystemProfile: Loads the prompt from the .qrc resource
         * @param type
         * @return
         */
        QString getSystemProfile(Persona type);

        /**
         * @brief getPreparedTask: Returns the User Task with data injected
         *        into the {{placeholders}}
         * @param task
         * @param clipData
         * @return
         */
        QString getPreparedTask(TaskTemplate task, const ClipData& clipData);

    private:

        /**
         * @brief PromptManager
         */
        PromptManager();

        /**
         * @brief loadAll
         */
        void loadAll();

        /**
         * @brief loadFromFile
         * @param path
         * @return
         */
        QString loadFromFile(const QString &path);

    private:

        QMap<Persona, QString> m_prompts;

        QMap<TaskTemplate, QString> m_taskTemplates;
};

#endif // PROMPTMANAGER_H
