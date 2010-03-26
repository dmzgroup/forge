#ifndef DMZ_FORGE_QT_WEB_SERVICE_DOT_H
#define DMZ_FORGE_QT_WEB_SERVICE_DOT_H

#include <dmzForgeQtWebServiceExport.h>
#include <dmzTypesString.h>
#include <dmzTypesStringContainer.h>
#include <QtCore/QObject>

class QNetworkReply;
class QNetworkRequest;
class QUrl;


namespace dmz {
   
   class Config;
   class Log;
   
   class DMZ_FORGE_QT_WEB_SERVICE_LINK_SYMBOL ForgeQtWebService : public QObject {
   
      Q_OBJECT
      
      public:
         ForgeQtWebService (QObject *parent = 0, Log *log = 0);
         virtual ~ForgeQtWebService ();

         void set_host (const String &Host);
         void set_port (const Int32 Port);
      
      public Q_SLOTS:
         void get_asset_list (const UInt32 Limit = 0);
         void get_asset (const UUID &Id);
      
      Q_SIGNALS:
         void asset_list_fetched (const StringContainer &Results);
         void asset_fetched (const Config &Results);
         void error (const String &Error);
      
      protected Q_SLOTS:
         void _get_asset_list_finished ();
         void _get_asset_finished ();
      
      protected:
         void _handle_asset_list (const Config &Response);
         
         QNetworkReply *_get (const QUrl &Url);
         QNetworkReply *_get (const QNetworkRequest &Request);
         
      protected:
         struct State;
         State &_state;
   };
};


#endif // DMZ_FORGE_QT_WEB_SERVICE_DOT_H