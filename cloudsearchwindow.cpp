#include "cloudsearchwindow.h"

#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QDialogButtonBox>

CloudSearchWindow::CloudSearchWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("搜索结果"));
    setModal(false);
    resize(520, 620);

    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(10);

    m_hint = new QLabel(QStringLiteral("双击或选择后点击“播放”"), this);
    root->addWidget(m_hint);

    m_list = new QListWidget(this);
    m_list->setSelectionMode(QAbstractItemView::SingleSelection);
    root->addWidget(m_list, 1);

    auto *bottom = new QHBoxLayout();
    m_playButton = new QPushButton(QStringLiteral("播放"), this);
    m_playButton->setDefault(true);
    auto *closeBtn = new QPushButton(QStringLiteral("关闭"), this);
    bottom->addStretch(1);
    bottom->addWidget(m_playButton);
    bottom->addWidget(closeBtn);
    root->addLayout(bottom);

    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    connect(m_playButton, &QPushButton::clicked, this, &CloudSearchWindow::activateCurrent);
    connect(m_list, &QListWidget::itemActivated, this, [this](QListWidgetItem *) { activateCurrent(); });
}

void CloudSearchWindow::setResults(const QVector<CloudMusicService::CloudSongBrief>& songs)
{
    m_songs = songs;
    if (m_list) m_list->clear();

    for (const auto& s : m_songs) {
        const QString title = s.name.trimmed().isEmpty() ? QStringLiteral("未知曲目") : s.name.trimmed();
        const QString artist = s.artist.trimmed().isEmpty() ? QStringLiteral("未知艺术家") : s.artist.trimmed();
        QStringList tags;
        // fee=1 常见为 VIP 歌曲；fee=4 常见为数字专辑/单曲购买
        if (s.fee == 1) tags << QStringLiteral("VIP");
        if (s.fee == 4) tags << QStringLiteral("付费");
        // st<0 常见为无版权/不可播，给出提示
        if (s.st < 0) tags << QStringLiteral("受限");

        const QString suffix = tags.isEmpty()
                                   ? QString()
                                   : QStringLiteral("  [%1]").arg(tags.join(QStringLiteral(" / ")));
        auto *it = new QListWidgetItem(QStringLiteral("%1 - %2%3").arg(artist, title, suffix));
        it->setData(Qt::UserRole, s.songId);
        if (!tags.isEmpty()) {
            // 受限曲目用暖色强调，便于快速识别。
            it->setForeground(QColor(255, 196, 120));
        }
        m_list->addItem(it);
    }

    if (!m_songs.isEmpty() && m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }

    if (m_hint) {
        m_hint->setText(QStringLiteral("结果：%1 条（双击或选择后点击“播放”）").arg(m_songs.size()));
    }
}

void CloudSearchWindow::activateCurrent()
{
    if (!m_list) return;
    QListWidgetItem *cur = m_list->currentItem();
    if (!cur) return;
    const QString songId = cur->data(Qt::UserRole).toString().trimmed();
    if (songId.isEmpty()) return;
    emit songActivated(songId);
}

