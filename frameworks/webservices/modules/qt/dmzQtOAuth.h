#ifndef DMZ_QT_OAUTH_DOT_H
#define DMZ_QT_OAUTH_DOT_H

#include <QtCore/QObject>
#include <QtCore/QUrl>

#define AUTH_HEADER "Authorization"
   
class QByteArray;

namespace dmz {
   
   class  QtOAuth : public QObject {
      
      Q_OBJECT
      Q_ENUMS(HttpMethod)
      Q_PROPERTY(QByteArray oauthToken READ oauthToken WRITE setOAuthToken)
      Q_PROPERTY(QByteArray oauthTokenSecret READ oauthTokenSecret WRITE setOAuthTokenSecret)

   public:
      QtOAuth(QObject *parent = 0);
      QtOAuth(const QByteArray& consumerKey, const QByteArray& consumerSecret, QObject *parent = 0);

      enum HttpMethod {GET, POST, PUT, DELETE};

      void parseTokens(const QByteArray& response);
      QByteArray generateAuthorizationHeader(const QUrl& url, HttpMethod method);
      void setOAuthToken(const QByteArray& token);
      void setOAuthTokenSecret(const QByteArray& tokenSecret);
      void clearTokens();
      QByteArray oauthToken() const;
      QByteArray oauthTokenSecret() const;

   private:
      QByteArray generateSignatureHMACSHA1(const QByteArray& signatureBase);
      QByteArray generateSignatureBase(const QUrl& url, HttpMethod method, const QByteArray& timestamp, const QByteArray& nonce);

      QByteArray m_oauthToken;
      QByteArray m_oauthTokenSecret;
      QByteArray m_oauthConsumerSecret;
      QByteArray m_oauthConsumerKey;
   };
};


#endif //DMZ_QT_OAUTH_DOT_H
