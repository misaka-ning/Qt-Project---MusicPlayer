#include "playliststore.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSaveFile>
#include <QUrl>

namespace {
constexpr int kPlaylistVersion = 1;

QString cloudSongIdFromLegacyKey(const QString& key)
{
    const QString prefix = QStringLiteral("cloud_");
    if (!key.startsWith(prefix)) return {};
    const QString encoded = key.mid(prefix.size());
    if (encoded.isEmpty()) return {};
    return QUrl::fromPercentEncoding(encoded.toLatin1());
}
} // namespace

PlaylistStore::PlaylistStore()
    : m_appDir(QCoreApplication::applicationDirPath())
{
}

QString PlaylistStore::playlistJsonAbsPath() const
{
    return QDir(m_appDir).filePath(QStringLiteral("playlist.json"));
}

QString PlaylistStore::metadataDirAbsPath() const
{
    return QDir(m_appDir).filePath(QStringLiteral("Metadata"));
}

bool PlaylistStore::ensureMetadataDir() const
{
    QDir d(metadataDirAbsPath());
    if (d.exists()) return true;
    return QDir(m_appDir).mkpath(QStringLiteral("Metadata"));
}

QString PlaylistStore::makeKeyFromUrlString(const QString& urlString)
{
    // 使用 URL 编码后的字符串作为稳定 key，避免路径特殊字符造成文件名问题。
    return QString::fromLatin1(QUrl::toPercentEncoding(urlString));
}

QString PlaylistStore::makeKeyForCloudSongId(const QString& songId)
{
    if (songId.isEmpty()) return {};
    return QStringLiteral("cloud_%1").arg(QString::fromLatin1(QUrl::toPercentEncoding(songId)));
}

QString PlaylistStore::coverRelPathForKey(const QString& key)
{
    return QStringLiteral("Metadata/%1.png").arg(key);
}

QString PlaylistStore::coverAbsPathForKey(const QString& key) const
{
    return QDir(m_appDir).filePath(coverRelPathForKey(key));
}

int PlaylistStore::findIndexByKey(const QString& key) const
{
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks[i].key == key) return i;
    }
    return -1;
}

int PlaylistStore::findIndexByUrl(const QString& urlString) const
{
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks[i].url == urlString) return i;
    }
    return -1;
}

int PlaylistStore::findIndexByCloudSongId(const QString& songId) const
{
    if (songId.isEmpty()) return -1;
    for (int i = 0; i < m_tracks.size(); ++i) {
        if (m_tracks[i].cloudSongId == songId) return i;
    }
    return -1;
}

bool PlaylistStore::load()
{
    m_tracks.clear();

    QFile f(playlistJsonAbsPath());
    if (!f.exists()) return true;
    if (!f.open(QIODevice::ReadOnly)) return false;

    const QByteArray data = f.readAll();
    f.close();

    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return false;

    QJsonObject root = doc.object();
    if (!root.contains(QStringLiteral("tracks")) || !root.value(QStringLiteral("tracks")).isArray()) {
        root.insert(QStringLiteral("tracks"), QJsonArray{});
    }
    if (!root.contains(QStringLiteral("version")) || !root.value(QStringLiteral("version")).isDouble()) {
        root.insert(QStringLiteral("version"), kPlaylistVersion);
    }

    // 兼容旧版本/异常数据：缺字段时使用默认值并尽量恢复可用条目。
    const QJsonArray tracks = root.value(QStringLiteral("tracks")).toArray();
    m_tracks.reserve(tracks.size());
    for (const QJsonValue& v : tracks) {
        if (!v.isObject()) continue;
        const QJsonObject o = v.toObject();
        Track t;
        t.key = o.value(QStringLiteral("key")).toString();
        t.url = o.value(QStringLiteral("url")).toString();
        t.isCloudTrack = o.value(QStringLiteral("isCloudTrack")).toBool(false);
        t.hasMetadata = o.value(QStringLiteral("hasMetadata")).toBool(false);
        t.title = o.value(QStringLiteral("title")).toString();
        t.artist = o.value(QStringLiteral("artist")).toString();
        t.coverPath = o.value(QStringLiteral("coverPath")).toString();
        t.cloudSongId = o.value(QStringLiteral("cloudSongId")).toString();
        if (t.cloudSongId.isEmpty()) {
            // 兼容旧数据：历史版本仅通过 key=cloud_<id> 区分云歌曲。
            t.cloudSongId = cloudSongIdFromLegacyKey(t.key);
        }

        if (t.url.isEmpty()) continue;
        if (!t.cloudSongId.isEmpty()) {
            t.key = makeKeyForCloudSongId(t.cloudSongId);
            t.isCloudTrack = true;
        } else if (t.key.isEmpty()) {
            t.key = makeKeyFromUrlString(t.url);
        }
        if (t.coverPath.isEmpty()) t.coverPath = coverRelPathForKey(t.key);

        m_tracks.push_back(std::move(t));
    }

    return true;
}

bool PlaylistStore::saveAtomic() const
{
    QJsonObject root;
    root.insert(QStringLiteral("version"), kPlaylistVersion);

    QJsonArray tracks;
    for (const auto& t : m_tracks) {
        QJsonObject o;
        o.insert(QStringLiteral("key"), t.key);
        o.insert(QStringLiteral("url"), t.url);
        o.insert(QStringLiteral("isCloudTrack"), t.isCloudTrack);
        o.insert(QStringLiteral("hasMetadata"), t.hasMetadata);
        o.insert(QStringLiteral("title"), t.title);
        o.insert(QStringLiteral("artist"), t.artist);
        o.insert(QStringLiteral("coverPath"), t.coverPath);
        if (!t.cloudSongId.isEmpty()) {
            o.insert(QStringLiteral("cloudSongId"), t.cloudSongId);
        }
        tracks.append(o);
    }
    root.insert(QStringLiteral("tracks"), tracks);

    // QSaveFile 通过临时文件 + commit 保证写入原子性，避免写坏 playlist.json。
    QSaveFile f(playlistJsonAbsPath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
    const QJsonDocument doc(root);
    f.write(doc.toJson(QJsonDocument::Indented));
    return f.commit();
}

int PlaylistStore::upsertTrack(const QString& urlString)
{
    if (urlString.isEmpty()) return -1;
    const QString key = makeKeyFromUrlString(urlString);

    int idx = findIndexByKey(key);
    if (idx >= 0) return idx;

    // key 不存在但 url 已存在时复用旧条目，避免重复插入同一歌曲。
    idx = findIndexByUrl(urlString);
    if (idx >= 0) {
        if (m_tracks[idx].key.isEmpty()) m_tracks[idx].key = key;
        if (m_tracks[idx].coverPath.isEmpty()) m_tracks[idx].coverPath = coverRelPathForKey(m_tracks[idx].key);
        return idx;
    }

    Track t;
    t.key = key;
    t.url = urlString;
    t.isCloudTrack = false;
    t.hasMetadata = false;
    t.coverPath = coverRelPathForKey(key);
    m_tracks.push_back(std::move(t));
    return m_tracks.size() - 1;
}

bool PlaylistStore::markMetadata(const QString& urlString, const QPixmap& cover, const QString& title, const QString& artist)
{
    const int idx = upsertTrack(urlString);
    if (idx < 0) return false;
    Track& t = m_tracks[idx];
    t.hasMetadata = true;
    if (t.key.isEmpty()) t.key = makeKeyFromUrlString(urlString);
    return applyMetadataToTrack(t, cover, title, artist);
}

bool PlaylistStore::markMetadataByKey(const QString& key, const QPixmap& cover, const QString& title, const QString& artist)
{
    const int idx = findIndexByKey(key);
    if (idx < 0) return false;
    return applyMetadataToTrack(m_tracks[idx], cover, title, artist);
}

bool PlaylistStore::applyMetadataToTrack(Track& track, const QPixmap& cover, const QString& title, const QString& artist)
{
    if (!ensureMetadataDir()) return false;

    track.hasMetadata = true;
    track.title = title;
    track.artist = artist;
    track.coverPath = coverRelPathForKey(track.key);

    // 无封面时回退到内置默认图，保证封面文件始终可加载。
    const QPixmap finalCover = cover.isNull() ? QPixmap(QStringLiteral(":/res/misaka.png")) : cover;
    return finalCover.save(coverAbsPathForKey(track.key), "PNG");
}

int PlaylistStore::upsertCloudTrack(const QString& songId, const QString& playUrlString)
{
    if (songId.isEmpty() || playUrlString.isEmpty()) return -1;
    const QString key = makeKeyForCloudSongId(songId);
    int idx = findIndexByCloudSongId(songId);
    if (idx >= 0) {
        Track &t = m_tracks[idx];
        t.url = playUrlString;
        t.key = key;
        t.cloudSongId = songId;
        t.isCloudTrack = true;
        if (t.coverPath.isEmpty()) t.coverPath = coverRelPathForKey(key);
        return idx;
    }

    Track t;
    t.key = key;
    t.url = playUrlString;
    t.cloudSongId = songId;
    t.isCloudTrack = true;
    t.hasMetadata = false;
    t.coverPath = coverRelPathForKey(key);
    m_tracks.push_back(std::move(t));
    return m_tracks.size() - 1;
}

bool PlaylistStore::removeTrackByKey(const QString& key)
{
    if (key.isEmpty()) return false;
    const int idx = findIndexByKey(key);
    if (idx < 0) return false;

    const QString coverAbs = coverAbsPathForKey(m_tracks[idx].key);
    m_tracks.removeAt(idx);
    if (QFile::exists(coverAbs)) {
        QFile::remove(coverAbs);
    }
    return true;
}

QPixmap PlaylistStore::loadCoverForTrack(const Track& t) const
{
    // 优先使用 json 中的显式 coverPath（相对应用目录）。
    if (!t.coverPath.isEmpty()) {
        const QString abs = QDir(m_appDir).filePath(t.coverPath);
        QPixmap pix;
        if (pix.load(abs)) return pix;
    }

    // 回退到 key 约定路径。
    if (!t.key.isEmpty()) {
        QPixmap pix;
        if (pix.load(coverAbsPathForKey(t.key))) return pix;
    }

    return {};
}

