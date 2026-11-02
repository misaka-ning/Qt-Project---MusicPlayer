#ifndef PLAYLISTSTORE_H
#define PLAYLISTSTORE_H

#include <QString>
#include <QStringList>
#include <QVector>
#include <QPixmap>

class PlaylistStore
{
public:
    struct Track {
        QString key;
        QString url;        // file:///... 或 http(s)...
        QString cloudSongId;
        bool isCloudTrack{false};
        bool hasMetadata{false};
        QString title;
        QString artist;
        QString coverPath;  // 相对路径，例如 Metadata/<key>.png
    };

    PlaylistStore();

    QString playlistJsonAbsPath() const;
    QString metadataDirAbsPath() const;

    bool ensureMetadataDir() const;

    // 将 URL 做百分号编码，生成稳定且文件名安全的 key。
    static QString makeKeyFromUrlString(const QString& urlString);
    static QString makeKeyForCloudSongId(const QString& songId);
    static QString coverRelPathForKey(const QString& key);
    QString coverAbsPathForKey(const QString& key) const;

    bool load();                 // 从磁盘加载内部曲目列表
    bool saveAtomic() const;     // 原子写入内部曲目列表到磁盘

    const QVector<Track>& tracks() const { return m_tracks; }

    // 若不存在则插入，返回曲目下标。
    int upsertTrack(const QString& urlString);

    // 标记元数据已加载；缓存封面（png）并更新曲目字段。
    bool markMetadata(const QString& urlString, const QPixmap& cover, const QString& title, const QString& artist);
    bool markMetadataByKey(const QString& key, const QPixmap& cover, const QString& title, const QString& artist);

    int upsertCloudTrack(const QString& songId, const QString& playUrlString);
    bool removeTrackByKey(const QString& key);

    QPixmap loadCoverForTrack(const Track& t) const;

private:
    QString m_appDir;
    QVector<Track> m_tracks;

    int findIndexByKey(const QString& key) const;
    int findIndexByUrl(const QString& urlString) const;
    int findIndexByCloudSongId(const QString& songId) const;
    bool applyMetadataToTrack(Track& track, const QPixmap& cover, const QString& title, const QString& artist);
};

#endif // PLAYLISTSTORE_H
