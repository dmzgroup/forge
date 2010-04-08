#ifndef DMZ_FORGE_WEB_SERVICE_QT_DOT_H
#define DMZ_FORGE_WEB_SERVICE_QT_DOT_H

#include <dmzRuntimeConfig.h>
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
         ForgeWebServiceQt (Config &local, Log *log = 0, QObject *parent = 0);
         virtual ~ForgeWebServiceQt ();

         void set_host (const String &Host);
         String get_host () const;
         
         void set_port (const Int32 Port);
         Int32 get_port () const;
      
      public Q_SLOTS:
         QNetworkReply *search (const String &Value, const UInt32 Limit = 0);
         
         QNetworkReply *get_uuids (const UInt32 Count = 0);
         
         QNetworkReply *get_asset (const String &AssetId);
         QNetworkReply *put_asset (const String &AssetId, const String &JsonData);
         QNetworkReply *delete_asset (const String &AssetId, const String &Revision);
         
         QNetworkReply *get_asset_media (const String &AssetId, const String &File);
         
         QNetworkReply *put_asset_media (
            const String &AssetId,
            const String &Revision,
            const String &File,
            const String &MimeType);
         
         QNetworkReply *get_asset_preview (const String &AssetId, const String &File);
         
         QNetworkReply *put_asset_preview (
            const String &AssetId,
            const String &Revision,
            const String &File);
         
      Q_SIGNALS:
         void finished (QNetworkReply *reply);

      protected Q_SLOTS:
         void _upload_progress (qint64 bytesSent, qint64 bytesTotal);
         
      protected:
         QNetworkReply *_get (const QUrl &Url, const String &RequestType);
         
         QNetworkReply *_put (
            const QUrl &Url,
            const String &RequestType,
            const String &Data);

         QNetworkReply *_put_file (
            const QUrl &Url,
            const String &RequestType,
            const String &File);
         
         QNetworkReply *_delete (const QUrl &Url, const String &RequestType);
            
         QNetworkReply *_get_attachment (
            const String &AssetId,
            const String &Attachemnt,
            const String &RequestType);
            
         void _init (Config &local);
         
      protected:
         Log *_log;
         QNetworkAccessManager *_nam;
         QUrl _baseUrl;
         QString _db;
         UInt64 _requestCounter;
   };
};


#endif // DMZ_FORGE_WEB_SERVICE_QT_DOT_H
