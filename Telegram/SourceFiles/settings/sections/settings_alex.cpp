/*
This file is part of Alexgram Desktop,
the official desktop application for the Alexgram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include "settings/sections/settings_alex.h"

#include <memory>
#include "settings/sections/settings_main.h"
#include "settings/settings_builder.h"
#include "settings/settings_common_session.h"
#include "ui/wrap/vertical_layout.h"
#include "ui/wrap/slide_wrap.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/labels.h"
#include "ui/ui_utility.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "lang/lang_keys.h"
#include "styles/style_menu_icons.h"
#include "styles/style_settings.h"
#include "window/window_session_controller.h"
#include "main/main_session.h"
#include "api/api_updates.h"
#include "ui/boxes/confirm_box.h"
#include "ui/widgets/discrete_sliders.h"

namespace Settings {

class AlexgramMain final : public Section<AlexgramMain> {
public:
	AlexgramMain(QWidget *parent, not_null<Window::SessionController*> controller) : Section(parent, controller) {
		setupContent();
	}
	[[nodiscard]] rpl::producer<QString> title() override {
		return tr::lng_settings_alexgram();
	}
private:
	void setupContent();
};

class AlexgramGeneral final : public Section<AlexgramGeneral> {
public:
	AlexgramGeneral(QWidget *parent, not_null<Window::SessionController*> controller) : Section(parent, controller) {
		setupContent();
	}
	[[nodiscard]] rpl::producer<QString> title() override {
		return tr::lng_settings_alexgram_general();
	}
private:
	void setupContent();
};

class AlexgramTranslator final : public Section<AlexgramTranslator> {
public:
	AlexgramTranslator(QWidget *parent, not_null<Window::SessionController*> controller) : Section(parent, controller) {
		setupContent();
	}
	[[nodiscard]] rpl::producer<QString> title() override {
		return tr::lng_settings_alexgram_translator();
	}
private:
	void setupContent();
};

class AlexgramChats final : public Section<AlexgramChats> {
public:
	AlexgramChats(QWidget *parent, not_null<Window::SessionController*> controller) : Section(parent, controller) {
		setupContent();
	}
	[[nodiscard]] rpl::producer<QString> title() override {
		return tr::lng_settings_alexgram_chats();
	}
private:
	void setupContent();
};

class AlexgramExperimental final : public Section<AlexgramExperimental> {
public:
	AlexgramExperimental(QWidget *parent, not_null<Window::SessionController*> controller) : Section(parent, controller) {
		setupContent();
	}
	[[nodiscard]] rpl::producer<QString> title() override {
		return tr::lng_settings_alexgram_experimental();
	}
private:
	void setupContent();
};

class AlexgramGhostMode final : public Section<AlexgramGhostMode> {
public:
	AlexgramGhostMode(QWidget *parent, not_null<Window::SessionController*> controller) : Section(parent, controller) {
		setupContent();
	}
	[[nodiscard]] rpl::producer<QString> title() override {
		return tr::lng_settings_alexgram_ghost_mode();
	}
private:
	void setupContent();
};


void AlexgramMain::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto builder = Builder::SectionBuilder(Builder::WidgetContext{ content, controller(), showOtherMethod() });
	builder.addDivider();
	builder.addSkip();
	
	Builder::SectionBuilder::SectionArgs generalArgs;
	generalArgs.title = tr::lng_settings_alexgram_general();
	generalArgs.targetSection = AlexgramGeneralSectionId();
	generalArgs.icon = { &st::menuIconSettings };
	builder.addSectionButton(std::move(generalArgs));

	Builder::SectionBuilder::SectionArgs translatorArgs;
	translatorArgs.title = tr::lng_settings_alexgram_translator();
	translatorArgs.targetSection = AlexgramTranslatorSectionId();
	translatorArgs.icon = { &st::menuIconTranslate };
	builder.addSectionButton(std::move(translatorArgs));

	Builder::SectionBuilder::SectionArgs chatsArgs;
	chatsArgs.title = tr::lng_settings_alexgram_chats();
	chatsArgs.targetSection = AlexgramChatsSectionId();
	chatsArgs.icon = { &st::menuIconChatBubble };
	builder.addSectionButton(std::move(chatsArgs));

	Builder::SectionBuilder::SectionArgs experimentalArgs;
	experimentalArgs.title = tr::lng_settings_alexgram_experimental();
	experimentalArgs.targetSection = AlexgramExperimentalSectionId();
	experimentalArgs.icon = { &st::menuIconExperimental };
	builder.addSectionButton(std::move(experimentalArgs));

	Builder::SectionBuilder::SectionArgs ghostModeArgs;
	ghostModeArgs.title = tr::lng_settings_alexgram_ghost_mode();
	ghostModeArgs.targetSection = AlexgramGhostModeSectionId();
	ghostModeArgs.icon = { &st::menuIconStealth };
	builder.addSectionButton(std::move(ghostModeArgs));

	builder.addSkip();
	Ui::ResizeFitChild(this, content);
}

void AlexgramGeneral::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto builder = Builder::SectionBuilder(Builder::WidgetContext{ content, controller(), showOtherMethod() });
	builder.addDivider();
	builder.addSkip();
	
	Builder::SectionBuilder::CheckboxArgs roundArgs;
	roundArgs.title = tr::lng_settings_disable_number_rounding();
	roundArgs.checked = Core::App().settings().disableNumberRounding();
	const auto checkbox = builder.addCheckbox(std::move(roundArgs));
	checkbox->checkedChanges() | rpl::on_next([=](bool checked) { 
		Core::App().settings().setDisableNumberRounding(checked); 
		Core::App().saveSettingsDelayed(); 
	}, checkbox->lifetime());
	builder.addDividerText(tr::lng_settings_disable_number_rounding_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs hideTabsArgs;
	hideTabsArgs.title = tr::lng_settings_hide_all_chats_tab();
	hideTabsArgs.checked = Core::App().settings().hideAllChatsTab();
	const auto hideTabsCb = builder.addCheckbox(std::move(hideTabsArgs));
	hideTabsCb->checkedChanges() | rpl::on_next([=](bool checked) { 
		Core::App().settings().setHideAllChatsTab(checked); 
		Core::App().saveSettingsDelayed(); 
	}, hideTabsCb->lifetime());

	builder.addDividerText(tr::lng_settings_hide_all_chats_tab_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs removeArchivedArgs;
	removeArchivedArgs.title = tr::lng_settings_remove_archived_from_list();
	removeArchivedArgs.checked = Core::App().settings().removeArchivedFromList();
	const auto removeArchivedCb = builder.addCheckbox(std::move(removeArchivedArgs));
	removeArchivedCb->checkedChanges() | rpl::on_next([=](bool checked) { 
		Core::App().settings().setRemoveArchivedFromList(checked); 
		Core::App().saveSettingsDelayed(); 
	}, removeArchivedCb->lifetime());

	builder.addDividerText(tr::lng_settings_remove_archived_from_list_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs hidePremiumArgs;
	hidePremiumArgs.title = tr::lng_settings_hide_premium();
	hidePremiumArgs.checked = Core::App().settings().hidePremium();
	const auto hidePremiumCb = builder.addCheckbox(std::move(hidePremiumArgs));
	hidePremiumCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setHidePremium(checked);
		Core::App().saveSettingsDelayed();
	}, hidePremiumCb->lifetime());

	builder.addDividerText(tr::lng_settings_hide_premium_about());
	builder.addSkip();
	
	Builder::SectionBuilder::CheckboxArgs hideHelpArgs;
	hideHelpArgs.title = tr::lng_settings_hide_help();
	hideHelpArgs.checked = Core::App().settings().hideHelp();
	const auto hideHelpCb = builder.addCheckbox(std::move(hideHelpArgs));
	hideHelpCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setHideHelp(checked);
		Core::App().saveSettingsDelayed();
	}, hideHelpCb->lifetime());

	builder.addDividerText(tr::lng_settings_hide_help_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs hidePhoneArgs;
	hidePhoneArgs.title = tr::lng_settings_hide_phone_number();
	hidePhoneArgs.checked = Core::App().settings().hidePhoneNumber();
	const auto hidePhoneCb = builder.addCheckbox(std::move(hidePhoneArgs));
	hidePhoneCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setHidePhoneNumber(checked);
		Core::App().saveSettingsDelayed();
	}, hidePhoneCb->lifetime());

	builder.addDividerText(tr::lng_settings_hide_phone_number_about());
	builder.addSkip();

	Ui::ResizeFitChild(this, content);
}

void AlexgramTranslator::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto builder = Builder::SectionBuilder(Builder::WidgetContext{ content, controller(), showOtherMethod() });
	builder.addDivider();
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs showTranslateArgs;
	showTranslateArgs.title = tr::lng_settings_show_translate_button();
	showTranslateArgs.checked = Core::App().settings().showTranslateButton();
	const auto showTranslateCb = builder.addCheckbox(std::move(showTranslateArgs));
	showTranslateCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setShowTranslateButton(checked);
		Core::App().saveSettingsDelayed();
	}, showTranslateCb->lifetime());

	builder.addDividerText(tr::lng_settings_show_translate_button_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs translateChatArgs;
	translateChatArgs.title = tr::lng_translate_settings_chat();
	translateChatArgs.checked = Core::App().settings().translateChatEnabled();
	const auto translateChatCb = builder.addCheckbox(std::move(translateChatArgs));
	translateChatCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setTranslateChatEnabled(checked);
		Core::App().saveSettingsDelayed();
	}, translateChatCb->lifetime());

	builder.addDividerText(tr::lng_translate_settings_about());
	builder.addSkip();

	Ui::ResizeFitChild(this, content);
}

void AlexgramChats::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto builder = Builder::SectionBuilder(Builder::WidgetContext{ content, controller(), showOtherMethod() });
	builder.addDivider();
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs linkPreviewArgs;
	linkPreviewArgs.title = tr::lng_settings_alexgram_disable_link_preview();
	linkPreviewArgs.checked = Core::App().settings().disableLinkPreviewByDefault();
	const auto linkPreviewCb = builder.addCheckbox(std::move(linkPreviewArgs));
	linkPreviewCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setDisableLinkPreviewByDefault(checked);
		Core::App().saveSettingsDelayed();
	}, linkPreviewCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_disable_link_preview_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs showMsgIdArgs;
	showMsgIdArgs.title = tr::lng_settings_alexgram_show_message_id();
	showMsgIdArgs.checked = Core::App().settings().showMessageId();
	const auto showMsgIdCb = builder.addCheckbox(std::move(showMsgIdArgs));
	showMsgIdCb->checkedChanges(
	) | rpl::filter([=](bool checked) {
		return (checked != Core::App().settings().showMessageId());
	}) | rpl::on_next([=](bool checked) {
		Core::App().settings().setShowMessageId(checked);
		Core::App().saveSettingsDelayed();
		Core::App().reprocessAlexSettings();
	}, showMsgIdCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_show_message_id_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs showSecArgs;
	showSecArgs.title = tr::lng_settings_alexgram_show_timestamp_seconds();
	showSecArgs.checked = Core::App().settings().showTimestampSeconds();
	const auto showSecCb = builder.addCheckbox(std::move(showSecArgs));
	showSecCb->checkedChanges(
	) | rpl::filter([=](bool checked) {
		return (checked != Core::App().settings().showTimestampSeconds());
	}) | rpl::on_next([=](bool checked) {
		Core::App().settings().setShowTimestampSeconds(checked);
		Core::App().saveSettingsDelayed();
		Core::App().reprocessAlexSettings();
	}, showSecCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_show_timestamp_seconds_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs showEditedIconArgs;
	showEditedIconArgs.title = tr::lng_settings_alexgram_show_edited_icon();
	showEditedIconArgs.checked = Core::App().settings().showEditedIcon();
	const auto showEditedIconCb = builder.addCheckbox(std::move(showEditedIconArgs));
	showEditedIconCb->checkedChanges(
	) | rpl::filter([=](bool checked) {
		return (checked != Core::App().settings().showEditedIcon());
	}) | rpl::on_next([=](bool checked) {
		Core::App().settings().setShowEditedIcon(checked);
		Core::App().saveSettingsDelayed();
		Core::App().reprocessAlexSettings();
	}, showEditedIconCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_show_edited_icon_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs showForwardedDateArgs;
	showForwardedDateArgs.title = tr::lng_settings_alexgram_show_forwarded_date();
	showForwardedDateArgs.checked = Core::App().settings().showForwardedDate();
	const auto showForwardedDateCb = builder.addCheckbox(std::move(showForwardedDateArgs));
	showForwardedDateCb->checkedChanges(
	) | rpl::filter([=](bool checked) {
		return (checked != Core::App().settings().showForwardedDate());
	}) | rpl::on_next([=](bool checked) {
		Core::App().settings().setShowForwardedDate(checked);
		Core::App().saveSettingsDelayed();
		Core::App().reprocessAlexSettings();
	}, showForwardedDateCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_show_forwarded_date_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs showOnlineStatusArgs;
	showOnlineStatusArgs.title = tr::lng_settings_alexgram_show_online_status();
	showOnlineStatusArgs.checked = Core::App().settings().showOnlineStatus();
	const auto showOnlineStatusCb = builder.addCheckbox(std::move(showOnlineStatusArgs));
	showOnlineStatusCb->checkedChanges(
	) | rpl::filter([=](bool checked) {
		return (checked != Core::App().settings().showOnlineStatus());
	}) | rpl::on_next([=](bool checked) {
		Core::App().settings().setShowOnlineStatus(checked);
		Core::App().saveSettingsDelayed();
		Core::App().reprocessAlexSettings();
	}, showOnlineStatusCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_show_online_status_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs showSmallGifsArgs;
	showSmallGifsArgs.title = tr::lng_settings_alexgram_show_small_gifs();
	showSmallGifsArgs.checked = Core::App().settings().showSmallGifs();
	const auto showSmallGifsCb = builder.addCheckbox(std::move(showSmallGifsArgs));
	showSmallGifsCb->checkedChanges(
	) | rpl::filter([=](bool checked) {
		return (checked != Core::App().settings().showSmallGifs());
	}) | rpl::on_next([=](bool checked) {
		Core::App().settings().setShowSmallGifs(checked);
		Core::App().saveSettingsDelayed();
		Core::App().reprocessAlexSettings();
	}, showSmallGifsCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_show_small_gifs_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs treatGifsAsVideosArgs;
	treatGifsAsVideosArgs.title = tr::lng_settings_alexgram_treat_gifs_as_videos();
	treatGifsAsVideosArgs.checked = Core::App().settings().treatGifsAsVideos();
	const auto treatGifsAsVideosCb = builder.addCheckbox(std::move(treatGifsAsVideosArgs));
	treatGifsAsVideosCb->checkedChanges(
	) | rpl::filter([=](bool checked) {
		return (checked != Core::App().settings().treatGifsAsVideos());
	}) | rpl::on_next([=](bool checked) {
		const auto confirmed = crl::guard(treatGifsAsVideosCb, [=](Fn<void()> close) {
			Core::App().settings().setTreatGifsAsVideos(checked);
			Core::App().saveSettingsDelayed();
			Core::Restart();
			close();
		});
		const auto cancelled = crl::guard(treatGifsAsVideosCb, [=](Fn<void()> close) {
			treatGifsAsVideosCb->setChecked(
				Core::App().settings().treatGifsAsVideos(),
				Ui::Checkbox::NotifyAboutChange::DontNotify);
			close();
		});
		controller()->show(Ui::MakeConfirmBox({
			.text = tr::lng_settings_need_restart(),
			.confirmed = confirmed,
			.cancelled = cancelled,
			.confirmText = tr::lng_settings_restart_now(),
		}));
	}, treatGifsAsVideosCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_treat_gifs_as_videos_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs autoPauseVideoArgs;
	autoPauseVideoArgs.title = tr::lng_settings_alexgram_auto_pause_video();
	autoPauseVideoArgs.checked = Core::App().settings().autoPauseVideo();
	const auto autoPauseVideoCb = builder.addCheckbox(std::move(autoPauseVideoArgs));
	autoPauseVideoCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setAutoPauseVideo(checked);
		Core::App().saveSettingsDelayed();
	}, autoPauseVideoCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_auto_pause_video_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs noAutoPlayNextVoiceArgs;
	noAutoPlayNextVoiceArgs.title = tr::lng_settings_alexgram_no_autoplay_next_voice();
	noAutoPlayNextVoiceArgs.checked = Core::App().settings().noAutoPlayNextVoice();
	const auto noAutoPlayNextVoiceCb = builder.addCheckbox(std::move(noAutoPlayNextVoiceArgs));
	noAutoPlayNextVoiceCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setNoAutoPlayNextVoice(checked);
		Core::App().saveSettingsDelayed();
	}, noAutoPlayNextVoiceCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_no_autoplay_next_voice_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs showSpoilersArgs;
	showSpoilersArgs.title = tr::lng_settings_alexgram_show_spoilers_directly();
	showSpoilersArgs.checked = Core::App().settings().showSpoilersDirectly();
	const auto showSpoilersCb = builder.addCheckbox(std::move(showSpoilersArgs));
	showSpoilersCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setShowSpoilersDirectly(checked);
		Core::App().saveSettingsDelayed();
		Core::App().reprocessAlexSettings();
	}, showSpoilersCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_show_spoilers_directly_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs hideGroupStickersArgs;
	hideGroupStickersArgs.title = tr::lng_settings_alexgram_hide_group_stickers();
	hideGroupStickersArgs.checked = Core::App().settings().hideGroupStickers();
	const auto hideGroupStickersCb = builder.addCheckbox(std::move(hideGroupStickersArgs));
	hideGroupStickersCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setHideGroupStickers(checked);
		Core::App().saveSettingsDelayed();
	}, hideGroupStickersCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_hide_group_stickers_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs unlimitedRecentStickersArgs;
	unlimitedRecentStickersArgs.title = tr::lng_settings_alexgram_unlimited_recent_stickers();
	unlimitedRecentStickersArgs.checked = Core::App().settings().unlimitedRecentStickers();
	const auto unlimitedRecentStickersCb = builder.addCheckbox(std::move(unlimitedRecentStickersArgs));
	unlimitedRecentStickersCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setUnlimitedRecentStickers(checked);
		Core::App().saveSettingsDelayed();
	}, unlimitedRecentStickersCb->lifetime());

	builder.scope([&] {
		builder.addSkip(st::settingsSendTypeSkip);
		const auto group = std::make_shared<Ui::RadiobuttonGroup>(
			Core::App().settings().maxRecentStickers());

		const auto addOption = [&](int value) {
			builder.container()->add(
				object_ptr<Ui::Radiobutton>(
					builder.container(),
					group,
					value,
					QString::number(value),
					st::settingsSendType),
				st::settingsSendTypePadding);
		};

		for (const auto value : { 40, 60, 80, 100, 120, 150, 200 }) {
			addOption(value);
		}

		group->setChangedCallback([=](int value) {
			Core::App().settings().setMaxRecentStickers(value);
			Core::App().saveSettingsDelayed();
		});
		builder.addSkip(st::settingsSendTypeSkip);
	}, unlimitedRecentStickersCb->checkedValue());

	builder.addDividerText(tr::lng_settings_alexgram_unlimited_recent_stickers_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs hideSideShareButtonArgs;
	hideSideShareButtonArgs.title = tr::lng_settings_alexgram_hide_side_share_button();
	hideSideShareButtonArgs.checked = Core::App().settings().hideSideShareButton();
	const auto hideSideShareButtonCb = builder.addCheckbox(std::move(hideSideShareButtonArgs));
	hideSideShareButtonCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setHideSideShareButton(checked);
		Core::App().saveSettingsDelayed();
		Core::App().reprocessAlexSettings();
	}, hideSideShareButtonCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_hide_side_share_button_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs hideSendAsButtonArgs;
	hideSendAsButtonArgs.title = tr::lng_settings_alexgram_hide_send_as_button();
	hideSendAsButtonArgs.checked = Core::App().settings().hideSendAsButton();
	const auto hideSendAsButtonCb = builder.addCheckbox(std::move(hideSendAsButtonArgs));
	hideSendAsButtonCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setHideSendAsButton(checked);
		Core::App().saveSettingsDelayed();
	}, hideSendAsButtonCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_hide_send_as_button_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs hideChannelBottomBarArgs;
	hideChannelBottomBarArgs.title = tr::lng_settings_alexgram_hide_channel_bottom_bar();
	hideChannelBottomBarArgs.checked = Core::App().settings().hideChannelBottomBar();
	const auto hideChannelBottomBarCb = builder.addCheckbox(std::move(hideChannelBottomBarArgs));
	hideChannelBottomBarCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setHideChannelBottomBar(checked);
		Core::App().saveSettingsDelayed();
	}, hideChannelBottomBarCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_hide_channel_bottom_bar_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs askBeforeLinkArgs;
	askBeforeLinkArgs.title = tr::lng_settings_alexgram_ask_before_link();
	askBeforeLinkArgs.checked = Core::App().settings().askBeforeLink();
	const auto askBeforeLinkCb = builder.addCheckbox(std::move(askBeforeLinkArgs));
	askBeforeLinkCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setAskBeforeLink(checked);
		Core::App().saveSettingsDelayed();
	}, askBeforeLinkCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_ask_before_link_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs askBeforeInlineLinkArgs;
	askBeforeInlineLinkArgs.title = tr::lng_settings_alexgram_ask_before_inline_link();
	askBeforeInlineLinkArgs.checked = Core::App().settings().askBeforeInlineLink();
	const auto askBeforeInlineLinkCb = builder.addCheckbox(std::move(askBeforeInlineLinkArgs));
	askBeforeInlineLinkCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setAskBeforeInlineLink(checked);
		Core::App().saveSettingsDelayed();
	}, askBeforeInlineLinkCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_ask_before_inline_link_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs askBeforeCallingArgs;
	askBeforeCallingArgs.title = tr::lng_settings_alexgram_ask_before_calling();
	askBeforeCallingArgs.checked = Core::App().settings().askBeforeCalling();
	const auto askBeforeCallingCb = builder.addCheckbox(std::move(askBeforeCallingArgs));
	askBeforeCallingCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setAskBeforeCalling(checked);
		Core::App().saveSettingsDelayed();
	}, askBeforeCallingCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_ask_before_calling_about());
	builder.addSkip();

	Ui::ResizeFitChild(this, content);
}

void AlexgramExperimental::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto builder = Builder::SectionBuilder(Builder::WidgetContext{ content, controller(), showOtherMethod() });
	builder.addDivider();
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs unlimitedPinnedArgs;
	unlimitedPinnedArgs.title = tr::lng_settings_alexgram_unlimited_pinned();
	unlimitedPinnedArgs.checked = Core::App().settings().unlimitedPinned();
	const auto unlimitedPinnedCb = builder.addCheckbox(std::move(unlimitedPinnedArgs));
	unlimitedPinnedCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setUnlimitedPinned(checked);
		Core::App().saveSettingsDelayed();
	}, unlimitedPinnedCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_unlimited_pinned_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs unlimitedFavoriteStickersArgs;
	unlimitedFavoriteStickersArgs.title = tr::lng_settings_alexgram_unlimited_favorite_stickers();
	unlimitedFavoriteStickersArgs.checked = Core::App().settings().unlimitedFavoriteStickers();
	const auto unlimitedFavoriteStickersCb = builder.addCheckbox(std::move(unlimitedFavoriteStickersArgs));
	unlimitedFavoriteStickersCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setUnlimitedFavoriteStickers(checked);
		Core::App().saveSettingsDelayed();
	}, unlimitedFavoriteStickersCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_unlimited_favorite_stickers_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs uploadSpeedBoostArgs;
	uploadSpeedBoostArgs.title = tr::lng_settings_alexgram_upload_speed_boost();
	uploadSpeedBoostArgs.checked = Core::App().settings().uploadSpeedBoost();
	const auto uploadSpeedBoostCb = builder.addCheckbox(std::move(uploadSpeedBoostArgs));
	uploadSpeedBoostCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setUploadSpeedBoost(checked);
		Core::App().saveSettingsDelayed();
	}, uploadSpeedBoostCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_upload_speed_boost_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs downloadSpeedBoostArgs;
	downloadSpeedBoostArgs.title = tr::lng_settings_alexgram_download_speed_boost();
	downloadSpeedBoostArgs.checked = Core::App().settings().downloadSpeedBoost();
	const auto downloadSpeedBoostCb = builder.addCheckbox(std::move(downloadSpeedBoostArgs));
	downloadSpeedBoostCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setDownloadSpeedBoost(checked);
		Core::App().saveSettingsDelayed();
	}, downloadSpeedBoostCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_download_speed_boost_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs typingInsteadStickerArgs;
	typingInsteadStickerArgs.title = tr::lng_settings_alexgram_send_typing_instead_sticker();
	typingInsteadStickerArgs.checked = Core::App().settings().sendTypingInsteadOfSticker();
	const auto typingInsteadStickerCb = builder.addCheckbox(std::move(typingInsteadStickerArgs));
	typingInsteadStickerCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setSendTypingInsteadOfSticker(checked);
		Core::App().saveSettingsDelayed();
	}, typingInsteadStickerCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_send_typing_instead_sticker_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs hideStoriesArgs;
	hideStoriesArgs.title = tr::lng_settings_alexgram_hide_stories_from_header();
	hideStoriesArgs.checked = Core::App().settings().hideStoriesFromHeader();
	const auto hideStoriesCb = builder.addCheckbox(std::move(hideStoriesArgs));
	hideStoriesCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setHideStoriesFromHeader(checked);
		Core::App().saveSettingsDelayed();
	}, hideStoriesCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_hide_stories_from_header_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs disableStoriesArgs;
	disableStoriesArgs.title = tr::lng_settings_alexgram_disable_stories();
	disableStoriesArgs.checked = Core::App().settings().disableStories();
	const auto disableStoriesCb = builder.addCheckbox(std::move(disableStoriesArgs));
	disableStoriesCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setDisableStories(checked);
		if (checked) {
			hideStoriesCb->setChecked(true);
		}
		Core::App().saveSettingsDelayed();
	}, disableStoriesCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_disable_stories_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs sendMp4AsVideoArgs;
	sendMp4AsVideoArgs.title = tr::lng_settings_alexgram_send_mp4_as_video();
	sendMp4AsVideoArgs.checked = Core::App().settings().sendMp4AsVideo();
	const auto sendMp4AsVideoCb = builder.addCheckbox(std::move(sendMp4AsVideoArgs));
	sendMp4AsVideoCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setSendMp4AsVideo(checked);
		Core::App().saveSettingsDelayed();
	}, sendMp4AsVideoCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_send_mp4_as_video_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs dolby8DArgs;
	dolby8DArgs.title = tr::lng_settings_alexgram_dolby_8d();
	dolby8DArgs.checked = Core::App().settings().dolby8D();
	const auto dolby8DCb = builder.addCheckbox(std::move(dolby8DArgs));
	dolby8DCb->checkedChanges(
	) | rpl::on_next([=](bool checked) {
		Core::App().settings().setDolby8D(checked);
		Core::App().saveSettingsDelayed();
	}, dolby8DCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_dolby_8d_about());
	builder.addSkip();

	Ui::ResizeFitChild(this, content);
}

void AlexgramGhostMode::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto builder = Builder::SectionBuilder(Builder::WidgetContext{ content, controller(), showOtherMethod() });
	builder.addDivider();
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs noReadArgs;
	noReadArgs.title = tr::lng_settings_alexgram_ghost_mode_no_read();
	noReadArgs.checked = Core::App().settings().ghostModeNoRead();
	const auto noReadCb = builder.addCheckbox(std::move(noReadArgs));
	noReadCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setGhostModeNoRead(checked);
		Core::App().saveSettingsDelayed();
	}, noReadCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_no_read_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs readOnInteractArgs;
	readOnInteractArgs.title = tr::lng_settings_alexgram_ghost_mode_read_on_interact();
	readOnInteractArgs.checked = Core::App().settings().ghostReadOnInteract();
	const auto readOnInteractCb = builder.addCheckbox(std::move(readOnInteractArgs));
	readOnInteractCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setGhostReadOnInteract(checked);
		Core::App().saveSettingsDelayed();
	}, readOnInteractCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_read_on_interact_about());
	builder.addSkip();


	Builder::SectionBuilder::CheckboxArgs noReadStoriesArgs;
	noReadStoriesArgs.title = tr::lng_settings_alexgram_ghost_mode_no_read_stories();
	noReadStoriesArgs.checked = Core::App().settings().ghostDontReadStories();
	const auto noReadStoriesCb = builder.addCheckbox(std::move(noReadStoriesArgs));
	noReadStoriesCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setGhostDontReadStories(checked);
		Core::App().saveSettingsDelayed();
	}, noReadStoriesCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_no_read_stories_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs noOnlineArgs;
	noOnlineArgs.title = tr::lng_settings_alexgram_ghost_mode_no_online();
	noOnlineArgs.checked = Core::App().settings().ghostDontSendOnline();
	const auto noOnlineCb = builder.addCheckbox(std::move(noOnlineArgs));
	noOnlineCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setGhostDontSendOnline(checked);
		Core::App().saveSettingsDelayed();
	}, noOnlineCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_no_online_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs noTypingArgs;
	noTypingArgs.title = tr::lng_settings_alexgram_ghost_mode_no_typing();
	noTypingArgs.checked = Core::App().settings().ghostDontSendTyping();
	const auto noTypingCb = builder.addCheckbox(std::move(noTypingArgs));
	noTypingCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setGhostDontSendTyping(checked);
		Core::App().saveSettingsDelayed();
	}, noTypingCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_no_typing_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs goOfflineArgs;
	goOfflineArgs.title = tr::lng_settings_alexgram_ghost_mode_go_offline();
	goOfflineArgs.checked = Core::App().settings().ghostGoOffline();
	const auto goOfflineCb = builder.addCheckbox(std::move(goOfflineArgs));
	goOfflineCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setGhostGoOffline(checked);
		Core::App().saveSettingsDelayed();
		if (checked) {
			controller()->session().updates().updateOnline(0, true);
		}
	}, goOfflineCb->lifetime());

	builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_go_offline_about());
	builder.addSkip();

	Builder::SectionBuilder::CheckboxArgs saveDeletedArgs;
	saveDeletedArgs.title = tr::lng_settings_alexgram_ghost_mode_save_deleted();
	saveDeletedArgs.checked = Core::App().settings().ghostSaveDeletedMessages();
	const auto saveDeletedCb = builder.addCheckbox(std::move(saveDeletedArgs));
	saveDeletedCb->checkedChanges() | rpl::on_next([=](bool checked) {
		Core::App().settings().setGhostSaveDeletedMessages(checked);
		Core::App().saveSettingsDelayed();
	}, saveDeletedCb->lifetime());

	builder.scope([&] {
		builder.addSkip(st::settingsCheckboxesSkip);

		Builder::SectionBuilder::CheckboxArgs saveBotDeletedArgs;
		saveBotDeletedArgs.title = tr::lng_settings_alexgram_ghost_mode_save_bot_deleted();
		saveBotDeletedArgs.checked = Core::App().settings().ghostSaveBotDeleted();
		const auto saveBotDeletedCb = builder.addCheckbox(std::move(saveBotDeletedArgs));
		saveBotDeletedCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setGhostSaveBotDeleted(checked);
			Core::App().saveSettingsDelayed();
		}, saveBotDeletedCb->lifetime());

		builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_save_bot_deleted_about());
		builder.addSkip(st::settingsCheckboxesSkip);

		Builder::SectionBuilder::CheckboxArgs showIconArgs;
		showIconArgs.title = tr::lng_settings_alexgram_ghost_mode_deleted_show_icon();
		showIconArgs.checked = Core::App().settings().ghostDeletedShowIcon();
		const auto showIconCb = builder.addCheckbox(std::move(showIconArgs));
		showIconCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setGhostDeletedShowIcon(checked);
			Core::App().saveSettingsDelayed();
			Core::App().reprocessAlexSettings();
		}, showIconCb->lifetime());

		builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_deleted_show_icon_about());
		builder.addSkip(st::settingsCheckboxesSkip);

		Builder::SectionBuilder::CheckboxArgs translucentArgs;
		translucentArgs.title = tr::lng_settings_alexgram_ghost_mode_translucent_deleted();
		translucentArgs.checked = Core::App().settings().ghostTranslucentDeleted();
		const auto translucentCb = builder.addCheckbox(std::move(translucentArgs));
		translucentCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setGhostTranslucentDeleted(checked);
			Core::App().saveSettingsDelayed();
			Core::App().reprocessAlexSettings();
		}, translucentCb->lifetime());


		builder.scope([&] {
			builder.addSubsectionTitle(tr::lng_settings_alexgram_ghost_mode_deleted_opacity());
			if (const auto container = builder.container()) {
				const auto slider = container->add(
					object_ptr<Ui::SettingsSlider>(container, st::settingsSlider),
					st::settingsBigScalePadding);
				for (int i = 1; i <= 10; ++i) {
					slider->addSection(QString::number(i * 10) + u"%"_q);
				}
				slider->setActiveSectionFast(Core::App().settings().ghostDeletedOpacity() / 10 - 1);
				slider->sectionActivated() | rpl::on_next([=](int section) {
					Core::App().settings().setGhostDeletedOpacity((section + 1) * 10);
					Core::App().saveSettingsDelayed();
					Core::App().reprocessAlexSettings();
				}, slider->lifetime());
			}
			builder.addSkip(st::settingsCheckboxesSkip);
		}, translucentCb->checkedValue());

		builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_translucent_deleted_about());
		builder.addSkip();

	}, saveDeletedCb->checkedValue());

	builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_save_deleted_about());
	builder.addSkip();

	Ui::ResizeFitChild(this, content);
}

void BuildAlexSection(Builder::SectionBuilder &builder) {
}

Type AlexgramSectionId() {
	return SectionFactory<AlexgramMain>::Instance();
}

Type AlexgramGeneralSectionId() {
	return SectionFactory<AlexgramGeneral>::Instance();
}

Type AlexgramTranslatorSectionId() {
	return SectionFactory<AlexgramTranslator>::Instance();
}

Type AlexgramChatsSectionId() {
	return SectionFactory<AlexgramChats>::Instance();
}

Type AlexgramExperimentalSectionId() {
	return SectionFactory<AlexgramExperimental>::Instance();
}

Type AlexgramGhostModeSectionId() {
	return SectionFactory<AlexgramGhostMode>::Instance();
}

const auto kMeta = Builder::BuildHelper(Builder::SectionMeta{ AlexgramSectionId(), MainId(), &tr::lng_settings_alexgram, &st::menuIconManage }, BuildAlexSection);
const auto kGeneralMeta = Builder::BuildHelper(Builder::SectionMeta{ AlexgramGeneralSectionId(), AlexgramSectionId(), &tr::lng_settings_alexgram_general, &st::menuIconSettings }, BuildAlexSection);
const auto kTranslatorMeta = Builder::BuildHelper(Builder::SectionMeta{ AlexgramTranslatorSectionId(), AlexgramSectionId(), &tr::lng_settings_alexgram_translator, &st::menuIconTranslate }, BuildAlexSection);
const auto kChatsMeta = Builder::BuildHelper(Builder::SectionMeta{ AlexgramChatsSectionId(), AlexgramSectionId(), &tr::lng_settings_alexgram_chats, &st::menuIconChatBubble }, BuildAlexSection);
const auto kExperimentalMeta = Builder::BuildHelper(Builder::SectionMeta{ AlexgramExperimentalSectionId(), AlexgramSectionId(), &tr::lng_settings_alexgram_experimental, &st::menuIconExperimental }, BuildAlexSection);
const auto kGhostModeMeta = Builder::BuildHelper(Builder::SectionMeta{ AlexgramGhostModeSectionId(), AlexgramSectionId(), &tr::lng_settings_alexgram_ghost_mode, &st::menuIconStealth }, BuildAlexSection);

} // namespace Settings
