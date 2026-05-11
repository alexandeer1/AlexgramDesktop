#include "alex/messages_storage.h"

#include "alex/alex_mapper.h"
#include "history/history_item.h"

namespace Alex {
namespace Messages {

void addDeletedMessage(not_null<HistoryItem*> item) {
	DeletedMessage message;
	Mapper::map(item, message);

	Database::addDeletedMessage(message);
}

void addEditedMessage(not_null<HistoryItem*> item) {
	EditedMessage message;
	Mapper::map(item, message);

	Database::addEditedMessage(message);
}

std::vector<DeletedMessage> getDeletedMessages(
		uint64 userId,
		uint64 dialogId,
		int64 topicId,
		int64 minId,
		int64 maxId,
		int limit,
		const QString &searchQuery) {
	return Database::getDeletedMessages(
		userId,
		dialogId,
		topicId,
		minId,
		maxId,
		limit,
		searchQuery.toStdString());
}

std::vector<EditedMessage> getEditedMessages(
		uint64 userId,
		uint64 dialogId,
		int64 messageId,
		int64 minId,
		int64 maxId,
		int limit) {
	return Database::getEditedMessages(
		userId,
		dialogId,
		messageId,
		minId,
		maxId,
		limit);
}

bool hasDeletedMessages(uint64 userId, uint64 dialogId, int64 topicId) {
	return Database::hasDeletedMessages(userId, dialogId, topicId);
}

bool hasLocalEdits(uint64 userId, uint64 dialogId, int64 messageId) {
	return Database::hasLocalEdits(userId, dialogId, messageId);
}

void clearDeletedMessages(uint64 userId, uint64 dialogId, int64 topicId) {
	Database::clearDeletedMessages(userId, dialogId, topicId);
}

void removeDeletedMessage(uint64 userId, uint64 dialogId, int64 messageId) {
	Database::removeDeletedMessage(userId, dialogId, messageId);
}


} // namespace Messages
} // namespace Alex
