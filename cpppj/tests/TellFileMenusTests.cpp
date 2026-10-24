// 실제 @/# 원문384행과 Config 등록/치환·고리/선택 경계를 대조한다.
#include "RawSceneSupport.h"
#include "client/TellFileMenus.h"
#include "o/OriginalText.h"
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 원문 도구와 같은 파일/기존 Config 입력만 공급한다.
std::string DecodeFileHex(std::string_view value) { std::string result;for (std::size_t index=0;index<value.size();index+=2) result+=static_cast<char>(std::stoul(std::string(value.substr(index,2)),nullptr,16));return result; } // 각 두 자리를 원본 한BYTE로 읽는다.
std::string EncodeFileHex(std::string_view value) { return Hex({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); } // UTF-8 변환을 추가하지 않는 원문 바이트 기록이다.
std::string BaseName(std::string_view path) { const auto at=path.find_last_of("/\\:");auto name=std::string(path.substr(at==path.npos ? 0 : at+1));name.resize(name.find('.')==name.npos ? name.size() : name.find('.'));return name; } // 명시 FileSpec 입력의 같은 기본 이름이다.
std::vector<std::uint8_t> FileBytes(std::string_view value) { return {value.begin(),value.end()}; } // 실제 Config 적재에 원본 평문 바이트를 공급한다.
struct FileMenuScene { // 실제 Config·장기 파일 목록과 문자열 고리를 같은 주소에 보유한다.
    std::vector<std::string> paths;std::uint32_t profile{};std::string trace; // 원문 관측의 입력과 선택 효과 순서다.
    netstorm::o::ConfigInterface config;ScriptStringRing strings;std::unique_ptr<netstorm::o::Config> existing; // 실제 장기 등록부와 고리다.
    std::unique_ptr<TellFileMenus> menus; // 기존 객체/Config보다 먼저 반납한다.
    void Event(std::string value) { if (!trace.empty()) trace+=';';trace+=value; } // 실제 호출한 파일/메뉴 효과를 기록한다.
    std::string Title(std::string_view name) const { if (profile==1 && netstorm::o::AsciiLower(name)=="a") return {};if (profile==1 && netstorm::o::AsciiLower(name)=="b") return "--divider";return "Title "+std::string(name); } // 기대 정렬과 무관한 원문 파일 내용의 명시 입력이다.
    FileMenuScene():config([this](auto path)->std::optional<std::vector<std::uint8_t>> { // 파일 적재 시 실제 최초 메타/등록을 읽는다.
        const auto name=BaseName(path);const auto* object=config.Registry().FindByName(name);if (!object) throw std::logic_error("파일 적재 전에 실제 Config 등록 누락");const auto index=object->GetRaw("index");CHECK(object->GetRaw("path")==path && object->GetRaw("fileName")==name && index);Event("L,"+EncodeFileHex(name)+','+EncodeFileHex(path)+','+index.value_or(""));return FileBytes("title=\""+Title(name)+"\"\n");
    }) { config.Configuration().Assign("ready=1\n");menus=std::make_unique<TellFileMenus>(config,[this](auto pattern) { CHECK(pattern=="input");return paths; }); } // 실제 파일 목록은 원문 장치와 같은 순서로만 공급한다.
    std::string Run(const std::vector<std::string>& files,std::uint32_t titleProfile,bool hasExisting,std::string_view argument,bool fallback) {
        paths=files;profile=titleProfile;if (hasExisting) { existing=std::make_unique<netstorm::o::Config>(config.Registry(),"b");existing->Assign("title=Existing\nindex=99\n"); }
        menus->Append("input",0x02000095,argument,fallback,strings,[this](const auto& item) { const auto current=config.SubstituteLocal("{cur.file}");if (item.separator) Event("S,"+EncodeFileHex(current));else Event("R,"+EncodeFileHex(item.label)+','+std::to_string(item.command)+','+EncodeFileHex(strings.Resolve(item.parameter))+','+EncodeFileHex(current)); }); // 같은 cur.file이 실제 행 생성이 끝날 때까지 살아 있어야 한다.
        CHECK(config.Registry().FindByName("cur")==nullptr);return trace;
    }
};
}

TEST_CASE(TellFileMenus_Native384) { // 실제 두 원문 부모의 파일 적재/중복 제거·정렬·제목/고리/구분선 전체 효과를 대조한다.
    const auto rows=LoadFixture(NETSTORM_TELL_FILE_MENUS_FIXTURE).rows;CHECK(rows.size()==384);
    for (const auto& row:rows) { // 매 관측은 실제 Config 등록부와 파일 목록을 독립 생성한다.
        CHECK(row.size()==7);std::vector<std::string> paths;if (!row[1].empty()) for (const auto& path:Split(row[1],',')) paths.push_back(DecodeFileHex(path)); // 원문 순서를 유지하며 중복 입력도 지우지 않는다.
        FileMenuScene scene;CHECK(scene.Run(paths,Number(row[2]),Number(row[3])!=0,DecodeFileHex(row[4]),Number(row[5])!=0)==row[6]);
    }
}

TEST_CASE(TellFileMenus_ActualPersistentReuseFirstPathIndexAndCurLifetime) { // 첫 파일의 메타를 유지하고 다음 목록의 새 파일만 적재한다.
    FileMenuScene scene;scene.paths={"d/b.english","d/a.english","e/a.french"};std::vector<std::string> rows;const auto before=scene.config.Registry().Objects().size();
    const auto emit=[&](const TellFileMenuItem& item) { rows.push_back(scene.config.SubstituteLocal("{cur.file}")+":"+std::string(scene.strings.Resolve(item.parameter)));CHECK(scene.config.Registry().FindByName("cur")!=nullptr); }; // 생성 중 실제 지역 객체를 재조회한다.
    scene.menus->Append("input",7,"{cur.file}:{a.index}:{b.index}",true,scene.strings,emit);CHECK(rows==std::vector<std::string>({"a:a:2:1","b:b:2:1"}));const auto* first=scene.config.Registry().FindByName("a");CHECK(first && first->GetRaw("path")=="d/a.english" && scene.config.Registry().Objects().size()==before+2);
    scene.paths={"different/a.german","d/c.english"};rows.clear();scene.menus->Append("input",7,"{cur.file}",true,scene.strings,emit);CHECK(rows==std::vector<std::string>({"a:a","c:c"}) && scene.config.Registry().FindByName("a")==first && first->GetRaw("path")=="d/a.english" && scene.config.Registry().FindByName("c")->GetRaw("index")=="2");
    scene.menus.reset();CHECK(scene.config.Registry().Objects().size()==before && scene.config.Registry().FindByName("cur")==nullptr); // 소유 목록을 반납할 때만 장기 Config가 사라진다.
}

TEST_CASE(TellFileMenus_ActualOuterCurAndNumericStringRing) { // 숫자 인자도 같은 고리에 쓰고 지역 cur는 기존 값을 일시 가린다.
    FileMenuScene scene;netstorm::o::Config outer(scene.config.Registry(),"cur");outer.Assign("file=outer\n");scene.paths={"d/a.english"};std::uint32_t selected{};
    scene.menus->Append("input",9,"123",false,scene.strings,[&](const auto& item) { selected=item.parameter;CHECK(scene.config.SubstituteLocal("{cur.file}")=="a" && scene.strings.Resolve(item.parameter)=="123"); });
    CHECK(selected!=123 && scene.strings.Index()==1 && scene.config.SubstituteLocal("{cur.file}")=="outer" && scene.config.Registry().FindByName("cur")==&outer);
}

TEST_CASE(TellFileMenus_ActualMetadataPrecedesFileAndMissingReadKeepsConfig) { // 파일 본문이 최초 메타를 덮지 못하고 없는 파일도 이미 등록된 객체를 남긴다.
    netstorm::o::ConfigInterface config([](auto path)->std::optional<std::vector<std::uint8_t>> { return path=="d/missing.english" ? std::nullopt : std::optional<std::vector<std::uint8_t>>(FileBytes("path=forged\nfileName=forged\nindex=99\ntitle=Actual\n")); });config.Configuration().Assign("ready=1\n");ScriptStringRing strings;TellFileMenus menus(config,[](auto) { return std::vector<std::string>{"d/found.english","d/missing.english"}; });std::vector<std::string> labels;
    menus.Append("input",7,"{cur.file}",true,strings,[&](const auto& item) { labels.push_back(item.label); });const auto* found=config.Registry().FindByName("found");const auto* missing=config.Registry().FindByName("missing");CHECK(found && found->GetRaw("path")=="d/found.english" && found->GetRaw("fileName")=="found" && found->GetRaw("index")=="1" && missing && missing->GetRaw("index")=="2" && labels==std::vector<std::string>({"Actual","{missing.TITLE}"})); // 중첩 local.1의 값은 원본처럼 대소문자를 바꾸지 않는다.
}

TEST_CASE(TellFileMenus_SelectedFailurePreservesLoadedConfigsAndRetiresCur) { // 선택된 메뉴 작성 실패와 지역 NUL/40개 범위를 별도로 확인한다.
    FileMenuScene scene;scene.paths={"d/a.english"};const auto before=scene.config.Registry().Objects().size();bool failed{};
    try { scene.menus->Append("input",9,"value",true,scene.strings,{}); }catch (const std::logic_error&) { failed=true; }CHECK(failed && scene.config.Registry().Objects().size()==before+1 && scene.config.Registry().FindByName("cur")==nullptr && scene.strings.Index()==1);
    bool oversize{},tooMany{};try { scene.menus->Append(std::string(256,'A'),9,"value",true,scene.strings,{}); }catch (const std::length_error&) { oversize=true; }
    scene.paths.clear();for (int index=0;index<41;++index) scene.paths.push_back("d/new"+std::to_string(index)+".english"); // 원문 배열 밖 입력에서도 앞서 적재한 실제 부수 효과는 남는다.
    try { scene.menus->Append("input",9,"value",true,scene.strings,{}); }catch (const std::length_error&) { tooMany=true; }CHECK(oversize && tooMany && scene.config.Registry().FindByName("new40")!=nullptr && scene.config.Registry().FindByName("cur")==nullptr);
}
