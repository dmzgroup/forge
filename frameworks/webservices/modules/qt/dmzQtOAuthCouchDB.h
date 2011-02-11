#ifndef DMZ_QT_OAUTH_COUCHDB_DOT_H
#define DMZ_QT_OAUTH_COUCHDB_DOT_H

#include "dmzQtOAuth.h"

class QNetworkAccessManager;

namespace dmz {
   
   class QtOAuthCouchDB : public QtOAuth {

      Q_OBJECT
      Q_PROPERTY(QNetworkAccessManager* networkAccessManager
                  READ networkAccessManager
                  WRITE setNetworkAccessManager)
                  
   public:
      QtOAuthCouchDB(QObject *parent = 0);
      QtOAuthCouchDB(QNetworkAccessManager* netManager, QObject *parent = 0);
      void setNetworkAccessManager(QNetworkAccessManager* netManager);
      QNetworkAccessManager* networkAccessManager() const;
      void authorizeXAuth(const QString& username, const QString& password);
      void authorizePin();

   signals:
       /** Emited when XAuth authorization is finished */
       void authorizeXAuthFinished();
       
       /** Emited when there is error in XAuth authorization */
       // ### TODO Error detection
       void authorizeXAuthError();

   protected:
       virtual int authorizationWidget();
       virtual void requestAuthorization();

   private slots:
       void finishedAuthorization();
       void requestAccessToken(int pin);

   private:
   	QNetworkAccessManager *m_netManager;
   };	
};


#endif // DMZ_QT_OAUTH_COUCHDB_DOT_H
