#include "LrcParser.h"
#include <QFile>
#include <QRegularExpression>
#include <QStringDecoder>
#include <QMap>
#include <QTextStream>

LrcParser::LrcParser(QObject *parent) : QObject(parent)
{
}

/**
 * @brief 解析LRC歌词文件
 * @param filePath 歌词文件路径
 * @return 解析成功返回true，否则返回false
 *
 * 该函数读取指定路径的文件，尝试多种编码解码歌词内容，
 * 然后使用正则表达式提取时间戳和歌词文本，按时间排序后
 * 存储到成员变量m_lyrics中。同一时间戳的多行歌词会合并。
 */
bool LrcParser::parseFile(const QString &filePath)
{
    // 清空旧的歌词数据
    m_lyrics.clear();

    // 打开文件（只读模式）
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
        return false;                       // 打开失败直接返回

    // 读取所有文件数据到字节数组
    QByteArray data = file.readAll();
    file.close();

    // ----- 多编码解码尝试 -----
    // 定义常见编码列表，按优先级顺序尝试
    const char* encodings[] = {"UTF-8", "GBK", "GB2312", "GB18030", "UTF-16LE", "UTF-16BE"};
    QString content;
    for (const char* enc : encodings) {
        QStringDecoder decoder(enc);        // 创建对应编码的解码器
        if (decoder.isValid()) {
            QString decoded = decoder.decode(data);
            if (!decoder.hasError()) {      // 解码成功且无错误
                content = decoded;
                break;                       // 停止尝试后续编码
            }
        }
    }
    // 如果所有指定编码都失败，回退到系统本地编码（通过QTextStream）
    if (content.isEmpty()) {
        QTextStream stream(&data);
        content = stream.readAll();
    }

    // ----- 正则匹配时间戳和歌词文本 -----
    // 支持格式：[mm:ss.xx] 或 [mm:ss.xxx] （毫秒部分2位或3位）
    QRegularExpression re(R"(\[(\d{2}):(\d{2})(?:\.(\d{2,3}))?\](.*))");
    QStringList lines = content.split('\n', Qt::SkipEmptyParts);  // 按行分割

    // 使用QMap按时间戳自动排序，键为毫秒时间，值为该时间对应的多行歌词列表
    QMap<qint64, QStringList> linesMap;

    for (const QString &line : lines) {
        QString trimmed = line.trimmed();                         // 去除首尾空白
        QRegularExpressionMatch match = re.match(trimmed);
        if (!match.hasMatch())
            continue;                                              // 非歌词行则跳过

        // 解析分钟、秒、毫秒
        int minute = match.captured(1).toInt();
        int second = match.captured(2).toInt();
        QString msStr = match.captured(3);
        int millisecond = 0;
        if (!msStr.isEmpty()) {
            if (msStr.length() == 2)
                millisecond = msStr.toInt() * 10;   // 两位毫秒（如.12）-> 120毫秒
            else if (msStr.length() == 3)
                millisecond = msStr.toInt();        // 三位毫秒直接使用
        }
        // 计算总毫秒时间
        qint64 time = minute * 60000 + second * 1000 + millisecond;
        QString text = match.captured(4).trimmed();   // 获取歌词文本并去除空白

        if (!text.isEmpty())
            linesMap[time].append(text);               // 同一时间戳追加歌词
    }

    // ----- 合并同一时间戳的歌词，存入最终列表 -----
    m_lyrics.clear();
    for (auto it = linesMap.begin(); it != linesMap.end(); ++it) {
        QString combined = it.value().join("\n");      // 多行用换行符连接
        m_lyrics.append({it.key(), combined});         // 存储为(LyricLine)结构
    }

    // 解析成功条件：至少有一条有效歌词
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
