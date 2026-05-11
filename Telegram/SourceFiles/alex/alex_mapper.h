#pragma once

#include "alex/alex_database.h"
#include <QString>
#include <vector>

class HistoryItem;

namespace Alex {
namespace Mapper {

int mapItemFlagsToMTPFlags(not_null<HistoryItem*> item);
std::pair<std::string, std::string> serializeTextWithEntities(not_null<HistoryItem*> item);

void map(not_null<HistoryItem*> item, DeletedMessage &message);
void map(not_null<HistoryItem*> item, EditedMessage &message);

} // namespace Mapper
} // namespace Alex
