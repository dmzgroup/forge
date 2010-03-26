#ifndef DMZ_FORGE_QT_WEB_SERVICE_EXPORT_DOT_H
#define DMZ_FORGE_QT_WEB_SERVICE_EXPORT_DOT_H

#ifdef _WIN32
#   ifdef DMZ_FORGE_QT_WEB_SERVICE_EXPORT
#      define DMZ_FORGE_QT_WEB_SERVICE_LINK_SYMBOL __declspec (dllexport)
#   else
#      define DMZ_FORGE_QT_WEB_SERVICE_LINK_SYMBOL __declspec (dllimport)
#   endif
#else
#   define DMZ_FORGE_QT_WEB_SERVICE_LINK_SYMBOL
#endif

#endif // DMZ_FORGE_QT_WEB_SERVICE_EXPORT_DOT_H
