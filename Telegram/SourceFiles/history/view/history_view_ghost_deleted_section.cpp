/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "history/view/history_view_ghost_deleted_section.h"

#include "history/view/history_view_top_bar_widget.h"
#include "history/view/history_view_list_widget.h"
#include "history/view/history_view_element.h"
#include "history/history.h"
#include "history/history_item.h"
#include "window/window_session_controller.h"
#include "ui/widgets/scroll_area.h"
#include "ui/widgets/shadow.h"
#include "ui/chat/chat_theme.h"
#include "ui/chat/chat_style.h"
#include "ui/ui_utility.h"
#include "data/data_session.h"
#include "data/data_peer_values.h"
#include "history/history_view_swipe_back_session.h"
#include "styles/style_chat.h"
#include "lang/lang_keys.h"

namespace HistoryView {

GhostDeletedMemento::GhostDeletedMemento(not_null<History*> history)
: _history(history) {
}

object_ptr<Window::SectionWidget> GhostDeletedMemento::createWidget(
	QWidget *parent,
	not_null<Window::SessionController*> controller,
	Window::Column column,
	const QRect &geometry) {
	if (column == Window::Column::Third) {
		return nullptr;
	}
	auto result = object_ptr<GhostDeletedWidget>(
		parent,
		controller,
		_history);
	result->setInternalState(geometry, this);
	return result;
}

GhostDeletedWidget::GhostDeletedWidget(
	QWidget *parent,
	not_null<Window::SessionController*> controller,
	not_null<History*> history)
: Window::SectionWidget(parent, controller, history->peer)
, WindowListDelegate(controller)
, _history(history)
, _scroll(
	this,
	controller->chatStyle()->value(lifetime(), st::historyScroll),
	false)
, _topBar(this, controller)
, _topBarShadow(this)
, _cornerButtons(
	_scroll.data(),
	controller->chatStyle(),
	static_cast<HistoryView::CornerButtonsDelegate*>(this)) {
	
	Window::ChatThemeValueFromPeer(
		controller,
		history->peer
	) | rpl::on_next([=](std::shared_ptr<Ui::ChatTheme> &&theme) {
		_theme = std::move(theme);
		controller->setChatStyleTheme(_theme);
	}, lifetime());

	_topBar->setActiveChat({ history, Dialogs::EntryState::Section::History }, nullptr);

	base::flat_set<FullMsgId> ids;
	for (const auto &item : _history->clientSideMessages()) {
		if (item->isGhostDeleted()) {
			ids.insert(item->fullId());
		}
	}
	for (const auto &block : _history->blocks) {
		for (const auto &view : block->messages) {
			if (view->data()->isGhostDeleted()) {
				ids.insert(view->data()->fullId());
			}
		}
	}
	const auto count = int(ids.size());
	_topBar->setCustomStatus(count
		? u"%1 deleted messages"_q.arg(count)
		: u"No deleted messages"_q);

	_topBar->move(0, 0);
	_topBar->resizeToWidth(width());
	_topBar->show();

	_topBarShadow->raise();
	controller->adaptive().value(
	) | rpl::on_next([=] {
		updateAdaptiveLayout();
	}, lifetime());

	_inner = _scroll->setOwnedWidget(object_ptr<ListWidget>(
		this,
		&controller->session(),
		static_cast<ListDelegate*>(this)));
	_scroll->move(0, _topBar->height());
	_scroll->show();
	_scroll->scrolls() | rpl::on_next([=] {
		onScroll();
	}, lifetime());

	_topBar->searchRequest(
	) | rpl::on_next([=] {
		_topBar->toggleSearch(true, anim::type::normal);
	}, lifetime());

	_topBar->searchQuery(
	) | rpl::on_next([=](const QString &query) {
		_searchQuery = query;
		_inner->refreshViewer();
	}, lifetime());

	_topBar->searchCancelled(
	) | rpl::on_next([=] {
		_searchQuery = QString();
		_inner->refreshViewer();
	}, lifetime());

	Window::SetupSwipeBackSection(this, _scroll, _inner);
}

GhostDeletedWidget::~GhostDeletedWidget() = default;

not_null<History*> GhostDeletedWidget::history() const {
	return _history;
}

Dialogs::RowDescriptor GhostDeletedWidget::activeChat() const {
	return { _history, FullMsgId(_history->peer->id, ShowAtUnreadMsgId) };
}

QPixmap GhostDeletedWidget::grabForShowAnimation(
	const Window::SectionSlideParams &params) {
	return Ui::GrabWidget(this);
}

bool GhostDeletedWidget::showInternal(
	not_null<Window::SectionMemento*> memento,
	const Window::SectionShow &params) {
	if (auto ghostMemento = dynamic_cast<GhostDeletedMemento*>(memento.get())) {
		if (ghostMemento->getHistory() == _history) {
			restoreState(ghostMemento);
			return true;
		}
	}
	return false;
}

std::shared_ptr<Window::SectionMemento> GhostDeletedWidget::createMemento() {
	auto result = std::make_shared<GhostDeletedMemento>(_history);
	saveState(result.get());
	return result;
}

void GhostDeletedWidget::setInternalState(
	const QRect &geometry,
	not_null<GhostDeletedMemento*> memento) {
	setGeometry(geometry);
	Ui::SendPendingMoveResizeEvents(this);
	restoreState(memento);
}

bool GhostDeletedWidget::floatPlayerHandleWheelEvent(QEvent *e) {
	return _scroll->viewportEvent(e);
}

QRect GhostDeletedWidget::floatPlayerAvailableRect() {
	return mapToGlobal(_scroll->geometry());
}

Context GhostDeletedWidget::listContext() {
	return Context::GhostDeleted;
}

bool GhostDeletedWidget::listScrollTo(int top, bool syntetic) {
	_scroll->scrollToY(top);
	return true;
}

void GhostDeletedWidget::listCancelRequest() {
	controller()->showBackFromStack();
}

void GhostDeletedWidget::listDeleteRequest() {
}

void GhostDeletedWidget::listTryProcessKeyInput(not_null<QKeyEvent*> e) {
}

rpl::producer<Data::MessagesSlice> GhostDeletedWidget::listSource(
	Data::MessagePosition aroundId,
	int limitBefore,
	int limitAfter) {
	return rpl::single() | rpl::map([=] {
		auto result = Data::MessagesSlice();
		const auto query = _searchQuery.toLower();
		auto check = [&](not_null<HistoryItem*> item) {
			if (!item->isGhostDeleted()) {
				return false;
			}
			if (query.isEmpty()) {
				return true;
			}
			return item->notificationText().text.toLower().contains(query)
				|| item->originalText().text.toLower().contains(query);
		};
		for (const auto &item : _history->clientSideMessages()) {
			if (check(item)) {
				result.ids.push_back(item->fullId());
			}
		}
		for (const auto &block : _history->blocks) {
			for (const auto &view : block->messages) {
				if (check(view->data())) {
					const auto id = view->data()->fullId();
					if (!ranges::contains(result.ids, id)) {
						result.ids.push_back(id);
					}
				}
			}
		}
		ranges::sort(result.ids, [](const FullMsgId &a, const FullMsgId &b) {
			return a.msg < b.msg;
		});
		return result;
	});
}

bool GhostDeletedWidget::listAllowsMultiSelect() {
	return false;
}

bool GhostDeletedWidget::listIsItemGoodForSelection(not_null<HistoryItem*> item) {
	return true;
}

bool GhostDeletedWidget::listIsLessInOrder(
	not_null<HistoryItem*> first,
	not_null<HistoryItem*> second) {
	return first->position() < second->position();
}

void GhostDeletedWidget::listSelectionChanged(SelectedItems &&items) {
}

void GhostDeletedWidget::listMarkReadTill(not_null<HistoryItem*> item) {
}

void GhostDeletedWidget::listMarkContentsRead(
	const base::flat_set<not_null<HistoryItem*>> &items) {
}

MessagesBarData GhostDeletedWidget::listMessagesBar(
	const std::vector<not_null<Element*>> &elements,
	bool markLastAsRead) {
	return {};
}

void GhostDeletedWidget::listContentRefreshed() {
}

void GhostDeletedWidget::listUpdateDateLink(
	ClickHandlerPtr &link,
	not_null<Element*> view) {
}

bool GhostDeletedWidget::listElementHideReply(not_null<const Element*> view) {
	return false;
}

bool GhostDeletedWidget::listElementShownUnread(not_null<const Element*> view) {
	return false;
}

bool GhostDeletedWidget::listIsGoodForAroundPosition(
	not_null<const Element*> view) {
	return true;
}

void GhostDeletedWidget::listSendBotCommand(
	const QString &command,
	const FullMsgId &context) {
}

void GhostDeletedWidget::listSearch(
	const QString &query,
	const FullMsgId &context) {
	_topBar->toggleSearch(true, anim::type::normal);
	_topBar->searchSetText(query);
}

void GhostDeletedWidget::listHandleViaClick(not_null<UserData*> bot) {
}

not_null<Ui::ChatTheme*> GhostDeletedWidget::listChatTheme() {
	return _theme ? _theme.get() : controller()->defaultChatTheme().get();
}

CopyRestrictionType GhostDeletedWidget::listCopyRestrictionType(HistoryItem *item) {
	return CopyRestrictionType::None;
}

CopyRestrictionType GhostDeletedWidget::listCopyMediaRestrictionType(
	not_null<HistoryItem*> item) {
	return CopyRestrictionType::None;
}

CopyRestrictionType GhostDeletedWidget::listSelectRestrictionType() {
	return CopyRestrictionType::None;
}

auto GhostDeletedWidget::listAllowedReactionsValue()
-> rpl::producer<Data::AllowedReactions> {
	return rpl::single(Data::AllowedReactions());
}

void GhostDeletedWidget::listShowPremiumToast(not_null<DocumentData*> document) {
}

void GhostDeletedWidget::listOpenPhoto(
	not_null<PhotoData*> photo,
	FullMsgId context) {
	controller()->openPhoto(photo, { context });
}

void GhostDeletedWidget::listOpenDocument(
	not_null<DocumentData*> document,
	FullMsgId context,
	bool showInMediaView) {
	controller()->openDocument(document, showInMediaView, { context });
}

void GhostDeletedWidget::listPaintEmpty(
	Painter &p,
	const Ui::ChatPaintContext &context) {
}

QString GhostDeletedWidget::listElementAuthorRank(not_null<const Element*> view) {
	return {};
}

bool GhostDeletedWidget::listElementHideTopicButton(not_null<const Element*> view) {
	return true;
}

History *GhostDeletedWidget::listTranslateHistory() {
	return _history;
}

void GhostDeletedWidget::listAddTranslatedItems(
	not_null<TranslateTracker*> tracker) {
}

void GhostDeletedWidget::cornerButtonsShowAtPosition(
	Data::MessagePosition position) {
}

Data::Thread *GhostDeletedWidget::cornerButtonsThread() {
	return _history;
}

FullMsgId GhostDeletedWidget::cornerButtonsCurrentId() {
	return {};
}

bool GhostDeletedWidget::cornerButtonsIgnoreVisibility() {
	return false;
}

std::optional<bool> GhostDeletedWidget::cornerButtonsDownShown() {
	return false;
}

bool GhostDeletedWidget::cornerButtonsUnreadMayBeShown() {
	return false;
}

bool GhostDeletedWidget::cornerButtonsHas(CornerButtonType type) {
	return false;
}

void GhostDeletedWidget::resizeEvent(QResizeEvent *e) {
	_topBar->resizeToWidth(width());
	_topBarShadow->resize(width(), st::lineWidth);
	_topBarShadow->move(0, _topBar->height());
	const auto scrollHeight = height() - _topBar->height();
	_scroll->setGeometry(0, _topBar->height(), width(), scrollHeight);
	_inner->resizeToWidth(width(), scrollHeight);
}

void GhostDeletedWidget::paintEvent(QPaintEvent *e) {
	SectionWidget::PaintBackground(
		controller(),
		(_theme ? _theme.get() : controller()->defaultChatTheme().get()),
		this,
		e->rect());
}

void GhostDeletedWidget::showAnimatedHook(
	const Window::SectionSlideParams &params) {
}

void GhostDeletedWidget::showFinishedHook() {
}

void GhostDeletedWidget::doSetInnerFocus() {
	_inner->setFocus();
}

void GhostDeletedWidget::checkActivation() {
}

void GhostDeletedWidget::onScroll() {
	const auto top = _scroll->scrollTop();
	_inner->setVisibleTopBottom(top, top + _scroll->height());
}

void GhostDeletedWidget::updateInnerVisibleArea() {
	onScroll();
}

void GhostDeletedWidget::updateControlsGeometry() {
}

void GhostDeletedWidget::updateAdaptiveLayout() {
}

void GhostDeletedWidget::saveState(not_null<GhostDeletedMemento*> memento) {
	_inner->saveState(memento->list());
}

void GhostDeletedWidget::restoreState(not_null<GhostDeletedMemento*> memento) {
	_inner->restoreState(memento->list());
}

void GhostDeletedWidget::showAtPosition(
	Data::MessagePosition position,
	FullMsgId originId) {
	if (const auto top = _inner->scrollTopForPosition(position)) {
		_scroll->scrollToY(*top);
	}
}

} // namespace HistoryView
