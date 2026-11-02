// 실제41ab10의독립기대값과VFS metadata/디스크/검색/동일고리를대조한다.
#include "FileProbeTestSupport.h"
#include "o/BaseFile.h" //실제VFS 존재조회 검사에서 값형완료정의를명시사용한다.
#include "o/RawBaseFileReadPolicy.h"
#include "o/RawFileName.h"
#include <limits>

namespace { // 입력BYTE/사건열만해석하며존재출처기대는계산하지않는다.
using namespace netstorm::o;using namespace netstorm::test::rawscene;
std::string ProbeBytes(std::string_view hex) { std::string result;if (hex=="-") return result;for (std::size_t at=0;at<hex.size();at+=2) result+=static_cast<char>(std::stoul(std::string(hex.substr(at,2)),nullptr,16));return result; } // NUL뒤물리도그대로입력한다.
std::string ProbeHex(std::string_view value) { return Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); } // 현재C문자열을재인코딩하지않는다.
std::string ProbeEvents(const std::vector<std::string>& events) { std::string result;for (const auto& event:events) { if (!result.empty()) result+=';';result+=event; }return result; } // Native순서를그대로직렬화한다.
}

TEST_CASE(FileProbe_Native640SourcesFirstDeviceSlotSearchLifetimeAndNoPayload) { // 모든5bit출처조합·Find실패/성공·선행고리를원문과대조한다.
    std::size_t count{};
    for (const auto& row:LoadFixture(NETSTORM_FILE_PROBE_FIXTURE).rows) { // 최초640행을독립입력과출력으로분리한다.
        CHECK(row.size()==9);const auto input=ProbeBytes(row[0]);const auto mask=Number(row[1]),found=Number(row[2]),initial=Number(row[3]);RawFileName names;std::array<const std::uint8_t*,10> slots{};
        for (unsigned call=0;call<10;++call) { static_cast<void>(names.Format({}));slots[names.Index()]=names.CurrentBytes().data(); } // 실제고리주소를빌린뒤번호0을유지한다.
        for (std::uint32_t call=0;call<initial;++call) static_cast<void>(names.Format({})); // 원문입력번호만설정한다.
        bool active{};std::vector<std::string> events;FileSpecQueryHooks search;search.find=[&](auto pattern) { CHECK(!active);active=true;events.push_back("find:"+ProbeHex(pattern));return FileSpecFindResult{found?0x1234:std::numeric_limits<std::uintptr_t>::max(),"chosen.gif"}; };search.close=[&](auto handle) { CHECK(active);events.push_back("searchClose:"+std::to_string(static_cast<std::uint32_t>(handle)));active=false; };
        const auto literal=std::string_view(input).substr(0,input.find('\0'));const bool patterned=literal.find_first_of("*?")!=literal.npos;
        const auto result=ProbeBaseFileByPolicy(input,[&](auto site,auto query) { if (patterned) CHECK(active);const auto which=static_cast<std::uint32_t>(site);if (site==BaseFileReadSite::PrimaryArchive || site==BaseFileReadSite::SecondaryArchive) { events.push_back("A:"+std::to_string(site==BaseFileReadSite::SecondaryArchive?1:0)+":"+ProbeHex(query));return (mask&(1U<<which))!=0; }
            const auto name=site==BaseFileReadSite::DirectDisk?std::string(query):JoinBaseFileRoot(site==BaseFileReadSite::PrimaryDisk?"C:\\base":"D:\\disc",query,names);events.push_back("D:"+std::to_string(which)+":"+ProbeHex(name));if (mask&(1U<<which)) { events.push_back("C:304087040");return true; }return false; // FILE토큰은Native PLAYERS이며열기/닫기만있고payload/길이장치는없다.
        },&names,search);
        CHECK(!active && result==Number(row[4]) && ProbeEvents(events)==row[5] && names.Index()==Number(row[6]) && Hex(std::span<const std::uint8_t>(slots[(initial+1)%10],256))==row[7] && Hex(names.CurrentBytes())==row[8]);++count;
    }
    CHECK(count==640);
}

TEST_CASE(FileProbe_ActualUnicodeMetadataTruncatedLooseAndSecondarySources) { // 존재판정이짧은payload를파싱/해독하지않고원문출처번호를유지한다.
    using namespace netstorm;using namespace test::fileprobe;const auto root=Work(),primary=root/"primary",secondary=root/"secondary";const std::array<std::uint8_t,3> shortPayload{{'b','a','d'}};Write(primary/"d/meta.gif",shortPayload);Write(primary/"primary.tarc",Taff("d/meta.gif",shortPayload));Write(primary/"d/disk.gif",shortPayload);Write(secondary/"d/second.gif",shortPayload);Write(secondary/"secondary.tarc",Taff("d/archive.gif",shortPayload));BaseFileSystem files(primary,secondary);CHECK(files.RegisterArchive(primary/"primary.tarc") && files.RegisterArchive(secondary/"secondary.tarc",true));auto names=files.FileNameOwner();
    const auto check=[&](std::string_view path,std::uint32_t source,std::uint32_t consumed) { const auto before=names->Index();CHECK(files.Probe(path)==source && names->Index()==(before+consumed)%10); }; // 실제출처별고리소비도함께대조한다.
    BaseFileSystem metadataOnly(std::filesystem::path{});CHECK(metadataOnly.RegisterArchive(primary/"primary.tarc") && metadataOnly.Probe("d/meta.gif")==2 && metadataOnly.FileNameOwner()->Index()==1); // 등록metadata성공에는빈게임root의절대화/CWD조회나디스크열기가필요없다.
    check("d/meta.gif",2,1);check("d/disk.gif",1,2);check("d/second.gif",3,3);check("d/archive.gif",4,3);check("d/missing.gif",0,3);check(platform::PathToUtf8(primary/"d/disk.gif"),5,1); // direct부재는보조/TAFF로fallback하지않는다.
    check(platform::PathToUtf8(primary/"d/missing.gif"),0,1);
    std::filesystem::rename(primary/"primary.tarc",primary/"moved.tarc");check("d/meta.gif",2,1); // 등록metadata는원래파일의존재/내용재열기를요구하지않는다.
}

TEST_CASE(FileProbe_ActualWildcardUsesVirtualDirectoryAndKeepsRawPrefixLifetime) { // 실제OS검색은게임root이고Device칸은원래slash규칙의첫이름이다.
    using namespace netstorm;using namespace test::fileprobe;const auto root=Work();const std::array<std::uint8_t,1> bytes{{7}};Write(root/"d/only.gif",bytes);BaseFileSystem files(root);auto names=files.FileNameOwner();std::array<const std::uint8_t*,10> slots{};
    for (unsigned call=0;call<10;++call) { static_cast<void>(names->Format({}));slots[names->Index()]=names->CurrentBytes().data(); } // 고리관측은실제검색보다먼저준비한다.
    const auto directory=std::filesystem::current_path();CHECK(files.Probe("d/*.gif")==0 && names->Index()==3);std::array<std::uint8_t,256> expected{};const std::string found="only.gif";std::copy(found.begin(),found.end(),expected.begin());expected[0]=0;CHECK(std::equal(expected.begin(),expected.end(),slots[1]) && std::filesystem::current_path()==directory); // 존재부모는선택이름으로fopen하지않고원래wildcard를열어실패한다.
}

TEST_CASE(FileProbe_LookupExceptionClosesSameActiveSearchAndKeepsDeviceSlot) { // metadata예외를미발견으로숨기지않고같은검색핸들을반납한다.
    RawFileName names;FileSpecQueryHooks search;bool active{},closed{},failed{};search.find=[&](auto) { active=true;return FileSpecFindResult{0x1234,"found.gif"}; };search.close=[&](auto handle) { CHECK(active && handle==0x1234);active=false;closed=true; };
    try { static_cast<void>(ProbeBaseFileByPolicy("d\\*.gif",[&](auto,auto)->bool { CHECK(active);throw std::runtime_error("명시metadata조회예외"); },&names,search)); }catch (const std::runtime_error&) { failed=true; }
    CHECK(failed && closed && !active && names.Index()==1);
}
