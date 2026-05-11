#include "alex/alex_mapper.h"

#include "api/api_text_entities.h"
#include "history/history.h"
#include "history/history_item.h"
#include "history/history_item_components.h"
#include "data/data_session.h"
#include "data/data_media_types.h"
#include "data/data_document.h"
#include "history/view/history_view_item_preview.h"

#include "main/main_session.h"
#include "base/unixtime.h"

namespace Alex {
namespace Mapper {

namespace {

constexpr auto kMessageFlagUnread = 0x00000001;
constexpr auto kMessageFlagOut = 0x00000002;
constexpr auto kMessageFlagForwarded = 0x00000004;
constexpr auto kMessageFlagReply = 0x00000008;
constexpr auto kMessageFlagMention = 0x00000010;
constexpr auto kMessageFlagContentUnread = 0x00000020;
constexpr auto kMessageFlagHasMarkup = 0x00000040;
constexpr auto kMessageFlagHasEntities = 0x00000080;
constexpr auto kMessageFlagHasFromId = 0x00000100;
constexpr auto kMessageFlagHasMedia = 0x00000200;
constexpr auto kMessageFlagHasViews = 0x00000400;
constexpr auto kMessageFlagHasBotId = 0x00000800;
constexpr auto kMessageFlagIsSilent = 0x00001000;
constexpr auto kMessageFlagIsPost = 0x00004000;
constexpr auto kMessageFlagEdited = 0x00008000;
constexpr auto kMessageFlagHasPostAuthor = 0x00010000;
constexpr auto kMessageFlagIsGrouped = 0x00020000;
constexpr auto kMessageFlagFromScheduled = 0x00040000;
constexpr auto kMessageFlagHasReactions = 0x00100000;
constexpr auto kMessageFlagHideEdit = 0x00200000;
constexpr auto kMessageFlagRestricted = 0x00400000;
constexpr auto kMessageFlagHasReplies = 0x00800000;
constexpr auto kMessageFlagIsPinned = 0x01000000;
constexpr auto kMessageFlagHasTTL = 0x02000000;
constexpr auto kMessageFlagInvertMedia = 0x08000000;
constexpr auto kMessageFlagHasSavedPeer = 0x10000000;

template<typename MTPObject>
std::string serializeObject(const MTPObject &object) {
	mtpBuffer buffer;
	object.write(buffer);

	const auto from = reinterpret_cast<const char*>(buffer.data());
	const auto size = buffer.size() * sizeof(mtpPrime);

	return std::string(from, size);
}

uint64 GetDialogIdFromPeer(not_null<PeerData*> peer) {
	return peer->id.value & PeerId::kChatTypeMask;
}

} // namespace

int mapItemFlagsToMTPFlags(not_null<HistoryItem*> item) {
	int flags = 0;

	const auto thread = item->topic()
		? reinterpret_cast<Data::Thread*>(item->topic())
		: item->history();
	if (item->unread(thread)) {
		flags |= kMessageFlagUnread;
	}
	if (item->out()) {
		flags |= kMessageFlagOut;
	}
	if (item->Get<HistoryMessageForwarded>()) {
		flags |= kMessageFlagForwarded;
	}
	if (item->Get<HistoryMessageReply>()) {
		flags |= kMessageFlagReply;
	}
	if (item->mentionsMe()) {
		flags |= kMessageFlagMention;
	}
	if (item->hasUnreadMediaFlag()) {
		flags |= kMessageFlagContentUnread;
	}
	if (item->definesReplyKeyboard()) {
		flags |= kMessageFlagHasMarkup;
	}
	if (!item->originalText().entities.empty()) {
		flags |= kMessageFlagHasEntities;
	}
	if (item->displayFrom()) {
		flags |= kMessageFlagHasFromId;
	}
	if (item->media()) {
		flags |= kMessageFlagHasMedia;
	}
	if (item->hasViews()) {
		flags |= kMessageFlagHasViews;
	}
	if (item->viaBot()) {
		flags |= kMessageFlagHasBotId;
	}
	if (item->isSilent()) {
		flags |= kMessageFlagIsSilent;
	}
	if (item->isPost()) {
		flags |= kMessageFlagIsPost;
	}
	if (item->Get<HistoryMessageEdited>()) {
		flags |= kMessageFlagEdited;
	}
	if (item->Get<HistoryMessageSigned>()) {
		flags |= kMessageFlagHasPostAuthor;
	}
	if (item->groupId()) {
		flags |= kMessageFlagIsGrouped;
	}
	if (item->isScheduled()) {
		flags |= kMessageFlagFromScheduled;
	}
	if (!item->reactions().empty()) {
		flags |= kMessageFlagHasReactions;
	}
	if (item->hideEditedBadge()) {
		flags |= kMessageFlagHideEdit;
	}
	if (item->hasPossibleRestrictions()) {
		flags |= kMessageFlagRestricted;
	}
	if (item->repliesCount() > 0) {
		flags |= kMessageFlagHasReplies;
	}
	if (item->isPinned()) {
		flags |= kMessageFlagIsPinned;
	}
	if (item->ttlDestroyAt() > 0) {
		flags |= kMessageFlagHasTTL;
	}
	if (item->invertMedia()) {
		flags |= kMessageFlagInvertMedia;
	}
	if (item->savedFromSender()) {
		flags |= kMessageFlagHasSavedPeer;
	}

	return flags;
}

std::pair<std::string, std::string> serializeTextWithEntities(not_null<HistoryItem*> item) {
	if (item->emptyText()) {
		return { "", "" };
	}
	const auto textWithEntities = item->originalText();
	std::string entities;
	if (!textWithEntities.entities.empty()) {
		const auto mtpEntities = Api::EntitiesToMTP(
			&item->history()->session(),
			textWithEntities.entities,
			Api::ConvertOption::WithLocal);
		entities = serializeObject(mtpEntities);
	}
	return { textWithEntities.text.toStdString(), entities };
}

void map(not_null<HistoryItem*> item, DeletedMessage &message) {
	const auto session = &item->history()->owner().session();
	message.userId = session->userId().bare;
	message.dialogId = item->history()->peer->id.value;
	message.groupedId = item->groupId().raw();
	message.peerId = item->history()->peer->id.value;
	message.fromId = item->from()->id.value;
	message.topicId = item->topic() ? item->topicRootId().bare : 0;
	message.messageId = item->id.bare;

	message.date = item->date();
	message.flags = mapItemFlagsToMTPFlags(item);

	if (const auto edited = item->Get<HistoryMessageEdited>()) {
		message.editDate = edited->date;
	} else {
		message.editDate = base::unixtime::now();
	}

	message.views = item->viewsCount();
	
	if (const auto msgsigned = item->Get<HistoryMessageSigned>()) {
		message.fwdPostAuthor = msgsigned->author.toStdString();
	}

	message.entityCreateDate = base::unixtime::now();

	const auto [text, entities] = serializeTextWithEntities(item);
	message.text = text;
	message.textEntities = entities;
	const auto raw = item->ghostDeletedData();
	message.messageData = std::vector<char>(raw.constData(), raw.constData() + raw.size());

	if (const auto media = item->media()) {
		const auto preview = media->toPreview(Data::Media::ToPreviewOptions{
			.ignoreGroup = true,
		});
		message.mediaPath = preview.text.text.toStdString();
		if (const auto photo = media->photo()) {
			message.documentType = 1; // Photo
		} else if (const auto document = media->document()) {
			if (document->isVideoFile() || document->isGifv()) {
				message.documentType = 2; // Video/Gif
			} else if (document->isVoiceMessage() || document->isAudioFile()) {
				message.documentType = 3; // Audio/Voice
			} else {
				message.documentType = 4; // Document/File
			}
		}
	} else {
		message.mediaPath = "/";
		message.documentType = 0;
	}
}

void map(not_null<HistoryItem*> item, EditedMessage &message) {
	const auto session = &item->history()->owner().session();
	message.userId = session->userId().bare;
	message.dialogId = item->history()->peer->id.value;
	message.groupedId = item->groupId().raw();
	message.peerId = item->history()->peer->id.value;
	message.fromId = item->from()->id.value;
	message.topicId = item->topic() ? item->topicRootId().bare : 0;
	message.messageId = item->id.bare;


	message.date = item->date();
	message.flags = mapItemFlagsToMTPFlags(item);

	if (const auto edited = item->Get<HistoryMessageEdited>()) {
		message.editDate = edited->date;
	} else {
		message.editDate = base::unixtime::now();
	}

	message.views = item->viewsCount();
	
	if (const auto msgsigned = item->Get<HistoryMessageSigned>()) {
		message.fwdPostAuthor = msgsigned->author.toStdString();
	}

	message.entityCreateDate = base::unixtime::now();

	const auto [text, entities] = serializeTextWithEntities(item);
	message.text = text;
	message.textEntities = entities;
	const auto raw = item->ghostDeletedData();
	message.messageData = std::vector<char>(raw.constData(), raw.constData() + raw.size());

	if (const auto media = item->media()) {
		const auto preview = media->toPreview(Data::Media::ToPreviewOptions{
			.ignoreGroup = true,
		});
		message.mediaPath = preview.text.text.toStdString();
		if (const auto photo = media->photo()) {
			message.documentType = 1; // Photo
		} else if (const auto document = media->document()) {
			if (document->isVideoFile() || document->isGifv()) {
				message.documentType = 2; // Video/Gif
			} else if (document->isVoiceMessage() || document->isAudioFile()) {
				message.documentType = 3; // Audio/Voice
			} else {
				message.documentType = 4; // Document/File
			}
		}
	} else {
		message.mediaPath = "/";
		message.documentType = 0;
	}
}

} // namespace Mapper
} // namespace Alex
