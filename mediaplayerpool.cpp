#include "MediaPlayerPool.h"
#include <QMediaMetaData>
#include <QDebug>

/*
 *  采用并发式的对象池架构进程会崩溃，有bug不能解决
 *  但现在一次只从对象池中取出一个工作单元加载音乐程序就能跑了
 *  既然能跑，暂时就这样写了，以后可以升级成使用taglib库的方法
 */

MediaPlayerPool::MediaPlayerPool(int maxConcurrent, QObject *parent)
    : QObject(parent), m_maxConcurrent(maxConcurrent)
{
    for (int i = 0; i < m_maxConcurrent; ++i) {
        Worker *worker = new Worker(this);
        worker->player = new QMediaPlayer(worker);
        worker->busy = false;
        m_workers.append(worker);
        m_idleWorkers.enqueue(worker);

        // 当这个工作单元完成读取，通过发射taskFinished信号更新musicplaylist的显示
        connect(worker->player, &QMediaPlayer::metaDataChanged, this,[this, worker]() {
            if (!worker->busy) return;
            int taskId = worker->currentTask.id;
            const auto &meta = worker->player->metaData();

            // 读取标题和艺术家
            QString title = meta.value(QMediaMetaData::Title).toString();
            if (title.isEmpty()) title = "未知曲目";

            QString artist = meta.value(QMediaMetaData::ContributingArtist).toString();
            if (artist.isEmpty()) artist = meta.value(QMediaMetaData::AlbumArtist).toString();
            if (artist.isEmpty()) artist = "未知艺术家";

            QPixmap cover;
            QVariant thumb = meta.value(QMediaMetaData::ThumbnailImage);
            if (thumb.isValid() && thumb.canConvert<QImage>()) {
                cover = QPixmap::fromImage(thumb.value<QImage>());
            } else {
                QVariant coverArt = meta.value(QMediaMetaData::CoverArtImage);
                if (coverArt.isValid() && coverArt.canConvert<QImage>()) {
                    cover = QPixmap::fromImage(coverArt.value<QImage>());
                }
            }

            emit taskFinished(taskId, cover, title, artist);
            releaseWorker(worker);
            start();
        });

        connect(worker->player, &QMediaPlayer::errorOccurred, this,
                [this, worker](QMediaPlayer::Error error, const QString &errorString) {
                    if (!worker->busy) return;
                    int taskId = worker->currentTask.id;
                    emit taskFailed(taskId, errorString);
                    releaseWorker(worker);
                    start();
                });
    }
}

MediaPlayerPool::~MediaPlayerPool() {}

// 添加一个任务
void MediaPlayerPool::addTask(const QUrl &url, int taskId)
{
    m_pendingTasks.enqueue({url, taskId});
    // start(); // 立即尝试分配任务
}

// 开始一轮待处理任务的处理
void MediaPlayerPool::start()
{
    if(!m_idleWorkers.isEmpty() && !m_pendingTasks.isEmpty())
    {
        Worker *worker = m_idleWorkers.dequeue();
        assignTask(worker);
    }
    // while (!m_idleWorkers.isEmpty() && !m_pendingTasks.isEmpty()) {
    //     Worker *worker = m_idleWorkers.dequeue();
    //     assignTask(worker);
    // }
}

// 分配任务
void MediaPlayerPool::assignTask(Worker *worker)
{
    if (m_pendingTasks.isEmpty()) return;
    Task task = m_pendingTasks.dequeue();
    worker->busy = true;
    worker->currentTask = task;
    worker->player->setSource(task.url);
}

// 回收工作单元
void MediaPlayerPool::releaseWorker(Worker *worker)
{
    worker->busy = false;
    m_idleWorkers.enqueue(worker);
}
