// 원문두목록의실제등록/검색순서와전체고리·Full의물리입력/예외수명을확인한다.
#include "RawSceneSupport.h"
#include "o/StartupArchiveFiles.h"
#include "o/RawFileName.h"
#include <limits>
#include <map>

namespace { // 원문fixture의BYTE와명시OS입력만해독한다.
std::string ArchiveBytes(std::string_view hex) { // 빈문자열/첫NUL뒤물리BYTE를모두보존한다.
    std::string result;if (hex=="-") return result;for (std::size_t at=0;at<hex.size();at+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(at,2)),nullptr,16)); // 두자리당한BYTE다.
    return result;
}
std::string ArchiveHex(std::string_view text) { return netstorm::test::rawscene::Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } // 원문문자열의인코딩을재해석하지않는다.
}

TEST_CASE(StartupArchives_Native54FullRingsAndSameHandleAcrossLists) { // 동일객체재대입/실패핸들/설정경계뒤닫기순서를독립기계어와대조한다.
    using namespace netstorm;using namespace test::rawscene;std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_STARTUP_ARCHIVES_FIXTURE).rows) { // 각입력은새BSS0고리/명시초기번호에서시작한다.
        CHECK(row.size()==9);o::RawFileName names;std::array<const std::uint8_t*,10> slots{};
        for (unsigned call=0;call<10;++call) { static_cast<void>(names.Format({}));slots[names.Index()]=names.CurrentBytes().data(); } // 물리주소를조회한뒤번호0을유지한다.
        for (std::uint32_t call=0;call<Number(row[3]);++call) static_cast<void>(names.Format({})); // 원문초기번호0/9와맞춘다.
        std::vector<std::string> entries{"z.tarc","A.tarc","bad.tarc"},events;entries.resize(Number(row[2]));std::map<std::uintptr_t,std::size_t> cursors;std::uintptr_t opened{};o::StartupArchiveHooks hooks;
        hooks.search.find=[&](auto pattern) { events.push_back("find:"+ArchiveHex(pattern));++opened;const auto handle=entries.empty()?std::numeric_limits<std::uintptr_t>::max():0x1200+opened;if (!entries.empty()) cursors[handle]=0;return o::FileSpecFindResult{handle,entries.empty()?std::string{}:entries.front()}; };
        hooks.search.next=[&](auto handle)->std::optional<std::string> { events.push_back("next:"+std::to_string(handle));const auto at=++cursors[handle];return at<entries.size()?std::optional<std::string>{entries[at]}:std::nullopt; };
        hooks.search.close=[&](auto handle) { events.push_back("close:"+std::to_string(static_cast<std::uint32_t>(handle))); };
        hooks.registerFile=[&](auto path,bool secondary) { events.push_back(std::string(secondary?"secondary:":"primary:")+ArchiveHex(path)); };hooks.setSecondary=[&](auto path) { events.push_back("secondary-root:"+ArchiveHex(path)); };
        const auto primary=ArchiveBytes(row[0]),cd=ArchiveBytes(row[1]=="NULL"?"-":row[1]);auto spec=o::BeginStartupArchives(primary,row[1]=="NULL"?std::nullopt:std::optional<std::string_view>{cd},names,hooks);
        CHECK(!spec->Valid() && ArchiveHex(spec->Text())==(row[5]=="-"?"":row[5]));CHECK(names.Index()==Number(row[7]));CHECK(!spec->Next()); // 마지막실패뒤다음호출은OS검색/문자열삭제를하지않는다.
        for (std::size_t slot=0;slot<slots.size();++slot) CHECK(Hex(std::span<const std::uint8_t>(slots[slot],256))==std::string_view(row[8]).substr(slot*512,512)); // 두목록에서쓴모든칸과NUL뒤내용까지비교한다.
        events.push_back("load-boundary");spec.reset();CHECK(events==Split(row[4],','));++count;
    }
    CHECK(count==54);
}

TEST_CASE(FileFull_Native16PhysicalInputsAndWholeSlots) { // flag0은.\\제거가아니며첫NUL뒤+2물리주소도따로읽는다.
    using namespace netstorm;using namespace test::rawscene;std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_FILE_FULL_FIXTURE).rows) { // 255BYTE입력과상위BYTE/장치/UNC를그대로전달한다.
        CHECK(row.size()==3);o::RawFileName names;const auto input=ArchiveBytes(row[0]);const auto value=names.Full(input,Number(row[1])!=0);const auto expected=ArchiveBytes(row[2]);CHECK(value==std::string_view(expected.data(),expected.find('\0')));CHECK(names.Index()==1 && Hex(names.CurrentBytes())==row[2]);++count;
    }
    CHECK(count==16);
}

TEST_CASE(FileFull_AliasAndBoundsKeepReservationAndOtherSlots) { // 호스트에서정의되지않은주소읽기/256BYTE밖쓰기를정상절단으로합성하지않는다.
    using namespace netstorm::o;RawFileName names;const auto kept=names.Full(".\\kept");
    for (unsigned call=0;call<9;++call) static_cast<void>(names.Format({})); // 다음호출이별칭과같은물리칸을clear한다.
    CHECK(names.Full(kept).empty() && names.Index()==1);bool overflow{},shortInput{};
    try { static_cast<void>(names.Full(std::string(256,'x'))); }catch (const std::length_error&) { overflow=true; }
    try { static_cast<void>(names.Full("a",false)); }catch (const std::invalid_argument&) { shortInput=true; }
    CHECK(overflow && shortInput && names.Index()==3);CHECK(std::all_of(names.CurrentBytes().begin(),names.CurrentBytes().end(),[](auto byte) { return byte==0; }));
}

TEST_CASE(StartupArchives_RegisterFailureClosesActiveSearchAndKeepsPartialRing) { // 등록콜백의실패에서도활성핸들을반납하고이미쓴고리는보존한다.
    using namespace netstorm::o;RawFileName names;std::vector<std::string> events;StartupArchiveHooks hooks;hooks.search.find=[&](auto) { events.push_back("find");return FileSpecFindResult{0x1234,"one.tarc"}; };hooks.search.close=[&](auto handle) { CHECK(handle==0x1234);events.push_back("close"); };
    hooks.registerFile=[&](auto path,bool secondary) { CHECK(path=="C:\\Game\\one.tarc" && !secondary);events.push_back("register");throw std::runtime_error("등록장치실패"); };hooks.setSecondary=[&](auto) { events.push_back("secondary"); };
    bool failed{};try { static_cast<void>(BeginStartupArchives("C:\\Game",{},names,hooks)); }catch (const std::runtime_error&) { failed=true; }
    CHECK(failed && names.Index()==2 && events==std::vector<std::string>({"find","register","close"}));CHECK(names.Full("still-alive")=="still-alive");
}

TEST_CASE(FileSpecQuery_NonPatternNextAndAssignKeepLastValueAndCloseOldHandle) { // 일반/유효검색/종료/실패핸들을서로구별한다.
    using namespace netstorm::o;std::vector<std::uintptr_t> closed;unsigned nextCalls{},findCalls{};FileSpecQueryHooks hooks;
    hooks.find=[&](auto) { ++findCalls;return FileSpecFindResult{findCalls==1?0x1234:std::numeric_limits<std::uintptr_t>::max(),"one"}; };hooks.close=[&](auto handle) { closed.push_back(handle); };hooks.next=[&](auto)->std::optional<std::string> { ++nextCalls;return {}; };
    FileSpecQuery spec("ordinary",hooks);CHECK(spec.Valid() && !spec.Next() && spec.Text()==".\\ordinary");spec.Assign("d\\*");CHECK(spec.Valid() && spec.Text()=="d\\one");CHECK(!spec.Next() && spec.Text()=="d\\one" && nextCalls==1);
    spec.Assign("C:/other");CHECK(spec.Valid() && !spec.Next() && spec.Text()=="C:\\other" && closed==std::vector<std::uintptr_t>{0x1234});spec.Assign("missing*");CHECK(!spec.Valid() && !spec.Next() && spec.Text().empty() && nextCalls==1);spec.Assign("done");CHECK(closed==std::vector<std::uintptr_t>({0x1234,std::numeric_limits<std::uintptr_t>::max()}));
}
