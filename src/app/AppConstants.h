#ifndef APPCONSTANTS_H
#define APPCONSTANTS_H

namespace AppConstants {
namespace Ui {
constexpr int MainWindowWidth = 1235;
constexpr int MainWindowHeight = 833;
constexpr int CoverSize = 300;
constexpr int MoreMenuButtonSize = 30;
constexpr int NeteaseQrImageSize = 320;
constexpr int NeteaseQrDialogWidth = 380;
constexpr int NeteaseQrDialogHeight = 420;
constexpr int SliderUnfreezeDelayMs = 150;
} // namespace Ui

namespace Playback {
constexpr int PlaybackStallProbeDelayMs = 200;
constexpr int PositionWakeupMs = 1;
constexpr int MetadataPoolMaxConcurrency = 4;
constexpr qreal DefaultVolume = 1.0;
constexpr int MetadataSaveDelayMs = 500;
} // namespace Playback

namespace Network {
constexpr int QrPollDelayMs = 1800;
} // namespace Network
} // namespace AppConstants

#endif // APPCONSTANTS_H
