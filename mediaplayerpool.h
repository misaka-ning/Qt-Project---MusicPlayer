#ifndef MEDIAPLAYERPOOL_H
#define MEDIAPLAYERPOOL_H

#include <QObject>
#include <QQueue>
#include <QList>
#include <QMediaPlayer>
#include <QUrl>
#include <QPixmap>

class MediaPlayerPool : public QObject
{
    Q_OBJECT
public:
    explicit MediaPlayerPool(int maxConcurrent = 4, QObject *parent = nullptr);
    ~MediaPlayerPool();

    void addTask(const QUrl &url, int taskId);
    void start();

signals:
    void taskFinished(int taskId, const QPixmap &cover, const QString &title, const QString &artist);
    void taskFailed(int taskId, const QString &error);

private:
    struct Task {
        QUrl url;
        int id;
    };
    struct Worker {
        QMediaPlayer *player;
        bool busy = false;
        Task currentTask;
    };

    void assignTask(Worker *worker);
    void releaseWorker(Worker *worker);

    QList<Worker*> m_workers;
    QQueue<Worker*> m_idleWorkers;
    QQueue<Task> m_pendingTasks;
    int m_maxConcurrent;
};

#endif // MEDIAPLAYERPOOL_H
