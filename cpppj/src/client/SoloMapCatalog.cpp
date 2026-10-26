// 전체 지형 파싱 없이 같은 원본 파일/섹션 읽기 부모에서 맵 목록의 메타 자료만 읽는다.
#include "client/SoloMapCatalog.h"
#include "client/CampaignTemplateStorage.h"
#include "o/BaseFile.h"
#include "o/ConfigInterface.h"
#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
// 현재 키는 원본처럼 동일한 고정 버퍼를 가리키며 문자열 포인터를 교체하지 않는다.
std::string_view SoloMapFileSpec::Current() const { return {key_.data(),key_.size()-1}; }
// 정상 목록 키의 NUL까지 같은9BYTE에 복사한다. dirty 수명은 이 저장소가 소유하지 않는다.
void SoloMapFileSpec::SelectFort() { std::copy_n("fortSpec",key_.size(),key_.begin()); }
// 단독 시나리오 목록 키의 NUL까지 같은 버퍼에 복사한다.
void SoloMapFileSpec::SelectSolo() { std::copy_n("SOLOSpec",key_.size(),key_.begin()); }
namespace {
// 실제로 선택한 외부 경계만 요구한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("단독 맵 파일 목록의 현재 장치 미연결");return function;
}
// 00459c40(1)의004595a0은 같은 파일명에 있는 모든 점 뒤를 지운다. 마지막 확장자만 제거하지 않는다.
std::string MapName(std::string_view path) {
    auto name=std::filesystem::path(path).filename().string();const auto dot=name.find('.');if (dot!=std::string::npos) name.resize(dot);return name;
}
// 분류용과 목록용 Template은 파일을 두 번 읽고 State도 각자 소유한다.
struct MapProbe {
    CampaignTemplateStorage storage; // 실제 파일 읽기/현재 섹션/빈 State 생성의 소유자다.
    std::uint32_t depth{}; // 원본 getter가 빌리는 로딩 깊이다.
    bool mission{}; // 목록의 임시 Template은 미션 소유자가 아니다.
    o::RawPlayerTemplateState state; // 같은 현재 manager의 State를 읽는 실제 getter다.
    // 원본 10.78의 기존/빈/짧은 State 부모를 모두 동일 저장소에 연결한다.
    explicit MapProbe(SoloMapCatalogEffects& effects):storage(effects.serializationGlobals),state(o::OriginalEdition::Patch1078,storage.Headers(),
        [this](auto slot)->o::PlayerTemplateStateStream& { return storage.SelectState(slot); },
        [this](auto slot)->const std::string& { return storage.Name(slot); },depth,mission,
        [](auto) { throw std::logic_error("단독 맵 메타 조회의 실제 Template State 진단"); },
        [](auto) { throw std::logic_error("10.78 단독 맵 조회의 예상하지 않은 CD 로딩 깊이 변경"); }) {
        state.BindInitialization({[&effects](auto,std::span<std::uint8_t,28> bytes) { Need(effects.allocationBytes)(bytes); },
            [this](auto slot,std::span<const std::uint8_t,28> bytes)->o::PlayerTemplateStateStream& { return storage.InitializeState(slot,bytes); }});
        state.BindMinimumSize([this](auto slot,auto size)->o::PlayerTemplateStateStream& { return storage.EnsureStateSize(slot,size); });
    }
    // 파일이 없어지면 빈 Template을 유지한다. 서명/본문 오류를 성공으로 대신하지 않는다.
    void Load(const o::BaseFileSystem& files,o::ConfigInterface& config,SoloMapCatalogEffects& effects,std::string_view name,bool mine) {
        const auto path=config.PathSpec(Need(effects.fileSpecKey)(),name);const auto bytes=effects.readFile ? effects.readFile(path) : files.TryRead(path);if (!bytes) return;
        storage.InstallFile(0,{},*bytes,path,Need(effects.typeCount)());
        storage.Headers()[8]=mine ? 1 : 0; // 목록 임시 Template의 isMine=1은 실제 파일 쓰기를 발생시키지 않는다.
    }
    // 같은 현재 헤더의 첫 DWORD를 조회한다.
    bool Present() { const auto bytes=storage.Headers();return bytes[0] || bytes[1] || bytes[2] || bytes[3]; }
};
}
// 별도 게임 세션 없이 현재 실제 파일/설정과 원본 목록 계산을 연결한다.
struct SoloMapCatalog::Impl {
    const o::BaseFileSystem& files; // 기본 디스크 열거와 디스크/TAFF/보조 경로의 내용 읽기를 빌린다.
    o::ConfigInterface& config; // fortSpec과 mayEditMissions의 현재 설정이다.
    SoloMapFileSpec defaultFileSpec; // 별도 Client 전역을 공급하지 않은 기존 호출도 실제 catalog보다 짧지 않은 기본 버퍼를 빌린다.
    SoloMapCatalogEffects effects; // 언어/사용자/미정 입력의 실제 장치다.
    o::SoloMapListState localCache; o::SoloMapListState& cache; // 실제 Client가 제공한 같은 목록/dirty를 빌리고 독립 호출만 자체 저장소를 쓴다.
    std::unique_ptr<MapProbe> metadata; // 마지막 construct/load가 선택한 실제 임시 Template이다.
    o::RawSoloMapList list; // 파일 분류를 대신 계산하지 않는 원본 목록 몸체다.
    // 콜백이 빌리는 주소가 고정된 뒤 원본 목록을 구성한다.
    Impl(const o::BaseFileSystem& source,o::ConfigInterface& settings,SoloMapCatalogEffects devices)
        :files(source),config(settings),effects(std::move(devices)),cache(effects.listState ? *effects.listState : localCache),list(cache,Hooks()) {
        if (!effects.serializationGlobals) effects.serializationGlobals=std::make_shared<o::FortSerializationGlobals>(); // 같은catalog의분류/메타가공유하는한개DWORD이며Storage생성시마다복제하지않는다.
        if (!effects.fileSpecKey) effects.fileSpecKey=[this] { return defaultFileSpec.Current(); }; // 현재 기본 키도 파일 읽기 직전에 조회한다.
    }
    // 원본 분류 함수의 임시 Template은 목록 메타 Template을 건드리지 않는다.
    std::uint8_t Kind(std::string_view name) {
        MapProbe probe(effects);probe.Load(files,config,effects,name,false);return probe.state.Get(0)[17];
    }
    // 선택된 실제 Template이 없으면 원본의 잘못된 메모리 경계를 명시 거부한다.
    MapProbe& Current() const {
        if (!metadata) throw std::logic_error("단독 맵 목록의 임시 Template 생성 누락");return *metadata;
    }
    // 실제 파일 읽기와 현재 자료만 원본 호출 순서의 각 경계에 공급한다.
    o::SoloMapListHooks Hooks() {
        o::SoloMapListHooks hooks;hooks.mayEditMissions=[this] { return config.GetInt("mayEditMissions"); };
        hooks.enumerate=[this] {
            std::vector<std::string> names;
            const auto pattern=config.PathSpec(Need(effects.fileSpecKey)(),"*"); //실제 열거 직전의 현재 키/설정만 읽으며 분류와 메타 읽기까지 캡처하지 않는다.
            const auto paths=effects.enumeratePaths ? effects.enumeratePaths(pattern) : files.MatchDisk(pattern); //새 장치를 제공하지 않은 독립/기존 호출의 원래 디스크 경계를 보존한다.
            for (const auto& path:paths) names.push_back(MapName(path)); // 원본 OS 순서와 전체 점 제거를 유지하고 TAFF 목록은 합치지 않는다.
            return names;
        };
        hooks.kind=[this](auto name) { return Kind(name); };
        hooks.construct=[this] { metadata=std::make_unique<MapProbe>(effects); };
        hooks.load=[this](auto name) { Current().Load(files,config,effects,name,true); };
        hooks.present=[this] { return Current().Present(); };
        hooks.subscriber=[this] { return Current().storage.ReadSubscriberName(0); };
        hooks.stateByte=[this] { return Current().state.Get(0)[0]; };
        hooks.userName=[this] { return Need(effects.userName)(); };
        hooks.reserved=[this] { if (!Current().Present()) throw std::logic_error("단독 맵 예약 조회의 누락 Template 진단");return 0U; };
        hooks.translate=[this](auto text) { return Need(effects.translate)(text); };return hooks;
    }
};
// 실제 파일을 읽는 시점은 Kind/Refresh 호출 때다.
SoloMapCatalog::SoloMapCatalog(const o::BaseFileSystem& files,o::ConfigInterface& config,SoloMapCatalogEffects effects):impl_(std::make_unique<Impl>(files,config,std::move(effects))) {}
// 고정 콜백보다 먼저 원본 목록 수명을 끝내고 소유 할당을 반납한다.
SoloMapCatalog::~SoloMapCatalog()=default;
// 분류마다 별도 임시 State를 사용한다.
std::uint8_t SoloMapCatalog::Kind(std::string_view name) { return impl_->Kind(name); }
// 캐시가 유효하면 원본처럼 파일을 다시 읽지 않는다.
std::int32_t SoloMapCatalog::Refresh(std::int32_t mode) { return impl_->list.Refresh(mode); }
// 표시 시점의 현재 선택과 번역을 그대로 사용한다.
std::optional<o::SoloMapRow> SoloMapCatalog::Row(std::int32_t index,std::string_view current,std::string_view fallback) const { return impl_->list.Row(index,current,fallback); }
// 다음 목록 조회에만 dirty 효과가 적용된다.
void SoloMapCatalog::Invalidate() { impl_->cache.dirty=1; }
// 복사본 대신 같은 원본 목록을 읽는다.
const o::SoloMapListState& SoloMapCatalog::Cached() const { return impl_->cache; }
}
