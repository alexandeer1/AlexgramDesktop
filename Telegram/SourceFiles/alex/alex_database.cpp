#include "alex/alex_database.h"

#include "alex/libs/sqlite/sqlite_orm.h"
#include "base/unixtime.h"
#include <map>
#include <QtCore/QFileInfo>
#include <rpl/event_stream.h>
#include <rpl/producer.h>
#include <rpl/range.h>
#include <rpl/map.h>
#include <rpl/then.h>
#include <rpl/flatten_latest.h>
#include <crl/crl_async.h>
#include <crl/crl_on_main.h>

using namespace sqlite_orm;

namespace Alex {

auto storage = make_storage(
	"./tdata/alexdata.db",
	make_table<SchemaVersion>(
		"SchemaVersion",
		make_column("id", &SchemaVersion::id, primary_key()),
		make_column("version", &SchemaVersion::version)
	),
	make_index("idx_deleted_message_userId_dialogId_topicId_messageId",
			   column<DeletedMessage>(&DeletedMessage::userId),
			   column<DeletedMessage>(&DeletedMessage::dialogId),
			   column<DeletedMessage>(&DeletedMessage::topicId),
			   column<DeletedMessage>(&DeletedMessage::messageId)),
	make_index("idx_edited_message_userId_dialogId_messageId",
			   column<EditedMessage>(&EditedMessage::userId),
			   column<EditedMessage>(&EditedMessage::dialogId),
			   column<EditedMessage>(&EditedMessage::messageId)),
	make_table<DeletedMessage>(
		"DeletedMessage",
		make_column("fakeId", &DeletedMessage::fakeId, primary_key().autoincrement()),
		make_column("userId", &DeletedMessage::userId),
		make_column("dialogId", &DeletedMessage::dialogId),
		make_column("groupedId", &DeletedMessage::groupedId),
		make_column("peerId", &DeletedMessage::peerId),
		make_column("fromId", &DeletedMessage::fromId),
		make_column("topicId", &DeletedMessage::topicId),
		make_column("messageId", &DeletedMessage::messageId),
		make_column("date", &DeletedMessage::date),
		make_column("flags", &DeletedMessage::flags),
		make_column("editDate", &DeletedMessage::editDate),
		make_column("views", &DeletedMessage::views),
		make_column("fwdFlags", &DeletedMessage::fwdFlags),
		make_column("fwdFromId", &DeletedMessage::fwdFromId),
		make_column("fwdName", &DeletedMessage::fwdName),
		make_column("fwdDate", &DeletedMessage::fwdDate),
		make_column("fwdPostAuthor", &DeletedMessage::fwdPostAuthor),
		make_column("replyFlags", &DeletedMessage::replyFlags),
		make_column("replyMessageId", &DeletedMessage::replyMessageId),
		make_column("replyPeerId", &DeletedMessage::replyPeerId),
		make_column("replyTopId", &DeletedMessage::replyTopId),
		make_column("replyForumTopic", &DeletedMessage::replyForumTopic),
		make_column("replySerialized", &DeletedMessage::replySerialized),
		make_column("entityCreateDate", &DeletedMessage::entityCreateDate),
		make_column("text", &DeletedMessage::text),
		make_column("textEntities", &DeletedMessage::textEntities),
		make_column("messageData", &DeletedMessage::messageData),
		make_column("mediaPath", &DeletedMessage::mediaPath),
		make_column("hqThumbPath", &DeletedMessage::hqThumbPath),
		make_column("documentType", &DeletedMessage::documentType),
		make_column("documentSerialized", &DeletedMessage::documentSerialized),
		make_column("thumbsSerialized", &DeletedMessage::thumbsSerialized),
		make_column("documentAttributesSerialized", &DeletedMessage::documentAttributesSerialized),
		make_column("mimeType", &DeletedMessage::mimeType)
	),
	make_table<EditedMessage>(
		"EditedMessage",
		make_column("fakeId", &EditedMessage::fakeId, primary_key().autoincrement()),
		make_column("userId", &EditedMessage::userId),
		make_column("dialogId", &EditedMessage::dialogId),
		make_column("groupedId", &EditedMessage::groupedId),
		make_column("peerId", &EditedMessage::peerId),
		make_column("fromId", &EditedMessage::fromId),
		make_column("topicId", &EditedMessage::topicId),
		make_column("messageId", &EditedMessage::messageId),
		make_column("date", &EditedMessage::date),
		make_column("flags", &EditedMessage::flags),
		make_column("editDate", &EditedMessage::editDate),
		make_column("views", &EditedMessage::views),
		make_column("fwdFlags", &EditedMessage::fwdFlags),
		make_column("fwdFromId", &EditedMessage::fwdFromId),
		make_column("fwdName", &EditedMessage::fwdName),
		make_column("fwdDate", &EditedMessage::fwdDate),
		make_column("fwdPostAuthor", &EditedMessage::fwdPostAuthor),
		make_column("replyFlags", &EditedMessage::replyFlags),
		make_column("replyMessageId", &EditedMessage::replyMessageId),
		make_column("replyPeerId", &EditedMessage::replyPeerId),
		make_column("replyTopId", &EditedMessage::replyTopId),
		make_column("replyForumTopic", &EditedMessage::replyForumTopic),
		make_column("replySerialized", &EditedMessage::replySerialized),
		make_column("entityCreateDate", &EditedMessage::entityCreateDate),
		make_column("text", &EditedMessage::text),
		make_column("textEntities", &EditedMessage::textEntities),
		make_column("messageData", &EditedMessage::messageData),
		make_column("mediaPath", &EditedMessage::mediaPath),
		make_column("hqThumbPath", &EditedMessage::hqThumbPath),
		make_column("documentType", &EditedMessage::documentType),
		make_column("documentSerialized", &EditedMessage::documentSerialized),
		make_column("thumbsSerialized", &EditedMessage::thumbsSerialized),
		make_column("documentAttributesSerialized", &EditedMessage::documentAttributesSerialized),
		make_column("mimeType", &EditedMessage::mimeType)
	)
);

rpl::event_stream<> storageChanged;

namespace Database {

void runMigrations() {
	constexpr int kLatestVersion = 0; // No migrations yet
	int currentVersion = 0;
	try {
		if (auto versionRow = storage.get_pointer<SchemaVersion>(1)) {
			currentVersion = versionRow->version;
		} else {
			storage.insert(SchemaVersion{1, 0});
		}
	} catch (...) {
		storage.insert(SchemaVersion{1, 0});
	}

	if (currentVersion >= kLatestVersion) {
		return;
	}
}

void initialize() {
	try {
		storage.sync_schema(true);
		runMigrations();
		storage.sync_schema(true);
	} catch (const std::exception &ex) {
		LOG(("Alex::Database initialization failed: %1").arg(ex.what()));
		storage.sync_schema(true);
		if (!storage.get_pointer<SchemaVersion>(1)) {
			storage.insert(SchemaVersion{1, 0});
		}
	}
}

void addEditedMessage(const EditedMessage &message) {
	try {
		storage.begin_transaction();
		storage.insert(message);
		storage.commit();
		storageChanged.fire({});
	} catch (std::exception &ex) {
		try {
			storage.rollback();
		} catch (...) {
		}
		LOG(("Alex::Database: Failed to save edited message: %1").arg(ex.what()));
	}
}

std::vector<EditedMessage> getEditedMessages(ID userId, ID dialogId, ID messageId, ID minId, ID maxId, int totalLimit) {
	try {
		return storage.get_all<EditedMessage>(
			where(
				column<EditedMessage>(&EditedMessage::userId) == userId and
				column<EditedMessage>(&EditedMessage::dialogId) == dialogId and
				column<EditedMessage>(&EditedMessage::messageId) == messageId and
				(column<EditedMessage>(&EditedMessage::fakeId) > minId or minId == 0) and
				(column<EditedMessage>(&EditedMessage::fakeId) < maxId or maxId == 0)
			),
			order_by(column<EditedMessage>(&EditedMessage::fakeId)).desc(),
			limit(totalLimit)
		);
	} catch (...) {
		return {};
	}
}

bool hasLocalEdits(ID userId, ID dialogId, ID messageId) {
	try {
		return !storage.select(
			columns(column<EditedMessage>(&EditedMessage::messageId)),
			where(
				column<EditedMessage>(&EditedMessage::userId) == userId and
				column<EditedMessage>(&EditedMessage::dialogId) == dialogId and
				column<EditedMessage>(&EditedMessage::messageId) == messageId
			),
			limit(1)
		).empty();
	} catch (...) {
		return false;
	}
}

void addDeletedMessage(const DeletedMessage &message) {
	try {
		storage.begin_transaction();
		storage.insert(message);
		storage.commit();
		storageChanged.fire({});
	} catch (std::exception &ex) {
		try {
			storage.rollback();
		} catch (...) {
		}
		LOG(("Alex::Database: Failed to save deleted message: %1").arg(ex.what()));
	}
}

std::vector<DeletedMessage> getDeletedMessages(ID userId, ID dialogId, ID topicId, ID minId, ID maxId, int totalLimit, const std::string &searchQuery) {
	try {
		if (searchQuery.empty()) {
			return storage.get_all<DeletedMessage>(
				where(
					column<DeletedMessage>(&DeletedMessage::userId) == userId and
					(column<DeletedMessage>(&DeletedMessage::dialogId) == dialogId or dialogId == 0) and
					(column<DeletedMessage>(&DeletedMessage::topicId) == topicId or topicId == 0) and
					(column<DeletedMessage>(&DeletedMessage::messageId) > minId or minId == 0) and
					(column<DeletedMessage>(&DeletedMessage::messageId) < maxId or maxId == 0)
				),
				order_by(column<DeletedMessage>(&DeletedMessage::messageId)).desc(),
				limit(totalLimit)
			);
		}

		std::string escaped;
		escaped.reserve(searchQuery.size());
		for (const auto c : searchQuery) {
			if (c == '%' || c == '_' || c == '\\') {
				escaped += '\\';
			}
			escaped += c;
		}
		const auto pattern = "%" + escaped + "%";
		return storage.get_all<DeletedMessage>(
			where(
				column<DeletedMessage>(&DeletedMessage::userId) == userId and
				(column<DeletedMessage>(&DeletedMessage::dialogId) == dialogId or dialogId == 0) and
				(column<DeletedMessage>(&DeletedMessage::topicId) == topicId or topicId == 0) and
				(column<DeletedMessage>(&DeletedMessage::messageId) > minId or minId == 0) and
				(column<DeletedMessage>(&DeletedMessage::messageId) < maxId or maxId == 0) and
				like(column<DeletedMessage>(&DeletedMessage::text), pattern, "\\")
			),
			order_by(column<DeletedMessage>(&DeletedMessage::messageId)).desc(),
			limit(totalLimit)
		);
	} catch (...) {
		return {};
	}
}

bool hasDeletedMessages(ID userId, ID dialogId, ID topicId) {
	try {
		return !storage.select(
			columns(column<DeletedMessage>(&DeletedMessage::dialogId)),
			where(
				column<DeletedMessage>(&DeletedMessage::userId) == userId and
				column<DeletedMessage>(&DeletedMessage::dialogId) == dialogId and
				(column<DeletedMessage>(&DeletedMessage::topicId) == topicId or topicId == 0)
			),
			limit(1)
		).empty();
	} catch (...) {
		return false;
	}
}

void clearDeletedMessages(ID userId, ID dialogId, ID topicId) {
	try {
		storage.remove_all<DeletedMessage>(
			where(
				column<DeletedMessage>(&DeletedMessage::userId) == userId and
				column<DeletedMessage>(&DeletedMessage::dialogId) == dialogId and
				(column<DeletedMessage>(&DeletedMessage::topicId) == topicId or topicId == 0)
			)
		);
		storageChanged.fire({});
	} catch (...) {
	}
}

void removeDeletedMessage(ID userId, ID dialogId, ID messageId) {
	try {
		storage.remove_all<DeletedMessage>(
			where(
				column<DeletedMessage>(&DeletedMessage::userId) == userId and
				column<DeletedMessage>(&DeletedMessage::dialogId) == dialogId and
				column<DeletedMessage>(&DeletedMessage::messageId) == messageId
			)
		);
		storageChanged.fire({});
	} catch (...) {
	}
}

StorageStats getStorageStats(ID userId) {
	StorageStats stats;
	try {
		const auto dbPath = u"./tdata/alexdata.db"_q;
		stats.databaseFileSize = QFileInfo(dbPath).size();

		auto getCatStats = [&](int type) -> Alex::StorageStats::Entry {
			auto count = storage.count<DeletedMessage>(
				where(column<DeletedMessage>(&DeletedMessage::userId) == userId and
					  column<DeletedMessage>(&DeletedMessage::documentType) == type)
			);
			auto sizePtr = std::move(storage.select(
				sum(column<DeletedMessage>(&DeletedMessage::messageData)),
				where(column<DeletedMessage>(&DeletedMessage::userId) == userId and
					  column<DeletedMessage>(&DeletedMessage::documentType) == type)
			).front());
			return { (int)count, (int64)(sizePtr ? *sizePtr : 0) };
		};

		stats.text = getCatStats(0);
		stats.photo = getCatStats(1);
		stats.video = getCatStats(2);
		stats.audio = getCatStats(3);
		stats.document = getCatStats(4);
		stats.other = getCatStats(5);

		stats.total.count = stats.text.count + stats.photo.count + stats.video.count + stats.audio.count + stats.document.count + stats.other.count;
		stats.total.size = stats.text.size + stats.photo.size + stats.video.size + stats.audio.size + stats.document.size + stats.other.size;

		auto editCount = storage.count<EditedMessage>(
			where(column<EditedMessage>(&EditedMessage::userId) == userId)
		);
		auto editSizePtr = std::move(storage.select(
			sum(column<EditedMessage>(&EditedMessage::messageData)),
			where(column<EditedMessage>(&EditedMessage::userId) == userId)
		).front());
		stats.edits = { (int)editCount, (int64)(editSizePtr ? *editSizePtr : 0) };

	} catch (const std::exception &ex) {
		LOG(("Alex::Database Stats Error: %1").arg(ex.what()));
	}
	return stats;
}

rpl::producer<StorageStats> storageStatsValue(ID userId) {
	return rpl::single(rpl::empty_value())
		| rpl::then(storageChanged.events())
		| rpl::map([=](auto) {
			return rpl::make_producer<StorageStats>([=](auto consumer) {
				crl::async([=] {
					auto stats = getStorageStats(userId);
					crl::on_main([=, stats = std::move(stats)]() mutable {
						consumer.put_next(std::move(stats));
						consumer.put_done();
					});
				});
				return rpl::lifetime();
			});
		}) | rpl::flatten_latest();
}

void clearStorage(ID userId, int documentType) {
	try {
		if (documentType == -1) {
			storage.remove_all<EditedMessage>(
				where(column<EditedMessage>(&EditedMessage::userId) == userId)
			);
		} else {
			storage.remove_all<DeletedMessage>(
				where(
					column<DeletedMessage>(&DeletedMessage::userId) == userId and
					column<DeletedMessage>(&DeletedMessage::documentType) == documentType
				)
			);
		}
		storage.vacuum();
		storageChanged.fire({});
	} catch (...) {
	}
}

void clearAllStorage(ID userId) {
	try {
		storage.remove_all<DeletedMessage>(
			where(column<DeletedMessage>(&DeletedMessage::userId) == userId)
		);
		storage.remove_all<EditedMessage>(
			where(column<EditedMessage>(&EditedMessage::userId) == userId)
		);
		storage.vacuum();
		storageChanged.fire({});
	} catch (...) {
	}
}

} // namespace Database
} // namespace Alex
