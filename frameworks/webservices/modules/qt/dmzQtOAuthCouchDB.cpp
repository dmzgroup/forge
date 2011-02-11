#include "dmzQtOAuthCouchDB.h"
#include <QtCore/QDebug>
#include <QtCore/QUrl>
#include <QtNetwork/QNetworkReply>
#include <QtCore/QTimer>
#include <QtNetwork/QNetworkAccessManager>
#include <QtCore/QEventLoop>
#include <QtGui/QDesktopServices>

#define COUCHDB_REQUEST_TOKEN_URL "http://localhost:5984/_oauth/request_token"
#define COUCHDB_ACCESS_TOKEN_URL "http://localhost:5984/_oauth/access_token"
#define COUCHDB_AUTHORIZE_URL "http://localhost:5984/_oauth/authorize"
#define COUCHDB_ACCESS_TOKEN_XAUTH_URL "http://localhost:5984/_oauth/access_token"


dmz::QtOAuthCouchDB::QtOAuthCouchDB(QObject *parent) : QtOAuth(parent), m_netManager(0) {
       
}


dmz::QtOAuthCouchDB::QtOAuthCouchDB(QNetworkAccessManager *netManager, QObject *parent) :
    QtOAuth(parent), m_netManager(netManager) {
       
}


void
dmz::QtOAuthCouchDB::setNetworkAccessManager(QNetworkAccessManager* netManager) {
   
	m_netManager = netManager;
}


QNetworkAccessManager *
dmz::QtOAuthCouchDB::networkAccessManager () const {
   
	return m_netManager;
}


void
dmz::QtOAuthCouchDB::authorizeXAuth(const QString &username, const QString &password) {
   
    Q_ASSERT(m_netManager != 0);

    QUrl url(COUCHDB_ACCESS_TOKEN_XAUTH_URL);
    url.addQueryItem("x_auth_username", username);
    url.addQueryItem("x_auth_password", password);
    url.addQueryItem("x_auth_mode", "client_auth");

    QByteArray oauthHeader = generateAuthorizationHeader(url, QtOAuth::POST);

    QNetworkRequest req(url);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    QNetworkReply *reply = m_netManager->post(req, QByteArray());
    connect(reply, SIGNAL(finished()), this, SLOT(finishedAuthorization()));
}


void
dmz::QtOAuthCouchDB::finishedAuthorization() {
   
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray response = reply->readAll();
            parseTokens(response);

            emit authorizeXAuthFinished();
        } else {
            //dump error
            qDebug() << "Network Error: " << reply->error();
            qDebug() << "Response error: " << reply->readAll();
            emit authorizeXAuthError();

        }
        reply->deleteLater();
    }
}


void
dmz::QtOAuthCouchDB::authorizePin () {
   
    Q_ASSERT(m_netManager != 0);

    QUrl url(COUCHDB_REQUEST_TOKEN_URL);

    QByteArray oauthHeader = generateAuthorizationHeader(url, QtOAuth::POST);

    QNetworkRequest req(url);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    //enters event loop
    QEventLoop q;
    QTimer t;
    t.setSingleShot(true);
    connect(&t, SIGNAL(timeout()), &q, SLOT(quit()));

    QNetworkReply *reply = m_netManager->post(req, QByteArray());
    connect(reply, SIGNAL(finished()), &q, SLOT(quit()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error()));

    t.start(5000);
    q.exec();

    if (t.isActive()) {
        t.stop();
        QByteArray response = reply->readAll();
        parseTokens(response);

        reply->deleteLater();
        requestAuthorization();

        int pin = authorizationWidget();
        if (pin) {
            requestAccessToken(pin);
        }
    } else {
        qDebug() << "Timeout";
    }
}

void
dmz::QtOAuthCouchDB::requestAuthorization() {
   
    QUrl authorizeUrl(COUCHDB_AUTHORIZE_URL);
    authorizeUrl.addEncodedQueryItem("oauth_token", oauthToken());

    QDesktopServices::openUrl(authorizeUrl);
}


void
dmz::QtOAuthCouchDB::requestAccessToken(int pin) {
   
    Q_ASSERT(m_netManager != 0);

    QUrl url(COUCHDB_ACCESS_TOKEN_URL);
    url.addEncodedQueryItem("oauth_verifier", QByteArray::number(pin));

    QByteArray oauthHeader = generateAuthorizationHeader(url, QtOAuth::POST);

    QEventLoop q;
    QTimer t;
    t.setSingleShot(true);

    connect(&t, SIGNAL(timeout()), &q, SLOT(quit()));

    QNetworkRequest req(url);
    req.setRawHeader(AUTH_HEADER, oauthHeader);

    QNetworkReply *reply = m_netManager->post(req, QByteArray());
    connect(reply, SIGNAL(finished()), &q, SLOT(quit()));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(error()));

    t.start(5000);
    q.exec();

    if(t.isActive()){
        QByteArray response = reply->readAll();
        parseTokens(response);
        reply->deleteLater();
    } else {
        qDebug() << "Timeout";
    }
}


int
dmz::QtOAuthCouchDB::authorizationWidget() {
   
    return 0;
}
