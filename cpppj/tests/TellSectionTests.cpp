// 원문4cf730의 선택 투영858행과 실제 Config 등록/적재 수명을 확인한다.
#include "RawSceneSupport.h"
#include "client/TellSection.h"
#include "client/ClientHtmlWindowGlobals.h"
#include "client/TellParameters.h"
#include "o/ConfigInterface.h"
#include <cstdio>
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace { // 원문 관측과 같은 입력/하위 장치만 준비한다.
std::string DecodeHex(std::string_view text) { // UTF-8 변환 없이 원문 바이트를 되돌린다.
    std::string result;for (std::size_t index=0;index<text.size();index+=2) result+=static_cast<char>(std::stoul(std::string(text.substr(index,2)),nullptr,16));return result; // 각 두 자리를 원본 한BYTE로 읽는다.
}
std::string EncodeHex(std::string_view text) { return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}); } // 원문 문자열의 바이트만 기록한다.
std::string Fingerprint(const TellSectionState& state) { // NUL 뒤를 포함해 전체256BYTE의 순서를 기록한다.
    std::uint32_t value=2166136261U;for (const unsigned char byte:state.current) value=(value^byte)*16777619U; // 원문 투영의 독립 물리 지문이다.
    char output[9]{};std::snprintf(output,sizeof(output),"%08x",value);return output;
}
struct SelectionScene { // 현재 상태와 선택 장치의 존재 여부만 입력한다.
    TellSectionState state;std::uint32_t mask{};bool available{};std::string trace; // 원문 입력과 같은 장기 상태/호출 기록이다.
    void Event(std::string text) { if (!trace.empty()) trace+=';';trace+=text; } // 선택된 실제 하위 효과 순서다.
    std::optional<std::string> Body(bool present,std::string_view name) const { return present ? std::optional<std::string>(mask&32 ? "" : name) : std::nullopt; } // 없음과 빈 문자열 포인터를 구별한다.
    std::string Run(std::string_view target,std::string_view prior,std::uint32_t flags) { // 같은 원문 입력과 현재256BYTE로 공개 제품 함수를 실행한다.
        mask=flags;available=(mask&4)!=0;state.current.fill('Z');std::copy(prior.begin(),prior.end(),state.current.begin());state.current[prior.size()]=0;
        TellSectionHooks hooks;hooks.prior=[&](auto name) { Event("P,"+EncodeHex(name));if (mask&64) { available=!available;const std::string changed="changed";std::copy(changed.begin(),changed.end(),state.current.begin());state.current[changed.size()]=0; } };
        hooks.named=[&](auto prefix,auto section) { Event("N,"+EncodeHex(prefix)+','+EncodeHex(section));return Body((mask&3)==3,"named"); };
        hooks.mission=[&](auto name) { if (!available) return std::optional<std::string>{};Event("M,"+EncodeHex(name));return Body((mask&8)!=0,"mission"); };
        hooks.fallback=[&](auto name) { Event("F,"+EncodeHex(name));return Body((mask&16)!=0,"fallback"); };
        const auto result=FindTellSection(state,target,hooks);return EncodeHex(state.current.data())+'|'+Fingerprint(state)+'|'+(result ? EncodeHex(*result) : "-")+'|'+trace;
    }
};
std::vector<std::uint8_t> Bytes(std::string_view text) { return {text.begin(),text.end()}; } // 실제 Config 파일 공급자의 UTF-8 평문 입력이다.
}

TEST_CASE(TellSection_Native858) { // 두 정밀도 전체 입력/효과·현재256BYTE를 독립 원문 투영과 대조한다.
    const auto rows=LoadFixture(NETSTORM_TELL_SECTION_FIXTURE).rows;CHECK(rows.size()==858);
    for (const auto& row:rows) { CHECK(row.size()==5);SelectionScene scene;CHECK(scene.Run(DecodeHex(row[1]),DecodeHex(row[2]),Number(row[3]))==row[4]); } // 매 행의 현재 상태를 독립 초기화한다.
}

TEST_CASE(TellSection_ActualRegistryPriorityMissingPrefixAndEmptySection) { // 등록된 섹션 우선과 미등록 접두어의 미션 폴백을 실제 Config로 확인한다.
    std::vector<std::string> reads;netstorm::o::ConfigInterface config([&](auto path)->std::optional<std::vector<std::uint8_t>> { reads.emplace_back(path);return Bytes("[file.part]\nFallback\n[missing.part]\nFallback missing\n[END]\n"); });
    config.Configuration().Assign("missionSpec=d/{local.1}.english\n");netstorm::o::Config file(config.Registry(),"file");file.Assign("[part]\nNamed\n[empty]\n[END]\n");TellSectionState state;std::size_t missionCalls{};
    const auto mission=[&](auto name)->std::optional<std::string> { ++missionCalls;return name=="missing.part" ? std::optional<std::string>("Mission") : std::nullopt; }; // 실제 현재 미션 조회의 호출 인자도 검증한다.
    CHECK(FindTellSectionInConfig(state,"file.part",config,mission)=="Named\n" && missionCalls==0 && reads.empty());
    const auto empty=FindTellSectionInConfig(state,"file.empty",config,mission);CHECK(empty && empty->empty() && missionCalls==0 && reads.empty());
    CHECK(FindTellSectionInConfig(state,"missing.part",config,mission)=="Mission" && missionCalls==1 && reads.empty());
    CHECK(config.Registry().FindByName("missing")==nullptr && config.SubstituteLocal("{global.priorTell}")=="file.empty");
}

TEST_CASE(TellSection_ActualFreshFallbackLifetimeAndMissingTargetHistory) { // 매번 파일을 재적재하고 실패한 이름도 다음 priorTell에 남겨야 한다.
    std::size_t version{},reads{},duringObjects{};netstorm::o::ConfigInterface* active{};
    netstorm::o::ConfigInterface config([&](auto path)->std::optional<std::vector<std::uint8_t>> { ++reads;CHECK(path=="d/tell.english");duringObjects=active->Registry().Objects().size();CHECK(active->Registry().FindByName("tell")!=nullptr);return Bytes(version ? "[About]\nSecond\n[END]\n" : "[About]\nFirst\n[END]\n"); });active=&config;
    config.Configuration().Assign("missionSpec=d/{local.1}.english\n");netstorm::o::Config existing(config.Registry(),"tell");existing.Assign("[About]\nCached\n[END]\n");const auto before=config.Registry().Objects().size();TellSectionState state;
    CHECK(FindTellSectionInConfig(state,"About",config,{})=="First\n" && reads==1 && duringObjects==before+1 && config.Registry().Objects().size()==before && config.Registry().FindByName("tell")==&existing);
    version=1;CHECK(FindTellSectionInConfig(state,"ABOUT",config,{})=="Second\n" && reads==2 && config.SubstituteLocal("{global.priorTell|none}")=="");
    CHECK(!FindTellSectionInConfig(state,"missing.part",config,{}) && reads==3 && config.SubstituteLocal("{global.priorTell}")=="ABOUT");
    CHECK(FindTellSectionInConfig(state,"About",config,{})=="Second\n" && config.SubstituteLocal("{global.priorTell}")=="missing.part" && config.Registry().Objects().size()==before);
}

TEST_CASE(TellSection_ActualParamScopeFirstRegisteredObjectAndClientLifetime) { // 이름 조회는 첫 등록 객체를, 값 치환은 현재 지역 파라미터를 사용한다.
    netstorm::o::ConfigInterface config({});config.Configuration().Assign("missionSpec=d/{local.1}.english\nwidth=800\n");netstorm::o::Config outer(config.Registry(),"param");outer.Assign("1=outer\n");netstorm::o::Config older(config.Registry(),"file");older.Assign("[part]\n{param.1}\n[END]\n");ClientHtmlWindowGlobals owner;
    {
        netstorm::o::Config latest(config.Registry(),"file");latest.Assign("[part]\nNew\n[END]\n");netstorm::o::Config parameters(config.Registry(),"param");const auto name=ParseTellParameters("file.part({width})",{[&](auto text) { return config.SubstituteLocal(text); },[&](auto line) { parameters.Append(line); }});
        const auto body=FindTellSectionInConfig(owner.tell,name,config,{});CHECK(config.Registry().FindByName("file")==&older && body=="{param.1}\n" && config.SubstituteLocal(*body)=="800\n");owner.window.instances=3; // 실제 Config 치환은 본문의 LF를 보존한다.
    }
    const auto body=FindTellSectionInConfig(owner.tell,"file.part",config,{});CHECK(config.SubstituteLocal("{param.1}")=="outer" && body=="{param.1}\n" && config.SubstituteLocal(*body)=="outer\n" && std::string(owner.tell.current.data())=="file.part" && owner.window.instances==3);
}

TEST_CASE(TellSection_SelectedMissingServicesAndPhysicalBounds) { // 선택하지 않은 장치는 요구하지 않으며 물리 범위 밖 복사만 거부한다.
    TellSectionState state;TellSectionHooks hooks;hooks.prior=[](auto) {};hooks.named=[](auto,auto) { return std::optional<std::string>(""); };CHECK(FindTellSection(state,"file.",hooks)=="");
    bool missing{},oversize{},unterminated{};try { FindTellSection(state,"next",hooks); }catch (const std::logic_error&) { missing=true; }CHECK(std::string(state.current.data())=="next");
    try { FindTellSection(state,std::string(256,'A'),hooks); }catch (const std::length_error&) { oversize=true; }
    state.current.fill('A');try { FindTellSection(state,"next",hooks); }catch (const std::length_error&) { unterminated=true; }CHECK(missing && oversize && unterminated);
}
