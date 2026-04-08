#include "cloudmusicservice.h"
#include "utils/ErrorHandlingUtils.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QtGlobal>

CloudMusicService::CloudMusicService(QObject *parent)
    : QObject(parent)
    , m_baseUrl(QStringLiteral("http://127.0.0.1:8000"))
    , m_nam(new QNetworkAccessManager(this))
{
}

void CloudMusicService::setBaseUrl(const QUrl& baseUrl)
{
    m_baseUrl = baseUrl;
}

QUrl CloudMusicService::endpoint(const QString& path) const
{
    // 在 baseUrl 后安全追加相对路径，避免 query 串扰到下一次请求。
    QUrl u = m_baseUrl;
    QString p = u.path();
    if (!p.endsWith('/')) p += '/';
    p += path;
    u.setPath(p);
    u.setQuery(QString());
    return u;
}

void CloudMusicService::handleHttpFailure(QNetworkReply *reply, quint64 requestId)
{
    QString errorMessage;
    if (ErrorHandlingUtils::hasNetworkOrHttpError(reply, errorMessage)) {
        emit requestFailed(requestId, NetworkError, errorMessage);
    }
}

void CloudMusicService::searchSongs(const QString& keyword, int page, int pageSize, quint64 requestId)
{
    // 空关键词直接返回空结果，避免无效请求。
    if (keyword.trimmed().isEmpty()) {
        emit searchFinished(requestId, {});
        return;
    }

    QUrl url = endpoint(QStringLiteral("search"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("keyword"), keyword.trimmed());
    q.addQueryItem(QStringLiteral("page"), QString::number(qMax(1, page)));
    q.addQueryItem(QStringLiteral("size"), QString::number(qMax(1, pageSize)));
    url.setQuery(q);

    QNetworkReply *reply = m_nam->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId]() {
        reply->deleteLater();
        QString errorMessage;
        if (ErrorHandlingUtils::hasNetworkOrHttpError(reply, errorMessage)) {
            emit requestFailed(requestId, NetworkError, errorMessage);
            return;
        }

        QJsonObject obj;
        if (!ErrorHandlingUtils::parseJsonObject(reply, obj, errorMessage)) {
            emit requestFailed(requestId, ParseError, QStringLiteral("搜索结果解析失败"));
            return;
        }

        // 兼容不同后端返回结构：songs/data/result.songs。
        QJsonArray arr;
        if (obj.value(QStringLiteral("songs")).isArray()) {
            arr = obj.value(QStringLiteral("songs")).toArray();
        } else if (obj.value(QStringLiteral("data")).isArray()) {
            arr = obj.value(QStringLiteral("data")).toArray();
        } else if (obj.value(QStringLiteral("data")).isObject()) {
            arr = obj.value(QStringLiteral("data")).toObject().value(QStringLiteral("songs")).toArray();
        } else if (obj.value(QStringLiteral("result")).isObject()) {
            arr = obj.value(QStringLiteral("result")).toObject().value(QStringLiteral("songs")).toArray();
        }

        // 抽取列表展示所需字段，并做字段名兼容。
        QVector<CloudSongBrief> out;
        out.reserve(arr.size());
        for (const auto& v : arr) {
            if (!v.isObject()) continue;
            const QJsonObject s = v.toObject();
            CloudSongBrief songBrief;
            songBrief.songId = s.value(QStringLiteral("id")).toVariant().toString();
            if (songBrief.songId.isEmpty()) songBrief.songId = s.value(QStringLiteral("songId")).toVariant().toString();
            songBrief.name = s.value(QStringLiteral("name")).toString();
            if (songBrief.name.isEmpty()) songBrief.name = s.value(QStringLiteral("title")).toString();
            songBrief.artist = s.value(QStringLiteral("artist")).toString();
            if (songBrief.artist.isEmpty()) {
                const auto artistsValue = s.value(QStringLiteral("artists"));
                if (artistsValue.isString()) {
                    songBrief.artist = artistsValue.toString();
                } else if (artistsValue.isArray()) {
                    QStringList artistNames;
                    const QJsonArray artistsArray = artistsValue.toArray();
                    for (const auto &artistVal : artistsArray) {
                        if (!artistVal.isObject()) continue;
                        const QString name = artistVal.toObject().value(QStringLiteral("name")).toString();
                        if (!name.isEmpty()) artistNames.push_back(name);
                    }
                    songBrief.artist = artistNames.join('/');
                }
            }
            songBrief.coverUrl = QUrl(s.value(QStringLiteral("coverUrl")).toString());
            if (!songBrief.coverUrl.isValid() || songBrief.coverUrl.isEmpty()) {
                songBrief.coverUrl = QUrl(s.value(QStringLiteral("cover")).toString());
            }
            songBrief.fee = s.value(QStringLiteral("fee")).toInt(0);
            songBrief.st = s.value(QStringLiteral("st")).toInt(0);
            if (!songBrief.songId.isEmpty()) out.push_back(songBrief);
        }
        emit searchFinished(requestId, out);
    });
}

void CloudMusicService::fetchPlayableUrl(const QString& songId, quint64 requestId)
{
    if (songId.trimmed().isEmpty()) {
        emit requestFailed(requestId, ApiError, QStringLiteral("songId 为空"));
        return;
    }

    QUrl url = endpoint(QStringLiteral("song/url"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("id"), songId.trimmed());
    url.setQuery(q);

    QNetworkReply *reply = m_nam->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId, songId]() {
        reply->deleteLater();
        QString errorMessage;
        if (ErrorHandlingUtils::hasNetworkOrHttpError(reply, errorMessage)) {
            emit requestFailed(requestId, NetworkError, errorMessage);
            return;
        }

        QJsonObject obj;
        if (!ErrorHandlingUtils::parseJsonObject(reply, obj, errorMessage)) {
            emit requestFailed(requestId, ParseError, QStringLiteral("播放地址解析失败"));
            return;
        }
        const int apiCode = ErrorHandlingUtils::extractApiCode(obj);
        if (apiCode != 0 && apiCode != 200) {
            emit requestFailed(requestId, ApiError, ErrorHandlingUtils::extractApiMessage(obj, QStringLiteral("云端返回错误")));
            return;
        }

        // 部分接口直接返回对象，部分包裹在 data 字段下。
        const QJsonObject data = obj.value(QStringLiteral("data")).isObject()
                                     ? obj.value(QStringLiteral("data")).toObject()
                                     : obj;

        QUrl playUrl(data.value(QStringLiteral("playUrl")).toString());
        if (!playUrl.isValid() || playUrl.isEmpty()) {
            playUrl = QUrl(data.value(QStringLiteral("url")).toString());
        }
        const QUrl originUrl(data.value(QStringLiteral("originUrl")).toString());
        // /song/stream 在部分环境会被上游 403，优先尝试后端返回的直连地址。
        if (originUrl.isValid() && !originUrl.isEmpty()) {
            const bool isLocalProxy = (playUrl.host() == QStringLiteral("127.0.0.1")
                                       || playUrl.host() == QStringLiteral("localhost"))
                                      && playUrl.path().contains(QStringLiteral("/song/stream"));
            if (isLocalProxy || !playUrl.isValid() || playUrl.isEmpty()) {
                playUrl = originUrl;
            }
        }
        if (!playUrl.isValid() || playUrl.isEmpty()) {
            emit requestFailed(requestId, ApiError, QStringLiteral("返回中缺少可播放 URL"));
            return;
        }

        // 标题字段兼容 title/name。
        const QString title = data.value(QStringLiteral("title")).toString().isEmpty()
                                  ? data.value(QStringLiteral("name")).toString()
                                  : data.value(QStringLiteral("title")).toString();
        const QString artist = data.value(QStringLiteral("artist")).toString();
        QUrl coverUrl(data.value(QStringLiteral("coverUrl")).toString());
        if (!coverUrl.isValid() || coverUrl.isEmpty()) {
            coverUrl = QUrl(data.value(QStringLiteral("cover")).toString());
        }
        emit playUrlReady(requestId, songId, playUrl, title, artist, coverUrl);
    });
}

void CloudMusicService::fetchLyrics(const QString& songId, quint64 requestId)
{
    if (songId.trimmed().isEmpty()) {
        emit requestFailed(requestId, ApiError, QStringLiteral("songId 为空"));
        return;
    }

    QUrl url = endpoint(QStringLiteral("lyrics"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("songId"), songId.trimmed());
    url.setQuery(q);

    QNetworkReply *reply = m_nam->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId, songId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(requestId, NetworkError, reply->errorString());
            return;
        }

        QJsonParseError parseErr{};
        const QByteArray data = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
        // 优先按 JSON 结构提取 lyrics；失败则回退为纯文本歌词。
        if (parseErr.error == QJsonParseError::NoError && doc.isObject()) {
            const QJsonObject obj = doc.object();
            const QString lrc = obj.value(QStringLiteral("lyrics")).toString();
            if (!lrc.isNull()) {
                emit lyricsReady(requestId, songId, lrc);
                return;
            }
            if (obj.value(QStringLiteral("data")).isObject()) {
                const QString l2 = obj.value(QStringLiteral("data")).toObject().value(QStringLiteral("lyrics")).toString();
                emit lyricsReady(requestId, songId, l2);
                return;
            }
        }

        emit lyricsReady(requestId, songId, QString::fromUtf8(data));
    });
}

void CloudMusicService::loginNeteasePhone(const QString& phone, const QString& password, int countryCode,
                                          quint64 requestId)
{
    if (phone.trimmed().isEmpty()) {
        emit neteaseLoginFinished(requestId, false, QStringLiteral("手机号为空"), QString());
        return;
    }
    if (password.isEmpty()) {
        emit neteaseLoginFinished(requestId, false, QStringLiteral("密码为空"), QString());
        return;
    }

    QUrl url = endpoint(QStringLiteral("auth/login"));
    QJsonObject root;
    root.insert(QStringLiteral("phone"), phone.trimmed());
    root.insert(QStringLiteral("password"), password);
    root.insert(QStringLiteral("countrycode"), qMax(1, countryCode));

    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QNetworkReply *reply = m_nam->post(req, QJsonDocument(root).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId]() {
        reply->deleteLater();
        QString errorMessage;
        if (ErrorHandlingUtils::hasNetworkOrHttpError(reply, errorMessage)) {
            emit neteaseLoginFinished(requestId, false, errorMessage, QString());
            return;
        }

        QJsonObject obj;
        if (!ErrorHandlingUtils::parseJsonObject(reply, obj, errorMessage)) {
            emit neteaseLoginFinished(requestId, false, QStringLiteral("登录响应解析失败"), QString());
            return;
        }
        const int apiCode = ErrorHandlingUtils::extractApiCode(obj);
        QString nickname;
        const QJsonValue dataVal = obj.value(QStringLiteral("data"));
        if (dataVal.isObject()) {
            nickname = dataVal.toObject().value(QStringLiteral("nickname")).toString();
        }
        // 约定 code=0 表示登录成功。
        if (apiCode == 0) {
            emit neteaseLoginFinished(requestId, true, QString(), nickname);
        } else {
            emit neteaseLoginFinished(requestId, false, ErrorHandlingUtils::extractApiMessage(obj, QStringLiteral("登录失败")), QString());
        }
    });
}

void CloudMusicService::requestNeteaseQrUnikey(quint64 requestId)
{
    QUrl url = endpoint(QStringLiteral("auth/qr/unikey"));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *reply = m_nam->post(req, QByteArrayLiteral("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit neteaseQrUnikeyReady(requestId, false, reply->errorString(), QString());
            return;
        }

        QJsonParseError parseErr{};
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            emit neteaseQrUnikeyReady(requestId, false, QStringLiteral("unikey 响应解析失败"), QString());
            return;
        }
        const QJsonObject obj = doc.object();
        const int apiCode = obj.value(QStringLiteral("code")).toInt(-1);
        const QString msg = obj.value(QStringLiteral("message")).toString();
        QString unikey;
        if (obj.value(QStringLiteral("data")).isObject()) {
            unikey = obj.value(QStringLiteral("data")).toObject().value(QStringLiteral("unikey")).toString();
        }
        if (apiCode == 0 && !unikey.isEmpty()) {
            emit neteaseQrUnikeyReady(requestId, true, QString(), unikey);
        } else {
            emit neteaseQrUnikeyReady(requestId, false, msg.isEmpty() ? QStringLiteral("获取 unikey 失败") : msg, QString());
        }
    });
}

void CloudMusicService::requestNeteaseQrUrl(const QString& unikey, quint64 requestId)
{
    if (unikey.trimmed().isEmpty()) {
        emit neteaseQrUrlReady(requestId, false, QStringLiteral("unikey 为空"), QString(), QUrl());
        return;
    }
    QUrl url = endpoint(QStringLiteral("auth/qr/url"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("unikey"), unikey.trimmed());
    url.setQuery(q);

    QNetworkReply *reply = m_nam->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId, unikey]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit neteaseQrUrlReady(requestId, false, reply->errorString(), unikey, QUrl());
            return;
        }
        QJsonParseError parseErr{};
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            emit neteaseQrUrlReady(requestId, false, QStringLiteral("二维码 URL 响应解析失败"), unikey, QUrl());
            return;
        }
        const QJsonObject obj = doc.object();
        const int apiCode = obj.value(QStringLiteral("code")).toInt(-1);
        const QString msg = obj.value(QStringLiteral("message")).toString();
        QString keyOut = unikey;
        QUrl qrUrl;
        if (obj.value(QStringLiteral("data")).isObject()) {
            const QJsonObject data = obj.value(QStringLiteral("data")).toObject();
            keyOut = data.value(QStringLiteral("unikey")).toString(keyOut);
            qrUrl = QUrl(data.value(QStringLiteral("qrUrl")).toString());
        }
        if (apiCode == 0 && qrUrl.isValid() && !qrUrl.isEmpty()) {
            emit neteaseQrUrlReady(requestId, true, QString(), keyOut, qrUrl);
        } else {
            emit neteaseQrUrlReady(requestId, false, msg.isEmpty() ? QStringLiteral("获取二维码 URL 失败") : msg, keyOut, QUrl());
        }
    });
}

void CloudMusicService::checkNeteaseQrStatus(const QString& unikey, quint64 requestId)
{
    if (unikey.trimmed().isEmpty()) {
        emit neteaseQrStatus(requestId, false, QStringLiteral("unikey 为空"), QString(), QString(), -1, false, QString());
        return;
    }
    QUrl url = endpoint(QStringLiteral("auth/qr/check"));
    QUrlQuery q;
    q.addQueryItem(QStringLiteral("unikey"), unikey.trimmed());
    url.setQuery(q);

    QNetworkReply *reply = m_nam->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId, unikey]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit neteaseQrStatus(requestId, false, reply->errorString(), unikey, QString(), -1, false, QString());
            return;
        }
        QJsonParseError parseErr{};
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            emit neteaseQrStatus(requestId, false, QStringLiteral("二维码状态响应解析失败"), unikey, QString(), -1, false, QString());
            return;
        }
        const QJsonObject obj = doc.object();
        const int apiCode = obj.value(QStringLiteral("code")).toInt(-1);
        const QString msg = obj.value(QStringLiteral("message")).toString();
        if (apiCode != 0 || !obj.value(QStringLiteral("data")).isObject()) {
            emit neteaseQrStatus(requestId, false, msg.isEmpty() ? QStringLiteral("二维码状态查询失败") : msg,
                                 unikey, QString(), -1, false, QString());
            return;
        }
        const QJsonObject data = obj.value(QStringLiteral("data")).toObject();
        const QString keyOut = data.value(QStringLiteral("unikey")).toString(unikey);
        const QString status = data.value(QStringLiteral("status")).toString();
        const int statusCode = data.value(QStringLiteral("statusCode")).toInt(-1);
        const bool loggedIn = data.value(QStringLiteral("loggedIn")).toBool(false);
        const QString nickname = data.value(QStringLiteral("nickname")).toString();
        const QString statusMessage = data.value(QStringLiteral("statusMessage")).toString();

        // 后端查询成功，具体扫码状态由 status/statusCode 继续区分。
        emit neteaseQrStatus(requestId, true,
                             statusMessage.isEmpty() ? QStringLiteral("ok") : statusMessage,
                             keyOut, status, statusCode, loggedIn, nickname);
    });
}

void CloudMusicService::fetchNeteaseAuthStatus(quint64 requestId)
{
    QUrl url = endpoint(QStringLiteral("auth/status"));
    QNetworkReply *reply = m_nam->get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit neteaseAuthStatusReady(requestId, false, reply->errorString(), false, QString());
            return;
        }

        QJsonParseError parseErr{};
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            emit neteaseAuthStatusReady(requestId, false, QStringLiteral("登录状态响应解析失败"), false, QString());
            return;
        }
        const QJsonObject obj = doc.object();
        const int apiCode = obj.value(QStringLiteral("code")).toInt(-1);
        const QString msg = obj.value(QStringLiteral("message")).toString();
        if (apiCode != 0 || !obj.value(QStringLiteral("data")).isObject()) {
            emit neteaseAuthStatusReady(requestId, false, msg.isEmpty() ? QStringLiteral("获取登录状态失败") : msg, false, QString());
            return;
        }

        const QJsonObject data = obj.value(QStringLiteral("data")).toObject();
        const bool loggedIn = data.value(QStringLiteral("loggedIn")).toBool(false);
        const QString nickname = data.value(QStringLiteral("nickname")).toString();
        emit neteaseAuthStatusReady(requestId, true, QString(), loggedIn, nickname);
    });
}

void CloudMusicService::logoutNetease(quint64 requestId)
{
    QUrl url = endpoint(QStringLiteral("auth/logout"));
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *reply = m_nam->post(req, QByteArrayLiteral("{}"));
    connect(reply, &QNetworkReply::finished, this, [this, reply, requestId]() {
        reply->deleteLater();
        if (reply->error() != QNetworkReply::NoError) {
            emit neteaseLogoutFinished(requestId, false, reply->errorString());
            return;
        }
        QJsonParseError parseErr{};
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &parseErr);
        if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
            emit neteaseLogoutFinished(requestId, false, QStringLiteral("退出登录响应解析失败"));
            return;
        }
        const QJsonObject obj = doc.object();
        const int apiCode = obj.value(QStringLiteral("code")).toInt(-1);
        const QString msg = obj.value(QStringLiteral("message")).toString();
        emit neteaseLogoutFinished(requestId, apiCode == 0, apiCode == 0 ? QString() : (msg.isEmpty() ? QStringLiteral("退出登录失败") : msg));
    });
}
