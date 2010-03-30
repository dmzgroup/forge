#ifndef DMZ_FORGE_WEB_SERVICE_QT_DOT_H
#define DMZ_FORGE_WEB_SERVICE_QT_DOT_H

#include <dmzRuntimeLog.h>
#include <dmzTypesString.h>
#include <QtCore/QObject>
#include <QtCore/QUrl>

class QNetworkAccessManager;
class QNetworkReply;
class QNetworkRequest;


namespace dmz {
   
   class ForgeWebServiceQt : public QObject {
   
      Q_OBJECT
      
      public:
         ForgeWebServiceQt (Log *log = 0, QObject *parent = 0);
         virtual ~ForgeWebServiceQt ();

         void set_host (const String &Host);
         void set_port (const Int32 Port);
      
      public Q_SLOTS:
         QNetworkReply *search (const String &Value, const UInt32 Limit = 0);
         
         QNetworkReply *get_uuids (const UInt32 Count = 0);
         
         QNetworkReply *get_asset (const String &AssetId);
         QNetworkReply *put_asset (const String &AssetId, const String &JsonData);
         QNetworkReply *delete_asset (const String &AssetId);
         
         QNetworkReply *get_asset_media (const String &AssetId, const String &File);
         
         QNetworkReply *put_asset_media (
            const String &AssetId,
            const String &File,
            const String &MimeType);
         
         QNetworkReply *get_asset_preview (const String &AssetId, const String &File);
         QNetworkReply *put_asset_preview (const String &AssetId, const String &File);
         
      Q_SIGNALS:
         void finished (QNetworkReply *reply);
         
      protected:
         QNetworkReply *_get (const QUrl &Url, const String &RequestType);
         
         QNetworkReply *_get_attachment (
            const String &AssetId,
            const String &Attachemnt,
            const String &RequestType);
         
      protected:
         Log *_log;
         QNetworkAccessManager *_nam;
         QUrl _baseUrl;
         UInt64 _requestCounter;
   };
};


#endif // DMZ_FORGE_WEB_SERVICE_QT_DOT_H
