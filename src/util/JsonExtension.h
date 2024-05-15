#ifndef JSONEXTENSION
#define JSONEXTENSION

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QString>


class JsonExtension
{
public:
	static QString RenderJsonValue(const QJsonValue &value, QJsonDocument::JsonFormat format=QJsonDocument::Compact);

};


#endif // JSONEXTENSION
