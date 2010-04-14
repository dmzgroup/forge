#ifndef DMZ_FORGE_WEB_SERVICE_QT_DOT_H
#define DMZ_FORGE_WEB_SERVICE_QT_DOT_H

#include <dmzForgeConsts.h>
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
         
         static const QString ApiEndpoint;
         
         enum RoleEnum {
            RoleSearch = 101,
            RoleSetUuids,
            RoleGetAsset,
            RolePutAsset,
            RoleDeleteAsset,
            RolePutAssetMedia,
            RoleGetAssetMedia,
            RoleGetAssetPreview,
            RolePutAssetPreview
         };
         
         enum AttributeEnum {
            AttributeRole = QNetworkRequest::User,
            AttributeId
         };
         
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
         QNetworkReply *_get (const QUrl &Url, const RoleEnum &Role);
         
         QNetworkReply *_put (
            const QUrl &Url,
            const RoleEnum &Role,
            const String &Data);

         QNetworkReply *_put_file (
            const QUrl &Url,
            const RoleEnum &Role,
            const String &File);
         
         QNetworkReply *_delete (const QUrl &Url, const RoleEnum &Role);
         
         QNetworkReply *_get_attachment (
            const String &AssetId,
            const String &Attachemnt,
            const RoleEnum &Role);
            
         void _init (Config &local);
         
      protected:
         Log *_log;
         QNetworkAccessManager *_nam;
         QUrl _baseUrl;
         QString _db;
   };
};


#endif // DMZ_FORGE_WEB_SERVICE_QT_DOT_H
