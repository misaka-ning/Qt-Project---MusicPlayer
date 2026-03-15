#include "LrcParser.h"
#include <QFile>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QMap>
#include <QTextStream>

LrcParser::LrcParser(QObject *parent) : QObject(parent)
{
}

bool LrcParser::parseFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    QByteArray data = file.readAll();
    file.close();

    // 尝试多种编码读取歌词文件
    QString content;
    const char* encodings[] = {"UTF-8", "GBK", "GB2312", "GB18030", "UTF-16LE", "UTF-16BE"};
    for (const char* enc : encodings) {
        QStringDecoder decoder(enc);
        if (decoder.isValid()) {
            QString decoded = decoder.decode(data);
            if (!decoder.hasError()) {
                content = decoded;
                break;
            }
        }
    }
    if (content.isEmpty()) {
        // 回退到系统本地编码
        QTextStream stream(&data);
        content = stream.readAll();
    }

    // 正则匹配时间戳，支持 [mm:ss.xx] 或 [mm:ss.xxx] 格式
    QRegularExpression re(R"(\[(\d{2}):(\d{2})(?:\.(\d{2,3}))?\](.*))");
    QStringList lines = content.split('\n', Qt::SkipEmptyParts);

    // 使用 QMap 收集同一时间戳的所有歌词行（QMap 会自动按键排序）
    QMap<qint64, QStringList> linesMap;

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();
        QRegularExpressionMatch match = re.match(trimmed);
        if (!match.hasMatch())
            continue;

        int minute = match.captured(1).toInt();
        int second = match.captured(2).toInt();
        QString msStr = match.captured(3);
        int millisecond = 0;
        if (!msStr.isEmpty()) {
            if (msStr.length() == 2)
                millisecond = msStr.toInt() * 10;   // 两位毫秒 -> 乘以10得到毫秒
            else if (msStr.length() == 3)
                millisecond = msStr.toInt();        // 三位毫秒直接使用
        }
        qint64 time = minute * 60000 + second * 1000 + millisecond;
        QString text = match.captured(4).trimmed();

        if (!text.isEmpty())
            linesMap[time].append(text);
    }

    // 合并同一时间戳的多行，用换行符连接
    m_lyrics.clear();
    for (auto it = linesMap.begin(); it != linesMap.end(); ++it) {
        QString combined = it.value().join("\n");
        m_lyrics.append({it.key(), combined});
    }

    return !m_lyrics.isEmpty();
}

int LrcParser::currentIndex(qint64 position) const
{
    if (m_lyrics.isEmpty())
        return -1;

    // 二分查找最后一个时间 <= position 的索引
    int left = 0, right = m_lyrics.size() - 1;
    int result = -1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (m_lyrics[mid].time <= position) {
            result = mid;
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return result;
}
