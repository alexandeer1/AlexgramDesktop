/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "history/view/history_view_ghost_edits_box.h"

#include "history/history_item.h"
#include "history/history.h"
#include "main/main_session.h"
#include "alex/messages_storage.h"
#include "alex/alex_database.h"
#include "window/window_session_controller.h"
#include "api/api_text_entities.h"
#include "ui/widgets/labels.h"
#include "ui/widgets/box_content_divider.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/text/text_entity.h"
#include "ui/painter.h"
#include "lang/lang_keys.h"
#include "styles/style_layers.h"
#include "styles/style_chat.h"
#include "base/unixtime.h"
#include "data/data_session.h"
#include "data/data_photo.h"
#include "data/data_document.h"
#include "history/view/history_view_item_preview.h"
#include "ui/text/text.h"

namespace HistoryView {
namespace {

class MediaPreviewWidget : public Ui::RpWidget {
public:
	MediaPreviewWidget(
		QWidget *parent,
		const QImage &image,
		Window::SessionController *controller,
		std::unique_ptr<HistoryItem> item)
	: Ui::RpWidget(parent)
	, _image(image)
	, _controller(controller)
	, _item(std::move(item)) {
		setFixedSize(st::msgReplyBarSize.height(), st::msgReplyBarSize.height());
		if (_controller && _item) {
			setCursor(style::cur_pointer);
		}
	}
protected:
	void paintEvent(QPaintEvent *e) override {
		Painter p(this);
		p.setRenderHint(QPainter::Antialiasing);
		auto rect = this->rect();
		QPainterPath path;
		path.addRoundedRect(rect, st::roundRadiusSmall, st::roundRadiusSmall);
		p.setClipPath(path);
		p.drawImage(rect, _image.scaled(size() * this->devicePixelRatioF(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
		
		if (_item && _item->media()) {
			if (const auto document = _item->media()->document()) {
				if (document->isVideoFile() || document->isVideoMessage() || document->isGifv()) {
					// Draw play icon or similar
					const auto &icon = st::videoIcon;
					const auto iconWidth = icon.width();
					const auto iconHeight = icon.height();
					icon.paint(p, (rect.width() - iconWidth) / 2, (rect.height() - iconHeight) / 2, rect.width());
				}
			}
		}
	}
	void mousePressEvent(QMouseEvent *e) override {
		if (_controller && _item) {
			const auto media = _item->media();
			if (!media) return;
			
			Window::SessionController::MessageContext context;
			context.id = _item->fullId();
			
			if (const auto photo = media->photo()) {
				_controller->openPhoto(photo, context);
			} else if (const auto document = media->document()) {
				_controller->openDocument(document, true, context);
			}
		}
	}
private:
	QImage _image;
	Window::SessionController *_controller = nullptr;
	std::unique_ptr<HistoryItem> _item;
};

class MessageTextWidget : public Ui::RpWidget {
public:
	MessageTextWidget(
		QWidget *parent,
		const TextWithEntities &textWithEntities,
		Window::SessionController *controller)
	: Ui::RpWidget(parent)
	, _controller(controller)
	, _text(st::boxLabel.style, textWithEntities, kMarkupTextOptions, Ui::kQFixedMax, { .repaint = [=] { update(); } }) {
		setMouseTracking(true);
	}

	int resizeGetHeight(int newWidth) override {
		return _text.countHeight(newWidth);
	}

protected:
	void paintEvent(QPaintEvent *e) override {
		Painter p(this);
		p.setPen(st::boxLabel.textFg);
		_text.draw(p, 0, 0, width());
	}
	void mouseMoveEvent(QMouseEvent *e) override {
		auto state = _text.getState(e->pos(), width());
		if (state.link) {
			setCursor(style::cur_pointer);
		} else {
			setCursor(style::cur_default);
		}
	}
	void mousePressEvent(QMouseEvent *e) override {
		auto state = _text.getState(e->pos(), width());
		if (state.link) {
			state.link->onClick({
				.button = e->button(),
			});
		}
	}
private:
	Window::SessionController *_controller = nullptr;
	Ui::Text::String _text;
};

} // namespace

void GhostEditsBox(not_null<Ui::GenericBox*> box, not_null<HistoryItem*> item, Window::SessionController *controller) {
	box->setTitle(tr::lng_context_alexgram_edits_history());

	const auto userId = item->history()->session().userId().bare;
	const auto dialogId = item->history()->peer->id.value;
	const auto messageId = item->id.bare;

	const auto revisions = Alex::Messages::getEditedMessages(userId, dialogId, messageId);

	if (revisions.empty()) {
		box->addRow(object_ptr<Ui::FlatLabel>(box, u"No revisions found."_q, st::boxLabel));
	} else {
		for (const auto &revision : revisions) {
			box->addSkip(st::boxPadding.top());

			const auto date = base::unixtime::parse(revision.editDate);
			const auto dateText = date.toString(u"dd.MM.yy HH:mm:ss"_q);
			box->addRow(object_ptr<Ui::FlatLabel>(box, dateText, st::defaultFlatLabel), st::boxPadding);

			auto textWithEntities = TextWithEntities();
			textWithEntities.text = QString::fromStdString(revision.text);
			if (!revision.textEntities.empty()) {
				auto mtpEntities = MTPVector<MTPMessageEntity>();
				auto from = reinterpret_cast<const mtpPrime*>(revision.textEntities.data());
				const auto end = from + (revision.textEntities.size() / sizeof(mtpPrime));
				if (mtpEntities.read(from, end)) {
					textWithEntities.entities = Api::EntitiesFromMTP(
						&item->history()->session(),
						mtpEntities.v);
				}
			}

			if (!revision.messageData.empty()) {
				MTPMessage mtp;
				auto from = reinterpret_cast<const mtpPrime*>(revision.messageData.data());
				const auto end = from + (revision.messageData.size() / sizeof(mtpPrime));
				if (mtp.read(from, end) && mtp.type() == mtpc_message) {
					const auto &data = mtp.c_message();
					auto tempItem = std::make_unique<HistoryItem>(
						item->history(),
						MsgId(0), // Avoid registration conflict
						data,
						MessageFlag::HistoryEntry | MessageFlag::HasFromId);

					const auto preview = tempItem->toPreview(ToPreviewOptions{
						.generateImages = true,
						.ignoreGroup = true,
					});

					if (!preview.images.empty() && preview.images.front()) {
						box->addRow(
							object_ptr<MediaPreviewWidget>(
								box,
								preview.images.front().data,
								controller,
								std::move(tempItem)),
							st::boxPadding);
					} else if (!revision.mediaPath.empty() && revision.mediaPath != "/") {
						box->addRow(
							object_ptr<Ui::FlatLabel>(
								box,
								QString::fromStdString("[" + revision.mediaPath + "]"),
								st::boxLabel),
							st::boxPadding);
					}
				}
			} else if (!revision.mediaPath.empty() && revision.mediaPath != "/") {
				box->addRow(
					object_ptr<Ui::FlatLabel>(
						box,
						QString::fromStdString("[" + revision.mediaPath + "]"),
						st::boxLabel),
					st::boxPadding);
			}

			box->addRow(
				object_ptr<MessageTextWidget>(
					box,
					textWithEntities,
					controller),
				st::boxPadding);

			box->addRow(object_ptr<Ui::BoxContentDivider>(box));
		}
	}

	box->addButton(tr::lng_close(), [=] { box->closeBox(); });
}

} // namespace HistoryView
