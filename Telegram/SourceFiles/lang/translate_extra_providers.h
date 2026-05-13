#pragma once

#include "lang/translate_provider.h"
#include <QtNetwork/QNetworkAccessManager>

namespace Ui {

class MicrosoftEdgeTranslateProvider final : public TranslateProvider {
public:
	explicit MicrosoftEdgeTranslateProvider(not_null<Main::Session*> session);

	[[nodiscard]] bool supportsMessageId() const override { return false; }
	void request(TranslateProviderRequest request, LanguageId to, Fn<void(TranslateProviderResult)> done) override;

private:
	void fetchToken(Fn<void()> done, Fn<void()> fail);
	void performTranslation(const TranslateProviderRequest &request, LanguageId to, Fn<void(TranslateProviderResult)> done);

	not_null<Main::Session*> _session;
	QNetworkAccessManager _network;
	rpl::lifetime _lifetime;
};

class TencentTranslateProvider final : public TranslateProvider {
public:
	explicit TencentTranslateProvider(not_null<Main::Session*> session);

	[[nodiscard]] bool supportsMessageId() const override { return false; }
	void request(TranslateProviderRequest request, LanguageId to, Fn<void(TranslateProviderResult)> done) override;

private:
	not_null<Main::Session*> _session;
	QNetworkAccessManager _network;
	rpl::lifetime _lifetime;
};

class CaiyunTranslateProvider final : public TranslateProvider {
public:
	explicit CaiyunTranslateProvider(not_null<Main::Session*> session);

	[[nodiscard]] bool supportsMessageId() const override { return false; }
	void request(TranslateProviderRequest request, LanguageId to, Fn<void(TranslateProviderResult)> done) override;

private:
	not_null<Main::Session*> _session;
	QNetworkAccessManager _network;
	rpl::lifetime _lifetime;
};

class LlmTranslateProvider final : public TranslateProvider {
public:
	explicit LlmTranslateProvider(not_null<Main::Session*> session);

	[[nodiscard]] bool supportsMessageId() const override { return false; }
	void request(TranslateProviderRequest request, LanguageId to, Fn<void(TranslateProviderResult)> done) override;

private:
	not_null<Main::Session*> _session;
	QNetworkAccessManager _network;
	rpl::lifetime _lifetime;
};

} // namespace Ui
