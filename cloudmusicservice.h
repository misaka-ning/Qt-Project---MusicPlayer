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
    // 云端搜索结果中的轻量歌曲信息，用于列表展示与点播。
    struct CloudSongBrief {
        QString songId;
        QString name;
        QString artist;
        QUrl coverUrl;
        int fee = 0;
        int st = 0;
    };

    // 统一的请求错误类型，供 UI 做提示与分流处理。
    enum RequestError {
        NetworkError = 1,
        HttpError = 2,
        ParseError = 3,
        ApiError = 4
    };
    Q_ENUM(RequestError)

    explicit CloudMusicService(QObject *parent = nullptr);

    // 设置后端服务基地址（默认 http://127.0.0.1:8000）。
    void setBaseUrl(const QUrl& baseUrl);

public slots:
    // 关键词搜索歌曲，结果通过 searchFinished 返回。
    void searchSongs(const QString& keyword, int page, int pageSize, quint64 requestId);
    // 拉取歌曲可播放地址及基础信息。
    void fetchPlayableUrl(const QString& songId, quint64 requestId);
    // 获取歌词文本（兼容纯文本与 JSON 两种返回）。
    void fetchLyrics(const QString& songId, quint64 requestId);

    // 手机号+密码登录网易云。
    void loginNeteasePhone(const QString& phone, const QString& password, int countryCode, quint64 requestId);
    // 获取二维码登录 unikey。
    void requestNeteaseQrUnikey(quint64 requestId);
    // 根据 unikey 获取二维码 URL。
    void requestNeteaseQrUrl(const QString& unikey, quint64 requestId);
    // 轮询二维码登录状态。
    void checkNeteaseQrStatus(const QString& unikey, quint64 requestId);
    // 获取当前网易云登录状态。
    void fetchNeteaseAuthStatus(quint64 requestId);
    // 退出网易云登录。
    void logoutNetease(quint64 requestId);

signals:
    // 通用失败信号：带 requestId 以便调用方做请求级别匹配。
    void requestFailed(quint64 requestId, CloudMusicService::RequestError error, const QString& message);

    // 搜索完成，返回歌曲列表。
    void searchFinished(quint64 requestId, const QVector<CloudMusicService::CloudSongBrief>& songs);
    // 可播放地址就绪，同时附带用于 UI 展示的标题/作者/封面。
    void playUrlReady(quint64 requestId,
                      const QString& songId,
                      const QUrl& playUrl,
                      const QString& title,
                      const QString& artist,
                      const QUrl& coverUrl);
    // 歌词就绪。
    void lyricsReady(quint64 requestId, const QString& songId, const QString& lyrics);

    // 网易云账号密码登录结果。
    void neteaseLoginFinished(quint64 requestId, bool ok, const QString& message, const QString& nickname);
    // 二维码 unikey 获取结果。
    void neteaseQrUnikeyReady(quint64 requestId, bool ok, const QString& message, const QString& unikey);
    // 二维码 URL 获取结果。
    void neteaseQrUrlReady(quint64 requestId, bool ok, const QString& message, const QString& unikey, const QUrl& qrUrl);
    // 二维码状态结果（用于轮询驱动 UI 状态机）。
    void neteaseQrStatus(quint64 requestId,
                         bool ok,
                         const QString& message,
                         const QString& unikey,
                         const QString& status,
                         int statusCode,
                         bool loggedIn,
                         const QString& nickname);
    // 当前登录态查询结果。
    void neteaseAuthStatusReady(quint64 requestId, bool ok, const QString& message, bool loggedIn, const QString& nickname);
    // 退出登录结果。
    void neteaseLogoutFinished(quint64 requestId, bool ok, const QString& message);

private:
    // 基于 baseUrl 拼接接口路径，并清理旧 query。
    QUrl endpoint(const QString& path) const;
    // 统一处理网络/HTTP 层失败（当前保留，便于后续复用）。
    void handleHttpFailure(QNetworkReply *reply, quint64 requestId);

private:
    QUrl m_baseUrl;
    QNetworkAccessManager *m_nam = nullptr;
};

Q_DECLARE_METATYPE(CloudMusicService::CloudSongBrief)
