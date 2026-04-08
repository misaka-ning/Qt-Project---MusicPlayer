#pragma once

#include <QObject>
#include <QUrl>
#include <QVector>

class QNetworkAccessManager;
class QNetworkReply;

class CloudMusicService : public QObject
{
    Q_OBJECT

public:
    struct CloudSongBrief {
        QString songId;
        QString name;
        QString artist;
        QUrl coverUrl;
        int fee = 0;
        int st = 0;
    };

    enum RequestError {
        NetworkError = 1,
        HttpError = 2,
        ParseError = 3,
        ApiError = 4
    };
    Q_ENUM(RequestError)

    explicit CloudMusicService(QObject *parent = nullptr);

    void setBaseUrl(const QUrl& baseUrl);

public slots:
    void searchSongs(const QString& keyword, int page, int pageSize, quint64 requestId);
    void fetchPlayableUrl(const QString& songId, quint64 requestId);
    void fetchLyrics(const QString& songId, quint64 requestId);

    void loginNeteasePhone(const QString& phone, const QString& password, int countryCode, quint64 requestId);
    void requestNeteaseQrUnikey(quint64 requestId);
    void requestNeteaseQrUrl(const QString& unikey, quint64 requestId);
    void checkNeteaseQrStatus(const QString& unikey, quint64 requestId);
    void fetchNeteaseAuthStatus(quint64 requestId);
    void logoutNetease(quint64 requestId);

signals:
    void requestFailed(quint64 requestId, CloudMusicService::RequestError error, const QString& message);

    void searchFinished(quint64 requestId, const QVector<CloudMusicService::CloudSongBrief>& songs);
    void playUrlReady(quint64 requestId,
                      const QString& songId,
                      const QUrl& playUrl,
                      const QString& title,
                      const QString& artist,
                      const QUrl& coverUrl);
    void lyricsReady(quint64 requestId, const QString& songId, const QString& lyrics);

    void neteaseLoginFinished(quint64 requestId, bool ok, const QString& message, const QString& nickname);
    void neteaseQrUnikeyReady(quint64 requestId, bool ok, const QString& message, const QString& unikey);
    void neteaseQrUrlReady(quint64 requestId, bool ok, const QString& message, const QString& unikey, const QUrl& qrUrl);
    void neteaseQrStatus(quint64 requestId,
                         bool ok,
                         const QString& message,
                         const QString& unikey,
                         const QString& status,
                         int statusCode,
                         bool loggedIn,
                         const QString& nickname);
    void neteaseAuthStatusReady(quint64 requestId, bool ok, const QString& message, bool loggedIn, const QString& nickname);
    void neteaseLogoutFinished(quint64 requestId, bool ok, const QString& message);

private:
    QUrl endpoint(const QString& path) const;
    void handleHttpFailure(QNetworkReply *reply, quint64 requestId);

private:
    QUrl m_baseUrl;
    QNetworkAccessManager *m_nam = nullptr;
};

Q_DECLARE_METATYPE(CloudMusicService::CloudSongBrief)
