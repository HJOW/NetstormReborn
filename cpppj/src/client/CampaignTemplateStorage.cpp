// 원본 명시 정리 시점까지 이전 이미지의 소유권을 Client에 유지한다.
#include "client/CampaignTemplateStorage.h"
#include "o/RawDataManagerLifetime.h"
#include "o/RawDataManagerSelection.h"
#include "o/RawDataManagerList.h"
#include "o/RawMemStream.h"
#include "o/RawDataManagerResize.h"
#include "o/RawDataManagerImage.h"
#include "o/RawTemplateLoad.h"
#include "o/RawDataManagerInitialize.h"
#include <new>
#include <algorithm>
#include <optional>
#include <stdexcept>
namespace netstorm::client {
namespace {
// 선택된 파일 장치를 호출한 자리에서만 요구하며 미선택 분기를 선행 거부하지 않는다.
template<class Function> const Function& NeedFile(const Function& function) { if (!function) throw std::logic_error("Template 실제 파일 Load 장치 미연결");return function; }
// State의 실제 파일 섹션 위치를 기존 파서의 이름 표에서 찾는다.
std::size_t StateSection() { const auto names=o::FortTemplate::SectionNames();const auto at=std::find(names.begin(),names.end(),"State");if (at==names.end()) throw std::logic_error("Template 실제 State 섹션 이름 누락");return static_cast<std::size_t>(at-names.begin()); }
// 지형 getter도 파서의 편의 사본 대신 같은 원시 섹션 버퍼를 조회한다.
std::size_t TerritorySection() { const auto names=o::FortTemplate::SectionNames();const auto at=std::find(names.begin(),names.end(),"Territory");if (at==names.end()) throw std::logic_error("Template 실제 Territory 섹션 이름 누락");return static_cast<std::size_t>(at-names.begin()); }
// 표준 파일 저장 위치와 동적 노드의 식별값을 분리한다. 이름이 바뀌어도 노드 handle은 바꾸지 않는다.
std::optional<std::size_t> CanonicalSection(std::string_view name) {
    const auto names=o::FortTemplate::SectionNames();
    for (std::size_t index=0;index<names.size();++index) if (!o::CompareDataManagerSectionNames(names[index],name)) return index; // 원본 검색과 같은 ASCII 대소문자 비교다.
    return {};
}
// manager 핸들의 원본DWORD를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> header) { std::uint32_t result{};
    // 같은 네 바이트를 낮은 비트부터 합친다.
    for (std::size_t index=0;index<4;++index) result|=std::uint32_t(header[index])<<(index*8);return result;
}
// 헤더의 지정DWORD만 써 다른 필드를 보존한다.
void Put(std::span<std::uint8_t> header,std::size_t at,std::uint32_t value) {
    // 지정 네 바이트에만 원본 little endian 값을 쓴다.
    for (std::size_t index=0;index<4;++index) header[at+index]=static_cast<std::uint8_t>(value>>(index*8));
}
}
// 하나의 manager를 빌린 여러 헤더도 이 실제 이미지/커서를 공유한다.
struct CampaignTemplateStorage::Image {
    // 파싱 자료의 각 실제 버퍼에 대응하는 원본60BYTE 섹션 노드다.
    struct Stream {
        std::array<std::uint8_t,60> bytes{}; // 첫16BYTE와 이름40BYTE 및 next DWORD를 같은 노드에 둔다.
        std::optional<std::size_t> canonical; // 현재 실제 버퍼가 놓인 Fort35칸의 위치이며 handle과 독립적이다.
        std::vector<std::uint8_t> extra; // Temp 같은 비표준 이름의 실제 할당이며 Rename 때 벡터 move로 같은 할당을 넘긴다.
        std::uint64_t generation{}; // 같은 이름 재생성/노드 반납과 구별하는 생성 당시의 비영 수명이다.
        bool accessing{}; // 이 노드의 raw cursor/버퍼를 인라인 쓰기에 빌려준 동안만 참이다.
    };
    std::uint64_t nextStreamGeneration{}; // 같은 manager 안에서 재생성마다 증가하는 비영 번호다.
    std::uint32_t nextStreamHandle{1}; // 이름/표준 파일 위치와 독립적인 단조 증가 노드 토큰이다.
    std::array<bool,o::FortTemplate::kSectionCount> seeded{}; // 외부 파싱 자료의 각 표준 위치를 최초 목록에 한 번만 연결한다.
    ClientTemplateSectionNames& sectionNames; // Client와같은lazy이름표이며이Image보다긴Storageowner에서빌린다.
    bool constructed{}; // 실제new된Image에원문ctor가끝났는지를구별한다.
    o::FortTemplate fort; // 원시 State 섹션을 포함한 실제 파싱 자료와 소유 버퍼다.
    o::PlayerTemplateStateStream state; // 같은 fort.sections[State]의 현재 cursor다.
    o::FortTerritoryStream territory; // 같은 fort.sections[Territory]의 현재 cursor다.
    o::FortMoneyCursor money; // 같은 Money 노드의 현재 pos/end이며 SP 값의 편의 사본을 사용하지 않는다.
    o::FortMetaStream badges,technology; // 같은 원시 섹션 버퍼와 각각의 현재 pos/end/present다.
    std::uint32_t loadCursors{}; // 공개한 Money/Badges/Technology cursor의 유효 비트1/2/4다. 미공개 노드의 초기 상태는 보존한다.
    std::uint32_t stateHandle{},territoryHandle{},moneyHandle{},badgesHandle{},technologyHandle{}; // 현재 typed cursor를 소유한 실제 노드이며 Rename/재생성 뒤 이름으로 다시 공개한다.
    std::string name; // 실제 manager+c의 현재 이름을 기존 const string API에 빌려주는 읽기 투영이다. 변경의 원본은 manager 물리다.
    std::vector<std::uint8_t> retiredState; // 확장 중 Get의 원문 복사가 끝날 때까지만 이전 할당을 보유한다.
    std::array<std::uint8_t,276> manager{}; // 원본 Template manager의 vptr/head/current·파일명256·서명/FE 판정DWORD를 같은 저장소에 둔다.
    std::map<std::uint32_t,std::unique_ptr<Stream>> streams; // 노드 반납 때 실제 객체 할당을 해제한다.
    std::vector<std::uint32_t> streamOrder; // 원본 추가/교체 뒤 목록 순서를 고정 파일 섹션 번호와 구별한다.
    // 노드마다 같은 포인터 토큰을 사용하므로 Rename의 버퍼 위치 move가 원문 data 필드를 바꾸지 않는다.
    static std::uint32_t BufferToken(std::uint32_t handle) { return 0x10000000U+handle; }
    // 같은 노드의 현재 실제 할당을 표준 파일 위치 또는 동적 벡터에서 빌린다.
    std::vector<std::uint8_t>& Buffer(std::uint32_t handle) {
        auto& stream=*streams.at(handle);if (!stream.canonical) return stream.extra;
        if (fort.sections.size()<=*stream.canonical) fort.sections.resize(*stream.canonical+1);return fort.sections.at(*stream.canonical);
    }
    // 원본60BYTE에 저장된 현재 이름을 읽는다. handle 값으로 표준 종류를 추정하지 않는다.
    std::string_view StreamName(std::uint32_t handle) const {
        const auto& bytes=streams.at(handle)->bytes;const auto first=bytes.begin()+16,end=std::find(first,first+40,std::uint8_t{});
        if (end==first+40) throw std::logic_error("Template 실제 섹션 이름 NUL 누락");return {reinterpret_cast<const char*>(bytes.data()+16),static_cast<std::size_t>(end-first)};
    }
    // 실제60BYTE 객체를 할당하고 같은 생성 수명을 부여한다. 원본 생성자는 호출자가 별도로 수행한다.
    std::uint32_t AllocateStream(std::optional<std::size_t> canonical={}) {
        if (!nextStreamHandle || nextStreamHandle>=0x10000000U || !++nextStreamGeneration) throw std::overflow_error("Template 동적 노드/생성 수명 소진");
        const auto handle=nextStreamHandle++;auto stream=std::make_unique<Stream>();stream->canonical=canonical;stream->generation=nextStreamGeneration;
        if (canonical) seeded.at(*canonical)=true;streams.emplace(handle,std::move(stream));return handle;
    }
    // 저장 중인 raw 빌림의 포인터/노드를 구조 변경으로 반납하지 않는다.
    void RequireNoAccess() const {
        for (const auto& [handle,stream]:streams) { static_cast<void>(handle);if (stream->accessing) throw std::logic_error("Template raw 쓰기 빌림 중 섹션 구조 변경 금지"); } // 현재 manager의 모든 실제 노드를 검사한다.
    }
    // 같은 typed cursor가 반납되거나 다른 이름으로 바뀌면 이전 위치/span을 새 노드에 쓰지 않는다.
    void RetireCursor(std::uint32_t handle) {
        if (stateHandle==handle) { stateHandle=0;state={{},0,0,false}; }
        if (territoryHandle==handle) { territoryHandle=0;territory={}; }
        if (moneyHandle==handle) { moneyHandle=0;money={};loadCursors&=~1U; }
        if (badgesHandle==handle) { badgesHandle=0;badges={};loadCursors&=~2U; }
        if (technologyHandle==handle) { technologyHandle=0;technology={};loadCursors&=~4U; }
    }
    // MemStream 소멸에서 요청한 같은 실제 raw 섹션 버퍼를 반납한다.
    void ReleaseBuffer(std::uint32_t token) {
        if (token<=0x10000000U || !streams.contains(token-0x10000000U)) throw std::out_of_range("Template 실제 버퍼 핸들 범위");
        std::vector<std::uint8_t>{}.swap(Buffer(token-0x10000000U));
    }
    // 현재 노드의 data/pos/end와 같은 raw 버퍼를 두 typed cursor에 다시 연결한다.
    void PublishCursor(std::uint32_t handle) {
        auto& bytes=Buffer(handle);const auto& node=streams.at(handle)->bytes;const auto sectionName=StreamName(handle);
        if (!o::CompareDataManagerSectionNames(sectionName,"State")) { stateHandle=handle;state={bytes,Read(std::span(node).subspan(4)),Read(std::span(node).subspan(8)),Read(node)!=0}; }
        if (!o::CompareDataManagerSectionNames(sectionName,"Territory")) { territoryHandle=handle;territory={bytes,Read(std::span(node).subspan(4)),Read(std::span(node).subspan(8))}; }
        if (!o::CompareDataManagerSectionNames(sectionName,"Money")) { moneyHandle=handle;money={Read(std::span(node).subspan(4)),Read(std::span(node).subspan(8))};loadCursors|=1U; }
        if (!o::CompareDataManagerSectionNames(sectionName,"Badges")) { badgesHandle=handle;badges={bytes,Read(std::span(node).subspan(4)),Read(std::span(node).subspan(8)),Read(node)!=0};loadCursors|=2U; }
        if (!o::CompareDataManagerSectionNames(sectionName,"Technology")) { technologyHandle=handle;technology={bytes,Read(std::span(node).subspan(4)),Read(std::span(node).subspan(8)),Read(node)!=0};loadCursors|=4U; }
    }
    // 실제 임시 할당을 원본 포인터 토큰으로 빌린 뒤 완료한 같은 버퍼를 raw 섹션에 넘긴다.
    template<class Operation> void WithMemory(std::uint32_t handle,Operation operation) {
        std::vector<std::uint8_t> allocation;bool allocated{};const auto temporary=0x20000000U+handle;auto& node=streams.at(handle)->bytes;
        o::MemStreamHooks hooks;
        hooks.buffer=[this,&allocation,temporary,&allocated](auto pointer)->std::span<std::uint8_t> {
            if (pointer==temporary && allocated) return allocation;
            if (pointer<=0x10000000U || !streams.contains(pointer-0x10000000U)) throw std::out_of_range("Template MemStream 실제 버퍼 핸들 범위");return Buffer(pointer-0x10000000U);
        }; // 옛 버퍼는 free 전까지 같은 실제 소유 벡터에서 읽는다.
        hooks.allocate=[&allocation,&allocated,temporary](auto size) { if (allocated) throw std::logic_error("Template MemStream 한 작업의 중복 할당");allocation.resize(size);allocated=true;return temporary; }; // 실제 임시 할당은 원본 본문이0으로 채운다.
        hooks.release=[this](auto pointer) { if (pointer) ReleaseBuffer(pointer); }; // 같은 원시 섹션의 옛 할당만 반납한다.
        hooks.assertion=[](auto) { throw std::logic_error("Template 실제 MemStream 진단"); }; // 잘못된 저장 버퍼를 성공으로 대신하지 않는다.
        operation(hooks);auto& buffer=Buffer(handle);
        if (Read(node)==temporary && allocated) { buffer=std::move(allocation);Put(node,0,BufferToken(handle)); }
        const auto end=Read(std::span(node).subspan(8));if (end>buffer.size()) throw std::out_of_range("Template MemStream 완료 뒤 실제 end 범위");buffer.resize(end);PublishCursor(handle);
    }
    // 같은 stream의 물리 할당을 노출하고 원본 쓰기 뒤 논리 end만 원시 섹션에 남긴다.
    void WriteStream(std::uint32_t handle,std::span<const std::uint8_t> input) {
        if (input.size()>0xffffffffU) throw std::out_of_range("Template MemStream 원문 DWORD 길이 초과");
        AccessSection(handle,[input](auto fields,const auto& hooks) { o::WriteMemStream(fields,input,static_cast<std::uint32_t>(input.size()),hooks); });
    }
    // 원본 ce10이 새 노드를 붙이기 전에 마지막 MemStream의 end에 맞춰 할당을 줄인다.
    void CompactTail(std::uint32_t handle) {
        auto& node=streams.at(handle)->bytes;const auto end=Read(std::span(node).subspan(8));
        if (end==Read(std::span(node).subspan(12))) return;
        WithMemory(handle,[this,handle](const auto& hooks) { o::CompactMemStream(streams.at(handle)->bytes,hooks); });
    }
    // 실제 노드/버퍼 반납과 마지막 압축을 같은 원본 목록 처리기에 공급한다.
    o::DataManagerListHooks ListEffects() {
        o::DataManagerListHooks hooks;hooks.lifetime.stream=[this](auto handle) { return std::span(streams.at(handle)->bytes); }; // 같은 현재60BYTE 노드를 빌린다.
        hooks.lifetime.releaseBuffer=[this](auto pointer) { ReleaseBuffer(pointer); }; // 실제 raw 섹션 할당이다.
        hooks.lifetime.releaseStream=[this](auto handle) {
            RetireCursor(handle);if (streams.erase(handle)!=1) throw std::logic_error("Template 실제 노드 중복 반납");std::erase(streamOrder,handle);
        }; // 버퍼 소멸을 마친 노드와 그 typed cursor만 반납한다.
        hooks.compact=[this](auto handle) { CompactTail(handle); };hooks.missingPredecessor=[] { throw std::logic_error("Template 실제 선행 노드 진단"); };return hooks;
    }
    // 같은 이름의 기존 노드 unlink/버퍼 반납 뒤 이름과 독립적인 새 마지막 노드를 붙인다.
    std::uint32_t CreateSection(std::string_view sectionName) {
        if (!retiredState.empty()) throw std::logic_error("State 확장 복사 중 새 섹션 생성 불가");
        RequireNoAccess();SyncStreams();std::uint32_t handle{};std::vector<std::uint8_t> allocation;bool allocated{};o::DataManagerResizeHooks hooks;
        hooks.selection.stream=[this](auto stream) { return std::span(streams.at(stream)->bytes); };hooks.list=ListEffects(); // 검색/기존 제거/새 추가도 같은 실제 부모에서 수행한다.
        hooks.allocateStream=[this,&handle,sectionName](auto size) { if (size!=60 || handle) throw std::logic_error("Template 새 섹션 실제 노드 할당 계약 위반");handle=AllocateStream(CanonicalSection(sectionName));return handle; };
        hooks.memory.buffer=[this,&allocation,&handle](auto pointer)->std::span<std::uint8_t> {
            if (pointer==0x20000000U+handle) return allocation;
            if (pointer<=0x10000000U || !streams.contains(pointer-0x10000000U)) throw std::out_of_range("Template 생성 부모의 실제 버퍼 범위");return Buffer(pointer-0x10000000U);
        }; // 새 할당은 완료 전까지 별도 소유하고 기존 tail 압축은 원래 실제 버퍼를 쓴다.
        hooks.memory.allocate=[&allocation,&allocated,&handle](auto size) { if (allocated) throw std::logic_error("Template 생성 부모의 중복 할당");allocation.resize(size);allocated=true;return 0x20000000U+handle; };
        hooks.memory.assertion=[](auto) { throw std::logic_error("Template 생성 부모의 실제 MemStream 진단"); };
        const auto current=o::CreateDataManagerStream(manager,sectionName,hooks);auto& node=streams.at(handle)->bytes;
        if (current!=handle || !allocated || Read(node)!=0x20000000U+handle) throw std::logic_error("Template 생성 부모의 실제 현재 섹션 불일치");
        Put(node,0,BufferToken(handle));allocation.resize(Read(std::span(node).subspan(8)));Buffer(handle)=std::move(allocation);streamOrder.push_back(handle);PublishCursor(handle);return handle;
    }
    // 기존 State 생성 호출도 같은 일반 생성/원본 쓰기 부모를 사용한다.
    void InitializeState(std::span<const std::uint8_t,28> input) { const auto handle=CreateSection("State");WriteStream(handle,input); }
    // 읽기 부모가 진행한 typed cursor의 현재 위치를 같은 실제 노드에 반영한다. 생성 수명/current 선택은 바꾸지 않는다.
    void SyncCursorPosition(std::uint32_t handle) {
        if (streams.at(handle)->accessing) return; // 바깥 인라인 쓰기가 진행한 raw 위치를 아직 공개하지 않은 typed 위치로 되돌리지 않는다.
        auto& node=streams.at(handle)->bytes;
        if (handle==stateHandle) Put(node,4,state.position);
        if (handle==territoryHandle) Put(node,4,territory.position);
        if (handle==moneyHandle && (loadCursors&1U)) Put(node,4,money.position);
        if (handle==badgesHandle && (loadCursors&2U)) Put(node,4,badges.position);
        if (handle==technologyHandle && (loadCursors&4U)) Put(node,4,technology.position);
    }
    // 현재 선택을 바꾸지 않고 명시 노드에 인라인 쓰기를 적용한 뒤 논리 끝만 원시 섹션에 공개한다.
    void AccessSection(std::uint32_t handle,const std::function<void(std::span<std::uint8_t>,const o::MemStreamHooks&)>& operation) {
        if (!operation) throw std::invalid_argument("Template 인라인 쓰기 장치 누락");
        if (streams.at(handle)->accessing) throw std::logic_error("Template 같은 stream의 중첩 쓰기 빌림 금지");
        SyncCursorPosition(handle);auto& buffer=Buffer(handle);auto& node=streams.at(handle)->bytes;const auto allocation=Read(std::span(node).subspan(12));if (buffer.size()<allocation) buffer.resize(allocation);
        streams.at(handle)->accessing=true;
        try { WithMemory(handle,[&](const auto& hooks) { operation(std::span(node).first(16),hooks); }); }
        catch (...) { streams.at(handle)->accessing=false;const auto end=Read(std::span(node).subspan(8));if (end<=buffer.size()) buffer.resize(end);PublishCursor(handle);throw; } // 예외 전의 pos/end/부분 쓰기를 보존하며 물리 할당을 논리 자료로 잘못 공개하지 않는다.
        streams.at(handle)->accessing=false;
    }
    // 실제 이름 교체가 끝난 같은 노드의 할당을 새 표준 위치/동적 저장소로 이동한다. 바이트/할당을 복사하지 않는다.
    void BindRenamedBuffer(std::uint32_t handle) {
        auto& stream=*streams.at(handle);const auto destination=CanonicalSection(StreamName(handle));
        if (destination==stream.canonical) { PublishCursor(handle);return; }
        auto allocation=std::move(Buffer(handle));RetireCursor(handle);stream.canonical=destination;
        if (destination) seeded.at(*destination)=true;Buffer(handle)=std::move(allocation);PublishCursor(handle);
    }
    // 이름 검색은 원문처럼 현재 선택을 갱신하되 현재 위치와 목록을 바꾸지 않는다.
    std::uint32_t FindSection(std::string_view sectionName) {
        SyncStreams();o::DataManagerSelectionHooks hooks;hooks.stream=[this](auto handle) { return std::span(streams.at(handle)->bytes); };
        return o::FindDataManagerSection(manager,sectionName,hooks);
    }
    // 원본 Rename이 대상 제거/현재 source 재검색/이름 쓰기를 끝낸 뒤 버퍼의 호스트 위치만 매핑한다.
    std::uint32_t RenameSection(std::string_view destination,std::string_view source) {
        RequireNoAccess();SyncStreams();o::DataManagerResizeHooks hooks;hooks.selection.stream=[this](auto handle) { return std::span(streams.at(handle)->bytes); };hooks.list=ListEffects();
        const auto current=o::RenameDataManagerStream(manager,destination,source,hooks);BindRenamedBuffer(current);return current;
    }
    // 실제 임시 노드가 살아 있는 동안 원본 부모를 실행하고 완료한 같은 할당을 State로 넘긴다.
    void EnsureStateSize(std::int32_t minimum) {
        if (!retiredState.empty()) throw std::logic_error("분리 State 복사 중 원본 최소 확장 불가");
        RequireNoAccess();SyncStreams();std::uint32_t temporary{};std::vector<std::uint8_t> buffer;bool allocated{};o::DataManagerResizeHooks hooks;
        hooks.selection.stream=[this](auto handle) { return std::span(streams.at(handle)->bytes); }; // 원문과 임시 노드 모두 같은 실제 저장소에 둔다.
        hooks.list=ListEffects();
        hooks.memory.buffer=[this,&buffer,&temporary](auto pointer)->std::span<std::uint8_t> {
            if (pointer==0x20000000U+temporary) return buffer;
            if (pointer<=0x10000000U || !streams.contains(pointer-0x10000000U)) throw std::out_of_range("Template 확장의 실제 원문 버퍼 핸들 범위");return Buffer(pointer-0x10000000U);
        }; // 복사는 원본 부모가 요청하는 현재 data/end를 사용한다.
        hooks.memory.allocate=[&buffer,&allocated,&temporary](auto size) { if (allocated) throw std::logic_error("Template 확장 임시 버퍼 중복 할당");buffer.resize(size);allocated=true;return 0x20000000U+temporary; };
        hooks.memory.release=[this](auto pointer) { if (pointer) ReleaseBuffer(pointer); };hooks.memory.assertion=[](auto) { throw std::logic_error("Template 원본 확장의 MemStream 진단"); };
        hooks.allocateStream=[this,&temporary](auto size) { if (size!=60 || temporary) throw std::logic_error("Template 실제 임시 노드 할당 계약 위반");temporary=AllocateStream();return temporary; };
        hooks.missingSource=[] { throw std::logic_error("Template 원본 State 확장의 원문 섹션 누락"); };
        const auto current=o::EnsureDataManagerStreamSize(manager,"State",minimum,hooks);
        if (!temporary) { PublishCursor(current);return; }
        auto& node=streams.at(current)->bytes;if (current!=temporary || !allocated || Read(node)!=0x20000000U+temporary) throw std::logic_error("Template 원본 확장의 임시 소유권 계약 위반");
        const auto end=Read(std::span(node).subspan(8));if (end>buffer.size()) throw std::out_of_range("Template 원본 확장 결과의 실제 end 범위");
        // 원문 Rename의 현재/head/next와 임시 node handle을 그대로 유지하고 같은 벡터 할당만 인계한다.
        Put(node,0,BufferToken(current));buffer.resize(end);Buffer(current)=std::move(buffer);streamOrder.push_back(current);BindRenamedBuffer(current);
    }
    // 공개 진단은 보조 순서 벡터 대신 실제 head/next와 이름을 읽는다.
    std::vector<std::string> SectionOrder() const {
        std::vector<std::string> result;auto current=Read(std::span(manager).subspan(4));
        // 실제 노드 수보다 긴 사슬은 손상/순환으로 거부한다.
        while (current) {
            if (result.size()>=streams.size()) throw std::logic_error("Template 실제 섹션 목록 순환");
            const auto& node=streams.at(current)->bytes;const auto first=node.begin()+16,end=std::find(first,first+40,std::uint8_t{});
            if (end==first+40) throw std::logic_error("Template 섹션 이름 NUL 누락");result.emplace_back(first,end);current=Read(std::span(node).subspan(0x38));
        }
        return result;
    }
    // 실제 원시State와 같은 커서를 노드에 반영한 뒤 원본 선택을 실행한다.
    void SelectState() {
        SelectLoadSection("State",1);
    }
    // 004bd4d0/004bcb60처럼 현재 원시 Subscriber를 선택하고 같은 노드의 cursor를 진행한다.
    std::string ReadSubscriberName() {
        SyncStreams();o::DataManagerSelectionHooks hooks;hooks.stream=[this](auto node) { return std::span(streams.at(node)->bytes); };
        hooks.assertion=[](auto) { throw std::logic_error("Template Subscriber 실제 읽기 섹션 선택 진단"); };
        const auto selected=o::SelectDataManagerSection(manager,"Subscriber",0,hooks);auto& node=streams.at(selected)->bytes;const auto& bytes=Buffer(selected);
        Put(node,4,Read(std::span(node).subspan(4))+4U); // 원본 DWORD ID는 반환하지 않고 cursor만 네 BYTE 진행한다.
        auto position=Read(std::span(node).subspan(4));const auto end=Read(std::span(node).subspan(8));
        if (position>end || position>bytes.size() || bytes.size()-position<2) throw std::out_of_range("Template Subscriber 길이 읽기의 실제 원문 범위");
        const auto length=static_cast<std::uint32_t>(bytes[position]) | (static_cast<std::uint32_t>(bytes[position+1])<<8);Put(node,4,position+2U);position+=2U;
        if (position>end || length>=0x8000U || length>=256 || length>end-position || length>bytes.size()-position) throw std::out_of_range("Template Subscriber signed 길이/목록256BYTE 출력 범위");
        std::string result(reinterpret_cast<const char*>(bytes.data()+position),length);Put(node,4,position+length);result.resize(result.find('\0')==std::string::npos ? result.size() : result.find('\0'));return result;
    }
    // Territory120BYTE 읽기는 비어 있지 않은 섹션 선택을 먼저 수행한다.
    void SelectTerritory() {
        SelectLoadSection("Territory",0);
    }
    // 현재 노드가 없는 읽기/빈 읽기는 원본 선택의 진단으로 남기고, 메타의 빈 쓰기 허용은 그대로 유지한다.
    std::uint32_t SelectLoadSection(std::string_view sectionName,std::int32_t writable) {
        SyncStreams();o::DataManagerSelectionHooks hooks;
        hooks.stream=[this](auto node) { return std::span(streams.at(node)->bytes); }; // 같은 manager의 실제 목록 노드를 선택한다.
        hooks.assertion=[](auto) { throw std::logic_error("Template 경제/메타 실제 섹션 선택 진단"); }; // 누락/읽기 불가를 가짜 빈 성공으로 바꾸지 않는다.
        const auto selected=o::SelectDataManagerSection(manager,sectionName,writable,hooks);PublishCursor(selected);return selected;
    }
    // 선택 없이 typed cursor를 처음 공개할 때도 같은 원시 노드에서 현재 상태를 읽는다.
    void LoadCursor(std::string_view sectionName) {
        SyncStreams();std::uint32_t handle{};
        for (const auto& [candidate,stream]:streams) { static_cast<void>(stream);if (!o::CompareDataManagerSectionNames(StreamName(candidate),sectionName)) { handle=candidate;break; } } // getter는 current를 바꾸지 않고 현재 이름의 실제 노드를 빌린다.
        if (!handle) throw std::logic_error("Template 경제/메타 현재 노드 누락");PublishCursor(handle);
    }
    // 선택된 노드는 소멸과 동일한 핸들/이름 저장소를 사용한다.
    std::string_view CurrentSection() const {
        const auto handle=Read(std::span(manager).subspan(8));if (!handle) return {};
        const auto& bytes=streams.at(handle)->bytes;const auto start=bytes.begin()+16,end=std::find(start,start+40,std::uint8_t{});
        if (end==start+40) throw std::logic_error("Template 현재 섹션 이름 NUL 누락");
        return {reinterpret_cast<const char*>(bytes.data()+16),static_cast<std::size_t>(end-start)};
    }
    // 공개된 Fort/State의 현재 버퍼를 같은 목록에 연결하며 파일 읽기 구현 자체를 대신하지 않는다.
    void SyncStreams() {
        if (fort.sections.size()>o::FortTemplate::kSectionCount) throw std::logic_error("Template 실제 섹션35칸 범위");
        const auto names=o::FortTemplate::SectionNames();const auto count=fort.sections.size(); // 파싱 자료/외부 추가 표준 위치만 최초 연결한다.
        for (std::size_t index=0;index<count;++index) if (!seeded[index]) { // 원문 생성/rename로 제거된 이름을 이름표 기준으로 되살리지 않는다.
            const auto handle=AllocateStream(index);auto& node=streams.at(handle)->bytes;std::copy(names[index].begin(),names[index].end(),node.begin()+16);
            auto tail=Read(std::span(manager).subspan(4));if (!tail) Put(manager,4,handle);else { while (const auto next=Read(std::span(streams.at(tail)->bytes).subspan(56))) tail=next;Put(streams.at(tail)->bytes,56,handle); } // 파싱/외부 자료의 최초 연결은 current를 바꾸지 않는다.
            streamOrder.push_back(handle);
        }
        for (const auto& [handle,stream]:streams) { // 실제 목록/head/current/next는 보존하고 현재 외부 버퍼와 공개 cursor만 동기화한다.
            if (stream->accessing) continue;auto& buffer=Buffer(handle);
            if (buffer.size()>0xffffffffU || buffer.capacity()>0xffffffffU) throw std::out_of_range("Template 실제 섹션 DWORD 크기 초과");
            Put(stream->bytes,0,buffer.capacity() ? BufferToken(handle) : 0U);Put(stream->bytes,8,static_cast<std::uint32_t>(buffer.size()));Put(stream->bytes,12,static_cast<std::uint32_t>(buffer.capacity()));SyncCursorPosition(handle);PublishCursor(handle);
        }
    }
    // 현재 manager의 서명/이름 검색 효과를 같은 파일 저장 부모에 공급한다.
    void SaveFile(const o::DataManagerFileSaveState& stateInputs,o::FortImageHooks imageHooks,const TemplateFileWriteHooks& file) {
        SyncStreams();o::FortImageData image;constexpr std::uint32_t kImage=0x50000; //이 동기 호출만 사용하는 이미지 할당의 불투명 토큰이다.
        imageHooks.signatureFlag=[this] { Put(manager,0x10c,0x46);if (Read(std::span(manager).subspan(0x110))==0xffffffffU) Put(manager,0x110,0);return Read(std::span(manager).subspan(0x110)) ? std::uint8_t{0xfe} : std::uint8_t{}; };
        imageHooks.section=[this](std::size_t index)->std::span<const std::uint8_t> {
            o::DataManagerSelectionHooks selection;selection.stream=[this](auto handle) { return std::span(streams.at(handle)->bytes); };
            const auto handle=o::FindDataManagerSection(manager,o::FortTemplate::SectionNames()[index],selection);if (!handle) throw std::logic_error("Template 실제 파일 저장의 누락 섹션 진단");
            const auto& bytes=Buffer(handle);const auto end=Read(std::span(streams.at(handle)->bytes).subspan(8));if (end>bytes.size()) throw std::out_of_range("Template 저장 섹션의 실제 end 범위");return std::span(bytes).first(end);
        }; //검색은 같은 목록/current를 바꾸며 Source stream의 pos는 초기화하지 않는다.
        o::DataManagerFileSaveHooks hooks;
        hooks.image=[this,&image,&imageHooks](auto source,std::span<std::uint8_t,20> stream) {
            if (source.data()!=manager.data()) throw std::logic_error("Template 저장의 실제 manager 불일치");image=o::RawFortImage(o::OriginalEdition::Patch1078,fort,imageHooks).Save();Put(stream,0,kImage);Put(stream,4,0);Put(stream,8,static_cast<std::uint32_t>(image.end));Put(stream,12,static_cast<std::uint32_t>(image.storage.size()));
        };
        hooks.fileName=[](auto source) { return o::TemplateDataManagerFileName(source); };
        hooks.permissions=[&file](auto name,auto mode) { if (!file.permissions) throw std::logic_error("Template 실제 파일 권한 장치 미연결");file.permissions(name,mode); };
        hooks.write=[&file,&image](auto name,auto pointer,auto length) {
            if (pointer!=kImage || length>image.storage.size()) throw std::out_of_range("Template 파일 저장의 실제 이미지 포인터/길이 범위");if (!file.write) throw std::logic_error("Template 실제 파일 쓰기 장치 미연결");file.write(name,std::span(image.storage).first(length));
        };
        hooks.release=[&image](auto pointer) { if (pointer!=kImage) throw std::logic_error("Template 저장 이미지 할당 반납 불일치");std::vector<std::uint8_t>{}.swap(image.storage); };
        o::SaveDataManagerFile(manager,stateInputs,hooks);
    }
    // 현재 각 섹션의 버퍼를 먼저 반납하고 같은 노드의 실제 할당을 뒤에 해제한다.
    void Destroy() {
        SyncStreams();o::DataManagerLifetimeHooks hooks;
        hooks.stream=[this](auto handle) { return std::span(streams.at(handle)->bytes); }; // 현재 같은60BYTE 노드만 빌린다.
        hooks.releaseBuffer=[this](auto pointer) { ReleaseBuffer(pointer); }; // 이름 변경 뒤에도 같은 data 토큰의 현재 할당만 반납한다.
        hooks.releaseStream=[this](auto handle) { if (streams.erase(handle)!=1) throw std::logic_error("DataManager 같은 섹션 중복 반납"); }; // 버퍼 소멸이 끝난 노드만 반납한다.
        o::DestroyDataManager(manager,hooks);
    }
    // 실제 head/next의 같은 버퍼만 표준 위치로 옮기고 current/pos/생성수명은 재설정하지 않는다.
    void BindFileStreams() {
        streamOrder.clear();auto current=Read(std::span(manager).subspan(4));
        while (current) { // 원문이 이미 붙인 노드만 같은 순서로 공개하며 미완료 임시 노드를 붙이지 않는다.
            if (streamOrder.size()>=streams.size()) throw std::logic_error("Template 파일 읽기 뒤 실제 목록 순환");
            auto& node=streams.at(current)->bytes;streamOrder.push_back(current);BindRenamedBuffer(current);
            auto& buffer=Buffer(current);const auto end=Read(std::span(node).subspan(8));if (end>buffer.size()) throw std::out_of_range("Template 파일 읽기 뒤 실제 end 범위");buffer.resize(end);PublishCursor(current);current=Read(std::span(node).subspan(56));
        }
    }
    // 같은 raw reader의 bool을 유지한다. 새 임시 버퍼는 기존 같은 이름을 제거하기 전까지 별개로 소유한다.
    bool ReadFile(const TemplateFileLoadHooks& fileEffects) {
        RequireNoAccess();
        if (streams.empty() && !Read(std::span(manager).subspan(4))) { fort.sections.clear();fort.sections.resize(o::FortTemplate::kSectionCount);seeded.fill(true);state={{},0,0,false};territory={}; }
        else SyncStreams();
        std::vector<std::uint8_t> source;std::uint32_t allocating{},created{}; // 파일할당은지역소유지만이름표는같은Client에서빌린다.
        o::DataManagerImageHooks hooks;hooks.names=[this] { return sectionNames.Names(); };hooks.assertion=[](auto) { throw std::logic_error("Template 실제 파일 이미지의 원본 진단"); };
        hooks.streams.selection.stream=[this](auto handle) { return std::span(streams.at(handle)->bytes); };hooks.streams.list=ListEffects();
        hooks.streams.allocateStream=[this,&allocating,&created](auto size) { if (size!=60 || created>=o::FortTemplate::kSectionCount) throw std::logic_error("Template 파일 이미지의 새 노드 계약 위반");++created;allocating=AllocateStream();return allocating; }; // 기존 canonical 버퍼는 원문 제거 전까지 덮어쓰지 않는다.
        hooks.streams.memory.buffer=[this,&source](auto pointer)->std::span<std::uint8_t> {
            if (pointer==0x30000) return source;if (pointer<=0x10000000U || !streams.contains(pointer-0x10000000U)) throw std::out_of_range("Template 파일 이미지의 실제 버퍼 토큰 범위");return Buffer(pointer-0x10000000U);
        }; // 입력과 새 임시 할당 및 기존 노드 모두 현재 실제 버퍼를 빌린다.
        hooks.streams.memory.allocate=[this,&allocating](auto size) { auto& buffer=Buffer(allocating);if (!buffer.empty()) throw std::logic_error("Template 파일 이미지의 새 버퍼 중복 할당");buffer.resize(size);return BufferToken(allocating); };
        hooks.streams.memory.assertion=[](auto) { throw std::logic_error("Template 파일 이미지의 실제 MemStream 진단"); };
        o::DataManagerFileHooks file;file.image=hooks;file.fileName=[this] { return o::TemplateDataManagerFileName(manager); };
        file.readFile=[&source,&fileEffects](std::string_view fileName,std::int32_t mode,std::span<std::uint8_t> length,std::int32_t flags) {
            if (mode!=1 || flags || length.size()!=4) throw std::logic_error("Template 파일 읽기의 실제 요청 인수 불일치");source=NeedFile(fileEffects.read)(fileName);
            if (source.size()>0xffffffffU) throw std::out_of_range("Template 실제 파일 읽기의 DWORD 길이 초과");Put(length,0,static_cast<std::uint32_t>(source.size()));return source.empty() ? 0U : 0x30000U;
        }; // manager/header가 공개되고 readonly/+16을 쓴 뒤에만 실제 파일 장치를 호출한다.
        file.signature=[this,&hooks,&file,&fileEffects](std::span<std::uint8_t> cursor) {
            o::TemplateFileSignatureHooks signature;signature.buffer=hooks.streams.memory.buffer;signature.typeCount=[&fileEffects] { return NeedFile(fileEffects.typeCount)(); };signature.fileName=file.fileName;
            signature.tooManyTypes=[&fileEffects] { NeedFile(fileEffects.tooManyTypes)(); };signature.rejectFile=[&fileEffects](auto fileName) { NeedFile(fileEffects.rejectFile)(fileName); };
            return o::ReadTemplateFileSignature(manager,cursor,signature);
        };
        file.releaseFile=[&source](auto pointer) { if (pointer!=0x30000U) throw std::logic_error("Template 파일 읽기 부모의 실제 할당 반납 불일치");std::vector<std::uint8_t>{}.swap(source); }; // 기존 원문 reader가 성공/거부/예외에서 한 번만 반납한다.
        try { const auto success=o::ReadDataManagerFile(manager,file);BindFileStreams();if (success && created!=o::FortTemplate::kSectionCount) throw std::logic_error("Template 실제 파일의 전체 이름 표 불일치");return success; }
        catch (...) { BindFileStreams();throw; } // host 예외 전에 원문이 이미 붙인 노드/부분 pos를 숨기거나 rollback하지 않는다.
    }
    // 기존 Campaign/목록 교체 API는 이전과 같이 빈 manager에 읽고 정상 false를 예외로 바꾼다.
    void ReadFile(std::span<const std::uint8_t> input,std::int32_t typeCount) {
        if (!streams.empty() || Read(std::span(manager).subspan(4)) || input.size()>0xffffffffU) throw std::invalid_argument("Template 실제 파일 이미지/빈 manager 계약 위반");
        TemplateFileLoadHooks file;file.read=[input](auto) { return std::vector<std::uint8_t>(input.begin(),input.end()); };file.typeCount=[typeCount] { return typeCount; };
        file.tooManyTypes=[] { throw std::logic_error("Template 파일의 numTypes<256 진단"); };file.rejectFile=[](auto) { throw std::runtime_error("Template 잘못된 서명의 원본 파일 제거 요청: 보호 원문은 삭제하지 않습니다"); };
        if (!ReadFile(file)) throw std::runtime_error("Template 실제 파일 열기/서명 거부");
    }
    // new276의같은할당이다.원문ctor/목록생성은아직하지않고metadata의선행노드투영도막는다.
    explicit Image(ClientTemplateSectionNames& names):sectionNames(names) { seeded.fill(true); }
    // 원문ctor는같은생성수명의실제manager와Client version을바꾼다.이름표할당은하지않는다.
    void ConstructName(std::string_view file,std::uint32_t& version) {
        if (constructed || !streams.empty() || file.size()>=256 || file.find('\0')!=std::string_view::npos) throw std::invalid_argument("Template 빈Image 실제ctor계약");
        const std::span<std::uint8_t,4> global(reinterpret_cast<std::uint8_t*>(&version),4);o::ConstructTemplateDataManager(manager,file,global);name.assign(file);constructed=true;
    }
    // State와 Territory 모두 같은 원시 섹션 바이트를 빌리며 파일 입력이면 원본 읽기 부모가 초기 목록을 만든다.
    Image(o::FortTemplate value,std::string file,std::uint32_t& version,ClientTemplateSectionNames& names,bool fileImage=false):sectionNames(names),fort(std::move(value)),name(std::move(file)) {
        if (name.size()>=256 || name.find('\0')!=std::string::npos) throw std::invalid_argument("Template 실제 파일명256BYTE 계약 위반");
        const std::span<std::uint8_t,4> global(reinterpret_cast<std::uint8_t*>(&version),4); // 실제DWORD객체의BYTE뷰다. 기존BYTE배열을DWORD참조로역캐스팅하지않는다.
        o::ConstructTemplateDataManager(manager,name,global); // 같은Client의실제005c89c0에0을쓰는효과를기존원본생성자에서수행한다.
        constructed=true;if (fileImage) return;
        const auto at=StateSection();const auto bytes=at<fort.sections.size() ? std::span<std::uint8_t>(fort.sections[at]) : std::span<std::uint8_t>{};state={bytes,0,static_cast<std::uint32_t>(bytes.size()),!bytes.empty()};
        const auto territoryAt=TerritorySection();const auto territoryBytes=territoryAt<fort.sections.size() ? std::span<const std::uint8_t>(fort.sections[territoryAt]) : std::span<const std::uint8_t>{};
        territory={territoryBytes,0,static_cast<std::uint32_t>(territoryBytes.size())};SyncStreams();
    }
};
// 시작 초기화는 기존 manager를 반납하는 전역 정리와 다르다.
CampaignTemplateStorage::CampaignTemplateStorage(std::shared_ptr<o::FortSerializationGlobals> serializationGlobals,std::shared_ptr<ClientTemplateSectionNames> sectionNames)
    :serializationGlobals_(serializationGlobals ? std::move(serializationGlobals) : std::make_shared<o::FortSerializationGlobals>()),sectionNames_(sectionNames ? std::move(sectionNames) : std::make_shared<ClientTemplateSectionNames>()) {
    // 아홉 개의 처음20BYTE만 원본 생성 몸체로 초기화한다.
    for (std::uint32_t slot=0;slot<9;++slot) o::InitializeTemplateHeader(Header(slot));
}
// C++ 종료의 소유권 반납이며 실제 미션 전환은 ClearAll을 명시 호출한다.
CampaignTemplateStorage::~CampaignTemplateStorage()=default;
// 같은 고정 물리 표를 빌린다.
std::span<std::uint8_t> CampaignTemplateStorage::Headers() { return headers_; }
// 원문new276만실행하며header/ctor/이름표는나중에선택된부모가진행한다.
std::uint32_t CampaignTemplateStorage::AllocateEmptyManager(std::uint32_t bytes) {
    if (bytes!=276 || !nextHandle_) throw std::invalid_argument("Template 실제빈Image276할당계약");
    auto image=std::unique_ptr<Image>(new(std::nothrow) Image(*sectionNames_));if (!image) return 0;const auto token=nextHandle_++;images_.emplace(token,std::move(image));return token;
}
// 존재하는같은미생성할당만원문ctor로전환한다.
std::uint32_t CampaignTemplateStorage::ConstructEmptyManager(std::uint32_t token,std::string_view name) { images_.at(token)->ConstructName(name,SerializationVersion());return token; }
// 고정token의물리는header교체후에도같은Image이다.
std::span<std::uint8_t> CampaignTemplateStorage::ManagerBytes(std::uint32_t token) { return images_.at(token)->manager; }
// 실제표는진단뒤한번받고같은Image의기존rawCreate를매이름마다실행한다.
void CampaignTemplateStorage::InitializeEmptyManager(std::uint32_t token,const std::function<void(o::DataManagerInitializeAssertion)>& assertion) {
    auto& image=*images_.at(token);if (!image.constructed) throw std::logic_error("Template 실제ctor이전초기화");o::DataManagerInitializeHooks hooks;hooks.assertion=assertion;hooks.names=[&image] { return image.sectionNames.Names(); };
    hooks.create=[&image](std::span<const std::uint8_t> name) { const auto end=std::find(name.begin(),name.end(),std::uint8_t{});if (end==name.end()) throw std::out_of_range("Template 실제Create이름NUL범위");image.CreateSection({reinterpret_cast<const char*>(name.data()),static_cast<std::size_t>(end-name.begin())}); };o::InitializeDataManager(image.manager,hooks);
}
// 같은실제Image의소멸/할당반납을원문바깥부모에빌린다.
o::TemplateLifetimeHooks CampaignTemplateStorage::ManagerLifetime() { return Effects(); }
// 다른Storage도동일Client이름표를명시공유할수있다.
std::shared_ptr<ClientTemplateSectionNames> CampaignTemplateStorage::SectionNamesOwner() const { return sectionNames_; }
// 같은DWORD의현재값과주소를호출자에게빌린다. 조회/Storage생성자에서공유값을재초기화하지않는다.
std::uint32_t& CampaignTemplateStorage::SerializationVersion() { return serializationGlobals_->version; }
// 유효 슬롯의 현재 기록만 반환한다.
std::span<std::uint8_t> CampaignTemplateStorage::Header(std::uint32_t slot) { if (slot>=9) throw std::out_of_range("Template 실제9칸 범위");return std::span(headers_).subspan(slot*20,20); }
// 헤더가 빌린 같은 handle도 같은 이미지/State cursor를 찾는다.
CampaignTemplateStorage::Image& CampaignTemplateStorage::Current(std::uint32_t slot) { const auto handle=Read(Header(slot));const auto at=images_.find(handle);if (!handle || at==images_.end()) throw std::logic_error("Template 현재 manager가 없거나 소유자보다 오래 산 빌림");return *at->second; }
// 실제 소멸은 안쪽 버퍼를 비우고 free는 manager 객체 할당을 반납한다.
o::TemplateLifetimeHooks CampaignTemplateStorage::Effects() {
    return {[this](auto handle) { auto& image=*images_.at(handle);image.Destroy();image.fort=o::FortTemplate{};image.state={{},0,0,false};image.territory={};std::vector<std::uint8_t>{}.swap(image.retiredState); },[this](auto handle) { if (images_.erase(handle)!=1) throw std::logic_error("Template 같은 manager 중복 반납"); }};
}
// 기존 한 manager를 정확히 해제한 뒤 새 실제 객체를 소유한다.
void CampaignTemplateStorage::Install(std::uint32_t slot,o::FortTemplate fort,std::string name) { Clear(slot);if (!nextHandle_) throw std::overflow_error("Template 호스트 manager 핸들 소진");const auto handle=nextHandle_++;images_.emplace(handle,std::make_unique<Image>(std::move(fort),std::move(name),SerializationVersion(),*sectionNames_));Put(Header(slot),0,handle); }
// 파싱된 보조 자료와 같은 실제 파일 원문을 새 manager에 읽으며 임의 빈 섹션 목록을 먼저 만들지 않는다.
void CampaignTemplateStorage::InstallFile(std::uint32_t slot,o::FortTemplate fort,std::span<const std::uint8_t> bytes,std::string name,std::int32_t typeCount) {
    Clear(slot);if (!nextHandle_) throw std::overflow_error("Template 파일 manager 핸들 소진");auto image=std::make_unique<Image>(std::move(fort),std::move(name),SerializationVersion(),*sectionNames_,true);image->ReadFile(bytes,typeCount);const auto handle=nextHandle_++;images_.emplace(handle,std::move(image));Put(Header(slot),0,handle);
}
// 실제 원문 부모가 설치/읽기/정상 false 정리를 지휘하며 기존 같은 manager는 건드리지 않는다.
void CampaignTemplateStorage::LoadFile(std::uint32_t slot,o::FortTemplate fort,std::string_view name,const TemplateFileLoadHooks& file) {
    o::TemplateLoadHooks hooks;
    hooks.fileName=[this](auto manager) { return o::TemplateDataManagerFileName(images_.at(manager)->manager); }; // 항상 전달받은 현재 handle의 실제 +12를 빌린다.
    hooks.sameName=[&file](auto first,auto second) { return NeedFile(file.sameName)(first,second); };hooks.wrongName=[&file] { NeedFile(file.wrongName)(); };
    hooks.allocate=[this](auto bytes) { if (bytes!=276 || !nextHandle_) throw std::overflow_error("Template Load의 실제276BYTE/호스트handle 소진");return nextHandle_++; };
    hooks.construct=[this,&fort](auto manager,auto fileName) {
        auto image=std::make_unique<Image>(std::move(fort),std::string(fileName),SerializationVersion(),*sectionNames_,true);image->fort.sections.clear();
        if (!images_.emplace(manager,std::move(image)).second) throw std::logic_error("Template Load manager의 같은 할당 중복");return manager;
    }; // 생성자의 공유 version0과 실제 Image 소유를 끝낸 뒤 원문 부모가 header.ptr를 공개한다.
    hooks.readOnly=[&file](auto fileName) { return NeedFile(file.readOnly)(fileName); };
    hooks.readFile=[this,&file](auto manager) { return images_.at(manager)->ReadFile(file); }; // readonly callback 뒤 header에서 재조회한 manager를 그대로 읽는다.
    hooks.isCurrent=[slot,&file] { return slot==NeedFile(file.currentSlot)(); };hooks.refreshTitle=[&file] { NeedFile(file.refreshTitle)(); };hooks.lifetime=Effects();
    o::LoadTemplateFile(Header(slot),name,hooks);
}
// 빌린 헤더를 반납해도 원래 소유자의 실제 버퍼/커서는 유지된다.
void CampaignTemplateStorage::Borrow(std::uint32_t slot,std::uint32_t source) { if (slot==source) throw std::invalid_argument("Template 자기 manager 빌림 계약");static_cast<void>(Current(source));const auto handle=Read(Header(source));const auto target=Header(slot);if (Read(target)==handle && !Read(target.subspan(4))) throw std::invalid_argument("Template 원래 소유자를 반납하는 같은 manager 빌림");Clear(slot);Put(Header(slot),0,handle);Put(Header(slot),4,1); }
// 비영 포인터만 정리하는 원본 전역 몸체를 그대로 사용한다.
void CampaignTemplateStorage::ClearAll() { o::ClearTemplateHeaders(headers_,Effects()); }
// 포인터0이어도 단일 기록은0으로 비운다.
void CampaignTemplateStorage::Clear(std::uint32_t slot) { o::ReleaseTemplateHeader(Header(slot),Effects()); }
// 모든 소비자는 같은 현재 manager를 다시 조회한다.
o::FortTemplate& CampaignTemplateStorage::Template(std::uint32_t slot) { return Current(slot).fort; }
// 한 manager를 빌린 헤더끼리도 같은 cursor 객체를 반환한다.
o::PlayerTemplateStateStream& CampaignTemplateStorage::State(std::uint32_t slot) { return Current(slot).state; }
// 실제 Player Stategetter가 같은 목록/current를 선택한 뒤 같은 cursor를 돌려받는다.
o::PlayerTemplateStateStream& CampaignTemplateStorage::SelectState(std::uint32_t slot) { auto& image=Current(slot);image.SelectState();return image.state; }
// 파싱 시점의 편의 사본이 아니라 실제 현재 섹션/노드를 읽는다.
std::string CampaignTemplateStorage::ReadSubscriberName(std::uint32_t slot) { return Current(slot).ReadSubscriberName(); }
// 미정 바이트를 포함한 완성28BYTE를 실제 새 섹션에 쓰고 같은 현재 cursor를 반환한다.
o::PlayerTemplateStateStream& CampaignTemplateStorage::InitializeState(std::uint32_t slot,std::span<const std::uint8_t,28> bytes) { auto& image=Current(slot);image.InitializeState(bytes);return image.state; }
// 생성한 manager/노드/수명을 고정하고 이후 current 변경과 분리한다.
TemplateWriteStream CampaignTemplateStorage::CreateSection(std::uint32_t slot,std::string_view name) { const auto manager=Read(Header(slot));auto& image=Current(slot);const auto section=image.CreateSection(name);return {manager,section,image.streams.at(section)->generation}; }
// 고정 manager 토큰의 실제 소유 Image만 선택하므로 Subscriber 부모의 Create(this) 주소를 그대로 유지한다.
TemplateWriteStream CampaignTemplateStorage::CreateManagerSection(std::uint32_t manager,std::string_view name) {
    const auto found=images_.find(manager);if (!manager || found==images_.end()) throw std::out_of_range("섹션 생성의 실제 manager 수명 누락");auto& image=*found->second;const auto section=image.CreateSection(name);return {manager,section,image.streams.at(section)->generation};
}
// 검색 실패도 원문 current0을 유지하며 참조 유효성과 빈 결과를 구별한다.
TemplateWriteStream CampaignTemplateStorage::FindSection(std::uint32_t slot,std::string_view name) { const auto manager=Read(Header(slot));auto& image=Current(slot);const auto section=image.FindSection(name);return section ? TemplateWriteStream{manager,section,image.streams.at(section)->generation} : TemplateWriteStream{}; }
// 선택 부모의 실제 pos0/읽기 진단을 끝낸 같은 노드의 생성 수명을 반환한다.
TemplateWriteStream CampaignTemplateStorage::SelectSection(std::uint32_t slot,std::string_view name,std::int32_t writable) { const auto manager=Read(Header(slot));auto& image=Current(slot);const auto section=image.SelectLoadSection(name,writable);return {manager,section,image.streams.at(section)->generation}; }
// 실제 Rename 부모가 source 노드를 그대로 남긴 결과를 사용하며 current/head/next를 편의 재기록하지 않는다.
TemplateWriteStream CampaignTemplateStorage::RenameSection(std::uint32_t slot,std::string_view destination,std::string_view source) { const auto manager=Read(Header(slot));auto& image=Current(slot);const auto section=image.RenameSection(destination,source);return {manager,section,image.streams.at(section)->generation}; }
// Create 반환값과 독립적으로 현재 선택을 보관한다. 과거 파일에서 읽은 노드에도 첫 빌림 수명을 부여한다.
TemplateWriteStream CampaignTemplateStorage::CurrentWriteStream(std::uint32_t slot) {
    const auto manager=Read(Header(slot));auto& image=Current(slot);const auto section=Read(std::span(image.manager).subspan(8));
    if (!section || !image.streams.contains(section)) throw std::logic_error("Template 현재 쓰기 stream의 실제 노드 누락");
    image.SyncCursorPosition(section);
    return {manager,section,image.streams.at(section)->generation};
}
// 반납된 manager와 재생성된 동일 이름의 새 노드를 이전 쓰기 대상으로 취급하지 않는다.
CampaignTemplateStorage::Image& CampaignTemplateStorage::Writable(TemplateWriteStream stream) {
    const auto found=images_.find(stream.manager);if (found==images_.end() || !stream.section || !stream.generation || !found->second->streams.contains(stream.section) || found->second->streams.at(stream.section)->generation!=stream.generation) throw std::logic_error("Template 쓰기 참조의 해제된 manager/섹션 수명");return *found->second;
}
// 원본 일반 MemStream 쓰기는 보관한 노드의 현재 cursor에 적용한다.
void CampaignTemplateStorage::WriteSection(TemplateWriteStream stream,std::span<const std::uint8_t> bytes) { Writable(stream).WriteStream(stream.section,bytes); }
// TypeNames처럼 current가 다른 섹션으로 바뀐 경우에도 보관한 실제 노드만 빌린다.
void CampaignTemplateStorage::AccessSection(TemplateWriteStream stream,const std::function<void(std::span<std::uint8_t>,const o::MemStreamHooks&)>& operation) { Writable(stream).AccessSection(stream.section,operation); }
// 같은 manager의 전체 최소 확장 부모가 만든 실제 현재 State를 빌린다.
o::PlayerTemplateStateStream& CampaignTemplateStorage::EnsureStateSize(std::uint32_t slot,std::int32_t minimum) { auto& image=Current(slot);image.EnsureStateSize(minimum);return image.state; }
// 빌린 헤더도 같은 manager의 실제 연결 순서를 확인한다.
std::vector<std::string> CampaignTemplateStorage::SectionOrder(std::uint32_t slot) { return Current(slot).SectionOrder(); }
// 빌린 헤더도 같은 manager의 현재 섹션을 확인한다.
std::string_view CampaignTemplateStorage::CurrentSection(std::uint32_t slot) { return Current(slot).CurrentSection(); }
// 지형 cursor도 같은 manager의 원문을 빌린다.
o::FortTerritoryStream& CampaignTemplateStorage::Territory(std::uint32_t slot) { return Current(slot).territory; }
// 월드 로더가 같은 manager에서 원본 읽기 선택을 한 뒤 같은 지형 cursor를 사용한다.
o::FortTerritoryStream& CampaignTemplateStorage::SelectTerritory(std::uint32_t slot) { auto& image=Current(slot);image.SelectTerritory();return image.territory; }
// 같은 manager의 현재 Money 노드를 선택 효과 없이 빌린다.
o::FortMoneyCursor& CampaignTemplateStorage::Money(std::uint32_t slot) { auto& image=Current(slot);image.LoadCursor("Money");return image.money; }
// Money는 원본 wrapper의 읽기 전용 선택을 적용한다.
o::FortMoneyCursor& CampaignTemplateStorage::SelectMoney(std::uint32_t slot) { auto& image=Current(slot);image.SelectLoadSection("Money",0);return image.money; }
// 같은 manager의 Badges 노드와 실제 버퍼를 빌린다.
o::FortMetaStream& CampaignTemplateStorage::Badges(std::uint32_t slot) { auto& image=Current(slot);image.LoadCursor("Badges");return image.badges; }
// 빈 Badges도 허용하는 원본 선택 모드를 유지한다.
o::FortMetaStream& CampaignTemplateStorage::SelectBadges(std::uint32_t slot) { auto& image=Current(slot);image.SelectLoadSection("Badges",1);return image.badges; }
// 같은 manager의 Technology 노드와 실제 버퍼를 빌린다.
o::FortMetaStream& CampaignTemplateStorage::Technology(std::uint32_t slot) { auto& image=Current(slot);image.LoadCursor("Technology");return image.technology; }
// 원본 기술 로더가 빈 섹션의 부모/UI 처리 여부를 직접 결정하게 한다.
o::FortMetaStream& CampaignTemplateStorage::SelectTechnology(std::uint32_t slot) { auto& image=Current(slot);image.SelectLoadSection("Technology",1);return image.technology; }
// 원문 선택 때 쓴 같은 파일 이름을 반환한다.
const std::string& CampaignTemplateStorage::Name(std::uint32_t slot) {
    auto& image=Current(slot);const auto bytes=std::span(image.manager).subspan(12,256);const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{}); // 원문 manager+c 파일명과 같은 물리를 읽는다.
    if (end==bytes.end()) throw std::out_of_range("Template 현재 manager 파일명의 NUL 범위");const std::string_view current(reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin()));
    if (image.name!=current) image.name=current;return image.name;
}
// manager를 교체하거나 현재 섹션/stream을 재생성하지 않고 같은 실제 물리만 제공한다.
o::FortCurrentNameHooks CampaignTemplateStorage::BindCurrentName(o::FortCurrentNameHooks hooks) {
    hooks.header=[this](auto slot) { return Header(slot); };
    hooks.managerName=[this](auto token)->std::span<std::uint8_t> { const auto found=images_.find(token);if (found==images_.end()) throw std::out_of_range("Fort 이름 변경의 현재 manager 없음");return std::span(found->second->manager).subspan(12,256); };
    return hooks;
}
// 빌린 헤더도 원래 소유자의 같은 manager 꼬리DWORD를 읽는다.
std::array<std::uint32_t,2> CampaignTemplateStorage::FileHeader(std::uint32_t slot) { const auto& manager=Current(slot).manager;return {Read(std::span(manager).subspan(0x10c)),Read(std::span(manager).subspan(0x110))}; }
// 빌린 헤더도 원래 소유자의 같은 현재 manager와 원시 섹션을 저장한다.
void CampaignTemplateStorage::SaveFile(std::uint32_t slot,const o::DataManagerFileSaveState& state,o::FortImageHooks image,const TemplateFileWriteHooks& file) { Current(slot).SaveFile(state,std::move(image),file); }
// 원본을 옮겨 보존한 뒤 같은 Fort State 섹션에 대체 할당을 만든다.
std::span<std::uint8_t> CampaignTemplateStorage::GrowState(std::uint32_t slot,std::uint32_t size) { auto& image=Current(slot);if (!image.retiredState.empty()) throw std::logic_error("Template State 확장 복사 미완료");const auto at=StateSection();if (image.fort.sections.size()<=at) image.fort.sections.resize(at+1);auto& bytes=image.fort.sections[at];image.retiredState=std::move(bytes);bytes.resize(size);return bytes; }
// 복사 뒤에만 이전 실제 버퍼를 반납한다.
void CampaignTemplateStorage::CommitStateGrowth(std::uint32_t slot) { std::vector<std::uint8_t>{}.swap(Current(slot).retiredState); }
// 실제 manager 할당 수만 읽는다.
std::size_t CampaignTemplateStorage::OwnedImages() const { return images_.size(); }
}
