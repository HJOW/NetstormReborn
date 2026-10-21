// 원본 명시 정리 시점까지 이전 이미지의 소유권을 Client에 유지한다.
#include "client/CampaignTemplateStorage.h"
#include "o/RawDataManagerLifetime.h"
#include "o/RawDataManagerSelection.h"
#include "o/RawDataManagerList.h"
#include "o/RawMemStream.h"
#include "o/RawDataManagerResize.h"
#include <algorithm>
#include <stdexcept>
namespace netstorm::client {
namespace {
// State의 실제 파일 섹션 위치를 기존 파서의 이름 표에서 찾는다.
std::size_t StateSection() { const auto names=o::FortTemplate::SectionNames();const auto at=std::find(names.begin(),names.end(),"State");if (at==names.end()) throw std::logic_error("Template 실제 State 섹션 이름 누락");return static_cast<std::size_t>(at-names.begin()); }
// 지형 getter도 파서의 편의 사본 대신 같은 원시 섹션 버퍼를 조회한다.
std::size_t TerritorySection() { const auto names=o::FortTemplate::SectionNames();const auto at=std::find(names.begin(),names.end(),"Territory");if (at==names.end()) throw std::logic_error("Template 실제 Territory 섹션 이름 누락");return static_cast<std::size_t>(at-names.begin()); }
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
    };
    o::FortTemplate fort; // 원시 State 섹션을 포함한 실제 파싱 자료와 소유 버퍼다.
    o::PlayerTemplateStateStream state; // 같은 fort.sections[State]의 현재 cursor다.
    o::FortTerritoryStream territory; // 같은 fort.sections[Territory]의 현재 cursor다.
    std::string name; // 같은 현재 파일 이름이다.
    std::vector<std::uint8_t> retiredState; // 확장 중 Get의 원문 복사가 끝날 때까지만 이전 할당을 보유한다.
    std::array<std::uint8_t,12> manager{}; // 소멸이 사용하는 vptr/head/current의 실제 현재 저장소다.
    std::map<std::uint32_t,std::unique_ptr<Stream>> streams; // 노드 반납 때 실제 객체 할당을 해제한다.
    std::vector<std::uint32_t> streamOrder; // 원본 추가/교체 뒤 목록 순서를 고정 파일 섹션 번호와 구별한다.
    // MemStream 소멸에서 요청한 같은 실제 raw 섹션 버퍼를 반납한다.
    void ReleaseBuffer(std::uint32_t token) {
        if (token<0x10001U || token>0x10000U+o::FortTemplate::kSectionCount) throw std::out_of_range("Template 실제 버퍼 핸들 범위");
        std::vector<std::uint8_t>{}.swap(fort.sections.at(token-0x10001U));
    }
    // 현재 노드의 data/pos/end와 같은 raw 버퍼를 두 typed cursor에 다시 연결한다.
    void PublishCursor(std::uint32_t handle) {
        const auto& bytes=fort.sections.at(handle-1);const auto& node=streams.at(handle)->bytes;
        if (handle==StateSection()+1) state={std::span<std::uint8_t>(fort.sections.at(handle-1)),Read(std::span(node).subspan(4)),Read(std::span(node).subspan(8)),Read(node)!=0};
        if (handle==TerritorySection()+1) territory={bytes,Read(std::span(node).subspan(4)),Read(std::span(node).subspan(8))};
    }
    // 실제 임시 할당을 원본 포인터 토큰으로 빌린 뒤 완료한 같은 버퍼를 raw 섹션에 넘긴다.
    template<class Operation> void WithMemory(std::uint32_t handle,Operation operation) {
        std::vector<std::uint8_t> allocation;bool allocated{};const auto temporary=0x20000U+handle;auto& node=streams.at(handle)->bytes;
        o::MemStreamHooks hooks;
        hooks.buffer=[this,&allocation,temporary,&allocated](auto pointer)->std::span<std::uint8_t> {
            if (pointer==temporary && allocated) return allocation;
            if (pointer<0x10001U || pointer>0x10000U+o::FortTemplate::kSectionCount) throw std::out_of_range("Template MemStream 실제 버퍼 핸들 범위");return fort.sections.at(pointer-0x10001U);
        }; // 옛 버퍼는 free 전까지 같은 실제 소유 벡터에서 읽는다.
        hooks.allocate=[&allocation,&allocated,temporary](auto size) { if (allocated) throw std::logic_error("Template MemStream 한 작업의 중복 할당");allocation.resize(size);allocated=true;return temporary; }; // 실제 임시 할당은 원본 본문이0으로 채운다.
        hooks.release=[this](auto pointer) { if (pointer) ReleaseBuffer(pointer); }; // 같은 원시 섹션의 옛 할당만 반납한다.
        hooks.assertion=[](auto) { throw std::logic_error("Template 실제 MemStream 진단"); }; // 잘못된 저장 버퍼를 성공으로 대신하지 않는다.
        operation(hooks);auto& buffer=fort.sections.at(handle-1);
        if (Read(node)==temporary && allocated) { buffer=std::move(allocation);Put(node,0,0x10000U+handle); }
        const auto end=Read(std::span(node).subspan(8));if (end>buffer.size()) throw std::out_of_range("Template MemStream 완료 뒤 실제 end 범위");buffer.resize(end);PublishCursor(handle);
    }
    // 같은 stream의 물리 할당을 노출하고 원본 쓰기 뒤 논리 end만 원시 섹션에 남긴다.
    void WriteStream(std::uint32_t handle,std::span<const std::uint8_t> input) {
        if (input.size()>0xffffffffU) throw std::out_of_range("Template MemStream 원문 DWORD 길이 초과");
        auto& buffer=fort.sections.at(handle-1);const auto allocation=Read(std::span(streams.at(handle)->bytes).subspan(12));if (buffer.size()<allocation) buffer.resize(allocation);
        WithMemory(handle,[this,handle,input](const auto& hooks) { o::WriteMemStream(streams.at(handle)->bytes,input,static_cast<std::uint32_t>(input.size()),hooks); });
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
            if (streams.erase(handle)!=1) throw std::logic_error("Template 실제 노드 중복 반납");std::erase(streamOrder,handle);
            if (handle==StateSection()+1) state={{},0,0,false};if (handle==TerritorySection()+1) territory={};
        }; // 버퍼 소멸을 마친 노드와 그 typed cursor만 반납한다.
        hooks.compact=[this](auto handle) { CompactTail(handle); };hooks.missingPredecessor=[] { throw std::logic_error("Template 실제 선행 노드 진단"); };return hooks;
    }
    // 생성된 State 노드의 교체는 기존 노드 unlink/버퍼 반납/노드 반납 뒤 새 마지막 노드를 붙인다.
    void InitializeState(std::span<const std::uint8_t,28> input) {
        if (!retiredState.empty()) throw std::logic_error("State 확장 복사 중 새 섹션 생성 불가");
        SyncStreams();const auto handle=static_cast<std::uint32_t>(StateSection()+1);
        if (fort.sections.size()<=StateSection()) fort.sections.resize(StateSection()+1);
        const auto temporary=0x20000U+handle;std::vector<std::uint8_t> allocation;bool allocated{};o::DataManagerResizeHooks hooks;
        hooks.selection.stream=[this](auto stream) { return std::span(streams.at(stream)->bytes); };hooks.list=ListEffects(); // 검색/기존 제거/새 추가도 같은 실제 부모에서 수행한다.
        hooks.allocateStream=[this,handle](auto size) { if (size!=60 || streams.contains(handle)) throw std::logic_error("Template 새 State 실제 노드 할당 계약 위반");streams.emplace(handle,std::make_unique<Stream>());return handle; };
        hooks.memory.buffer=[this,&allocation,temporary](auto pointer)->std::span<std::uint8_t> {
            if (pointer==temporary) return allocation;
            if (pointer<0x10001U || pointer>0x10000U+o::FortTemplate::kSectionCount) throw std::out_of_range("Template 생성 부모의 실제 버퍼 범위");return fort.sections.at(pointer-0x10001U);
        }; // 새 할당은 완료 전까지 별도 소유하고 기존 tail 압축은 원래 실제 버퍼를 쓴다.
        hooks.memory.allocate=[&allocation,&allocated,temporary](auto size) { if (allocated) throw std::logic_error("Template 생성 부모의 중복 할당");allocation.resize(size);allocated=true;return temporary; };
        hooks.memory.assertion=[](auto) { throw std::logic_error("Template 생성 부모의 실제 MemStream 진단"); };
        const auto current=o::CreateDataManagerStream(manager,"State",hooks);auto& node=streams.at(handle)->bytes;
        if (current!=handle || !allocated || Read(node)!=temporary) throw std::logic_error("Template 생성 부모의 실제 현재 State 불일치");
        Put(node,0,0x10000U+handle);allocation.resize(Read(std::span(node).subspan(8)));fort.sections.at(StateSection())=std::move(allocation);streamOrder.push_back(handle);PublishCursor(handle);WriteStream(handle,input);
    }
    // 실제 임시 노드가 살아 있는 동안 원본 부모를 실행하고 완료한 같은 할당을 State로 넘긴다.
    void EnsureStateSize(std::int32_t minimum) {
        if (!retiredState.empty()) throw std::logic_error("분리 State 복사 중 원본 최소 확장 불가");
        SyncStreams();const auto stateHandle=static_cast<std::uint32_t>(StateSection()+1);const auto original=streams.find(stateHandle);
        if (original!=streams.end()) { Put(original->second->bytes,4,state.position);Put(original->second->bytes,8,state.end); }
        constexpr std::uint32_t kTemporaryStream=0x100,kTemporaryData=0x40000; // 공개하지 않는 동기 작업의 임시 노드/버퍼 토큰이며 실제 주소를 자르지 않는다.
        if (streams.contains(kTemporaryStream)) throw std::logic_error("Template 임시 stream 토큰 충돌");
        std::unique_ptr<Stream> temporary;std::vector<std::uint8_t> buffer;bool allocated{};o::DataManagerResizeHooks hooks;
        hooks.selection.stream=[this,&temporary](auto handle)->std::span<std::uint8_t> { if (handle==kTemporaryStream && temporary) return temporary->bytes;return streams.at(handle)->bytes; }; // 검색/이름 교체도 같은 임시 노드를 조회한다.
        hooks.list=ListEffects();hooks.list.lifetime.stream=hooks.selection.stream;
        hooks.memory.buffer=[this,&buffer](auto pointer)->std::span<std::uint8_t> {
            if (pointer==kTemporaryData) return buffer;
            if (pointer<0x10001U || pointer>0x10000U+o::FortTemplate::kSectionCount) throw std::out_of_range("Template 확장의 실제 원문 버퍼 핸들 범위");return fort.sections.at(pointer-0x10001U);
        }; // 복사는 원본 부모가 요청하는 현재 data/end를 사용한다.
        hooks.memory.allocate=[&buffer,&allocated](auto size) { if (allocated) throw std::logic_error("Template 확장 임시 버퍼 중복 할당");buffer.resize(size);allocated=true;return kTemporaryData; };
        hooks.memory.release=[this](auto pointer) { if (pointer) ReleaseBuffer(pointer); };hooks.memory.assertion=[](auto) { throw std::logic_error("Template 원본 확장의 MemStream 진단"); };
        hooks.allocateStream=[&temporary](auto size) { if (size!=60 || temporary) throw std::logic_error("Template 실제 임시 노드 할당 계약 위반");temporary=std::make_unique<Stream>();return kTemporaryStream; };
        hooks.missingSource=[] { throw std::logic_error("Template 원본 State 확장의 원문 섹션 누락"); };
        const auto current=o::EnsureDataManagerStreamSize(manager,"State",minimum,hooks);
        if (current!=kTemporaryStream) { if (current!=stateHandle) throw std::logic_error("Template 확장 반환의 실제 State 불일치");PublishCursor(stateHandle);return; }
        if (!temporary || !allocated || streams.contains(stateHandle) || Read(temporary->bytes)!=kTemporaryData) throw std::logic_error("Template 원본 확장의 임시 소유권 계약 위반");
        const auto end=Read(std::span(temporary->bytes).subspan(8));if (end>buffer.size()) throw std::out_of_range("Template 원본 확장 결과의 실제 end 범위");
        // 외부에 노출하지 않은 임시 핸들을 동일 이미지의 State 핸들로 매핑하고 실제 새 노드/버퍼를 그대로 인계한다.
        for (auto& entry:streams) if (Read(std::span(entry.second->bytes).subspan(0x38))==kTemporaryStream) Put(entry.second->bytes,0x38,stateHandle);
        if (Read(std::span(manager).subspan(4))==kTemporaryStream) Put(manager,4,stateHandle);
        Put(manager,8,stateHandle);Put(temporary->bytes,0,0x10000U+stateHandle);buffer.resize(end);fort.sections.at(StateSection())=std::move(buffer);streams.emplace(stateHandle,std::move(temporary));streamOrder.push_back(stateHandle);PublishCursor(stateHandle);
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
        SyncStreams();const auto handle=static_cast<std::uint32_t>(StateSection()+1);const auto found=streams.find(handle);
        if (found!=streams.end()) { Put(found->second->bytes,4,state.position);Put(found->second->bytes,8,state.end); }
        o::DataManagerSelectionHooks hooks;hooks.stream=[this](auto node) { return std::span(streams.at(node)->bytes); }; // 선택은 같은 실제 목록 노드를 빌린다.
        hooks.assertion=[](auto) { throw std::logic_error("Template 현재 State 섹션 선택 진단: 생성/파일 연결 미복원"); }; // 누락 섹션을 다른 섹션으로 대신하지 않는다.
        const auto selected=o::SelectDataManagerSection(manager,"State",1,hooks);
        state.position=Read(std::span(streams.at(selected)->bytes).subspan(4));
    }
    // Territory120BYTE 읽기는 비어 있지 않은 섹션 선택을 먼저 수행한다.
    void SelectTerritory() {
        SyncStreams();const auto handle=static_cast<std::uint32_t>(TerritorySection()+1);const auto found=streams.find(handle);
        if (found!=streams.end()) { Put(found->second->bytes,4,territory.position);Put(found->second->bytes,8,territory.end); }
        o::DataManagerSelectionHooks hooks;hooks.stream=[this](auto node) { return std::span(streams.at(node)->bytes); }; // 같은 소유 노드의 현재 data/end와 이름을 읽는다.
        hooks.assertion=[](auto) { throw std::logic_error("Template Territory 실제 읽기 섹션 선택 진단"); }; // 없는 섹션/빈 읽기를 다른 지형으로 대신하지 않는다.
        const auto selected=o::SelectDataManagerSection(manager,"Territory",0,hooks);
        territory.position=Read(std::span(streams.at(selected)->bytes).subspan(4));
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
        const auto names=o::FortTemplate::SectionNames();const auto count=std::max(fort.sections.size(),streams.size());std::uint32_t previous{}; // 현재 목록의 마지막 실제 노드다.
        // 제거된 호스트 버퍼의 기존 노드도 빈 stream으로 반납하고 새 섹션은 목록 끝에 부착한다.
        for (std::size_t index=0;index<count;++index) {
            const auto handle=static_cast<std::uint32_t>(index+1);auto& stream=streams[handle];
            if (!stream) { stream=std::make_unique<Stream>();std::copy(names[index].begin(),names[index].end(),stream->bytes.begin()+16);streamOrder.push_back(handle); }
            const auto size=index<fort.sections.size() ? fort.sections[index].size() : 0U;const auto capacity=index<fort.sections.size() ? fort.sections[index].capacity() : 0U; // 제품의 현재 실제 벡터를 반납 경계에서 다시 조회한다.
            Put(stream->bytes,0,capacity ? 0x10000U+handle : 0U);Put(stream->bytes,8,static_cast<std::uint32_t>(size));Put(stream->bytes,12,static_cast<std::uint32_t>(capacity));Put(stream->bytes,0x38,0);
        }
        // State 교체가 만든 목록 순서를 이후 조회/소멸 때 파일 번호순으로 되돌리지 않는다.
        for (const auto handle:streamOrder) {
            if (previous) Put(streams.at(previous)->bytes,0x38,handle);else Put(manager,4,handle);
            previous=handle;
        }
        if (!previous) Put(manager,4,0);
    }
    // 현재 각 섹션의 버퍼를 먼저 반납하고 같은 노드의 실제 할당을 뒤에 해제한다.
    void Destroy() {
        SyncStreams();o::DataManagerLifetimeHooks hooks;
        hooks.stream=[this](auto handle) { return std::span(streams.at(handle)->bytes); }; // 현재 같은60BYTE 노드만 빌린다.
        hooks.releaseBuffer=[this](auto handle) { const auto index=static_cast<std::size_t>(handle-0x10001U);std::vector<std::uint8_t>{}.swap(fort.sections.at(index)); }; // 실제 raw 섹션 할당이다.
        hooks.releaseStream=[this](auto handle) { if (streams.erase(handle)!=1) throw std::logic_error("DataManager 같은 섹션 중복 반납"); }; // 버퍼 소멸이 끝난 노드만 반납한다.
        o::DestroyDataManager(manager,hooks);
    }
    // State와 Territory 모두 파서가 소유한 원시 섹션의 같은 바이트를 연결한다.
    Image(o::FortTemplate value,std::string file):fort(std::move(value)),name(std::move(file)) {
        const auto at=StateSection();const auto bytes=at<fort.sections.size() ? std::span<std::uint8_t>(fort.sections[at]) : std::span<std::uint8_t>{};state={bytes,0,static_cast<std::uint32_t>(bytes.size()),!bytes.empty()};
        const auto territoryAt=TerritorySection();const auto territoryBytes=territoryAt<fort.sections.size() ? std::span<const std::uint8_t>(fort.sections[territoryAt]) : std::span<const std::uint8_t>{};
        territory={territoryBytes,0,static_cast<std::uint32_t>(territoryBytes.size())};SyncStreams();
    }
};
// 시작 초기화는 기존 manager를 반납하는 전역 정리와 다르다.
CampaignTemplateStorage::CampaignTemplateStorage() {
    // 아홉 개의 처음20BYTE만 원본 생성 몸체로 초기화한다.
    for (std::uint32_t slot=0;slot<9;++slot) o::InitializeTemplateHeader(Header(slot));
}
// C++ 종료의 소유권 반납이며 실제 미션 전환은 ClearAll을 명시 호출한다.
CampaignTemplateStorage::~CampaignTemplateStorage()=default;
// 같은 고정 물리 표를 빌린다.
std::span<std::uint8_t> CampaignTemplateStorage::Headers() { return headers_; }
// 유효 슬롯의 현재 기록만 반환한다.
std::span<std::uint8_t> CampaignTemplateStorage::Header(std::uint32_t slot) { if (slot>=9) throw std::out_of_range("Template 실제9칸 범위");return std::span(headers_).subspan(slot*20,20); }
// 헤더가 빌린 같은 handle도 같은 이미지/State cursor를 찾는다.
CampaignTemplateStorage::Image& CampaignTemplateStorage::Current(std::uint32_t slot) { const auto handle=Read(Header(slot));const auto at=images_.find(handle);if (!handle || at==images_.end()) throw std::logic_error("Template 현재 manager가 없거나 소유자보다 오래 산 빌림");return *at->second; }
// 실제 소멸은 안쪽 버퍼를 비우고 free는 manager 객체 할당을 반납한다.
o::TemplateLifetimeHooks CampaignTemplateStorage::Effects() {
    return {[this](auto handle) { auto& image=*images_.at(handle);image.Destroy();image.fort=o::FortTemplate{};image.state={{},0,0,false};image.territory={};std::vector<std::uint8_t>{}.swap(image.retiredState); },[this](auto handle) { if (images_.erase(handle)!=1) throw std::logic_error("Template 같은 manager 중복 반납"); }};
}
// 기존 한 manager를 정확히 해제한 뒤 새 실제 객체를 소유한다.
void CampaignTemplateStorage::Install(std::uint32_t slot,o::FortTemplate fort,std::string name) { Clear(slot);if (!nextHandle_) throw std::overflow_error("Template 호스트 manager 핸들 소진");const auto handle=nextHandle_++;images_.emplace(handle,std::make_unique<Image>(std::move(fort),std::move(name)));Put(Header(slot),0,handle); }
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
// 미정 바이트를 포함한 완성28BYTE를 실제 새 섹션에 쓰고 같은 현재 cursor를 반환한다.
o::PlayerTemplateStateStream& CampaignTemplateStorage::InitializeState(std::uint32_t slot,std::span<const std::uint8_t,28> bytes) { auto& image=Current(slot);image.InitializeState(bytes);return image.state; }
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
// 원문 선택 때 쓴 같은 파일 이름을 반환한다.
const std::string& CampaignTemplateStorage::Name(std::uint32_t slot) { return Current(slot).name; }
// 원본을 옮겨 보존한 뒤 같은 Fort State 섹션에 대체 할당을 만든다.
std::span<std::uint8_t> CampaignTemplateStorage::GrowState(std::uint32_t slot,std::uint32_t size) { auto& image=Current(slot);if (!image.retiredState.empty()) throw std::logic_error("Template State 확장 복사 미완료");const auto at=StateSection();if (image.fort.sections.size()<=at) image.fort.sections.resize(at+1);auto& bytes=image.fort.sections[at];image.retiredState=std::move(bytes);bytes.resize(size);return bytes; }
// 복사 뒤에만 이전 실제 버퍼를 반납한다.
void CampaignTemplateStorage::CommitStateGrowth(std::uint32_t slot) { std::vector<std::uint8_t>{}.swap(Current(slot).retiredState); }
// 실제 manager 할당 수만 읽는다.
std::size_t CampaignTemplateStorage::OwnedImages() const { return images_.size(); }
}
