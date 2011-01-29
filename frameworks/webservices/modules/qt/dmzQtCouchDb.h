#ifndef DMZ_QT_COUCHDB_DOT_H
#define DMZ_QT_COUCHDB_DOT_H

#include <dmzRuntimeLog.h>
#include <dmzRuntimePluginInfo.h>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtNetwork/QAuthenticator>
#include <QtNetwork/QNetworkReply>


namespace dmz {

   class QtCouchDb: public QObject {
      
      Q_OBJECT
            
      public:
         QtCouchDb (
            const QUrl &Host,
            const QString &Database,
            const PluginInfo &Info,
            QObject *parent = 0);
            
         virtual ~QtCouchDb ();
         
         virtual UInt64 validate_user (const String &User, const String &Password);
         
         virtual UInt64 publish_document (const String &Id, const Config &Data);
         
         virtual UInt64 fetch_document (const String &Id);
         
         virtual UInt64 delete_document (const String &Id, const String &Rev);
         
         virtual UInt64 update_document (
            const String &Id,
            const String &Rev,
            const Config &Data);
         
      Q_SIGNALS:
         void busy ();
         void done ();
         
         void authentication_valid (const UInt64 RequestId, const Boolean Value);
         
         // void handle_error (
         //    const UInt64 RequestId,
         //    const Int32 StatusCode,
         //    const String &Message);
         
         void handle_document (
            const UInt64 RequestId,
            const String &Id,
            const String &Rev,
            const Config &Data);
            
         void handle_add_document (
            const UInt64 RequestId,
            const String &Id,
            const String &Rev);
            
         void handle_update_document (
            const UInt64 RequestId,
            const String &Id,
            const String &Rev);
            
         void handle_delete_document (
            const UInt64 RequestId,
            const String &Id,
            const String &Rev);
         
      public Q_SLOTS:

      protected Q_SLOTS:
         void _reply_finished (const UInt64 RequestId, QNetworkReply *reply);
      
      protected:
         
      protected:
         Log _log;
         QtHttpClient _client;
         QMap<UInt64, QString> _pending;
   };
};


#endif // DMZ_QT_COUCHDB_DOT_H
