/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "lang/translate_provider.h"

#include "base/options.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "data/data_msg_id.h"
#include "data/data_peer.h"
#include "data/data_session.h"
#include "history/history_item.h"
#include "history/history_item_components.h"
#include "lang/translate_mtproto_provider.h"
#include "lang/translate_url_provider.h"
#include "lang/translate_bing_provider.h"
#include "lang/translate_yandex_provider.h"
#include "lang/translate_extra_providers.h"
#include "main/main_session.h"
#include "data/data_user.h"
#include "platform/platform_translate_provider.h"

namespace {

base::options::option<QString> OptionTranslateUrlTemplate({
	.id = "translate-url-template",
	.name = "Translate URL template",
	.description = "Template URL for custom translation provider."
		" Supports %q text, %f source language and %t target language.",
});

} // namespace

namespace Ui {

std::unique_ptr<TranslateProvider> CreateTranslateProvider(
		not_null<Main::Session*> session) {
	const auto urlTemplate = OptionTranslateUrlTemplate.value();
	if (!urlTemplate.isEmpty()
		&& urlTemplate.contains(u"%q"_q)) {
		return CreateUrlTranslateProvider(session, urlTemplate);
	}

	const auto provider = Core::App().settings().translatorProvider();
	using Provider = Core::Settings::TranslatorProvider;
	if (provider == Provider::GoogleAt) {
		return CreateUrlTranslateProvider(
			session,
			u"https://translate.google.com/translate_a/single?dj=1&q=%q&sl=auto&tl=%t&ie=UTF-8&oe=UTF-8&client=at&dt=t&otf=2"_q);
	} else if (provider == Provider::Bing) {
		return std::make_unique<BingTranslateProvider>(session);
	} else if (provider == Provider::Yandex) {
		return std::make_unique<YandexTranslateProvider>(session);
	} else if (provider == Provider::MicrosoftEdge) {
		return std::make_unique<MicrosoftEdgeTranslateProvider>(session);
	} else if (provider == Provider::Tencent) {
		return std::make_unique<TencentTranslateProvider>(session);
	} else if (provider == Provider::Caiyun) {
		return std::make_unique<CaiyunTranslateProvider>(session);
	} else if (provider == Provider::ChatGpt) {
		return std::make_unique<LlmTranslateProvider>(session);
	} else if (provider == Provider::GoogleGtx) {
		return CreateUrlTranslateProvider(
			session,
			u"https://translate.googleapis.com/translate_a/single?client=gtx&sl=%f&tl=%t&dt=t&q=%q"_q);
	} else if (provider == Provider::Telegram) {
		if (session->premium()) {
			return CreateMTProtoTranslateProvider(session);
		}
	}

	if (Core::App().settings().usePlatformTranslation()
		&& Platform::IsTranslateProviderAvailable()) {
		return Platform::CreateTranslateProvider();
	}
	if (!session->premium()) {
		return CreateUrlTranslateProvider(
			session,
			u"https://translate.googleapis.com/translate_a/single?client=gtx&sl=%f&tl=%t&dt=t&q=%q"_q);
	}
	return CreateMTProtoTranslateProvider(session);
}

TranslateProviderRequest PrepareTranslateProviderRequest(
		not_null<TranslateProvider*> provider,
		not_null<PeerData*> peer,
		MsgId msgId,
		TextWithEntities text) {
	auto result = TranslateProviderRequest{
		.peerId = uint64(peer->id.value),
		.msgId = IsServerMsgId(msgId) ? msgId.bare : 0,
		.text = std::move(text),
	};
	if (const auto item = peer->owner().message(peer, msgId)) {
		if (const auto markup = item->Get<HistoryMessageReplyMarkup>()) {
			for (const auto &row : markup->data.rows) {
				for (const auto &button : row) {
					result.buttons.push_back(button.text);
				}
			}
		}
	}
	if (provider->supportsMessageId()) {
		return result;
	}
	if (result.msgId) {
		if (result.text.empty()) {
			if (const auto i = peer->owner().message(peer, MsgId(result.msgId))) {
				result.text = i->originalText();
			}
		}
		result.msgId = 0;
	}
	return result;
}

} // namespace Ui
