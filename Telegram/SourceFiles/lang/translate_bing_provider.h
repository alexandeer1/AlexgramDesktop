#pragma once

#include "lang/translate_provider.h"
#include <QtNetwork/QNetworkAccessManager>

namespace Ui {

class BingTranslateProvider final : public TranslateProvider {
public:
	explicit BingTranslateProvider(not_null<Main::Session*> session);

	[[nodiscard]] bool supportsMessageId() const override {
		return false;
	}

	void request(
		TranslateProviderRequest request,
		LanguageId to,
		Fn<void(TranslateProviderResult)> done) override;

private:
	void fetchConfig(Fn<void()> done, Fn<void()> fail);
	void performTranslation(
		const TranslateProviderRequest &request,
		LanguageId to,
		Fn<void(TranslateProviderResult)> done);

	struct Config {
		QString ig;
		QString iid;
		QString key;
		QString token;
		uint64 tokenTs = 0;
		uint64 tokenExpiryInterval = 0;
	};

	[[nodiscard]] Config loadConfig();
	void saveConfig(const Config &config);
	[[nodiscard]] bool isTokenExpired(const Config &config);

	not_null<Main::Session*> _session;
	QNetworkAccessManager _network;
	int _count = 0;
	rpl::lifetime _lifetime;

};

} // namespace Ui
