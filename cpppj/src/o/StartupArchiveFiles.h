// 원문두아카이브열거가같은FileSpec/출력고리와설정Load까지의검색수명을사용한다.
#pragma once
#include "o/FileSpecQuery.h"
#include <memory>

namespace netstorm::o { // 실제VFS와독립Native재생이같은부모순서를사용한다.
class RawFileName; // Client의장기10칸소유자를빌린다.
struct StartupArchiveHooks { // 등록/보조root효과만빌리며검색핸들은반환된객체가계속소유한다.
    std::function<void(std::string_view,bool)> registerFile; // false기본/true보조등록이며실패반환도원문처럼열거를중단하지않는다.
    std::function<void(std::string_view)> setSecondary; // 두열거뒤실제CD또는기본root를확정한다.
    FileSpecQueryHooks search; // 비어있으면실제Windows의Unicode검색장치다.
};
std::unique_ptr<FileSpecQuery> BeginStartupArchives(std::string_view primary,std::optional<std::string_view> cd,RawFileName& names,const StartupArchiveHooks& hooks); // Format→Full/등록/Next를기본·보조에연결하고설정Load뒤까지살려둘같은검색객체를반환한다.
}
