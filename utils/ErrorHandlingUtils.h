#pragma once

#include <QString>

class QNetworkReply;
class QJsonObject;

namespace ErrorHandlingUtils {

bool hasNetworkOrHttpError(QNetworkReply *reply, QString &errorMessage);
bool parseJsonObject(QNetworkReply *reply, QJsonObject &outObject, QString &errorMessage);
int extractApiCode(const QJsonObject &obj, int fallback = -1);
QString extractApiMessage(const QJsonObject &obj, const QString &fallback);

} // namespace ErrorHandlingUtils
