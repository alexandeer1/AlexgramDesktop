#pragma once

#include "lang/translate_provider.h"
#include <QtNetwork/QNetworkAccessManager>

namespace Ui {

class YandexTranslateProvider final : public TranslateProvider {
public:
	explicit YandexTranslateProvider(not_null<Main::Session*> session);

	[[nodiscard]] bool supportsMessageId() const override {
		return false;
	}

	void request(
		TranslateProviderRequest request,
		LanguageId to,
		Fn<void(TranslateProviderResult)> done) override;

private:
	not_null<Main::Session*> _session;
	QNetworkAccessManager _network;
	QString _uuid;
	rpl::lifetime _lifetime;

};

} // namespace Ui
