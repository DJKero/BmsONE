#ifndef APPINFO
#define APPINFO

#include <QtCore>

#define APP_NAME "BmsONE"
#define APP_VERSION_STRING "beta 0.2.1"
#define APP_URL "https://github.com/excln/BmsONE"
#define ORGANIZATION_NAME "ExclusionBms"

#ifdef Q_OS_LINUX
#define APP_PLATFORM_NAME "Linux"
#endif
#if Q_OS_WIN
#define APP_PLATFORM_NAME "Windows"
#endif
#if Q_OS_MACOS
#define APP_PLATFORM_NAME "macOS"
#endif

#define QT_URL "http://www.qt.io/"
#define OGG_VERSION_STRING "Xiph.Org libOgg 1.3.2"
#define OGG_URL "https://www.xiph.org/"
#define VORBIS_URL "https://www.xiph.org/"

#endif // APPINFO
