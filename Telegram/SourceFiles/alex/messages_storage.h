#pragma once

#include "alex/alex_database.h"
#include "data/data_msg_id.h"

class HistoryItem;

namespace Alex {
namespace Messages {

void addDeletedMessage(not_null<HistoryItem*> item);
void addEditedMessage(not_null<HistoryItem*> item);

std::vector<DeletedMessage> getDeletedMessages(
	uint64 userId,
	uint64 dialogId,
	int64 topicId,
	int64 minId = 0,
	int64 maxId = 0,
	int limit = 100,
	const QString &searchQuery = {});

std::vector<EditedMessage> getEditedMessages(
	uint64 userId,
	uint64 dialogId,
	int64 messageId,
	int64 minId = 0,
	int64 maxId = 0,
	int limit = 100);

bool hasDeletedMessages(uint64 userId, uint64 dialogId, int64 topicId);
bool hasLocalEdits(uint64 userId, uint64 dialogId, int64 messageId);
void clearDeletedMessages(uint64 userId, uint64 dialogId, int64 topicId);
void removeDeletedMessage(uint64 userId, uint64 dialogId, int64 messageId);

} // namespace Messages
} // namespace Alex
