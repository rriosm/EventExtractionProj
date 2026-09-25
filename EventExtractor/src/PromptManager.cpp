
#include <QDebug>

#include "inc/PromptManager.h"

PromptManager & PromptManager::instance()
{
    static PromptManager _instance;

    return _instance;
}

// Loads the prompt from the .qrc resource
QString PromptManager::getSystemProfile(Persona type)
{
    return this->m_prompts.value(type, "System Profile");
}

QString PromptManager::getPreparedTask(TaskTemplate task, const ClipData& clipData)
{
    QString templateText = this->m_taskTemplates.value(task);

    // Iterate through the map and replace {{key}} with value
    QMapIterator<QString, QString> i(clipData.data);

    while (i.hasNext())
    {
        i.next();

        qDebug() << "PromptManager::getPreparedTask(...) i.key(): " << i.key() << "\n i.value(): " << i.value();

        templateText.replace("{{" + i.key() + "}}", i.value());
    }

    qDebug() << "PromptManager::getPreparedTask(...) --- " << templateText;

    return templateText;
}

PromptManager::PromptManager()
{
    this->loadAll();
}

void PromptManager::loadAll()
{
    this->m_prompts[Persona::FIFA_Referee] = loadFromFile(":/Resources/SystemProfile");
    // this->m_prompts[Persona::FIFA_Referee] = loadFromFile("/Users/rrios/Documents/Development/EventExtractor/Resources/Prompts/SystemProfile.txt");

    // qDebug() << "PromptManager::loadAll() -- System Profile: " << this->m_prompts[Persona::FIFA_Referee];

    this->m_taskTemplates[TaskTemplate::Video_Analysis_Task] = loadFromFile(":/Resources/UserPromptTemplate");
    // this->m_taskTemplates[TaskTemplate::Video_Analysis_Task] = loadFromFile("/Users/rrios/Documents/Development/EventExtractor/Resources/Prompts/UserPromptTemplate.txt");

    qDebug() << "PromptManager::loadAll() -- User Prompt: " << this->m_taskTemplates[TaskTemplate::Video_Analysis_Task];

    this->m_taskTemplates[TaskTemplate::Video_Baseline_Analysis_Task] = loadFromFile(":/Resources/BaselinePrompt");

    qDebug() << "PromptManager::loadAll() -- User Baseline Prompt: " << this->m_taskTemplates[TaskTemplate::Video_Baseline_Analysis_Task];
}

QString PromptManager::loadFromFile(const QString &path)
{
    QFile file(path);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qWarning() << "Failed to open resource:" << path << "Error:" << file.errorString();

        return "";
    }

    QTextStream in(&file);

    in.setCodec("UTF-8");

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8"); // Note: setCodec is deprecated in Qt 6, but perfectly fine for Qt 5.15
#endif

    return in.readAll();
}
