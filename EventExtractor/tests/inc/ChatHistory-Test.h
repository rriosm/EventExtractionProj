#ifndef CHATHISTORYTEST_H
#define CHATHISTORYTEST_H

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#include "inc/Message.h"
#include "inc/MessageHistory.h"
#include "inc/SettingsManager.h"

void demonstrateConversation()
{
    // 1. Initialize Settings
    SettingsManager settings;
    settings.setApiKey(Message::Schema::Gemini, "GEMINI_API_KEY_HERE");
    settings.setApiKey(Message::Schema::OpenRouter, "OPENROUTER_API_KEY_HERE");

    // 2. Create Message History
    MessageHistory history;

    // 3. Add System Instruction (Persona)
    history.append(Message::RoleType::System,
                   "You are a professional C++ senior developer. "
                             "Analyze code and provide refactored versions focusing on objectivity.");

    // 4. Add User Message with Multi-modal data (Image)
    // We use the static factory to auto-detect MIME and encode to Base64
    QString filePath = "screenshot_error.png";
    Message userMsg = Message::fromLocalFile(Message::RoleType::User,
                                             "Why is my Qt code leaking memory in this screenshot?",
                                             filePath);
    history.append(userMsg);

    // 5. Simulate receiving an Assistant response from an API
    // (In a real app, this comes from the NetworkAgent response)
    Message assistantMsg(Message::RoleType::Assistant,
                         "The screenshot shows a manual 'new' without a corresponding 'delete'. "
                                      "I recommend using QSharedDataPointer for implicit sharing.");
    history.append(assistantMsg);

    // 6. Generate Payloads for different providers

    qDebug() << "--- Generating OpenRouter Payload ---";
    QJsonObject openRouterJson = history.toPayload(Message::Schema::OpenRouter, "openai/gpt-4o");
    //
    qDebug() << QJsonDocument(openRouterJson).toJson(QJsonDocument::Indented);

    qDebug() << "\n--- Generating Gemini Payload ---";
    QJsonObject geminiJson = history.toPayload(Message::Schema::Gemini, "gemini-1.5-pro");
    //
    qDebug() << QJsonDocument(geminiJson).toJson(QJsonDocument::Indented);

    // 7. Testing Persistence (Serialization/Deserialization)
    qDebug() << "\n--- Testing Serialization Round-trip ---";
    QJsonArray serializedArray = history.toJsonArray(Message::Schema::OpenRouter);

    // Reconstruct history from the JSON array
    MessageHistory restoredHistory = MessageHistory::fromJsonArray(serializedArray);

    qDebug() << "Restored messages count:" << restoredHistory.count();
    qDebug() << "Last message content:" << restoredHistory.last().content();
}

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);
    demonstrateConversation();
    return 0; // In a real app, use a.exec()
}

#endif // CHATHISTORYTEST_H
