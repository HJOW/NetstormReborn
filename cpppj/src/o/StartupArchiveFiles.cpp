// 원문441965~441a44의실제등록부모순서와남아있는검색핸들을연결한다.
#include "o/StartupArchiveFiles.h"
#include "o/RawFileName.h"
#include <array>
#include <stdexcept>

namespace netstorm::o { // 독립값형파일열거와달리실제Startup의고리수명을사용한다.
std::unique_ptr<FileSpecQuery> BeginStartupArchives(std::string_view primary,std::optional<std::string_view> cd,RawFileName& names,const StartupArchiveHooks& hooks) {
    if (!hooks.registerFile || !hooks.setSecondary) throw std::invalid_argument("Startup의등록/보조경로장치없음");
    const auto pattern=[&](std::string_view root) { const std::array<FileNamePart,2> parts{{{-2,root},{-3,"*.tarc"}}};return names.Format(parts); }; // 실제루트getter뒤한칸작성하고FileSpec이즉시복사한다.
    auto spec=std::make_unique<FileSpecQuery>(pattern(primary),hooks.search);
    const auto readList=[&](bool secondary) { while (spec->Valid()) { hooks.registerFile(names.Full(spec->Text()),secondary);spec->Next(); } }; // Full한칸→등록→Next이며등록결과로정렬/필터링하지않는다.
    readList(false);
    if (cd) { spec->Assign(pattern(*cd));readList(true); } // 두번째Format뒤에첫핸들을닫고같은FileSpec으로재검색한다.
    hooks.setSecondary(cd ? *cd:primary);return spec;
}
}
