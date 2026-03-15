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

    // 对象池中的单个工作对象
    struct Worker : public QObject{
        Worker(QObject *parent) : QObject(parent) {}
        ~Worker() {}

        QMediaPlayer *player;
        bool busy = false;
        Task currentTask;
    };

    void assignTask(Worker *worker);
    void releaseWorker(Worker *worker);

    QList<Worker*> m_workers;           // 工作单元列表
    QQueue<Worker*> m_idleWorkers;      // 空闲工作单元列表
    QQueue<Task> m_pendingTasks;        // 待处理项目列表
    int m_maxConcurrent;
};

#endif // MEDIAPLAYERPOOL_H
