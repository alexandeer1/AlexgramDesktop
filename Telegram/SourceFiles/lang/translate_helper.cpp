/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "lang/translate_helper.h"

#include <QtCore/QRegularExpression>

namespace Ui {

TranslateWrapped WrapForTranslation(const TextWithEntities &text) {
	auto protectedText = text.text;
	const auto &entities = text.entities;
	if (!entities.empty()) {
		// Wrap entities from back to front using standard XML-like tags.
		// These are universally preserved by translation APIs (Bing, Yandex, Google, LLMs)
		// and they naturally move WITH the words when word order changes.
		for (auto i = int(entities.size()); i != 0; --i) {
			const auto &entity = entities[i - 1];
			protectedText.insert(entity.offset() + entity.length(), u"</t%1>"_q.arg(i - 1));
			protectedText.insert(entity.offset(), u"<t%1>"_q.arg(i - 1));
		}
	}
	return { std::move(protectedText), entities, int(text.text.size()) };
}

TextWithEntities UnwrapFromTranslation(
		const QString &translated,
		const TranslateWrapped &wrapped) {
	if (wrapped.originalEntities.empty()) {
		return { translated };
	}

	struct Tag {
		int pos;
		int len;
		int id;
		bool isClose;
	};
	std::vector<Tag> tags;

	// Regex to match <t0>, </t0>, and robustly handle spaces or escaped brackets 
	// that a translation provider might introduce (e.g. &lt; /t 0 &gt; or ＜ t0 ＞)
	QRegularExpression re(u"(?:<|＜|&lt;)\\s*(/)?\\s*[tT]\\s*(\\d+)\\s*(?:>|＞|&gt;)"_q, QRegularExpression::CaseInsensitiveOption);
	
	auto it = re.globalMatch(translated);
	while (it.hasNext()) {
		auto match = it.next();
		bool isClose = !match.captured(1).isEmpty();
		int id = match.captured(2).toInt();
		tags.push_back({
			int(match.capturedStart()),
			int(match.capturedLength()),
			id,
			isClose
		});
	}

	QString resultText;
	int lastPos = 0;
	
	struct EntitySpan {
		int start = -1;
		int end = -1;
	};
	std::vector<EntitySpan> spans(wrapped.originalEntities.size());

	// Reconstruct the text without the tags, and track exactly where each tag 
	// landed in the new clean text.
	for (const auto &tag : tags) {
		resultText += translated.mid(lastPos, tag.pos - lastPos);
		int currentResultPos = resultText.size();
		
		if (tag.id >= 0 && tag.id < int(spans.size())) {
			if (tag.isClose) {
				if (spans[tag.id].end == -1) {
					spans[tag.id].end = currentResultPos;
				}
			} else {
				if (spans[tag.id].start == -1) {
					spans[tag.id].start = currentResultPos;
				}
			}
		}
		lastPos = tag.pos + tag.len;
	}
	resultText += translated.mid(lastPos);

	auto resultEntities = EntitiesInText();
	resultEntities.reserve(wrapped.originalEntities.size());

	for (auto i = 0; i != int(wrapped.originalEntities.size()); ++i) {
		const auto &original = wrapped.originalEntities[i];
		int start = spans[i].start;
		int end = spans[i].end;

		if (start != -1) {
			if (end == -1) {
				// Missing close tag. Handle gracefully based on entity type.
				if (original.type() == EntityType::CustomEmoji) {
					end = start + 1;
					if (start < resultText.size() && resultText.at(start).isHighSurrogate()) {
						end = start + 2;
					}
				} else {
					end = resultText.size();
				}
			}
			
			start = std::clamp(start, 0, int(resultText.size()));
			end = std::clamp(end, start, int(resultText.size()));

			// Trim spaces inside the tags so formatting doesn't leak into adjacent spaces
			if (original.type() != EntityType::Pre 
			    && original.type() != EntityType::Blockquote
			    && original.type() != EntityType::CustomEmoji) {
				while (end > start && resultText.at(end - 1).isSpace()) {
					--end;
				}
				while (start < end && resultText.at(start).isSpace()) {
					++start;
				}
			}

			if (end > start) {
				resultEntities.push_back(EntityInText(
					original.type(),
					start,
					end - start,
					original.data()));
			}
		}
	}

	std::sort(resultEntities.begin(), resultEntities.end(), [](const auto &a, const auto &b) {
		return a.offset() < b.offset();
	});

	return { resultText, std::move(resultEntities) };
}

} // namespace Ui
