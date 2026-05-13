#include "lang/translate_bing_provider.h"
#include "lang/translate_helper.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "mtproto/mtproto_proxy_data.h"
#include "main/main_session.h"
#include "ui/text/text_entity.h"
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QRegularExpression>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

namespace Ui {
namespace {

const auto kBingUrl = u"https://www.bing.com/translator"_q;
const auto kBingApiUrl = u"https://www.bing.com/ttranslatev3"_q;
const auto kUserAgent = u"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36 Edg/122.0.0.0"_q;

[[nodiscard]] QString ExtractValue(const QString &text, const QString &pattern) {
	auto regex = QRegularExpression(pattern);
	const auto match = regex.match(text);
	return match.hasMatch() ? match.captured(0) : QString();
}

[[nodiscard]] QString ExtractArrayValue(const QString &text, int index) {
	if (text.isEmpty()) return QString();
	auto parts = text.mid(1, text.size() - 2).split(',');
	if (index < parts.size()) {
		auto result = parts[index].trimmed();
		if (result.startsWith('"')) result.remove(0, 1);
		if (result.endsWith('"')) result.remove(result.size() - 1, 1);
		return result;
	}
	return QString();
}

} // namespace

BingTranslateProvider::BingTranslateProvider(not_null<Main::Session*> session)
: _session(session) {
	const auto updateProxy = [=] {
		_network.setProxy(MTP::ToNetworkProxy(Core::App().settings().proxy().selected()));
	};
	updateProxy();
	Core::App().settings().proxy().connectionTypeChanges(
	) | rpl::on_next(updateProxy, _lifetime);
}

void BingTranslateProvider::request(
		TranslateProviderRequest request,
		LanguageId to,
		Fn<void(TranslateProviderResult)> done) {
	const auto config = loadConfig();
	if (isTokenExpired(config)) {
		fetchConfig([=] {
			performTranslation(request, to, done);
		}, [=] {
			done({ .error = TranslateProviderError::Unknown });
		});
	} else {
		performTranslation(request, to, done);
	}
}

void BingTranslateProvider::fetchConfig(Fn<void()> done, Fn<void()> fail) {
	auto request = QNetworkRequest(QUrl(kBingUrl));
	request.setHeader(QNetworkRequest::UserAgentHeader, kUserAgent);

	const auto reply = _network.get(request);
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		if (reply->error() == QNetworkReply::NoError) {
			const auto html = QString::fromUtf8(reply->readAll());
			Config config;
			config.ig = ExtractValue(html, u"(?<=IG:\")[^\"]*"_q);
			config.iid = ExtractValue(html, u"(?<=data-iid=\")[^\"]*"_q);
			const auto params = ExtractValue(html, u"(?<=params_AbusePreventionHelper = )\\[[^\\]]+\\]"_q);
			config.key = ExtractArrayValue(params, 0);
			config.token = ExtractArrayValue(params, 1);
			config.tokenTs = config.key.toULongLong();
			config.tokenExpiryInterval = ExtractArrayValue(params, 2).toULongLong();

			if (!config.token.isEmpty()) {
				saveConfig(config);
				done();
			} else {
				fail();
			}
		} else {
			fail();
		}
		reply->deleteLater();
	});
}

void BingTranslateProvider::performTranslation(
		const TranslateProviderRequest &request,
		LanguageId to,
		Fn<void(TranslateProviderResult)> done) {
	const auto wrapped = WrapForTranslation(request.text);
	const auto protectedText = wrapped.text;

	const auto config = loadConfig();
	const auto eptIid = config.iid + u"."_q + QString::number(++_count);
	
	auto url = QUrl(kBingApiUrl);
	auto query = QUrlQuery();
	query.addQueryItem(u"isVertical"_q, u"1"_q);
	query.addQueryItem(u"IG"_q, config.ig);
	query.addQueryItem(u"IID"_q, eptIid);
	query.addQueryItem(u"ref"_q, u"TThis"_q);
	query.addQueryItem(u"edgepdftranslator"_q, u"1"_q);
	url.setQuery(query);

	auto networkRequest = QNetworkRequest(url);
	networkRequest.setHeader(QNetworkRequest::UserAgentHeader, kUserAgent);
	networkRequest.setRawHeader("Referer", kBingUrl.toUtf8());
	networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, u"application/x-www-form-urlencoded"_q);

	auto postData = QUrlQuery();
	postData.addQueryItem(u"fromLang"_q, u"auto-detect"_q);
	postData.addQueryItem(u"to"_q, (to.twoLetterCode() == u"zh"_q) ? u"zh-Hans"_q : to.twoLetterCode());
	postData.addQueryItem(u"text"_q, protectedText);
	postData.addQueryItem(u"token"_q, config.token);
	postData.addQueryItem(u"key"_q, config.key);
	postData.addQueryItem(u"tryFetchingGenderDebiasedTranslations"_q, u"true"_q);

	const auto reply = _network.post(networkRequest, postData.toString(QUrl::FullyEncoded).toUtf8());
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		if (reply->error() == QNetworkReply::NoError) {
			const auto body = reply->readAll();
			auto error = QJsonParseError();
			const auto parsed = QJsonDocument::fromJson(body, &error);
			if (error.error == QJsonParseError::NoError && parsed.isArray()) {
				const auto root = parsed.array();
				if (!root.isEmpty()) {
					const auto first = root[0].toObject();
					const auto translations = first[u"translations"_q].toArray();
					if (!translations.isEmpty()) {
						const auto translation = translations[0].toObject();
						const auto text = translation[u"text"_q].toString();
						
						TranslateProviderResult result;
						result.text = UnwrapFromTranslation(text, wrapped);
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

BingTranslateProvider::Config BingTranslateProvider::loadConfig() {
	auto &settings = Core::App().settings();
	Config config;
	config.ig = settings.readPref<QString>("bing-ig");
	config.iid = settings.readPref<QString>("bing-iid");
	config.key = settings.readPref<QString>("bing-key");
	config.token = settings.readPref<QString>("bing-token");
	config.tokenTs = settings.readPref<uint64>("bing-token-ts", 0);
	config.tokenExpiryInterval = settings.readPref<uint64>("bing-token-expiry", 0);
	return config;
}

void BingTranslateProvider::saveConfig(const Config &config) {
	auto &settings = Core::App().settings();
	settings.writePref<QString>("bing-ig", config.ig);
	settings.writePref<QString>("bing-iid", config.iid);
	settings.writePref<QString>("bing-key", config.key);
	settings.writePref<QString>("bing-token", config.token);
	settings.writePref<uint64>("bing-token-ts", config.tokenTs);
	settings.writePref<uint64>("bing-token-expiry", config.tokenExpiryInterval);
	Core::App().saveSettingsDelayed();
}

bool BingTranslateProvider::isTokenExpired(const Config &config) {
	if (config.token.isEmpty()) return true;
	const auto now = uint64(QDateTime::currentMSecsSinceEpoch());
	return (now - config.tokenTs) > config.tokenExpiryInterval;
}

} // namespace Ui
