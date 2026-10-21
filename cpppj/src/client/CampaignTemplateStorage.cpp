// 원본 명시 정리 시점까지 이전 이미지의 소유권을 Client에 유지한다.
#include "client/CampaignTemplateStorage.h"
#include "o/RawDataManagerLifetime.h"
#include <algorithm>
#include <stdexcept>
namespace netstorm::client {
namespace {
// State의 실제 파일 섹션 위치를 기존 파서의 이름 표에서 찾는다.
std::size_t StateSection() { const auto names=o::FortTemplate::SectionNames();const auto at=std::find(names.begin(),names.end(),"State");if (at==names.end()) throw std::logic_error("Template 실제 State 섹션 이름 누락");return static_cast<std::size_t>(at-names.begin()); }
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
    o::FortTerritoryStream territory; // 같은 fort.territory의 현재 cursor다.
    std::string name; // 같은 현재 파일 이름이다.
    std::vector<std::uint8_t> retiredState; // 확장 중 Get의 원문 복사가 끝날 때까지만 이전 할당을 보유한다.
    std::array<std::uint8_t,12> manager{}; // 소멸이 사용하는 vptr/head/current의 실제 현재 저장소다.
    std::map<std::uint32_t,std::unique_ptr<Stream>> streams; // 노드 반납 때 실제 객체 할당을 해제한다.
    // 공개된 Fort/State의 현재 버퍼를 같은 목록에 연결하며 파일 읽기 구현 자체를 대신하지 않는다.
    void SyncStreams() {
        if (fort.sections.size()>o::FortTemplate::kSectionCount) throw std::logic_error("Template 실제 섹션35칸 범위");
        const auto names=o::FortTemplate::SectionNames();const auto count=std::max(fort.sections.size(),streams.size());std::uint32_t previous{}; // 파일 섹션 순서의 마지막 실제 노드다.
        // 제거된 호스트 버퍼의 기존 노드도 빈 stream으로 반납하고 새 섹션은 목록 끝에 부착한다.
        for (std::size_t index=0;index<count;++index) {
            const auto handle=static_cast<std::uint32_t>(index+1);auto& stream=streams[handle];
            if (!stream) { stream=std::make_unique<Stream>();std::copy(names[index].begin(),names[index].end(),stream->bytes.begin()+16); }
            const auto size=index<fort.sections.size() ? fort.sections[index].size() : 0U;const auto capacity=index<fort.sections.size() ? fort.sections[index].capacity() : 0U; // 제품의 현재 실제 벡터를 반납 경계에서 다시 조회한다.
            Put(stream->bytes,0,capacity ? 0x10000U+handle : 0U);Put(stream->bytes,8,static_cast<std::uint32_t>(size));Put(stream->bytes,12,static_cast<std::uint32_t>(capacity));Put(stream->bytes,0x38,0);
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
    // 별도 State 사본을 만들지 않고 파서가 소유한 같은 바이트를 연결한다.
    Image(o::FortTemplate value,std::string file):fort(std::move(value)),name(std::move(file)) { const auto at=StateSection();const auto bytes=at<fort.sections.size() ? std::span<std::uint8_t>(fort.sections[at]) : std::span<std::uint8_t>{};state={bytes,0,static_cast<std::uint32_t>(bytes.size()),!bytes.empty()};territory={fort.territory,0,static_cast<std::uint32_t>(fort.territory.size())};SyncStreams(); }
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
// 지형 cursor도 같은 manager의 원문을 빌린다.
o::FortTerritoryStream& CampaignTemplateStorage::Territory(std::uint32_t slot) { return Current(slot).territory; }
// 원문 선택 때 쓴 같은 파일 이름을 반환한다.
const std::string& CampaignTemplateStorage::Name(std::uint32_t slot) { return Current(slot).name; }
// 원본을 옮겨 보존한 뒤 같은 Fort State 섹션에 대체 할당을 만든다.
std::span<std::uint8_t> CampaignTemplateStorage::GrowState(std::uint32_t slot,std::uint32_t size) { auto& image=Current(slot);if (!image.retiredState.empty()) throw std::logic_error("Template State 확장 복사 미완료");const auto at=StateSection();if (image.fort.sections.size()<=at) image.fort.sections.resize(at+1);auto& bytes=image.fort.sections[at];image.retiredState=std::move(bytes);bytes.resize(size);return bytes; }
// 복사 뒤에만 이전 실제 버퍼를 반납한다.
void CampaignTemplateStorage::CommitStateGrowth(std::uint32_t slot) { std::vector<std::uint8_t>{}.swap(Current(slot).retiredState); }
// 실제 manager 할당 수만 읽는다.
std::size_t CampaignTemplateStorage::OwnedImages() const { return images_.size(); }
}
