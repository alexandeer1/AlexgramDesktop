#include "lang/translate_extra_providers.h"
#include "lang/translate_helper.h"

#include "core/application.h"
#include "core/core_settings.h"
#include "main/main_session.h"
#include "mtproto/mtproto_proxy_data.h"
#include "ui/text/text_entity.h"
#include <QtCore/QUrl>
#include <QtCore/QUrlQuery>
#include <QtCore/QUuid>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QDateTime>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

namespace Ui {
namespace {

const auto kUserAgent = u"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36 Edg/122.0.0.0"_q;

// Microsoft Edge
const auto kEdgeAuthUrl = u"https://edge.microsoft.com/translate/auth"_q;
const auto kEdgeTranslateUrl = u"https://api.cognitive.microsofttranslator.com/translate?api-version=3.0"_q;

// Tencent
const auto kTencentUrl = u"https://transmart.qq.com/api/imt"_q;

// Caiyun
const auto kCaiyunUrl = u"https://api.interpreter.caiyunai.com/v1/translator"_q;
const auto kCaiyunToken = u"token 9sdftiq37bnv410eon2l"_q;

} // namespace

// Microsoft Edge
MicrosoftEdgeTranslateProvider::MicrosoftEdgeTranslateProvider(not_null<Main::Session*> session)
: _session(session) {
	const auto updateProxy = [=] {
		_network.setProxy(MTP::ToNetworkProxy(Core::App().settings().proxy().selected()));
	};
	updateProxy();
	Core::App().settings().proxy().connectionTypeChanges(
	) | rpl::on_next(updateProxy, _lifetime);
}

void MicrosoftEdgeTranslateProvider::request(TranslateProviderRequest request, LanguageId to, Fn<void(TranslateProviderResult)> done) {
	auto &settings = Core::App().settings();
	const auto token = settings.readPref<QString>("edge-token");
	const auto ts = settings.readPref<uint64>("edge-token-ts", 0);
	const auto now = uint64(QDateTime::currentMSecsSinceEpoch());
	
	if (token.isEmpty() || (now - ts) > (10 * 60 * 1000)) {
		fetchToken([=] { performTranslation(request, to, done); }, [=] { done({ .error = TranslateProviderError::Unknown }); });
	} else {
		performTranslation(request, to, done);
	}
}

void MicrosoftEdgeTranslateProvider::fetchToken(Fn<void()> done, Fn<void()> fail) {
	auto request = QNetworkRequest(QUrl(kEdgeAuthUrl));
	request.setHeader(QNetworkRequest::UserAgentHeader, kUserAgent);
	const auto reply = _network.get(request);
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		if (reply->error() == QNetworkReply::NoError) {
			const auto token = QString::fromUtf8(reply->readAll());
			auto &settings = Core::App().settings();
			settings.writePref<QString>("edge-token", token);
			settings.writePref<uint64>("edge-token-ts", uint64(QDateTime::currentMSecsSinceEpoch()));
			Core::App().saveSettingsDelayed();
			done();
		} else {
			fail();
		}
		reply->deleteLater();
	});
}

void MicrosoftEdgeTranslateProvider::performTranslation(const TranslateProviderRequest &request, LanguageId to, Fn<void(TranslateProviderResult)> done) {
	const auto wrapped = WrapForTranslation(request.text);
	const auto protectedText = wrapped.text;

	auto url = QUrl(kEdgeTranslateUrl + u"&to="_q + to.twoLetterCode());
	auto networkRequest = QNetworkRequest(url);
	networkRequest.setHeader(QNetworkRequest::UserAgentHeader, kUserAgent);
	networkRequest.setRawHeader("Authorization", "Bearer " + Core::App().settings().readPref<QString>("edge-token").toUtf8());
	networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, u"application/json"_q);

	QJsonArray array;
	QJsonObject obj;
	obj[u"Text"_q] = protectedText;
	array.append(obj);
	for (const auto &button : request.buttons) {
		QJsonObject btnObj;
		btnObj[u"Text"_q] = button;
		array.append(btnObj);
	}

	const auto reply = _network.post(networkRequest, QJsonDocument(array).toJson());
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		if (reply->error() == QNetworkReply::NoError) {
			const auto parsed = QJsonDocument::fromJson(reply->readAll());
			if (parsed.isArray() && !parsed.array().isEmpty()) {
				const auto resArray = parsed.array();
				TranslateProviderResult res;
				
				const auto resultObj = resArray[0].toObject();
				const auto translations = resultObj[u"translations"_q].toArray();
				if (!translations.isEmpty()) {
					res.text = UnwrapFromTranslation(translations[0].toObject()[u"text"_q].toString(), wrapped);
				}
				
				for (int i = 1; i < resArray.size(); ++i) {
					const auto btnResultObj = resArray[i].toObject();
					const auto btnTranslations = btnResultObj[u"translations"_q].toArray();
					if (!btnTranslations.isEmpty()) {
						res.buttons.push_back(btnTranslations[0].toObject()[u"text"_q].toString());
					}
				}
				
				done(std::move(res));
				reply->deleteLater();
				return;
			}
		}
		done({ .error = TranslateProviderError::Unknown });
		reply->deleteLater();
	});
}

// Tencent
TencentTranslateProvider::TencentTranslateProvider(not_null<Main::Session*> session)
: _session(session) {
	const auto updateProxy = [=] {
		_network.setProxy(MTP::ToNetworkProxy(Core::App().settings().proxy().selected()));
	};
	updateProxy();
	Core::App().settings().proxy().connectionTypeChanges(
	) | rpl::on_next(updateProxy, _lifetime);
}

void TencentTranslateProvider::request(TranslateProviderRequest request, LanguageId to, Fn<void(TranslateProviderResult)> done) {
	const auto wrapped = WrapForTranslation(request.text);
	const auto protectedText = wrapped.text;

	auto networkRequest = QNetworkRequest(QUrl(kTencentUrl));
	networkRequest.setHeader(QNetworkRequest::UserAgentHeader, kUserAgent);
	networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, u"application/json"_q);

	QJsonObject root;
	QJsonObject header;
	header[u"client_key"_q] = u"browser-chrome-120.0.0-Windows-"_q + QUuid::createUuid().toString(QUuid::WithoutBraces) + u"-"_q + QString::number(QDateTime::currentMSecsSinceEpoch());
	header[u"fn"_q] = u"auto_translation"_q;
	root[u"header"_q] = header;

	QJsonObject source;
	source[u"lang"_q] = u"auto"_q;
	QJsonArray textList;
	textList.append(protectedText);
	for (const auto &button : request.buttons) {
		textList.append(button);
	}
	source[u"text_list"_q] = textList;
	root[u"source"_q] = source;

	QJsonObject target;
	target[u"lang"_q] = to.twoLetterCode();
	root[u"target"_q] = target;

	root[u"model_category"_q] = u"normal"_q;
	root[u"type"_q] = u"plain"_q;

	const auto reply = _network.post(networkRequest, QJsonDocument(root).toJson());
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		if (reply->error() == QNetworkReply::NoError) {
			const auto parsed = QJsonDocument::fromJson(reply->readAll());
			if (parsed.isObject()) {
				const auto array = parsed.object()[u"auto_translation"_q].toArray();
				if (!array.isEmpty()) {
					TranslateProviderResult res;
					res.text = UnwrapFromTranslation(array[0].toString(), wrapped);
					for (int i = 1; i < array.size(); ++i) {
						res.buttons.push_back(array[i].toString());
					}
					done(std::move(res));
					reply->deleteLater();
					return;
				}
			}
		}
		done({ .error = TranslateProviderError::Unknown });
		reply->deleteLater();
	});
}

// Caiyun
CaiyunTranslateProvider::CaiyunTranslateProvider(not_null<Main::Session*> session)
: _session(session) {
	const auto updateProxy = [=] {
		_network.setProxy(MTP::ToNetworkProxy(Core::App().settings().proxy().selected()));
	};
	updateProxy();
	Core::App().settings().proxy().connectionTypeChanges(
	) | rpl::on_next(updateProxy, _lifetime);
}

void CaiyunTranslateProvider::request(TranslateProviderRequest request, LanguageId to, Fn<void(TranslateProviderResult)> done) {
	const auto wrapped = WrapForTranslation(request.text);
	const auto protectedText = wrapped.text;

	auto networkRequest = QNetworkRequest(QUrl(kCaiyunUrl));
	networkRequest.setHeader(QNetworkRequest::UserAgentHeader, kUserAgent);
	networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, u"application/json"_q);
	networkRequest.setRawHeader("X-Authorization", kCaiyunToken.toUtf8());

	QJsonObject root;
	QJsonArray source;
	source.append(protectedText);
	for (const auto &button : request.buttons) {
		source.append(button);
	}
	root[u"source"_q] = source;
	root[u"trans_type"_q] = u"auto2"_q + to.twoLetterCode();
	root[u"detect"_q] = true;

	const auto reply = _network.post(networkRequest, QJsonDocument(root).toJson());
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		if (reply->error() == QNetworkReply::NoError) {
			const auto parsed = QJsonDocument::fromJson(reply->readAll());
			if (parsed.isObject()) {
				const auto array = parsed.object()[u"target"_q].toArray();
				if (!array.isEmpty()) {
					TranslateProviderResult res;
					res.text = UnwrapFromTranslation(array[0].toString(), wrapped);
					for (int i = 1; i < array.size(); ++i) {
						res.buttons.push_back(array[i].toString());
					}
					done(std::move(res));
					reply->deleteLater();
					return;
				}
			}
		}
		done({ .error = TranslateProviderError::Unknown });
		reply->deleteLater();
	});
}

// LLM
LlmTranslateProvider::LlmTranslateProvider(not_null<Main::Session*> session)
: _session(session) {
	const auto updateProxy = [=] {
		_network.setProxy(MTP::ToNetworkProxy(Core::App().settings().proxy().selected()));
	};
	updateProxy();
	Core::App().settings().proxy().connectionTypeChanges(
	) | rpl::on_next(updateProxy, _lifetime);
}

void LlmTranslateProvider::request(TranslateProviderRequest request, LanguageId to, Fn<void(TranslateProviderResult)> done) {
	const auto wrapped = WrapForTranslation(request.text);
	const auto protectedText = wrapped.text;

	const auto url = Core::App().settings().translatorLlmUrl();
	const auto key = Core::App().settings().translatorLlmKey();

	if (key.isEmpty()) {
		done({ .error = TranslateProviderError::Unknown });
		return;
	}

	auto networkRequest = QNetworkRequest(QUrl(url));
	networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, u"application/json"_q);
	networkRequest.setRawHeader("Authorization", "Bearer " + key.toUtf8());

	QJsonObject root;
	root[u"model"_q] = u"gpt-4o-mini"_q;
	
	QJsonArray messages;
	QJsonObject system;
	system[u"role"_q] = u"system"_q;
	
	auto systemContent = u"You are a translation engine. Translate the following content to %1. Output only a JSON object with 'text' and 'buttons' (array of strings) fields."_q.arg(to.twoLetterCode());
	system[u"content"_q] = systemContent;
	messages.append(system);

	QJsonObject user;
	user[u"role"_q] = u"user"_q;
	
	QJsonObject userObj;
	userObj[u"text"_q] = protectedText;
	QJsonArray buttonsArray;
	for (const auto &button : request.buttons) {
		buttonsArray.append(button);
	}
	userObj[u"buttons"_q] = buttonsArray;
	
	user[u"content"_q] = QString::fromUtf8(QJsonDocument(userObj).toJson(QJsonDocument::Compact));
	messages.append(user);
	
	root[u"messages"_q] = messages;
	root[u"temperature"_q] = 0.3;

	const auto reply = _network.post(networkRequest, QJsonDocument(root).toJson());
	QObject::connect(reply, &QNetworkReply::finished, [=] {
		if (reply->error() == QNetworkReply::NoError) {
			const auto body = reply->readAll();
			auto error = QJsonParseError();
			const auto parsed = QJsonDocument::fromJson(body, &error);
			if (error.error == QJsonParseError::NoError && parsed.isObject()) {
				const auto choices = parsed.object()[u"choices"_q].toArray();
				if (!choices.isEmpty()) {
					const auto message = choices[0].toObject()[u"message"_q].toObject();
					const auto content = message[u"content"_q].toString();
					
					// Clean up potential markdown formatting around JSON
					auto jsonText = content.trimmed();
					if (jsonText.startsWith(u"```json"_q)) {
						jsonText.remove(0, 7);
						if (jsonText.endsWith(u"```"_q)) {
							jsonText.chop(3);
						}
						jsonText = jsonText.trimmed();
					} else if (jsonText.startsWith(u"```"_q)) {
						jsonText.remove(0, 3);
						if (jsonText.endsWith(u"```"_q)) {
							jsonText.chop(3);
						}
						jsonText = jsonText.trimmed();
					}

					const auto resDoc = QJsonDocument::fromJson(jsonText.toUtf8());
					if (resDoc.isObject()) {
						const auto resObj = resDoc.object();
						TranslateProviderResult res;
						res.text = UnwrapFromTranslation(resObj[u"text"_q].toString().trimmed(), wrapped);
						const auto resButtons = resObj[u"buttons"_q].toArray();
						for (const auto &btn : resButtons) {
							res.buttons.push_back(btn.toString());
						}
						done(std::move(res));
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
