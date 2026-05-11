#pragma once

#include "data/data_msg_id.h"
#include <string>
#include <vector>
#include <rpl/producer.h>

namespace Alex {

struct SchemaVersion {
	int id;
	int version;
};

struct DeletedMessage {
	int fakeId;
	uint64 userId;
	uint64 dialogId;
	uint64 groupedId;
	uint64 peerId;
	uint64 fromId;
	int64 topicId;
	int64 messageId;
	int32 date;
	int32 flags;
	int32 editDate;
	int32 views;
	int32 fwdFlags;
	uint64 fwdFromId;
	std::string fwdName;
	int32 fwdDate;
	std::string fwdPostAuthor;
	int32 replyFlags;
	int64 replyMessageId;
	uint64 replyPeerId;
	int64 replyTopId;
	int32 replyForumTopic;
	std::string replySerialized;
	int32 entityCreateDate;
	std::string text;
	std::string textEntities;
	std::vector<char> messageData;
	std::string mediaPath;
	std::string hqThumbPath;
	int32 documentType;
	std::string documentSerialized;
	std::string thumbsSerialized;
	std::string documentAttributesSerialized;
	std::string mimeType;
};

struct EditedMessage {
	int fakeId;
	uint64 userId;
	uint64 dialogId;
	uint64 groupedId;
	uint64 peerId;
	uint64 fromId;
	int64 topicId;
	int64 messageId;
	int32 date;
	int32 flags;
	int32 editDate;
	int32 views;
	int32 fwdFlags;
	uint64 fwdFromId;
	std::string fwdName;
	int32 fwdDate;
	std::string fwdPostAuthor;
	int32 replyFlags;
	int64 replyMessageId;
	uint64 replyPeerId;
	int64 replyTopId;
	int32 replyForumTopic;
	std::string replySerialized;
	int32 entityCreateDate;
	std::string text;
	std::string textEntities;
	std::vector<char> messageData;
	std::string mediaPath;
	std::string hqThumbPath;
	int32 documentType;
	std::string documentSerialized;
	std::string thumbsSerialized;
	std::string documentAttributesSerialized;
	std::string mimeType;
};


struct StorageStats {
	struct Entry {
		int count = 0;
		int64 size = 0;
	};
	Entry total;
	Entry text;
	Entry photo;
	Entry video;
	Entry audio;
	Entry document;
	Entry other;
	Entry edits;
	int64 databaseFileSize = 0;
};

namespace Database {

using ID = uint64;

void initialize();

void addEditedMessage(const EditedMessage &message);
std::vector<EditedMessage> getEditedMessages(ID userId, ID dialogId, ID messageId, ID minId = 0, ID maxId = 0, int totalLimit = 100);
bool hasLocalEdits(ID userId, ID dialogId, ID messageId);

void addDeletedMessage(const DeletedMessage &message);
std::vector<DeletedMessage> getDeletedMessages(ID userId, ID dialogId, ID topicId, ID minId = 0, ID maxId = 0, int totalLimit = 100, const std::string &searchQuery = {});
bool hasDeletedMessages(ID userId, ID dialogId, ID topicId);
void clearDeletedMessages(ID userId, ID dialogId, ID topicId);
void removeDeletedMessage(ID userId, ID dialogId, ID messageId);

StorageStats getStorageStats(ID userId);
rpl::producer<StorageStats> storageStatsValue(ID userId);
void clearStorage(ID userId, int documentType);
void clearAllStorage(ID userId);

} // namespace Database
} // namespace Alex
