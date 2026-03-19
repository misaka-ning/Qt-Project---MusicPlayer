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
        QString url;        // file:///...
        bool hasMetadata{false};
        QString title;
        QString artist;
        QString coverPath;  // relative path, e.g. Metadata/<key>.png
    };

    PlaylistStore();

    QString playlistJsonAbsPath() const;
    QString metadataDirAbsPath() const;

    bool ensureMetadataDir() const;

    // Percent-encoded url string as stable filename-safe key.
    static QString makeKeyFromUrlString(const QString& urlString);
    static QString coverRelPathForKey(const QString& key);
    QString coverAbsPathForKey(const QString& key) const;

    bool load();                 // loads internal tracks list
    bool saveAtomic() const;     // writes internal tracks list to disk

    const QVector<Track>& tracks() const { return m_tracks; }

    // Insert if missing; returns index.
    int upsertTrack(const QString& urlString);

    // Mark metadata as loaded; caches cover (png) and updates track fields.
    bool markMetadata(const QString& urlString, const QPixmap& cover, const QString& title, const QString& artist);

    QPixmap loadCoverForTrack(const Track& t) const;

private:
    QString m_appDir;
    QVector<Track> m_tracks;

    int findIndexByKey(const QString& key) const;
    int findIndexByUrl(const QString& urlString) const;
};

#endif // PLAYLISTSTORE_H
