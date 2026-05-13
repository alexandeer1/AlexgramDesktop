#include "lang/translate_yandex_provider.h"
#include "lang/translate_helper.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "mtproto/mtproto_proxy_data.h"
#include "main/main_session.h"
#include "ui/text/text_entity.h"
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QUuid>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

namespace Ui {
namespace {

const auto kYandexApiUrl = u"https://translate.yandex.net/api/v1/tr.json/translate"_q;
const auto kUserAgent = u"Mozilla/5.0 (iPhone; CPU iPhone OS 10_0 like Mac OS X) AppleWebKit/602.1.38 (KHTML, like Gecko) Version/10.0 Mobile/14A5297c Safari/602.1"_q;

} // namespace

YandexTranslateProvider::YandexTranslateProvider(not_null<Main::Session*> session)
: _session(session) {
	const auto updateProxy = [=] {
		_network.setProxy(MTP::ToNetworkProxy(Core::App().settings().proxy().selected()));
	};
	updateProxy();
	Core::App().settings().proxy().connectionTypeChanges(
	) | rpl::on_next(updateProxy, _lifetime);
	
	_uuid = QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-');
}

void YandexTranslateProvider::request(
		TranslateProviderRequest request,
		LanguageId to,
		Fn<void(TranslateProviderResult)> done) {
	const auto wrapped = WrapForTranslation(request.text);
	const auto protectedText = wrapped.text;

	auto url = QUrl(kYandexApiUrl);
	auto query = QUrlQuery();
	query.addQueryItem(u"srv"_q, u"android"_q);
	query.addQueryItem(u"uuid"_q, _uuid);
	query.addQueryItem(u"id"_q, QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-') + u"-9-0"_q);
	url.setQuery(query);

	auto networkRequest = QNetworkRequest(url);
	networkRequest.setHeader(QNetworkRequest::UserAgentHeader, kUserAgent);
	networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, u"application/x-www-form-urlencoded"_q);

	auto postData = QUrlQuery();
	postData.addQueryItem(u"text"_q, protectedText);
	postData.addQueryItem(u"lang"_q, to.twoLetterCode());

	const auto reply = _network.post(networkRequest, postData.toString(QUrl::FullyEncoded).toUtf8());
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		if (reply->error() == QNetworkReply::NoError) {
			const auto body = reply->readAll();
			auto error = QJsonParseError();
			const auto parsed = QJsonDocument::fromJson(body, &error);
			if (error.error == QJsonParseError::NoError && parsed.isObject()) {
				const auto root = parsed.object();
				if (root[u"code"_q].toInt() == 200) {
					const auto textArray = root[u"text"_q].toArray();
					if (!textArray.isEmpty()) {
						TranslateProviderResult result;
						result.text = UnwrapFromTranslation(textArray[0].toString(), wrapped);
						done(std::move(result));
						reply->deleteLater();
						return;
					}
				}
			}
		}
		done({ .error = TranslateProviderError::Unknown });
		reply->deleteLater();
	});
}

} // namespace Ui
