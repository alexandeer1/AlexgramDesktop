/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include "ui/text/text_entity.h"

namespace Ui {

struct TranslateWrapped {
	QString text;
	EntitiesInText originalEntities;
	int originalLength = 0;
};

[[nodiscard]] TranslateWrapped WrapForTranslation(const TextWithEntities &text);
[[nodiscard]] TextWithEntities UnwrapFromTranslation(const QString &translated, const TranslateWrapped &wrapped);

} // namespace Ui
