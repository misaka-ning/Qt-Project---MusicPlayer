#include "utils/ErrorHandlingUtils.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace ErrorHandlingUtils {

bool hasNetworkOrHttpError(QNetworkReply *reply, QString &errorMessage)
{
    if (!reply) {
        errorMessage = QStringLiteral("网络回复对象为空");
        return true;
    }

    if (reply->error() != QNetworkReply::NoError) {
        errorMessage = reply->errorString();
        return true;
    }

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode >= 400) {
        errorMessage = QStringLiteral("HTTP %1").arg(statusCode);
        return true;
    }

    return false;
}

bool parseJsonObject(QNetworkReply *reply, QJsonObject &outObject, QString &errorMessage)
{
    QJsonParseError parseError{};
    const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        errorMessage = parseError.errorString();
        return false;
    }
    outObject = doc.object();
    return true;
}

int extractApiCode(const QJsonObject &obj, int fallback)
{
    return obj.value(QStringLiteral("code")).toInt(fallback);
}

QString extractApiMessage(const QJsonObject &obj, const QString &fallback)
{
    const QString message = obj.value(QStringLiteral("message")).toString();
    return message.isEmpty() ? fallback : message;
}

} // namespace ErrorHandlingUtils
