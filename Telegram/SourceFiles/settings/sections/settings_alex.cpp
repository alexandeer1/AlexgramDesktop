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
#include "ui/widgets/buttons.h"
#include "ui/widgets/checkbox.h"
#include "ui/widgets/color_editor.h"
#include "ui/widgets/labels.h"
#include "ui/ui_utility.h"
#include "ui/painter.h"
#include "ui/effects/outline_segments.h"
#include "core/application.h"
#include "core/core_settings.h"
#include "lang/lang_keys.h"
#include "styles/style_chat_helpers.h"
#include "styles/style_chat.h"
#include "styles/style_settings.h"
#include "styles/style_menu_icons.h"
#include "styles/style_widgets.h"
#include "window/window_session_controller.h"
#include "main/main_session.h"
#include <rpl/map.h>
#include <rpl/filter.h>
#include <rpl/range.h>
#include "api/api_updates.h"
#include "alex/video_downloader_manager.h"
#include "alex/video_downloader_engine.h"
#include "alex/video_downloader_window.h"
#include "ui/boxes/confirm_box.h"
#include "ui/widgets/discrete_sliders.h"
#include "alex/alex_database.h"
#include "spellcheck/spellcheck_types.h"
#include "base/algorithm.h"
#include <rpl/variable.h>
#include <algorithm>
#include "ui/layers/generic_box.h"
#include "ui/text/format_values.h"
#include "ui/painter.h"
#include "styles/style_layers.h"
#include "ui/widgets/fields/input_field.h"
#include "ui/widgets/multi_select.h"
#include "ui/boxes/choose_language_box.h"
#include "ui/widgets/continuous_sliders.h"
#include "ui/image/image_prepare.h"
#include "base/unixtime.h"
#include "core/file_utilities.h"
#include "ui/effects/animations.h"


namespace Settings {

using namespace Builder;
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


void BuildAlexgramMainSection(SectionBuilder &builder) {	builder.addDivider();
	builder.addSkip();
	
	SectionBuilder::SectionArgs generalArgs;
	generalArgs.title = tr::lng_settings_alexgram_general();
	generalArgs.targetSection = AlexgramGeneralSectionId();
	generalArgs.icon = { &st::menuIconSettings };
	builder.addSectionButton(std::move(generalArgs));

	SectionBuilder::SectionArgs translatorArgs;
	translatorArgs.title = tr::lng_settings_alexgram_translator();
	translatorArgs.targetSection = AlexgramTranslatorSectionId();
	translatorArgs.icon = { &st::menuIconTranslate };
	builder.addSectionButton(std::move(translatorArgs));

	SectionBuilder::SectionArgs chatsArgs;
	chatsArgs.title = tr::lng_settings_alexgram_chats();
	chatsArgs.targetSection = AlexgramChatsSectionId();
	chatsArgs.icon = { &st::menuIconChatBubble };
	builder.addSectionButton(std::move(chatsArgs));

	SectionBuilder::SectionArgs experimentalArgs;
	experimentalArgs.title = tr::lng_settings_alexgram_experimental();
	experimentalArgs.targetSection = AlexgramExperimentalSectionId();
	experimentalArgs.icon = { &st::menuIconExperimental };
	builder.addSectionButton(std::move(experimentalArgs));

	SectionBuilder::SectionArgs ghostModeArgs;
	ghostModeArgs.title = tr::lng_settings_alexgram_ghost_mode();
	ghostModeArgs.targetSection = AlexgramGhostModeSectionId();
	ghostModeArgs.icon = { &st::menuIconStealth };
	builder.addSectionButton(std::move(ghostModeArgs));

	builder.addButton({
		.title = tr::lng_settings_alexgram_video_downloader(),
		.icon = { &st::menuIconDownload },
		.onClick = [=] {
			Alex::VideoDownloaderWindow::Show();
		},
	});

	builder.addDividerText(tr::lng_settings_alexgram_video_downloader_about());
	builder.addSkip();
}

void BuildAlexgramGeneralSection(SectionBuilder &builder) {	builder.addDivider();
	builder.addSkip();
	
	SectionBuilder::CheckboxArgs roundArgs;
	roundArgs.title = tr::lng_settings_disable_number_rounding();
	roundArgs.checked = Core::App().settings().disableNumberRounding();
	const auto checkbox = builder.addCheckbox(std::move(roundArgs));
	if (checkbox) {
		checkbox->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setDisableNumberRounding(checked);
			Core::App().saveSettingsDelayed();
			Core::App().reprocessAlexSettings();
		}, checkbox->lifetime());
	}
	builder.addDividerText(tr::lng_settings_disable_number_rounding_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs hideTabsArgs;
	hideTabsArgs.title = tr::lng_settings_hide_all_chats_tab();
	hideTabsArgs.checked = Core::App().settings().hideAllChatsTab();
	const auto hideTabsCb = builder.addCheckbox(std::move(hideTabsArgs));
	if (hideTabsCb) {
		hideTabsCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setHideAllChatsTab(checked);
			Core::App().saveSettingsDelayed();
		}, hideTabsCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_hide_all_chats_tab_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs removeArchivedArgs;
	removeArchivedArgs.title = tr::lng_settings_remove_archived_from_list();
	removeArchivedArgs.checked = Core::App().settings().removeArchivedFromList();
	const auto removeArchivedCb = builder.addCheckbox(std::move(removeArchivedArgs));
	if (removeArchivedCb) {
		removeArchivedCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setRemoveArchivedFromList(checked);
			Core::App().saveSettingsDelayed();
		}, removeArchivedCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_remove_archived_from_list_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs hidePremiumArgs;
	hidePremiumArgs.title = tr::lng_settings_hide_premium();
	hidePremiumArgs.checked = Core::App().settings().hidePremium();
	const auto hidePremiumCb = builder.addCheckbox(std::move(hidePremiumArgs));
	if (hidePremiumCb) {
		hidePremiumCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setHidePremium(checked);
			Core::App().saveSettingsDelayed();
		}, hidePremiumCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_hide_premium_about());
	builder.addSkip();
	
	SectionBuilder::CheckboxArgs hideHelpArgs;
	hideHelpArgs.title = tr::lng_settings_hide_help();
	hideHelpArgs.checked = Core::App().settings().hideHelp();
	const auto hideHelpCb = builder.addCheckbox(std::move(hideHelpArgs));
	if (hideHelpCb) {
		hideHelpCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setHideHelp(checked);
			Core::App().saveSettingsDelayed();
		}, hideHelpCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_hide_help_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs hidePhoneArgs;
	hidePhoneArgs.title = tr::lng_settings_hide_phone_number();
	hidePhoneArgs.checked = Core::App().settings().hidePhoneNumber();
	const auto hidePhoneCb = builder.addCheckbox(std::move(hidePhoneArgs));
	if (hidePhoneCb) {
		hidePhoneCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setHidePhoneNumber(checked);
			Core::App().saveSettingsDelayed();
		}, hidePhoneCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_hide_phone_number_about());
	builder.addSkip();

}

void ChooseTranslatorProviderBox(not_null<Ui::GenericBox*> box, not_null<Window::SessionController*> controller) {
	box->setTitle(rpl::single(u"Choose Provider"_q));

	const auto group = std::make_shared<Ui::RadioenumGroup<Core::Settings::TranslatorProvider>>(
		Core::App().settings().translatorProvider());
	
	const auto add = [&](Core::Settings::TranslatorProvider provider, const QString &text) {
		box->addRow(object_ptr<Ui::Radioenum<Core::Settings::TranslatorProvider>>(
			box.get(),
			group,
			provider,
			text));
	};

	add(Core::Settings::TranslatorProvider::Telegram, u"Telegram / Official"_q);
	add(Core::Settings::TranslatorProvider::GoogleGtx, u"Google Translate (Web)"_q);
	add(Core::Settings::TranslatorProvider::GoogleAt, u"Google Translate (App)"_q);
	add(Core::Settings::TranslatorProvider::Bing, u"Bing / Microsoft Translator"_q);
	add(Core::Settings::TranslatorProvider::MicrosoftEdge, u"Microsoft Edge Translator"_q);
	add(Core::Settings::TranslatorProvider::Yandex, u"Yandex Translate"_q);
	add(Core::Settings::TranslatorProvider::Tencent, u"Tencent TranSmart"_q);
	add(Core::Settings::TranslatorProvider::Caiyun, u"Caiyun / Lingo Translator"_q);

	group->setChangedCallback([=](Core::Settings::TranslatorProvider value) {
		Core::App().settings().setTranslatorProvider(value);
		Core::App().saveSettingsDelayed();
	});

	box->addButton(tr::lng_close(), [=] { box->closeBox(); });
}

void BuildAlexgramTranslatorSection(SectionBuilder &builder) {
	builder.addDivider();
	builder.addSkip();

	SectionBuilder::CheckboxArgs showTranslateArgs;
	showTranslateArgs.title = tr::lng_settings_show_translate_button();
	showTranslateArgs.checked = Core::App().settings().showTranslateButton();
	const auto showTranslateCb = builder.addCheckbox(std::move(showTranslateArgs));
	if (showTranslateCb) {
		showTranslateCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setShowTranslateButton(checked);
			Core::App().saveSettingsDelayed();
		}, showTranslateCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_show_translate_button_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs showTranslatedIconArgs;
	showTranslatedIconArgs.title = tr::lng_settings_alexgram_show_translated_icon();
	showTranslatedIconArgs.checked = Core::App().settings().showTranslatedIcon();
	const auto showTranslatedIconCb = builder.addCheckbox(std::move(showTranslatedIconArgs));
	if (showTranslatedIconCb) {
		showTranslatedIconCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setShowTranslatedIcon(checked);
			Core::App().saveSettingsDelayed();
			Core::App().reprocessAlexSettings();
		}, showTranslatedIconCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_show_translated_icon_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs translateChatArgs;
	translateChatArgs.title = tr::lng_translate_settings_chat();
	translateChatArgs.checked = Core::App().settings().translateChatEnabled();
	const auto translateChatCb = builder.addCheckbox(std::move(translateChatArgs));
	if (translateChatCb) {
		translateChatCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setTranslateChatEnabled(checked);
			Core::App().saveSettingsDelayed();
		}, translateChatCb->lifetime());
	}

	builder.addDividerText(tr::lng_translate_settings_about());
	builder.addSkip();

	builder.addButton({
		.title = rpl::single(u"Choose Translator Provider"_q),
		.icon = { &st::menuIconTranslate },
		.onClick = [=] {
			builder.controller()->show(Box(ChooseTranslatorProviderBox, builder.controller()));
		},
	});

	builder.addDividerText(rpl::single(u"Select your preferred translation engine."_q));
	builder.addSkip();

	builder.addButton({
		.title = tr::lng_translate_menu_to(),
		.icon = { &st::menuIconTranslate },
		.label = Core::App().settings().translateToValue() | rpl::map([](LanguageId id) {
			return Ui::LanguageName(id);
		}),
		.onClick = [=] {
			builder.controller()->show(Box(
				Ui::ChooseLanguageBox,
				tr::lng_translate_menu_to(),
				[=](std::vector<LanguageId> selected) {
					if (!selected.empty()) {
						Core::App().settings().setTranslateTo(selected.front());
						Core::App().saveSettingsDelayed();
					}
				},
				std::vector<LanguageId>{ Core::App().settings().translateTo() },
				false, // multiselect
				nullptr));
		},
	});

	builder.addDividerText(rpl::single(u"Select the language you want messages to be translated into."_q));
	builder.addSkip();

	builder.addButton({
		.title = rpl::single(u"Do Not Translate"_q),
		.icon = { &st::menuIconBlock },
		.label = Core::App().settings().skipTranslationLanguagesValue() | rpl::map([](const std::vector<LanguageId> &skip) {
			return skip.empty() ? QString() : QString::number(skip.size());
		}),
		.onClick = [=] {
			builder.controller()->show(Box(
				Ui::ChooseLanguageBox,
				rpl::single(u"Do Not Translate"_q),
				[=](std::vector<LanguageId> selected) {
					Core::App().settings().setSkipTranslationLanguages(selected);
					Core::App().saveSettingsDelayed();
				},
				Core::App().settings().skipTranslationLanguages(),
				true, // multiselect
				nullptr));
		},
	});

	builder.addDividerText(rpl::single(u"Languages that will not be offered for translation."_q));
	builder.addSkip();
}

void BuildAlexgramChatsSection(SectionBuilder &builder) {	builder.addDivider();
	builder.addSkip();

	if (const auto container = builder.container()) {
		const auto kDefault = 50;
		const auto kMax = 50;
		const auto currentRadius = Core::App().settings().dialogAvatarCornerRadius();

		const auto radiusValue = std::make_shared<rpl::variable<int>>(currentRadius);

		const auto applyRadius = [=](int radius) {
			*radiusValue = radius;
			Core::App().settings().setDialogAvatarCornerRadius(radius);
			Core::App().reprocessAlexSettings();
		};

		const auto headerRow = container->add(
			object_ptr<Ui::RpWidget>(container),
			QMargins(st::settingsCheckboxPadding.left(), st::settingsCheckboxPadding.top(), st::settingsCheckboxPadding.right(), 0));
		headerRow->setFixedHeight(st::normalFont->height + st::normalFont->height / 2);

		headerRow->paintRequest() | rpl::on_next([=](QRect) {
			Painter p(headerRow);
			p.setFont(st::boxTitleFont);
			p.setPen(st::windowActiveTextFg);
			const auto title = tr::lng_settings_alexgram_avatar_corners(tr::now);
			const auto radius = radiusValue->current();
			const auto badge = QString::number(radius);
			p.drawText(0, st::boxTitleFont->ascent, title);
			const auto titleW = st::boxTitleFont->width(title);
			const auto badgeHPad = st::lineWidth * 4;
			const auto badgeRect = QRect(
				titleW + badgeHPad,
				0,
				st::normalFont->width(badge) + badgeHPad * 2,
				st::boxTitleFont->height);
			p.setRenderHint(QPainter::Antialiasing);
			p.setBrush(anim::with_alpha(st::windowActiveTextFg->c, 0.25));
			p.setPen(Qt::NoPen);
			p.drawRoundedRect(badgeRect, badgeRect.height() / 3, badgeRect.height() / 3);
			p.setPen(st::windowActiveTextFg);
			p.setFont(st::normalFont);
			p.drawText(badgeRect, Qt::AlignCenter, badge);

			const auto resetLabel = tr::lng_settings_alexgram_avatar_corners_reset(tr::now);
			const auto resetW = st::normalFont->width(resetLabel);
			const auto isDefault = (radius == kDefault);
			p.setFont(st::normalFont);
			p.setPen(isDefault
				? anim::with_alpha(st::windowSubTextFg->c, 0.4)
				: st::windowActiveTextFg->c);
			p.drawText(headerRow->width() - resetW, st::normalFont->ascent + (st::boxTitleFont->height - st::normalFont->height) / 2, resetLabel);
		}, headerRow->lifetime());

		radiusValue->value() | rpl::on_next([=](int) {
			headerRow->update();
		}, headerRow->lifetime());

		headerRow->setCursor(Qt::PointingHandCursor);
		headerRow->events() | rpl::filter([](not_null<QEvent*> e) {
			return e->type() == QEvent::MouseButtonRelease;
		}) | rpl::on_next([=](not_null<QEvent*> e) {
			const auto me = static_cast<QMouseEvent*>(e.get());
			const auto resetLabel = tr::lng_settings_alexgram_avatar_corners_reset(tr::now);
			const auto resetW = st::normalFont->width(resetLabel);
			if (me->pos().x() >= headerRow->width() - resetW - st::lineWidth * 2) {
				applyRadius(kDefault);
				Core::App().saveSettingsDelayed();
			}
		}, headerRow->lifetime());

		const auto labelsRow = container->add(
			object_ptr<Ui::RpWidget>(container),
			QMargins(st::settingsCheckboxPadding.left(), st::lineWidth * 2, st::settingsCheckboxPadding.right(), 0));
		labelsRow->setFixedHeight(st::normalFont->height);
		labelsRow->paintRequest() | rpl::on_next([=](QRect) {
			Painter p(labelsRow);
			p.setFont(st::normalFont);
			p.setPen(st::windowSubTextFg);
			p.drawText(0, st::normalFont->ascent, tr::lng_settings_alexgram_avatar_corners_sharp(tr::now));
			const auto roundLabel = tr::lng_settings_alexgram_avatar_corners_round(tr::now);
			p.drawText(labelsRow->width() - st::normalFont->width(roundLabel), st::normalFont->ascent, roundLabel);
		}, labelsRow->lifetime());

		const auto slider = container->add(
			object_ptr<Ui::MediaSlider>(container, st::settingsScale),
			st::settingsBigScalePadding);
		const auto sliderIsMoving = std::make_shared<bool>(false);
		slider->setFixedHeight(st::settingsScale.seekSize.height());
		slider->setAlwaysDisplayMarker(true);
		slider->setDirection(Ui::ContinuousSlider::Direction::Horizontal);
		slider->setValue(currentRadius / float64(kMax));
		slider->setChangeProgressCallback([=](float64 value) {
			*sliderIsMoving = true;
			const auto radius = int(base::SafeRound(value * kMax));
			applyRadius(radius);
			*sliderIsMoving = false;
		});
		slider->setChangeFinishedCallback([=](float64 value) {
			*sliderIsMoving = true;
			const auto radius = int(base::SafeRound(value * kMax));
			applyRadius(radius);
			Core::App().saveSettingsDelayed();
			*sliderIsMoving = false;
		});

		radiusValue->value() | rpl::on_next([=](int r) {
			if (!*sliderIsMoving) {
				slider->setValue(r / float64(kMax));
			}
		}, slider->lifetime());

		const auto previewWidget = container->add(
			object_ptr<Ui::RpWidget>(container),
			QMargins(st::settingsCheckboxPadding.left(), st::lineWidth * 4, st::settingsCheckboxPadding.right(), st::lineWidth * 4));
		const auto previewH = st::settingsButton.height * 4 + st::lineWidth * 4;
		previewWidget->setFixedHeight(previewH);

		previewWidget->paintRequest() | rpl::on_next([=](QRect) {
			Painter p(previewWidget);
			p.setRenderHint(QPainter::Antialiasing);
			p.setRenderHint(QPainter::SmoothPixmapTransform);

			const auto w = previewWidget->width();
			const auto h = previewWidget->height();

			// Frosted card background
			{
				auto bg = anim::with_alpha(st::windowBgOver->c, 0.55);
				p.setPen(Qt::NoPen);
				p.setBrush(bg);
				p.drawRoundedRect(QRectF(0, 0, w, h), st::boxRadius, st::boxRadius);
				// subtle inner border
				p.setBrush(Qt::NoBrush);
				p.setPen(QPen(anim::with_alpha(st::windowFg->c, 0.06), st::lineWidth));
				p.drawRoundedRect(QRectF(0.5, 0.5, w - 1, h - 1), st::boxRadius, st::boxRadius);
			}

			const auto radius = radiusValue->current();
			const auto rowCount = 3;
			const auto rowH = h / rowCount;
			const auto pad = st::lineWidth * 5;
			const auto avatarSize = rowH - pad * 2;
			const auto halfAv = avatarSize / 2;
			const auto cornerR = (radius <= 0)
				? 0
				: (radius >= 50)
				? halfAv
				: radius * halfAv / 50;

			// row data: gradient colors, has-story, story-fraction, has-online, has-badge
			struct RowData {
				QColor c1, c2;
				bool hasStory;
				float64 storyRead; // 0 = all unread, 1 = all read
				bool hasOnline;
				int badge;
				QString time;
				float64 nameW;
				float64 msgW;
			};
			const RowData rows[rowCount] = {
				{ QColor(0,200,180), QColor(30,120,255), true,  0.35, false, 0,  u"12:34"_q, 0.60, 0.80 },
				{ QColor(160,60,220), QColor(255,80,140), false, 0.,   true,  0,  u"11:59"_q, 0.45, 0.65 },
				{ QColor(255,140,0),  QColor(230,60,60),  false, 0.,   false, 0,  u"Mon"_q,   0.50, 0.50 },
			};

			for (auto row = 0; row < rowCount; ++row) {
				const auto &rd = rows[row];
				const auto rowY = row * rowH;
				const auto avatarX = pad;
				const auto avatarY = rowY + pad;
				const auto avRect = QRectF(avatarX, avatarY, avatarSize, avatarSize);

				// Divider (except first row)
				if (row > 0) {
					p.setPen(QPen(anim::with_alpha(st::windowFg->c, 0.06), st::lineWidth));
					p.drawLine(pad + avatarSize + pad, rowY, w - pad, rowY);
				}

				// Story ring (first row only)
				if (rd.hasStory) {
					const auto ringW = st::lineWidth * 2.0;
					const auto ringAdd = ringW + st::lineWidth * 1.5;
					const auto ringRect = avRect.marginsAdded(
						QMarginsF(ringAdd, ringAdd, ringAdd, ringAdd));
					const auto ringR = cornerR + ringAdd;

					// unread gradient segment
					auto gradient = QLinearGradient(ringRect.topRight(), ringRect.bottomLeft());
					gradient.setColorAt(0, QColor(50, 220, 170));
					gradient.setColorAt(1, QColor(30, 120, 255));

					const auto unreadFrac = 1. - rd.storyRead;
					const auto readFrac   = rd.storyRead;

					std::vector<Ui::OutlineSegment> segs;
					if (unreadFrac > 0.01) {
						segs.push_back({ QBrush(gradient), ringW });
					}
					if (readFrac > 0.01) {
						segs.push_back({ QBrush(anim::with_alpha(st::windowFg->c, 0.25)), ringW });
					}
					if (!segs.empty()) {
						auto hq = PainterHighQualityEnabler(p);
						if (cornerR >= halfAv) {
							Ui::PaintOutlineSegments(p, ringRect, segs);
						} else {
							Ui::PaintOutlineSegments(p, ringRect, ringR, segs);
						}
					}
				}

				// Avatar gradient fill
				{
					auto grad = QLinearGradient(avRect.topLeft(), avRect.bottomRight());
					grad.setColorAt(0, rd.c1);
					grad.setColorAt(1, rd.c2);
					p.setPen(Qt::NoPen);
					p.setBrush(QBrush(grad));
					auto hq = PainterHighQualityEnabler(p);
					if (cornerR >= halfAv) {
						p.drawEllipse(avRect);
					} else if (cornerR <= 0) {
						p.drawRect(avRect);
					} else {
						p.drawRoundedRect(avRect, cornerR, cornerR);
					}
				}

				// Online dot
				if (rd.hasOnline) {
					const auto dotR = st::lineWidth * 3;
					const auto dotRect = QRectF(
						avRect.right() - dotR * 2 + st::lineWidth,
						avRect.bottom() - dotR * 2 + st::lineWidth,
						dotR * 2, dotR * 2);
					// White border
					p.setBrush(st::windowBgOver);
					p.setPen(Qt::NoPen);
					p.drawEllipse(dotRect.marginsAdded(QMarginsF(1.5, 1.5, 1.5, 1.5)));
					// Green dot
					p.setBrush(QColor(68, 213, 120));
					p.drawEllipse(dotRect);
				}

				// Text area
				const auto textX = pad + avatarSize + pad;
				const auto textW = w - textX - pad;
				const auto lineH = st::normalFont->height;
				const auto nameY = avatarY + lineH * 0.1;
				const auto msgY  = nameY + lineH + st::lineWidth * 2;

				// Name line
				p.setOpacity(0.85);
				p.setBrush(st::windowFg);
				p.setPen(Qt::NoPen);
				p.drawRoundedRect(QRectF(textX, nameY, textW * rd.nameW, lineH - 2), 3, 3);

				// Message line
				p.setOpacity(0.45);
				p.setBrush(st::windowSubTextFg);
				p.drawRoundedRect(QRectF(textX, msgY, textW * rd.msgW, lineH - 3), 3, 3);
				p.setOpacity(1.0);

				// Time
				const auto timeW = st::normalFont->width(rd.time);
				p.setFont(st::normalFont);
				p.setPen(anim::with_alpha(st::windowSubTextFg->c, 0.75));
				p.drawText(
					w - pad - timeW,
					int(nameY) + st::normalFont->ascent,
					rd.time);

				// Unread badge
				if (rd.badge > 0) {
					const auto badgeStr = QString::number(rd.badge);
					const auto badgeFont = st::normalFont;
					const auto badgeTW = badgeFont->width(badgeStr);
					const auto badgeH = lineH;
					const auto badgeW = std::max(badgeH, badgeTW + st::lineWidth * 4);
					const auto badgeX = w - pad - badgeW;
					const auto badgeY = int(msgY);
					auto bgBadge = QColor(30, 120, 255);
					p.setBrush(bgBadge);
					p.setPen(Qt::NoPen);
					p.drawRoundedRect(QRectF(badgeX, badgeY, badgeW, badgeH), badgeH / 2., badgeH / 2.);
					p.setFont(badgeFont);
					p.setPen(Qt::white);
					p.drawText(QRect(badgeX, badgeY, badgeW, badgeH), Qt::AlignCenter, badgeStr);
				}
			}
		}, previewWidget->lifetime());

		radiusValue->value() | rpl::on_next([=](int) {
			previewWidget->update();
		}, previewWidget->lifetime());

		const auto presetsRow = container->add(
			object_ptr<Ui::RpWidget>(container),
			QMargins(st::settingsCheckboxPadding.left(), st::lineWidth * 3, st::settingsCheckboxPadding.right(), st::lineWidth * 2));
		const auto presetBtnH = st::normalFont->height + st::lineWidth * 6;
		presetsRow->setFixedHeight(presetBtnH);

		struct Preset { QString label; int value; };
		const auto presets = std::vector<Preset>{
			{ tr::lng_settings_alexgram_avatar_corners_preset_sharp(tr::now), 0 },
			{ tr::lng_settings_alexgram_avatar_corners_preset_rounded(tr::now), 15 },
			{ tr::lng_settings_alexgram_avatar_corners_preset_circle(tr::now), 50 },
		};

		presetsRow->paintRequest() | rpl::on_next([=](QRect) {
			Painter p(presetsRow);
			p.setRenderHint(QPainter::Antialiasing);
			const auto w = presetsRow->width();
			const auto h = presetsRow->height();
			const auto gap = st::lineWidth * 3;
			const auto btnW = (w - gap * 2) / 3;
			const auto currentR = radiusValue->current();

			for (auto i = 0; i < int(presets.size()); ++i) {
				const auto &pr = presets[i];
				const auto bx = i * (btnW + gap);
				const auto rect = QRect(bx, 0, btnW, h);
				const auto active = (currentR == pr.value);

				if (active) {
					p.setBrush(anim::with_alpha(st::windowActiveTextFg->c, 0.18));
					p.setPen(QPen(st::windowActiveTextFg->c, st::lineWidth));
				} else {
					p.setBrush(anim::with_alpha(st::windowBgOver->c, 0.8));
					p.setPen(QPen(anim::with_alpha(st::windowSubTextFg->c, 0.4), st::lineWidth));
				}
				p.drawRoundedRect(rect, st::lineWidth * 3, st::lineWidth * 3);

				p.setFont(st::normalFont);
				p.setPen(active ? st::windowActiveTextFg : st::windowSubTextFg);
				p.drawText(rect, Qt::AlignCenter, pr.label);
			}
		}, presetsRow->lifetime());

		radiusValue->value() | rpl::on_next([=](int) {
			presetsRow->update();
		}, presetsRow->lifetime());

		presetsRow->setCursor(Qt::PointingHandCursor);
		presetsRow->events() | rpl::filter([](not_null<QEvent*> e) {
			return e->type() == QEvent::MouseButtonRelease;
		}) | rpl::on_next([=](not_null<QEvent*> e) {
			const auto me = static_cast<QMouseEvent*>(e.get());
			const auto w = presetsRow->width();
			const auto gap = st::lineWidth * 3;
			const auto btnW = (w - gap * 2) / 3;
			const auto x = me->pos().x();
			for (auto i = 0; i < int(presets.size()); ++i) {
				const auto bx = i * (btnW + gap);
				if (x >= bx && x < bx + btnW) {
					applyRadius(presets[i].value);
					Core::App().saveSettingsDelayed();
					break;
				}
			}
		}, presetsRow->lifetime());

		builder.addSkip(st::settingsCheckboxesSkip / 2);

		SectionBuilder::CheckboxArgs unifiedArgs;
		unifiedArgs.title = tr::lng_settings_alexgram_avatar_corners_unified();
		unifiedArgs.checked = Core::App().settings().dialogUnifiedAvatarCorner();
		const auto unifiedCb = builder.addCheckbox(std::move(unifiedArgs));
		if (unifiedCb) {
			unifiedCb->checkedChanges() | rpl::on_next([=](bool checked) {
				Core::App().settings().setDialogUnifiedAvatarCorner(checked);
				Core::App().saveSettingsDelayed();
				Core::App().reprocessAlexSettings();
			}, unifiedCb->lifetime());
		}

		builder.addDividerText(tr::lng_settings_alexgram_avatar_corners_unified_about());
		builder.addSkip();
	}

	SectionBuilder::CheckboxArgs linkPreviewArgs;
	linkPreviewArgs.title = tr::lng_settings_alexgram_disable_link_preview();
	linkPreviewArgs.checked = Core::App().settings().disableLinkPreviewByDefault();
	const auto linkPreviewCb = builder.addCheckbox(std::move(linkPreviewArgs));
	if (linkPreviewCb) {
		linkPreviewCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setDisableLinkPreviewByDefault(checked);
			Core::App().saveSettingsDelayed();
		}, linkPreviewCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_disable_link_preview_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs showMsgIdArgs;
	showMsgIdArgs.title = tr::lng_settings_alexgram_show_message_id();
	showMsgIdArgs.checked = Core::App().settings().showMessageId();
	const auto showMsgIdCb = builder.addCheckbox(std::move(showMsgIdArgs));
	if (showMsgIdCb) {
		showMsgIdCb->checkedChanges(
		) | rpl::filter([=](bool checked) {
			return (checked != Core::App().settings().showMessageId());
		}) | rpl::on_next([=](bool checked) {
			Core::App().settings().setShowMessageId(checked);
			Core::App().saveSettingsDelayed();
			Core::App().reprocessAlexSettings();
		}, showMsgIdCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_show_message_id_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs showSecArgs;
	showSecArgs.title = tr::lng_settings_alexgram_show_timestamp_seconds();
	showSecArgs.checked = Core::App().settings().showTimestampSeconds();
	const auto showSecCb = builder.addCheckbox(std::move(showSecArgs));
	if (showSecCb) {
		showSecCb->checkedChanges(
		) | rpl::filter([=](bool checked) {
			return (checked != Core::App().settings().showTimestampSeconds());
		}) | rpl::on_next([=](bool checked) {
			Core::App().settings().setShowTimestampSeconds(checked);
			Core::App().saveSettingsDelayed();
			Core::App().reprocessAlexSettings();
		}, showSecCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_show_timestamp_seconds_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs showEditedIconArgs;
	showEditedIconArgs.title = tr::lng_settings_alexgram_show_edited_icon();
	showEditedIconArgs.checked = Core::App().settings().showEditedIcon();
	const auto showEditedIconCb = builder.addCheckbox(std::move(showEditedIconArgs));
	if (showEditedIconCb) {
		showEditedIconCb->checkedChanges(
		) | rpl::filter([=](bool checked) {
			return (checked != Core::App().settings().showEditedIcon());
		}) | rpl::on_next([=](bool checked) {
			Core::App().settings().setShowEditedIcon(checked);
			Core::App().saveSettingsDelayed();
			Core::App().reprocessAlexSettings();
		}, showEditedIconCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_show_edited_icon_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs showForwardedDateArgs;
	showForwardedDateArgs.title = tr::lng_settings_alexgram_show_forwarded_date();
	showForwardedDateArgs.checked = Core::App().settings().showForwardedDate();
	const auto showForwardedDateCb = builder.addCheckbox(std::move(showForwardedDateArgs));
	if (showForwardedDateCb) {
		showForwardedDateCb->checkedChanges(
		) | rpl::filter([=](bool checked) {
			return (checked != Core::App().settings().showForwardedDate());
		}) | rpl::on_next([=](bool checked) {
			Core::App().settings().setShowForwardedDate(checked);
			Core::App().saveSettingsDelayed();
			Core::App().reprocessAlexSettings();
		}, showForwardedDateCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_show_forwarded_date_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs showOnlineStatusArgs;
	showOnlineStatusArgs.title = tr::lng_settings_alexgram_show_online_status();
	showOnlineStatusArgs.checked = Core::App().settings().showOnlineStatus();
	const auto showOnlineStatusCb = builder.addCheckbox(std::move(showOnlineStatusArgs));
	if (showOnlineStatusCb) {
		showOnlineStatusCb->checkedChanges(
		) | rpl::filter([=](bool checked) {
			return (checked != Core::App().settings().showOnlineStatus());
		}) | rpl::on_next([=](bool checked) {
			Core::App().settings().setShowOnlineStatus(checked);
			Core::App().saveSettingsDelayed();
			Core::App().reprocessAlexSettings();
		}, showOnlineStatusCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_show_online_status_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs showSmallGifsArgs;
	showSmallGifsArgs.title = tr::lng_settings_alexgram_show_small_gifs();
	showSmallGifsArgs.checked = Core::App().settings().showSmallGifs();
	const auto showSmallGifsCb = builder.addCheckbox(std::move(showSmallGifsArgs));
	if (showSmallGifsCb) {
		showSmallGifsCb->checkedChanges(
		) | rpl::filter([=](bool checked) {
			return (checked != Core::App().settings().showSmallGifs());
		}) | rpl::on_next([=](bool checked) {
			Core::App().settings().setShowSmallGifs(checked);
			Core::App().saveSettingsDelayed();
			Core::App().reprocessAlexSettings();
		}, showSmallGifsCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_show_small_gifs_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs treatGifsAsVideosArgs;
	treatGifsAsVideosArgs.title = tr::lng_settings_alexgram_treat_gifs_as_videos();
	treatGifsAsVideosArgs.checked = Core::App().settings().treatGifsAsVideos();
	const auto treatGifsAsVideosCb = builder.addCheckbox(std::move(treatGifsAsVideosArgs));
	if (treatGifsAsVideosCb) {
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
			builder.controller()->show(Ui::MakeConfirmBox({
				.text = tr::lng_settings_need_restart(),
				.confirmed = confirmed,
				.cancelled = cancelled,
				.confirmText = tr::lng_settings_restart_now(),
			}));
		}, treatGifsAsVideosCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_treat_gifs_as_videos_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs autoPauseVideoArgs;
	autoPauseVideoArgs.title = tr::lng_settings_alexgram_auto_pause_video();
	autoPauseVideoArgs.checked = Core::App().settings().autoPauseVideo();
	const auto autoPauseVideoCb = builder.addCheckbox(std::move(autoPauseVideoArgs));
	if (autoPauseVideoCb) {
		autoPauseVideoCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setAutoPauseVideo(checked);
			Core::App().saveSettingsDelayed();
		}, autoPauseVideoCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_auto_pause_video_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs noAutoPlayNextVoiceArgs;
	noAutoPlayNextVoiceArgs.title = tr::lng_settings_alexgram_no_autoplay_next_voice();
	noAutoPlayNextVoiceArgs.checked = Core::App().settings().noAutoPlayNextVoice();
	const auto noAutoPlayNextVoiceCb = builder.addCheckbox(std::move(noAutoPlayNextVoiceArgs));
	if (noAutoPlayNextVoiceCb) {
		noAutoPlayNextVoiceCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setNoAutoPlayNextVoice(checked);
			Core::App().saveSettingsDelayed();
		}, noAutoPlayNextVoiceCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_no_autoplay_next_voice_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs showSpoilersArgs;
	showSpoilersArgs.title = tr::lng_settings_alexgram_show_spoilers_directly();
	showSpoilersArgs.checked = Core::App().settings().showSpoilersDirectly();
	const auto showSpoilersCb = builder.addCheckbox(std::move(showSpoilersArgs));
	if (showSpoilersCb) {
		showSpoilersCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setShowSpoilersDirectly(checked);
			Core::App().saveSettingsDelayed();
			Core::App().reprocessAlexSettings();
		}, showSpoilersCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_show_spoilers_directly_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs hideGroupStickersArgs;
	hideGroupStickersArgs.title = tr::lng_settings_alexgram_hide_group_stickers();
	hideGroupStickersArgs.checked = Core::App().settings().hideGroupStickers();
	const auto hideGroupStickersCb = builder.addCheckbox(std::move(hideGroupStickersArgs));
	if (hideGroupStickersCb) {
		hideGroupStickersCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setHideGroupStickers(checked);
			Core::App().saveSettingsDelayed();
		}, hideGroupStickersCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_hide_group_stickers_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs unlimitedRecentStickersArgs;
	unlimitedRecentStickersArgs.title = tr::lng_settings_alexgram_unlimited_recent_stickers();
	unlimitedRecentStickersArgs.checked = Core::App().settings().unlimitedRecentStickers();
	const auto unlimitedRecentStickersCb = builder.addCheckbox(std::move(unlimitedRecentStickersArgs));
	if (unlimitedRecentStickersCb) {
		unlimitedRecentStickersCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setUnlimitedRecentStickers(checked);
			Core::App().saveSettingsDelayed();
		}, unlimitedRecentStickersCb->lifetime());
	}

	builder.scope([&] {
		builder.addSkip(st::settingsSendTypeSkip);
		const auto group = std::make_shared<Ui::RadiobuttonGroup>(
			Core::App().settings().maxRecentStickers());

		const auto addOption = [&](int value) {
			if (const auto container = builder.container()) {
				container->add(
					object_ptr<Ui::Radiobutton>(
						container,
						group,
						value,
						QString::number(value),
						st::settingsSendType),
					st::settingsSendTypePadding);
			}
		};

		for (const auto value : { 40, 60, 80, 100, 120, 150, 200 }) {
			addOption(value);
		}

		group->setChangedCallback([=](int value) {
			Core::App().settings().setMaxRecentStickers(value);
			Core::App().saveSettingsDelayed();
		});
		builder.addSkip(st::settingsSendTypeSkip);
	}, unlimitedRecentStickersCb ? unlimitedRecentStickersCb->checkedValue() : rpl::single(false));

	builder.addDividerText(tr::lng_settings_alexgram_unlimited_recent_stickers_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs hideSideShareButtonArgs;
	hideSideShareButtonArgs.title = tr::lng_settings_alexgram_hide_side_share_button();
	hideSideShareButtonArgs.checked = Core::App().settings().hideSideShareButton();
	const auto hideSideShareButtonCb = builder.addCheckbox(std::move(hideSideShareButtonArgs));
	if (hideSideShareButtonCb) {
		hideSideShareButtonCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setHideSideShareButton(checked);
			Core::App().saveSettingsDelayed();
			Core::App().reprocessAlexSettings();
		}, hideSideShareButtonCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_hide_side_share_button_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs hideSendAsButtonArgs;
	hideSendAsButtonArgs.title = tr::lng_settings_alexgram_hide_send_as_button();
	hideSendAsButtonArgs.checked = Core::App().settings().hideSendAsButton();
	const auto hideSendAsButtonCb = builder.addCheckbox(std::move(hideSendAsButtonArgs));
	if (hideSendAsButtonCb) {
		hideSendAsButtonCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setHideSendAsButton(checked);
			Core::App().saveSettingsDelayed();
		}, hideSendAsButtonCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_hide_send_as_button_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs hideChannelBottomBarArgs;
	hideChannelBottomBarArgs.title = tr::lng_settings_alexgram_hide_channel_bottom_bar();
	hideChannelBottomBarArgs.checked = Core::App().settings().hideChannelBottomBar();
	const auto hideChannelBottomBarCb = builder.addCheckbox(std::move(hideChannelBottomBarArgs));
	if (hideChannelBottomBarCb) {
		hideChannelBottomBarCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setHideChannelBottomBar(checked);
			Core::App().saveSettingsDelayed();
		}, hideChannelBottomBarCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_hide_channel_bottom_bar_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs askBeforeLinkArgs;
	askBeforeLinkArgs.title = tr::lng_settings_alexgram_ask_before_link();
	askBeforeLinkArgs.checked = Core::App().settings().askBeforeLink();
	const auto askBeforeLinkCb = builder.addCheckbox(std::move(askBeforeLinkArgs));
	if (askBeforeLinkCb) {
		askBeforeLinkCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setAskBeforeLink(checked);
			Core::App().saveSettingsDelayed();
		}, askBeforeLinkCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_ask_before_link_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs askBeforeInlineLinkArgs;
	askBeforeInlineLinkArgs.title = tr::lng_settings_alexgram_ask_before_inline_link();
	askBeforeInlineLinkArgs.checked = Core::App().settings().askBeforeInlineLink();
	const auto askBeforeInlineLinkCb = builder.addCheckbox(std::move(askBeforeInlineLinkArgs));
	if (askBeforeInlineLinkCb) {
		askBeforeInlineLinkCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setAskBeforeInlineLink(checked);
			Core::App().saveSettingsDelayed();
		}, askBeforeInlineLinkCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_ask_before_inline_link_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs askBeforeCallingArgs;
	askBeforeCallingArgs.title = tr::lng_settings_alexgram_ask_before_calling();
	askBeforeCallingArgs.checked = Core::App().settings().askBeforeCalling();
	const auto askBeforeCallingCb = builder.addCheckbox(std::move(askBeforeCallingArgs));
	if (askBeforeCallingCb) {
		askBeforeCallingCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setAskBeforeCalling(checked);
			Core::App().saveSettingsDelayed();
		}, askBeforeCallingCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_ask_before_calling_about());
	builder.addSkip();

	builder.addDivider();
	builder.addSkip();

	if (const auto container = builder.container()) {
		const auto pathValue = std::make_shared<rpl::variable<QString>>(
			Core::App().settings().liveWallpaperPath());
		const auto blurValue = std::make_shared<rpl::variable<int>>(
			Core::App().settings().liveWallpaperBlur());
		const auto enabledValue = std::make_shared<rpl::variable<bool>>(
			Core::App().settings().liveWallpaperEnabled());

		const auto kBlurMax = 100;

		// --- GOD LEVEL LIVE VIDEO WALLPAPER ---
		
		SectionBuilder::CheckboxArgs liveWallpaperArgs;
		liveWallpaperArgs.title = tr::lng_settings_alexgram_live_wallpaper();
		liveWallpaperArgs.checked = Core::App().settings().liveWallpaperEnabled();
		const auto enableWallpaperCb = builder.addCheckbox(std::move(liveWallpaperArgs));
		
		if (enableWallpaperCb) {
			enableWallpaperCb->checkedChanges(
			) | rpl::on_next([=](bool checked) {
				*enabledValue = checked;
				Core::App().settings().setLiveWallpaperEnabled(checked);
				Core::App().saveSettingsDelayed();
				Core::App().reprocessAlexSettings();
			}, enableWallpaperCb->lifetime());
		}

		// Content SlideWrap
		const auto contentWrap = container->add(
			object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
				container,
				object_ptr<Ui::VerticalLayout>(container)));
		contentWrap->toggleOn(enableWallpaperCb ? enableWallpaperCb->checkedValue() : rpl::single(false));
		const auto content = contentWrap->entity();

		const auto wrapper = content->add(
			object_ptr<Ui::VerticalLayout>(content),
			QMargins(st::settingsCheckboxPadding.left(), 0, st::settingsCheckboxPadding.right(), st::lineWidth * 4));

		wrapper->paintRequest() | rpl::on_next([=](QRect clip) {
			Painter p(wrapper);
			p.setRenderHint(QPainter::Antialiasing);
			const auto r = wrapper->rect();
			
			auto bg = QLinearGradient(r.topLeft(), r.bottomRight());
			bg.setColorAt(0, anim::with_alpha(QColor(30, 35, 45), 0.8));
			bg.setColorAt(1, anim::with_alpha(QColor(20, 22, 30), 0.8));
			p.setPen(Qt::NoPen);
			p.setBrush(bg);
			p.drawRoundedRect(r, st::boxRadius * 2, st::boxRadius * 2);

			auto border = QLinearGradient(r.topLeft(), r.bottomRight());
			border.setColorAt(0, QColor(0, 255, 200, 150));
			border.setColorAt(0.5, QColor(0, 150, 255, 150));
			border.setColorAt(1, QColor(200, 0, 255, 150));
			
			p.setBrush(Qt::NoBrush);
			p.setPen(QPen(QBrush(border), 1.5));
			p.drawRoundedRect(QRectF(r).adjusted(0.75, 0.75, -0.75, -0.75), st::boxRadius * 2, st::boxRadius * 2);
		}, wrapper->lifetime());

		// 2. Beautiful Preview
		const auto previewWidget = wrapper->add(
			object_ptr<Ui::RpWidget>(wrapper),
			QMargins(st::lineWidth * 4, st::lineWidth * 4, st::lineWidth * 4, st::lineWidth * 2));
		const auto previewH = st::settingsButton.height * 3.2; // Increased from 2.8
		previewWidget->setFixedHeight(previewH);

		previewWidget->paintRequest() | rpl::on_next([=](QRect) {
			Painter p(previewWidget);
			p.setRenderHint(QPainter::Antialiasing);
			p.setRenderHint(QPainter::SmoothPixmapTransform);
			const auto w = previewWidget->width();
			const auto h = previewWidget->height();
			const auto path = pathValue->current();
			const auto blur = blurValue->current();

			if (!path.isEmpty()) {
				const auto isVideo = path.endsWith(u".mp4"_q, Qt::CaseInsensitive)
					|| path.endsWith(u".mov"_q, Qt::CaseInsensitive)
					|| path.endsWith(u".avi"_q, Qt::CaseInsensitive);

				// Draw large pill background
				p.setPen(Qt::NoPen);
				p.setBrush(anim::with_alpha(QColor(10, 15, 25), 0.6));
				p.drawRoundedRect(QRectF(0, 0, w, h), st::boxRadius * 1.5, st::boxRadius * 1.5);

				auto glowGrad = QRadialGradient(w / 2., h / 2., h * 0.8);
				glowGrad.setColorAt(0, isVideo
					? anim::with_alpha(QColor(100, 200, 255), 0.3)
					: anim::with_alpha(QColor(255, 210, 90), 0.3));
				glowGrad.setColorAt(1, Qt::transparent);
				p.setPen(Qt::NoPen);
				p.setBrush(QBrush(glowGrad));
				p.drawRoundedRect(QRectF(0, 0, w, h), st::boxRadius * 1.5, st::boxRadius * 1.5);

				const auto iconY = h * 0.25;
				p.setFont(st::boxTitleFont);
				p.setPen(Qt::white);
				const auto label = isVideo ? u"Video Wallpaper Active"_q : u"Image Wallpaper Active"_q;
				p.drawText(QRect(0, iconY, w, st::boxTitleFont->height), Qt::AlignCenter, label);

				const auto pad = st::lineWidth * 12; // Increased padding from bottom
				const auto textY = h - pad - st::normalFont->height;
				
				// Filename on the left
				p.setFont(st::normalFont);
				p.setPen(anim::with_alpha(Qt::white, 0.7));
				const auto fileName = QFileInfo(path).fileName();
				const auto nameW = w - pad * 3 - (blur > 0 ? st::normalFont->width(u"Blur: 100%"_q) : 0);
				const auto nameStr = st::normalFont->elided(fileName, nameW);
				p.drawText(QRect(pad, textY, nameW, st::normalFont->height), nameStr);

				// Blur text on the right
				if (blur > 0) {
					const auto blurStr = u"Blur: %1%"_q.arg(blur);
					const auto blurW = st::normalFont->width(blurStr);
					p.drawText(QRect(w - pad - blurW, textY, blurW, st::normalFont->height), blurStr);
				}
			} else {
				// Draw large pill background
				p.setPen(Qt::NoPen);
				p.setBrush(anim::with_alpha(QColor(10, 15, 25), 0.6));
				p.drawRoundedRect(QRectF(0, 0, w, h), st::boxRadius * 1.5, st::boxRadius * 1.5);

				p.setFont(st::boxTitleFont);
				p.setPen(anim::with_alpha(Qt::white, 0.85));
				const auto txt = u"No File Selected"_q;
				p.drawText(QRect(0, 0, w, h), Qt::AlignCenter, txt);
				p.setFont(st::normalFont);
				p.setPen(anim::with_alpha(Qt::white, 0.55));
				p.drawText(QRect(0, h / 2 + st::lineWidth * 4, w, st::normalFont->height), Qt::AlignCenter, u"Choose a file below to preview"_q);
			}
		}, previewWidget->lifetime());

		pathValue->value() | rpl::on_next([=](const QString &) { previewWidget->update(); }, previewWidget->lifetime());
		blurValue->value() | rpl::on_next([=](int) { previewWidget->update(); }, previewWidget->lifetime());

		// 3. Actions (Choose File / Clear Pill Buttons)
		const auto actionsRow = wrapper->add(
			object_ptr<Ui::RpWidget>(wrapper),
			QMargins(st::lineWidth * 4, 0, st::lineWidth * 4, st::lineWidth * 2));
		actionsRow->setFixedHeight(st::settingsButton.height * 0.9);
		actionsRow->setCursor(Qt::PointingHandCursor);

		struct ActionState {
			float64 hover1 = 0.;
			float64 hover2 = 0.;
		};
		const auto actionState = std::make_shared<ActionState>();
		const auto hoverAnim1 = std::make_shared<Ui::Animations::Simple>();
		const auto hoverAnim2 = std::make_shared<Ui::Animations::Simple>();

		actionsRow->paintRequest() | rpl::on_next([=](QRect) {
			Painter p(actionsRow);
			p.setRenderHint(QPainter::Antialiasing);
			const auto w = actionsRow->width();
			const auto h = actionsRow->height();
			const auto gap = st::lineWidth * 3;
			const auto btnW = (w - gap) / 2;

			// Button 1: Choose File
			auto bg1 = anim::color(anim::with_alpha(QColor(30, 144, 255), 0.15), anim::with_alpha(QColor(30, 144, 255), 0.25), actionState->hover1);
			p.setBrush(bg1);
			p.setPen(Qt::NoPen);
			p.drawRoundedRect(QRectF(0, 0, btnW, h), h / 2., h / 2.);
			p.setFont(st::semiboldFont);
			p.setPen(QColor(60, 170, 255));
			p.drawText(QRect(0, 0, btnW, h), Qt::AlignCenter, u"Choose File"_q);

			// Button 2: Clear
			auto bg2 = anim::color(anim::with_alpha(QColor(255, 60, 80), 0.1), anim::with_alpha(QColor(255, 60, 80), 0.2), actionState->hover2);
			p.setBrush(bg2);
			p.setPen(Qt::NoPen);
			p.drawRoundedRect(QRectF(btnW + gap, 0, btnW, h), h / 2., h / 2.);
			p.setPen(QColor(255, 90, 110));
			p.drawText(QRect(btnW + gap, 0, btnW, h), Qt::AlignCenter, u"Clear"_q);
		}, actionsRow->lifetime());

		actionsRow->events() | rpl::filter([=](not_null<QEvent*> e) {
			return e->type() == QEvent::MouseMove || e->type() == QEvent::Leave || e->type() == QEvent::MouseButtonRelease;
		}) | rpl::on_next([=](not_null<QEvent*> e) {
			const auto w = actionsRow->width();
			const auto gap = st::lineWidth * 3;
			const auto btnW = (w - gap) / 2;

			if (e->type() == QEvent::Leave) {
				hoverAnim1->start([=] { actionState->hover1 = hoverAnim1->value(0.); actionsRow->update(); }, actionState->hover1, 0., st::slideDuration);
				hoverAnim2->start([=] { actionState->hover2 = hoverAnim2->value(0.); actionsRow->update(); }, actionState->hover2, 0., st::slideDuration);
				return;
			}

			const auto me = static_cast<QMouseEvent*>(e.get());
			const auto x = me->pos().x();
			const auto over1 = (x < btnW);
			const auto over2 = (x > btnW + gap);

			if (e->type() == QEvent::MouseMove) {
				const auto target1 = over1 ? 1. : 0.;
				if (actionState->hover1 != target1 && !hoverAnim1->animating()) {
					hoverAnim1->start([=] { actionState->hover1 = hoverAnim1->value(target1); actionsRow->update(); }, actionState->hover1, target1, st::slideDuration);
				}
				const auto target2 = over2 ? 1. : 0.;
				if (actionState->hover2 != target2 && !hoverAnim2->animating()) {
					hoverAnim2->start([=] { actionState->hover2 = hoverAnim2->value(target2); actionsRow->update(); }, actionState->hover2, target2, st::slideDuration);
				}
			} else if (e->type() == QEvent::MouseButtonRelease) {
				if (over1) {
					const auto filter = u"Image or Video (*.jpg *.jpeg *.png *.mp4 *.mov *.avi);;"_q + FileDialog::AllFilesFilter();
					FileDialog::GetOpenPath(Core::App().getFileDialogParent(), tr::lng_settings_alexgram_live_wallpaper_choose(tr::now), filter, [=](const FileDialog::OpenResult &result) {
						if (!result.paths.isEmpty()) {
							const auto path = result.paths.front();
							*pathValue = path;
							Core::App().settings().setLiveWallpaperPath(path);
							Core::App().saveSettingsDelayed();
							Core::App().reprocessAlexSettings();
						}
					});
				} else if (over2) {
					*pathValue = QString();
					Core::App().settings().setLiveWallpaperPath(QString());
					Core::App().saveSettingsDelayed();
					Core::App().reprocessAlexSettings();
				}
			}
		}, actionsRow->lifetime());
		actionsRow->setAttribute(Qt::WA_Hover);

		// 4. Blur Slider (Custom God Level Slider)
		const auto blurRow = wrapper->add(
			object_ptr<Ui::RpWidget>(wrapper),
			QMargins(st::lineWidth * 4, st::lineWidth * 2, st::lineWidth * 4, st::lineWidth * 16)); // Increased bottom margin
		blurRow->setFixedHeight(st::settingsButton.height * 1.4);

		struct SliderState {
			bool sliding = false;
			float64 hover = 0.;
		};
		const auto sliderState = std::make_shared<SliderState>();
		const auto sliderHoverAnim = std::make_shared<Ui::Animations::Simple>();

		blurRow->paintRequest() | rpl::on_next([=](QRect) {
			Painter p(blurRow);
			p.setRenderHint(QPainter::Antialiasing);
			const auto w = blurRow->width();
			const auto h = blurRow->height();

			p.setFont(st::normalFont);
			p.setPen(st::windowSubTextFg);
			p.drawText(0, st::normalFont->ascent + st::lineWidth * 2, u"Background Blur"_q);

			const auto valStr = QString::number(blurValue->current()) + u"%"_q;
			p.setPen(st::windowActiveTextFg);
			p.setFont(st::semiboldFont);
			p.drawText(w - st::semiboldFont->width(valStr), st::normalFont->ascent + st::lineWidth * 2, valStr);

			const auto sliderH = st::lineWidth * 4;
			const auto sliderY = h - sliderH - st::lineWidth * 6; // Move up from the bottom edge
			const auto sliderW = w;

			// Track
			p.setBrush(anim::with_alpha(st::windowFg->c, 0.1));
			p.setPen(Qt::NoPen);
			p.drawRoundedRect(QRectF(0, sliderY, sliderW, sliderH), sliderH / 2., sliderH / 2.);

			// Fill
			const auto progress = blurValue->current() / float64(kBlurMax);
			auto fillGrad = QLinearGradient(0, 0, sliderW, 0);
			fillGrad.setColorAt(0, QColor(0, 200, 255));
			fillGrad.setColorAt(1, QColor(200, 0, 255));
			p.setBrush(QBrush(fillGrad));
			p.drawRoundedRect(QRectF(0, sliderY, sliderW * progress, sliderH), sliderH / 2., sliderH / 2.);

			// Thumb
			const auto thumbR = st::lineWidth * 6 + sliderState->hover * st::lineWidth * 2;
			const auto thumbX = sliderW * progress;
			const auto thumbY = sliderY + sliderH / 2.;
			p.setBrush(Qt::white);
			// Glow
			p.setPen(Qt::NoPen);
			auto thumbGlow = QRadialGradient(thumbX, thumbY, thumbR * 2);
			thumbGlow.setColorAt(0, anim::with_alpha(QColor(200, 0, 255), 0.4));
			thumbGlow.setColorAt(1, Qt::transparent);
			p.setBrush(QBrush(thumbGlow));
			p.drawEllipse(QRectF(thumbX - thumbR * 2, thumbY - thumbR * 2, thumbR * 4, thumbR * 4));

			p.setBrush(Qt::white);
			p.setPen(QPen(anim::with_alpha(Qt::black, 0.1), st::lineWidth));
			p.drawEllipse(QRectF(thumbX - thumbR, thumbY - thumbR, thumbR * 2, thumbR * 2));
		}, blurRow->lifetime());

		blurRow->events() | rpl::filter([=](not_null<QEvent*> e) {
			return e->type() == QEvent::MouseMove || e->type() == QEvent::MouseButtonPress || e->type() == QEvent::MouseButtonRelease || e->type() == QEvent::Leave;
		}) | rpl::on_next([=](not_null<QEvent*> e) {
			const auto w = blurRow->width();
			const auto h = blurRow->height();
			const auto sliderH = st::lineWidth * 4;
			const auto sliderY = h - sliderH;

			if (e->type() == QEvent::Leave) {
				if (!sliderState->sliding && sliderState->hover > 0.) {
					sliderHoverAnim->start([=] { sliderState->hover = sliderHoverAnim->value(0.); blurRow->update(); }, sliderState->hover, 0., st::slideDuration);
				}
				return;
			}

			const auto me = static_cast<QMouseEvent*>(e.get());
			const auto x = std::clamp(me->pos().x(), 0, w);
			const auto y = me->pos().y();
			const auto overSlider = (y >= sliderY - st::lineWidth * 6 && y <= sliderY + sliderH + st::lineWidth * 6);

			if (e->type() == QEvent::MouseButtonPress) {
				if (overSlider) {
					sliderState->sliding = true;
					const auto blur = int(base::SafeRound((x / float64(w)) * kBlurMax));
					*blurValue = blur;
					Core::App().settings().setLiveWallpaperBlur(blur);
					Core::App().saveSettingsDelayed();
					Core::App().reprocessAlexSettings();
				}
			} else if (e->type() == QEvent::MouseMove) {
				const auto target = (overSlider || sliderState->sliding) ? 1. : 0.;
				if (sliderState->hover != target && !sliderHoverAnim->animating()) {
					sliderHoverAnim->start([=] { sliderState->hover = sliderHoverAnim->value(target); blurRow->update(); }, sliderState->hover, target, st::slideDuration);
				}
				if (sliderState->sliding) {
					const auto blur = int(base::SafeRound((x / float64(w)) * kBlurMax));
					if (blurValue->current() != blur) {
						*blurValue = blur;
						Core::App().settings().setLiveWallpaperBlur(blur);
						Core::App().saveSettingsDelayed();
						Core::App().reprocessAlexSettings();
					}
				}
			} else if (e->type() == QEvent::MouseButtonRelease) {
				sliderState->sliding = false;
				if (!overSlider) {
					sliderHoverAnim->start([=] { sliderState->hover = sliderHoverAnim->value(0.); blurRow->update(); }, sliderState->hover, 0., st::slideDuration);
				}
			}
		}, blurRow->lifetime());
		blurRow->setAttribute(Qt::WA_Hover);
		
		blurValue->value() | rpl::on_next([=](int) { blurRow->update(); }, blurRow->lifetime());
		
		builder.addSkip();

	}

}

void BuildAlexgramExperimentalSection(SectionBuilder &builder) {	builder.addDivider();

	builder.addSkip();

	SectionBuilder::CheckboxArgs unlimitedPinnedArgs;
	unlimitedPinnedArgs.title = tr::lng_settings_alexgram_unlimited_pinned();
	unlimitedPinnedArgs.checked = Core::App().settings().unlimitedPinned();
	const auto unlimitedPinnedCb = builder.addCheckbox(std::move(unlimitedPinnedArgs));
	if (unlimitedPinnedCb) {
		unlimitedPinnedCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setUnlimitedPinned(checked);
			Core::App().saveSettingsDelayed();
		}, unlimitedPinnedCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_unlimited_pinned_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs unlimitedFavoriteStickersArgs;
	unlimitedFavoriteStickersArgs.title = tr::lng_settings_alexgram_unlimited_favorite_stickers();
	unlimitedFavoriteStickersArgs.checked = Core::App().settings().unlimitedFavoriteStickers();
	const auto unlimitedFavoriteStickersCb = builder.addCheckbox(std::move(unlimitedFavoriteStickersArgs));
	if (unlimitedFavoriteStickersCb) {
		unlimitedFavoriteStickersCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setUnlimitedFavoriteStickers(checked);
			Core::App().saveSettingsDelayed();
		}, unlimitedFavoriteStickersCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_unlimited_favorite_stickers_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs uploadSpeedBoostArgs;
	uploadSpeedBoostArgs.title = tr::lng_settings_alexgram_upload_speed_boost();
	uploadSpeedBoostArgs.checked = Core::App().settings().uploadSpeedBoost();
	const auto uploadSpeedBoostCb = builder.addCheckbox(std::move(uploadSpeedBoostArgs));
	if (uploadSpeedBoostCb) {
		uploadSpeedBoostCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setUploadSpeedBoost(checked);
			Core::App().saveSettingsDelayed();
		}, uploadSpeedBoostCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_upload_speed_boost_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs downloadSpeedBoostArgs;
	downloadSpeedBoostArgs.title = tr::lng_settings_alexgram_download_speed_boost();
	downloadSpeedBoostArgs.checked = Core::App().settings().downloadSpeedBoost();
	const auto downloadSpeedBoostCb = builder.addCheckbox(std::move(downloadSpeedBoostArgs));
	if (downloadSpeedBoostCb) {
		downloadSpeedBoostCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setDownloadSpeedBoost(checked);
			Core::App().saveSettingsDelayed();
		}, downloadSpeedBoostCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_download_speed_boost_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs typingInsteadStickerArgs;
	typingInsteadStickerArgs.title = tr::lng_settings_alexgram_send_typing_instead_sticker();
	typingInsteadStickerArgs.checked = Core::App().settings().sendTypingInsteadOfSticker();
	const auto typingInsteadStickerCb = builder.addCheckbox(std::move(typingInsteadStickerArgs));
	if (typingInsteadStickerCb) {
		typingInsteadStickerCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setSendTypingInsteadOfSticker(checked);
			Core::App().saveSettingsDelayed();
		}, typingInsteadStickerCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_send_typing_instead_sticker_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs hideStoriesArgs;
	hideStoriesArgs.title = tr::lng_settings_alexgram_hide_stories_from_header();
	hideStoriesArgs.checked = Core::App().settings().hideStoriesFromHeader();
	const auto hideStoriesCb = builder.addCheckbox(std::move(hideStoriesArgs));
	if (hideStoriesCb) {
		hideStoriesCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setHideStoriesFromHeader(checked);
			Core::App().saveSettingsDelayed();
		}, hideStoriesCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_hide_stories_from_header_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs disableStoriesArgs;
	disableStoriesArgs.title = tr::lng_settings_alexgram_disable_stories();
	disableStoriesArgs.checked = Core::App().settings().disableStories();
	const auto disableStoriesCb = builder.addCheckbox(std::move(disableStoriesArgs));
	if (disableStoriesCb) {
		disableStoriesCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setDisableStories(checked);
			if (checked && hideStoriesCb) {
				hideStoriesCb->setChecked(true);
			}
			Core::App().saveSettingsDelayed();
		}, disableStoriesCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_disable_stories_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs sendMp4AsVideoArgs;
	sendMp4AsVideoArgs.title = tr::lng_settings_alexgram_send_mp4_as_video();
	sendMp4AsVideoArgs.checked = Core::App().settings().sendMp4AsVideo();
	const auto sendMp4AsVideoCb = builder.addCheckbox(std::move(sendMp4AsVideoArgs));
	if (sendMp4AsVideoCb) {
		sendMp4AsVideoCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setSendMp4AsVideo(checked);
			Core::App().saveSettingsDelayed();
		}, sendMp4AsVideoCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_send_mp4_as_video_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs dolby8DArgs;
	dolby8DArgs.title = tr::lng_settings_alexgram_dolby_8d();
	dolby8DArgs.checked = Core::App().settings().dolby8D();
	const auto dolby8DCb = builder.addCheckbox(std::move(dolby8DArgs));
	if (dolby8DCb) {
		dolby8DCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setDolby8D(checked);
			Core::App().saveSettingsDelayed();
		}, dolby8DCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_dolby_8d_about());
	builder.addSkip();

	SectionBuilder::CheckboxArgs localPremiumArgs;
	localPremiumArgs.title = tr::lng_settings_alexgram_local_premium();
	localPremiumArgs.checked = Core::App().settings().localPremium();
	const auto localPremiumCb = builder.addCheckbox(std::move(localPremiumArgs));
	if (localPremiumCb) {
		localPremiumCb->checkedChanges(
		) | rpl::on_next([=](bool checked) {
			Core::App().settings().setLocalPremium(checked);
			Core::App().saveSettingsDelayed();
		}, localPremiumCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_local_premium_about());
	builder.addSkip();

}

void GhostStorageBox(not_null<Ui::GenericBox*> box, not_null<Window::SessionController*> controller) {
	box->setTitle(tr::lng_settings_alexgram_ghost_storage_report());

	const auto userId = controller->session().userId().bare;

	auto stats = Alex::Database::getStorageStats(userId);

	auto container = box->addRow(object_ptr<Ui::VerticalLayout>(box));

	const auto addCategory = [&](const QString &label, const Alex::StorageStats::Entry &entry, int type, const style::icon *icon) {
		if (entry.count == 0) return;

		auto text = label + u" ("_q + QString::number(entry.count) + u", "_q + Ui::FormatSizeText(entry.size) + u")"_q;
		auto button = AddButtonWithIcon(
			container,
			rpl::single(text),
			st::settingsButton,
			{ icon });

		button->setClickedCallback([=] {
			Alex::Database::clearStorage(userId, type);
			box->closeBox();
			controller->show(Box(GhostStorageBox, controller));
		});
	};

	addCategory(tr::lng_settings_alexgram_ghost_storage_text(tr::now), stats.text, 0, &st::menuIconChatBubble);
	addCategory(tr::lng_settings_alexgram_ghost_storage_photos(tr::now), stats.photo, 1, &st::menuIconPhoto);
	addCategory(tr::lng_settings_alexgram_ghost_storage_videos(tr::now), stats.video, 2, &st::menuIconVideoChat);
	addCategory(tr::lng_settings_alexgram_ghost_storage_audio(tr::now), stats.audio, 3, &st::menuIconFile);
	addCategory(tr::lng_settings_alexgram_ghost_storage_files(tr::now), stats.document, 4, &st::menuIconFile);
	addCategory(tr::lng_settings_alexgram_ghost_storage_other(tr::now), stats.other, 5, &st::menuIconShowAll);
	addCategory(tr::lng_settings_alexgram_ghost_save_edited(tr::now), stats.edits, -1, &st::menuIconEdit);

	if (stats.total.count == 0 && stats.edits.count == 0) {
		container->add(object_ptr<Ui::FlatLabel>(container, u"No saved data found locally."_q, st::defaultFlatLabel), st::boxRowPadding);
	}

	box->addButton(tr::lng_settings_alexgram_ghost_storage_clear_all(), [=] {
		controller->show(Ui::MakeConfirmBox({
			.text = tr::lng_settings_alexgram_ghost_storage_clear_sure(),
			.confirmed = [=](Fn<void()> close) {
				Alex::Database::clearAllStorage(userId);
				close();
				box->closeBox();
			},
			.confirmText = tr::lng_settings_alexgram_ghost_storage_clear(tr::now),
		}));
	}, st::attentionBoxButton);

	box->addButton(tr::lng_close(), [=] { box->closeBox(); });
}

void BuildAlexgramGhostModeSection(SectionBuilder &builder) {
	builder.addDivider();
	builder.addSkip();

	builder.scope([&] {
		SectionBuilder::CheckboxArgs masterArgs;
		masterArgs.title = tr::lng_settings_alexgram_ghost_mode_master();
		masterArgs.checked = Core::App().settings().ghostModeEnabled();
		const auto masterCb = builder.addCheckbox(std::move(masterArgs));
		if (masterCb) {
			masterCb->checkedChanges() | rpl::on_next([=](bool checked) {
				auto &settings = Core::App().settings();
				if (checked) {
					settings.setGhostModeNoRead(true);
					settings.setGhostDontReadStories(true);
					settings.setGhostDontSendOnline(true);
					settings.setGhostDontSendTyping(true);
					settings.setGhostGoOffline(true);
					builder.session()->updates().updateOnline(0, true);
				} else {
					settings.setGhostModeNoRead(false);
					settings.setGhostDontReadStories(false);
					settings.setGhostDontSendOnline(false);
					settings.setGhostDontSendTyping(false);
					settings.setGhostGoOffline(false);
				}
				settings.setGhostModeEnabled(checked);
				Core::App().saveSettingsDelayed();
			}, masterCb->lifetime());
		}

		builder.scope([&] {
			builder.addSkip(st::settingsCheckboxesSkip / 2);

			SectionBuilder::CheckboxArgs noReadArgs;
			noReadArgs.title = tr::lng_settings_alexgram_ghost_mode_no_read();
			noReadArgs.checked = Core::App().settings().ghostModeNoRead();
			const auto noReadCb = builder.addCheckbox(std::move(noReadArgs));
			if (noReadCb) {
				noReadCb->checkedChanges() | rpl::on_next([=](bool checked) {
					Core::App().settings().setGhostModeNoRead(checked);
					Core::App().saveSettingsDelayed();
				}, noReadCb->lifetime());

				Core::App().settings().ghostModeNoReadChanges()
					| rpl::on_next([=](bool checked) {
						noReadCb->setChecked(checked);
					}, noReadCb->lifetime());
			}

			builder.add([&](const WidgetContext &ctx) {
				auto result = object_ptr<Ui::FlatLabel>(ctx.container, tr::lng_settings_alexgram_ghost_mode_no_read_about(), st::boxDividerLabel);
				result->setOpacity(0.7f);
				return SectionBuilder::WidgetToAdd{ std::move(result), st::settingsCheckboxAboutPadding };
			});
			builder.addSkip(st::settingsCheckboxesSkip / 2);

			SectionBuilder::CheckboxArgs noReadStoriesArgs;
			noReadStoriesArgs.title = tr::lng_settings_alexgram_ghost_mode_no_read_stories();
			noReadStoriesArgs.checked = Core::App().settings().ghostDontReadStories();
			const auto noReadStoriesCb = builder.addCheckbox(std::move(noReadStoriesArgs));
			if (noReadStoriesCb) {
				noReadStoriesCb->checkedChanges() | rpl::on_next([=](bool checked) {
					Core::App().settings().setGhostDontReadStories(checked);
					Core::App().saveSettingsDelayed();
				}, noReadStoriesCb->lifetime());

				Core::App().settings().ghostDontReadStoriesChanges()
					| rpl::on_next([=](bool checked) {
						noReadStoriesCb->setChecked(checked);
					}, noReadStoriesCb->lifetime());
			}

			builder.add([&](const WidgetContext &ctx) {
				auto result = object_ptr<Ui::FlatLabel>(ctx.container, tr::lng_settings_alexgram_ghost_mode_no_read_stories_about(), st::boxDividerLabel);
				result->setOpacity(0.7f);
				return SectionBuilder::WidgetToAdd{ std::move(result), st::settingsCheckboxAboutPadding };
			});
			builder.addSkip(st::settingsCheckboxesSkip / 2);

			SectionBuilder::CheckboxArgs noOnlineArgs;
			noOnlineArgs.title = tr::lng_settings_alexgram_ghost_mode_no_online();
			noOnlineArgs.checked = Core::App().settings().ghostDontSendOnline();
			const auto noOnlineCb = builder.addCheckbox(std::move(noOnlineArgs));
			if (noOnlineCb) {
				noOnlineCb->checkedChanges() | rpl::on_next([=](bool checked) {
					Core::App().settings().setGhostDontSendOnline(checked);
					Core::App().saveSettingsDelayed();
				}, noOnlineCb->lifetime());

				Core::App().settings().ghostDontSendOnlineChanges()
					| rpl::on_next([=](bool checked) {
						noOnlineCb->setChecked(checked);
					}, noOnlineCb->lifetime());
			}

			builder.add([&](const WidgetContext &ctx) {
				auto result = object_ptr<Ui::FlatLabel>(ctx.container, tr::lng_settings_alexgram_ghost_mode_no_online_about(), st::boxDividerLabel);
				result->setOpacity(0.7f);
				return SectionBuilder::WidgetToAdd{ std::move(result), st::settingsCheckboxAboutPadding };
			});
			builder.addSkip(st::settingsCheckboxesSkip / 2);

			SectionBuilder::CheckboxArgs noTypingArgs;
			noTypingArgs.title = tr::lng_settings_alexgram_ghost_mode_no_typing();
			noTypingArgs.checked = Core::App().settings().ghostDontSendTyping();
			const auto noTypingCb = builder.addCheckbox(std::move(noTypingArgs));
			if (noTypingCb) {
				noTypingCb->checkedChanges() | rpl::on_next([=](bool checked) {
					Core::App().settings().setGhostDontSendTyping(checked);
					Core::App().saveSettingsDelayed();
				}, noTypingCb->lifetime());

				Core::App().settings().ghostDontSendTypingChanges()
					| rpl::on_next([=](bool checked) {
						noTypingCb->setChecked(checked);
					}, noTypingCb->lifetime());
			}

			builder.add([&](const WidgetContext &ctx) {
				auto result = object_ptr<Ui::FlatLabel>(ctx.container, tr::lng_settings_alexgram_ghost_mode_no_typing_about(), st::boxDividerLabel);
				result->setOpacity(0.7f);
				return SectionBuilder::WidgetToAdd{ std::move(result), st::settingsCheckboxAboutPadding };
			});
			builder.addSkip(st::settingsCheckboxesSkip / 2);

			SectionBuilder::CheckboxArgs goOfflineArgs;
			goOfflineArgs.title = tr::lng_settings_alexgram_ghost_mode_go_offline();
			goOfflineArgs.checked = Core::App().settings().ghostGoOffline();
			const auto goOfflineCb = builder.addCheckbox(std::move(goOfflineArgs));
			if (goOfflineCb) {
				goOfflineCb->checkedChanges() | rpl::on_next([=](bool checked) {
					Core::App().settings().setGhostGoOffline(checked);
					Core::App().saveSettingsDelayed();
					if (checked) {
						builder.session()->updates().updateOnline(0, true);
					}
				}, goOfflineCb->lifetime());

				Core::App().settings().ghostGoOfflineChanges()
					| rpl::on_next([=](bool checked) {
						goOfflineCb->setChecked(checked);
					}, goOfflineCb->lifetime());
			}

			builder.add([&](const WidgetContext &ctx) {
				auto result = object_ptr<Ui::FlatLabel>(ctx.container, tr::lng_settings_alexgram_ghost_mode_go_offline_about(), st::boxDividerLabel);
				result->setOpacity(0.7f);
				return SectionBuilder::WidgetToAdd{ std::move(result), st::settingsCheckboxAboutPadding };
			});
			builder.addSkip(st::settingsCheckboxesSkip / 2);
		}, Core::App().settings().ghostModeEnabledChanges(), [=](SectionBuilder::ToggledScopePtr wrap) {
			wrap->entity()->setContentsMargins(st::settingsCheckboxesSkip, 0, 0, 0);
		});

		builder.scope([&] {
			builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_master_about());
		}, Core::App().settings().ghostModeEnabledChanges() | rpl::map([](bool enabled) {
			return !enabled;
		}));
	}, nullptr, [=](SectionBuilder::ToggledScopePtr wrap) {
		const auto container = wrap->entity();
		container->paintRequest() | rpl::on_next([container](QRect) {
			const auto enabled = Core::App().settings().ghostModeEnabled();
			if (!enabled) {
				return;
			}
			Painter p(container);
			p.setRenderHint(QPainter::Antialiasing);

			const auto skip = st::settingsCheckboxesSkip;
			const auto rect = container->rect().marginsRemoved(QMargins(0, skip / 8, 0, skip / 4));

			p.setPen(anim::with_alpha(st::windowActiveTextFg->c, 0.15));
			QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
			gradient.setColorAt(0, anim::with_alpha(st::windowActiveTextFg->c, 0.08));
			gradient.setColorAt(1, anim::with_alpha(st::windowActiveTextFg->c, 0.02));
			p.setBrush(gradient);
			p.drawRoundedRect(rect, st::boxRadius, st::boxRadius);

			p.setPen(Qt::NoPen);
			p.setBrush(st::windowActiveTextFg);
			const auto linePadding = st::boxRadius;
			p.drawRoundedRect(QRect(rect.x(), rect.y() + linePadding, st::lineWidth * 3, rect.height() - linePadding * 2), float(st::lineWidth * 1.5), float(st::lineWidth * 1.5));

			const auto icon = &st::menuIconStealth;
			icon->paint(p, rect.width() - icon->width() - skip / 2, rect.y() + (st::settingsButton.height - icon->height()) / 2, container->width());
		}, container->lifetime());

		Core::App().settings().ghostModeEnabledChanges() | rpl::on_next([container] {
			container->update();
		}, container->lifetime());
	});

	SectionBuilder::CheckboxArgs readOnInteractArgs;
	readOnInteractArgs.title = tr::lng_settings_alexgram_ghost_mode_read_on_interact();
	readOnInteractArgs.checked = Core::App().settings().ghostReadOnInteract();
	const auto readOnInteractCb = builder.addCheckbox(std::move(readOnInteractArgs));
	if (readOnInteractCb) {
		readOnInteractCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setGhostReadOnInteract(checked);
			Core::App().saveSettingsDelayed();
		}, readOnInteractCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_read_on_interact_about());
	builder.addSkip();

	builder.scope([&] {
		SectionBuilder::CheckboxArgs saveDeletedArgs;
		saveDeletedArgs.title = tr::lng_settings_alexgram_ghost_mode_save_deleted();
		saveDeletedArgs.checked = Core::App().settings().ghostSaveDeletedMessages();
		const auto saveDeletedCb = builder.addCheckbox(std::move(saveDeletedArgs));
		if (saveDeletedCb) {
			saveDeletedCb->checkedChanges() | rpl::on_next([=](bool checked) {
				Core::App().settings().setGhostSaveDeletedMessages(checked);
				Core::App().saveSettingsDelayed();
			}, saveDeletedCb->lifetime());
		}

		builder.scope([&] {
			builder.addSkip(st::settingsCheckboxesSkip / 2);

			SectionBuilder::CheckboxArgs saveBotDeletedArgs;
			saveBotDeletedArgs.title = tr::lng_settings_alexgram_ghost_mode_save_bot_deleted();
			saveBotDeletedArgs.checked = Core::App().settings().ghostSaveBotDeleted();
			const auto saveBotDeletedCb = builder.addCheckbox(std::move(saveBotDeletedArgs));
			if (saveBotDeletedCb) {
				saveBotDeletedCb->checkedChanges() | rpl::on_next([=](bool checked) {
					Core::App().settings().setGhostSaveBotDeleted(checked);
					Core::App().saveSettingsDelayed();
				}, saveBotDeletedCb->lifetime());
			}

			builder.add([&](const WidgetContext &ctx) {
				auto result = object_ptr<Ui::FlatLabel>(
					ctx.container,
					tr::lng_settings_alexgram_ghost_mode_save_bot_deleted_about(),
					st::defaultFlatLabel);
				result->setTextColorOverride(st::windowSubTextFg->c);
				return SectionBuilder::WidgetToAdd{
					std::move(result),
					st::settingsCheckboxAboutPadding
				};
			});
			builder.addSkip(st::settingsCheckboxesSkip / 2);

			SectionBuilder::CheckboxArgs showIconArgs;
			showIconArgs.title = tr::lng_settings_alexgram_ghost_mode_deleted_show_icon();
			showIconArgs.checked = Core::App().settings().ghostDeletedShowIcon();
			const auto showIconCb = builder.addCheckbox(std::move(showIconArgs));
			if (showIconCb) {
				showIconCb->checkedChanges() | rpl::on_next([=](bool checked) {
					Core::App().settings().setGhostDeletedShowIcon(checked);
					Core::App().saveSettingsDelayed();
					Core::App().reprocessAlexSettings();
				}, showIconCb->lifetime());
			}

			builder.add([&](const WidgetContext &ctx) {
				auto result = object_ptr<Ui::FlatLabel>(
					ctx.container,
					tr::lng_settings_alexgram_ghost_mode_deleted_show_icon_about(),
					st::defaultFlatLabel);
				result->setTextColorOverride(st::windowSubTextFg->c);
				return SectionBuilder::WidgetToAdd{
					std::move(result),
					st::settingsCheckboxAboutPadding
				};
			});
			builder.addSkip(st::settingsCheckboxesSkip / 2);

			const auto colorWrap = builder.container()->add(
				object_ptr<Ui::SlideWrap<Ui::VerticalLayout>>(
					builder.container(),
					object_ptr<Ui::VerticalLayout>(builder.container())
				)
			);
			const auto colorInner = colorWrap->entity();

			static auto transparentSettingsButton = st::defaultSettingsButton;
			transparentSettingsButton.textBg = st::transparent;

			auto windowController = builder.controller();
			const auto getCurrentColor = [] {
				const auto c = Core::App().settings().ghostDeletedIconColor();
				return (c == 0) ? st::historyOutIconFg->c : QColor::fromRgba(static_cast<QRgb>(c));
			};

			auto colorBtn = colorInner->add(
				object_ptr<Ui::SettingsButton>(
					colorInner,
					tr::lng_settings_alexgram_ghost_mode_deleted_icon_color(),
					transparentSettingsButton
				)
			);
			colorBtn->setColorOverride(st::windowFg->c);
			colorBtn->setPaddingOverride(style::margins(37, 8, 22, 8));

			auto colorVar = colorBtn->lifetime().make_state<rpl::variable<QColor>>(getCurrentColor());

			auto rightLabel = Ui::CreateChild<Ui::FlatLabel>(
				colorBtn,
				colorVar->value() | rpl::map([](const QColor &c) {
					return c.name().toUpper();
				}),
				st::defaultSettingsRightLabel
			);
			colorVar->value() | rpl::on_next([=](const QColor &c) {
				rightLabel->setTextColorOverride(c);
			}, rightLabel->lifetime());

			colorBtn->sizeValue() | rpl::on_next([=](const QSize &s) {
				rightLabel->moveToRight(st::defaultSettingsButton.padding.right(), (s.height() - rightLabel->height()) / 2, s.width());
			}, rightLabel->lifetime());

			colorBtn->addClickHandler([=] {
				const auto currentColor = colorVar->current();

				auto box = Box([=](not_null<Ui::GenericBox*> box) {
					const auto editor = box->addRow(object_ptr<ColorEditor>(
						box,
						ColorEditor::Mode::HSL,
						currentColor));

					const auto save = crl::guard(editor, [=] {
						Core::App().settings().setGhostDeletedIconColor(
							editor->color().rgba());
						Core::App().saveSettingsDelayed();
						Core::App().reprocessAlexSettings();
						*colorVar = editor->color();
						box->closeBox();
					});
					const auto reset = [=] {
						Core::App().settings().setGhostDeletedIconColor(0);
						Core::App().saveSettingsDelayed();
						Core::App().reprocessAlexSettings();
						*colorVar = st::historyOutIconFg->c;
						box->closeBox();
					};

					editor->submitRequests(
					) | rpl::on_next(save, editor->lifetime());

					box->setFocusCallback([=] {
						editor->setInnerFocus();
					});
					box->addButton(tr::lng_settings_save(), save);
					box->addButton(tr::lng_chat_intro_reset(), reset);
					box->addButton(tr::lng_cancel(), [=] { box->closeBox(); });
					box->setTitle(tr::lng_settings_alexgram_ghost_mode_deleted_icon_color());
					box->setWidth(editor->width());
				});
				windowController->show(std::move(box));
			});

			colorInner->add(
				object_ptr<Ui::FlatLabel>(
					colorInner,
					tr::lng_settings_alexgram_ghost_mode_deleted_icon_color_about(),
					st::defaultFlatLabel
				),
				QMargins(37, 0, 22, st::settingsCheckboxesSkip / 2)
			)->setTextColorOverride(st::windowSubTextFg->c);

			colorWrap->toggleOn(
				Core::App().settings().ghostDeletedShowIconChanges()
			);

			builder.addSkip(st::settingsCheckboxesSkip / 2);

			SectionBuilder::CheckboxArgs translucentArgs;
			translucentArgs.title = tr::lng_settings_alexgram_ghost_mode_translucent_deleted();
			translucentArgs.checked = Core::App().settings().ghostTranslucentDeleted();
			const auto translucentCb = builder.addCheckbox(std::move(translucentArgs));
			if (translucentCb) {
				translucentCb->checkedChanges() | rpl::on_next([=](bool checked) {
					Core::App().settings().setGhostTranslucentDeleted(checked);
					Core::App().saveSettingsDelayed();
					Core::App().reprocessAlexSettings();
				}, translucentCb->lifetime());
			}

			builder.scope([&] {
				builder.add([&](const WidgetContext &ctx) {
					auto result = object_ptr<Ui::FlatLabel>(
						ctx.container,
						tr::lng_settings_alexgram_ghost_mode_deleted_opacity(),
						st::defaultFlatLabel);
					result->setTextColorOverride(st::windowActiveTextFg->c);
					return SectionBuilder::WidgetToAdd{
						std::move(result),
						{ st::settingsCheckboxAboutPadding.left(), st::settingsCheckboxesSkip / 2, st::settingsCheckboxAboutPadding.right(), st::settingsCheckboxesSkip / 2 }
					};
				});

				if (const auto container = builder.container()) {
					const auto slider = container->add(
						object_ptr<Ui::SettingsSlider>(container, st::settingsSlider),
						st::settingsBigScalePadding + QMargins(st::settingsCheckboxesSkip, 0, 0, 0));
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
				builder.addSkip(st::settingsCheckboxesSkip / 2);
			}, translucentCb ? translucentCb->checkedValue() : rpl::single(false));

			builder.add([&](const WidgetContext &ctx) {
				auto result = object_ptr<Ui::FlatLabel>(
					ctx.container,
					tr::lng_settings_alexgram_ghost_mode_translucent_deleted_about(),
					st::defaultFlatLabel);
				result->setTextColorOverride(st::windowSubTextFg->c);
				return SectionBuilder::WidgetToAdd{
					std::move(result),
					st::settingsCheckboxAboutPadding
				};
			});
			builder.addSkip(st::settingsCheckboxesSkip / 2);

			const uint64 ghostUserId = builder.session()->userId().bare;

			SectionBuilder::ButtonArgs storageArgs;
			storageArgs.st = nullptr; // use default settings button style for proper alignment
			storageArgs.title = tr::lng_settings_alexgram_ghost_manage_storage();
			rpl::producer<Alex::StorageStats> statsProducer = Alex::Database::storageStatsValue(ghostUserId);
			storageArgs.label = std::move(statsProducer)
				| rpl::map([](const Alex::StorageStats &stats) {
					return Ui::FormatSizeText(stats.databaseFileSize);
				});
			storageArgs.icon = { &st::menuIconSettings };
			if (const auto button = builder.addButton(std::move(storageArgs))) {
				button->setClickedCallback([=] {
					builder.controller()->show(Box(GhostStorageBox, builder.controller()));
				});
			}

			builder.addSkip(st::settingsCheckboxesSkip / 2);

		}, saveDeletedCb ? saveDeletedCb->checkedValue() : rpl::single(false), [=](SectionBuilder::ToggledScopePtr wrap) {
			wrap->entity()->setContentsMargins(st::settingsCheckboxesSkip, 0, 0, 0);
		});

		builder.scope([&] {
			builder.addDividerText(tr::lng_settings_alexgram_ghost_mode_save_deleted_about());
			builder.addSkip();
		}, saveDeletedCb ? rpl::producer<bool>(saveDeletedCb->checkedValue() | rpl::map([](bool checked) { return !checked; })) : rpl::producer<bool>(rpl::single(true)));
	}, nullptr, [=](SectionBuilder::ToggledScopePtr wrap) {
		const auto container = wrap->entity();
		container->paintRequest() | rpl::on_next([container](QRect) {
			const auto enabled = Core::App().settings().ghostSaveDeletedMessages();
			if (!enabled) {
				return;
			}
			Painter p(container);
			p.setRenderHint(QPainter::Antialiasing);

			const auto skip = st::settingsCheckboxesSkip;
			const auto rect = container->rect().marginsRemoved(QMargins(0, skip / 8, 0, skip / 4));

			p.setPen(anim::with_alpha(st::windowActiveTextFg->c, 0.15));
			QLinearGradient gradient(rect.topLeft(), rect.bottomRight());
			gradient.setColorAt(0, anim::with_alpha(st::windowActiveTextFg->c, 0.08));
			gradient.setColorAt(1, anim::with_alpha(st::windowActiveTextFg->c, 0.02));
			p.setBrush(gradient);
			p.drawRoundedRect(rect, st::boxRadius, st::boxRadius);

			p.setPen(Qt::NoPen);
			p.setBrush(st::windowActiveTextFg);
			const auto linePadding = st::boxRadius;
			p.drawRoundedRect(QRect(rect.x(), rect.y() + linePadding, st::lineWidth * 3, rect.height() - linePadding * 2), float(st::lineWidth * 1.5), float(st::lineWidth * 1.5));
		}, container->lifetime());

		Core::App().settings().ghostSaveDeletedMessagesChanges() | rpl::on_next([container] {
			container->update();
		}, container->lifetime());
	});

	SectionBuilder::CheckboxArgs saveEditedArgs;
	saveEditedArgs.title = tr::lng_settings_alexgram_ghost_save_edited();
	saveEditedArgs.checked = Core::App().settings().ghostSaveEditedMessages();
	const auto saveEditedCb = builder.addCheckbox(std::move(saveEditedArgs));
	if (saveEditedCb) {
		saveEditedCb->checkedChanges() | rpl::on_next([=](bool checked) {
			Core::App().settings().setGhostSaveEditedMessages(checked);
			Core::App().saveSettingsDelayed();
		}, saveEditedCb->lifetime());
	}

	builder.addDividerText(tr::lng_settings_alexgram_ghost_save_edited_about());
	builder.addSkip();

}

void AlexgramMain::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto builder = SectionBuilder(WidgetContext{ content, controller(), showOtherMethod() });
	BuildAlexgramMainSection(builder);
	Ui::ResizeFitChild(this, content);
}

void AlexgramGeneral::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto builder = SectionBuilder(WidgetContext{ content, controller(), showOtherMethod() });
	BuildAlexgramGeneralSection(builder);
	Ui::ResizeFitChild(this, content);
}

void AlexgramTranslator::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto builder = SectionBuilder(WidgetContext{ content, controller(), showOtherMethod() });
	BuildAlexgramTranslatorSection(builder);
	Ui::ResizeFitChild(this, content);
}

void AlexgramChats::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto builder = SectionBuilder(WidgetContext{ content, controller(), showOtherMethod() });
	BuildAlexgramChatsSection(builder);
	Ui::ResizeFitChild(this, content);
}

void AlexgramExperimental::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto builder = SectionBuilder(WidgetContext{ content, controller(), showOtherMethod() });
	BuildAlexgramExperimentalSection(builder);
	Ui::ResizeFitChild(this, content);
}

void AlexgramGhostMode::setupContent() {
	const auto content = Ui::CreateChild<Ui::VerticalLayout>(this);
	auto builder = SectionBuilder(WidgetContext{ content, controller(), showOtherMethod() });
	BuildAlexgramGhostModeSection(builder);
	Ui::ResizeFitChild(this, content);
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

const auto kMeta = BuildHelper({ .id = AlexgramSectionId(), .parentId = MainId(), .title = &tr::lng_settings_alexgram, .icon = &st::menuIconManage }, [](SectionBuilder &builder) { BuildAlexgramMainSection(builder); });
const auto kGeneralMeta = BuildHelper({ .id = AlexgramGeneralSectionId(), .parentId = AlexgramSectionId(), .title = &tr::lng_settings_alexgram_general, .icon = &st::menuIconSettings }, [](SectionBuilder &builder) { BuildAlexgramGeneralSection(builder); });
const auto kTranslatorMeta = BuildHelper({ .id = AlexgramTranslatorSectionId(), .parentId = AlexgramSectionId(), .title = &tr::lng_settings_alexgram_translator, .icon = &st::menuIconTranslate }, [](SectionBuilder &builder) { BuildAlexgramTranslatorSection(builder); });
const auto kChatsMeta = BuildHelper({ .id = AlexgramChatsSectionId(), .parentId = AlexgramSectionId(), .title = &tr::lng_settings_alexgram_chats, .icon = &st::menuIconChatBubble }, [](SectionBuilder &builder) { BuildAlexgramChatsSection(builder); });
const auto kExperimentalMeta = BuildHelper({ .id = AlexgramExperimentalSectionId(), .parentId = AlexgramSectionId(), .title = &tr::lng_settings_alexgram_experimental, .icon = &st::menuIconExperimental }, [](SectionBuilder &builder) { BuildAlexgramExperimentalSection(builder); });
const auto kGhostModeMeta = BuildHelper({ .id = AlexgramGhostModeSectionId(), .parentId = AlexgramSectionId(), .title = &tr::lng_settings_alexgram_ghost_mode, .icon = &st::menuIconStealth }, [](SectionBuilder &builder) { BuildAlexgramGhostModeSection(builder); });

} // namespace Settings
