#include "dmzQtNetworkCookieJar.h"
#include <dmzRuntimeConfigToTypesBase.h>
#include <dmzRuntimeConfigWrite.h>
#include <QtCore/QtCore>


dmz::QtNetworkCookieJar::QtNetworkCookieJar (QObject *parent) :
		QNetworkCookieJar (parent) {;}


dmz::QtNetworkCookieJar::~QtNetworkCookieJar () {;}


void
dmz::QtNetworkCookieJar::load_cookies (const Config &Session) {

	QList<QNetworkCookie> list;

	Config cookieList;
	Session.lookup_all_config ("cookie", cookieList);

	ConfigIterator it;
	Config cd;

	while (cookieList.get_next_config (it, cd)) {

		String cookie = config_to_string ("value", cd);

		QList<QNetworkCookie> cookieList =
			QNetworkCookie::parseCookies (cookie.get_buffer ());

		list.append (cookieList);
	}

	setAllCookies (list);
}


void
dmz::QtNetworkCookieJar::save_cookies (Config &session) {

	QListIterator<QNetworkCookie> it(allCookies ());
	while (it.hasNext ()) {

		QByteArray cookie = it.next ().toRawForm ();
		session.add_config (string_to_config ("cookie", cookie.constData ()));
	}
}
