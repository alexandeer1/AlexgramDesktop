/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "lang/translate_url_provider.h"

#include "spellcheck/platform/platform_language.h"
#include "ui/text/text_entity.h"

#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QUrl>
#include <QtCore/QRegularExpression>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "main/main_session.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "mtproto/mtproto_proxy_data.h"

namespace Ui {
namespace {

[[nodiscard]] bool SkipJsonKey(const QString &key) {
	return (key.compare(u"code"_q, Qt::CaseInsensitive) == 0);
}

[[nodiscard]] QString DetectFromLanguage(const QString &text) {
#ifndef TDESKTOP_DISABLE_SPELLCHECK
	const auto result = Platform::Language::Recognize(text);
	return result.known() ? result.twoLetterCode() : u"auto"_q;
#else // TDESKTOP_DISABLE_SPELLCHECK
	return u"auto"_q;
#endif // !TDESKTOP_DISABLE_SPELLCHECK
}

[[nodiscard]] QString JsonValueToText(const QJsonValue &v) {
	switch (v.type()) {
	case QJsonValue::Null: return u"null"_q;
	case QJsonValue::Bool: return v.toBool()
		? u"true"_q
		: u"false"_q;
	case QJsonValue::Double: return QString::number(v.toDouble(), 'g', 15);
	case QJsonValue::String: return v.toString().trimmed();
	case QJsonValue::Array: return QString::fromUtf8(
		QJsonDocument(v.toArray()).toJson(QJsonDocument::Compact));
	case QJsonValue::Object: return QString::fromUtf8(
		QJsonDocument(v.toObject()).toJson(QJsonDocument::Compact));
	case QJsonValue::Undefined: return QString();
	}
	return QString();
}

[[nodiscard]] std::optional<QString> ParseSegmentedArrayResponse(
		const QJsonDocument &parsed) {
	if (!parsed.isArray()) {
		return std::nullopt;
	}
	const auto root = parsed.array();
	if (root.isEmpty() || !root[0].isArray()) {
		return std::nullopt;
	}
	const auto segments = root[0].toArray();
	auto translated = QString();
	for (const auto &segmentValue : segments) {
		if (!segmentValue.isArray()) {
			return std::nullopt;
		}
		const auto segment = segmentValue.toArray();
		if (segment.isEmpty()) {
			return std::nullopt;
		}
		if (!segment[0].isString()) {
			return std::nullopt;
		}
		translated += segment[0].toString();
	}
	if (translated.trimmed().isEmpty()) {
		return std::nullopt;
	}
	return translated;
}

struct JsonLine {
	QString name;
	QString value;
	int length = 0;
};

void CollectJsonLines(
		const QString &name,
		const QJsonValue &value,
		std::vector<JsonLine> &lines) {
	if (value.isObject()) {
		const auto object = value.toObject();
		for (auto i = object.constBegin(); i != object.constEnd(); ++i) {
			if (SkipJsonKey(i.key())) {
				continue;
			}
			CollectJsonLines(
				name.isEmpty() ? i.key() : (name + '.' + i.key()),
				i.value(),
				lines);
		}
		return;
	}
	if (value.isArray()) {
		const auto array = value.toArray();
		for (auto i = 0; i != array.size(); ++i) {
			CollectJsonLines(
				u"%1[%2]"_q.arg(name).arg(i),
				array.at(i),
				lines);
		}
		return;
	}
	const auto text = JsonValueToText(value);
	if (text.isEmpty()) {
		return;
	}
	lines.push_back(JsonLine{
		.name = name,
		.value = text,
		.length = int(text.size()),
	});
}

[[nodiscard]] std::optional<QString> FormatJsonResponse(
		const QByteArray &body) {
	auto error = QJsonParseError();
	const auto parsed = QJsonDocument::fromJson(body, &error);
	if (error.error != QJsonParseError::NoError) {
		return std::nullopt;
	}
	if (const auto parsedArray = ParseSegmentedArrayResponse(parsed)) {
		return parsedArray;
	}
	auto lines = std::vector<JsonLine>();
	if (parsed.isObject()) {
		const auto object = parsed.object();
		for (auto i = object.constBegin(); i != object.constEnd(); ++i) {
			if (SkipJsonKey(i.key())) {
				continue;
			}
			CollectJsonLines(i.key(), i.value(), lines);
		}
	} else if (parsed.isArray()) {
		const auto array = parsed.array();
		for (auto i = 0; i != array.size(); ++i) {
			CollectJsonLines(u"[%1]"_q.arg(i), array.at(i), lines);
		}
	}
	if (lines.empty()) {
		return QString::fromUtf8(body);
	}
	ranges::sort(lines, [](const JsonLine &a, const JsonLine &b) {
		return (a.length != b.length)
			? (a.length > b.length)
			: (a.name < b.name);
	});
	auto result = QString();
	result.reserve(lines.size() * 16);
	for (auto i = 0; i != int(lines.size()); ++i) {
		const auto &line = lines[i];
		if (!line.name.isEmpty()) {
			result += line.name;
			result += '\n';
		}
		result += line.value;
		if (i + 1 != int(lines.size())) {
			result += "\n\n";
		}
	}
	return result;
}

class UrlTranslateProvider final : public TranslateProvider {
public:
	explicit UrlTranslateProvider(not_null<Main::Session*> session, QString urlTemplate)
	: _urlTemplate(std::move(urlTemplate)) {
		const auto updateProxy = [=] {
			_network.setProxy(MTP::ToNetworkProxy(Core::App().settings().proxy().selected()));
		};
		updateProxy();
		Core::App().settings().proxy().connectionTypeChanges(
		) | rpl::on_next(updateProxy, _lifetime);
	}

	[[nodiscard]] bool supportsMessageId() const override {
		return false;
	}

	void request(
			TranslateProviderRequest request,
			LanguageId to,
			Fn<void(TranslateProviderResult)> done) override {
		if (request.text.text.isEmpty()) {
			done(TranslateProviderResult{
				.error = TranslateProviderError::Unknown,
			});
			return;
		}

		auto protectedText = request.text.text;
		const auto &entities = request.text.entities;
		if (!entities.empty()) {
			// Wrap entities from back to front.
			for (auto i = int(entities.size()); i != 0; --i) {
				const auto &entity = entities[i - 1];
				const auto isObject = (entity.type() == EntityType::CustomEmoji);
				if (isObject) {
					protectedText.replace(
						entity.offset(),
						entity.length(),
						u" {E%1} "_q.arg(i - 1));
				} else {
					// Wrap: {S0}text{F0}
					protectedText.insert(entity.offset() + entity.length(), u"{F%1}"_q.arg(i - 1));
					protectedText.insert(entity.offset(), u"{S%1}"_q.arg(i - 1));
				}
			}
		}

		auto url = _urlTemplate;
		const auto from = DetectFromLanguage(protectedText);
		const auto toCode = to.twoLetterCode();
		url.replace(u"%f"_q, QString::fromLatin1(QUrl::toPercentEncoding(from)));
		url.replace(u"%t"_q, QString::fromLatin1(QUrl::toPercentEncoding(toCode)));

		const auto isPost = (protectedText.size() > 512)
			&& url.contains(u"%q"_q);
		
		auto query = QString::fromLatin1(
			QUrl::toPercentEncoding(protectedText));
		
		auto requestUrl = QUrl();
		if (isPost) {
			url.replace(u"q=%q"_q, QString());
			url.replace(u"&q=%q"_q, QString());
			url.replace(u"%q"_q, QString());
			requestUrl = QUrl(url);
		} else {
			url.replace(u"%q"_q, query);
			requestUrl = QUrl(url);
		}

		if (!requestUrl.isValid()) {
			done(TranslateProviderResult{
				.error = TranslateProviderError::Unknown,
			});
			return;
		}

		auto networkRequest = QNetworkRequest(requestUrl);
		networkRequest.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
		networkRequest.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
		networkRequest.setHeader(QNetworkRequest::UserAgentHeader, u"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36"_q);
		networkRequest.setRawHeader("Referer", "https://translate.google.com/");

		if (isPost) {
			networkRequest.setHeader(
				QNetworkRequest::ContentTypeHeader,
				u"application/x-www-form-urlencoded"_q);
		}
		const auto reply = isPost
			? _network.post(networkRequest, ("q=" + query).toUtf8())
			: _network.get(networkRequest);

		QObject::connect(reply, &QNetworkReply::finished, [=] {
			auto result = TranslateProviderResult();
			result.error = (reply->error() != QNetworkReply::NoError)
				? TranslateProviderError::Unknown
				: TranslateProviderError::None;
			if (result.error == TranslateProviderError::None) {
				const auto body = reply->readAll();
				auto formatted = FormatJsonResponse(body).value_or(
					QString::fromUtf8(body));

				auto resultEntities = EntitiesInText();
				if (!entities.empty()) {
					for (auto i = 0; i != int(entities.size()); ++i) {
						const auto &original = entities[i];
						if (original.type() == EntityType::CustomEmoji) {
							const auto pattern = u"[\\s\\p{P}]*\\{\\s*E\\s*%1\\s*\\}[\\s\\p{P}]*"_q.arg(i);
							auto regex = QRegularExpression(pattern, QRegularExpression::CaseInsensitiveOption);
							const auto match = regex.match(formatted);
							if (match.hasMatch()) {
								const int start = match.capturedStart();
								formatted.replace(start, match.capturedLength(), u" "_q);
								resultEntities.push_back(EntityInText(original.type(), start, 1, original.data()));
							}
						} else {
							// Find start {S0} and end {F0}
							const auto sPattern = u"[\\s\\p{P}]*\\{\\s*S\\s*%1\\s*\\}[\\s\\p{P}]*"_q.arg(i);
							const auto fPattern = u"[\\s\\p{P}]*\\{\\s*F\\s*%1\\s*\\}[\\s\\p{P}]*"_q.arg(i);
							auto sRegex = QRegularExpression(sPattern, QRegularExpression::CaseInsensitiveOption);
							auto fRegex = QRegularExpression(fPattern, QRegularExpression::CaseInsensitiveOption);
							const auto sMatch = sRegex.match(formatted);
							const auto fMatch = fRegex.match(formatted);
							if (sMatch.hasMatch() && fMatch.hasMatch()) {
								int sStart = sMatch.capturedStart();
								int sLen = sMatch.capturedLength();
								formatted.remove(sStart, sLen);
								
								const auto fMatchAfter = fRegex.match(formatted);
								if (fMatchAfter.hasMatch()) {
									int fStart = fMatchAfter.capturedStart();
									int fLen = fMatchAfter.capturedLength();
									formatted.remove(fStart, fLen);
									resultEntities.push_back(EntityInText(original.type(), sStart, fStart - sStart, original.data()));
								}
							} else if (sMatch.hasMatch()) {
								formatted.remove(sMatch.capturedStart(), sMatch.capturedLength());
							} else if (fMatch.hasMatch()) {
								formatted.remove(fMatch.capturedStart(), fMatch.capturedLength());
							}
						}
					}
					std::sort(resultEntities.begin(), resultEntities.end(), [](const auto &a, const auto &b) {
						return a.offset() < b.offset();
					});
				}
				TextWithEntities resultText;
				resultText.text = formatted;
				resultText.entities = resultEntities;
				result.text = resultText;
			}
			done(std::move(result));
			reply->deleteLater();
		});
	}

private:
	const QString _urlTemplate;
	QNetworkAccessManager _network;
	rpl::lifetime _lifetime;

};

} // namespace

std::unique_ptr<TranslateProvider> CreateUrlTranslateProvider(
		not_null<Main::Session*> session,
		QString urlTemplate) {
	return std::make_unique<UrlTranslateProvider>(session, std::move(urlTemplate));
}

} // namespace Ui
