#include "MediaPlayerPool.h"
#include <QMediaMetaData>
#include <QDebug>

MediaPlayerPool::MediaPlayerPool(int maxConcurrent, QObject *parent)
    : QObject(parent), m_maxConcurrent(maxConcurrent)
{
    for (int i = 0; i < m_maxConcurrent; ++i) {
        Worker *worker = new Worker;
        worker->player = new QMediaPlayer(this);
        worker->busy = false;
        m_workers.append(worker);
        m_idleWorkers.enqueue(worker);

        connect(worker->player, &QMediaPlayer::metaDataChanged, this,[this, worker]() {
            if (!worker->busy) return;
            const auto &meta = worker->player->metaData();

            // 读取标题和艺术家
            QString title = meta.value(QMediaMetaData::Title).toString();
            QString artist = meta.value(QMediaMetaData::Author).toString();

            // 读取封面图：先尝试缩略图，如果没有再尝试完整封面
            QPixmap cover;
            QVariant thumbVariant = meta.value(QMediaMetaData::ThumbnailImage);
            if (thumbVariant.isValid() && thumbVariant.canConvert<QImage>()) {
                cover = QPixmap::fromImage(thumbVariant.value<QImage>());
            } else {
                QVariant coverVariant = meta.value(QMediaMetaData::CoverArtImage);
                if (coverVariant.isValid() && coverVariant.canConvert<QImage>()) {
                    cover = QPixmap::fromImage(coverVariant.value<QImage>());
                }
            }

            emit taskFinished(worker->currentTask.id, cover, title, artist);
            releaseWorker(worker);
        });

        connect(worker->player, &QMediaPlayer::errorOccurred, this,
                [this, worker](QMediaPlayer::Error error, const QString &errorString) {
                    if (!worker->busy) return;
                    qDebug() << "加载元数据失败:" << worker->currentTask.url << errorString;
                    emit taskFailed(worker->currentTask.id, errorString);
                    releaseWorker(worker);
                });
    }
}

MediaPlayerPool::~MediaPlayerPool() {}

void MediaPlayerPool::addTask(const QUrl &url, int taskId)
{
    m_pendingTasks.enqueue({url, taskId});
}

void MediaPlayerPool::start()
{
    while (!m_idleWorkers.isEmpty() && !m_pendingTasks.isEmpty()) {
        Worker *worker = m_idleWorkers.dequeue();
        assignTask(worker);
    }
}

void MediaPlayerPool::assignTask(Worker *worker)
{
    if (m_pendingTasks.isEmpty()) return;
    Task task = m_pendingTasks.dequeue();
    worker->busy = true;
    worker->currentTask = task;
    worker->player->setSource(task.url);
}

void MediaPlayerPool::releaseWorker(Worker *worker)
{
    worker->busy = false;
    m_idleWorkers.enqueue(worker);
    // 尝试分配下一个任务
    if (!m_pendingTasks.isEmpty()) {
        Worker *nextWorker = m_idleWorkers.dequeue();
        assignTask(nextWorker);
    }
}
