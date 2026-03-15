#ifndef LRCPARSER_H
#define LRCPARSER_H

#include <QObject>
#include <QVector>

struct LrcLine {
    qint64 time;   // 毫秒
    QString text;
};

class LrcParser : public QObject
{
    Q_OBJECT
public:
    explicit LrcParser(QObject *parent = nullptr);
    bool parseFile(const QString &filePath);
    QVector<LrcLine> lyrics() const { return m_lyrics; }
    int currentIndex(qint64 position) const;

private:
    QVector<LrcLine> m_lyrics;
};

#endif // LRCPARSER_H
