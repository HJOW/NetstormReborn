// existing Stategetter의pos0/최소28과 Notify의isMine재조회/선택저장거부를보존한다.
#include "o/RawPlayerTemplateState.h"
#include <bit>
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 원본 AND/OR와 개별 DWORD/WORD/BYTE 쓰기 밖의 미정 바이트는 변경하지 않는다.
void InitializePlayerTemplateStateBytes(std::span<std::uint8_t,28> bytes) {
    bytes[0]=static_cast<std::uint8_t>((bytes[0]&0xf9U)|1U);
    std::fill(bytes.begin()+4,bytes.begin()+10,std::uint8_t{});
    std::fill(bytes.begin()+12,bytes.begin()+18,std::uint8_t{});
    std::fill(bytes.begin()+22,bytes.begin()+26,std::uint8_t{});
}
// CD 빈 State는 별도 근거가 필요하므로 PATCH에만 이 소비자를 설치한다.
void RawPlayerTemplateState::BindInitialization(PlayerTemplateStateInitialization initialization) {
    if (edition_!=OriginalEdition::Patch1078 || !initialization.bytes || !initialization.replace) throw std::invalid_argument("빈 State의 PATCH 미정 입력/실제 생성 연결 누락");
    initialization_=std::move(initialization);
}
// 단순 새 버퍼 할당과 달리 실제 부모가 돌려준 새 현재 stream을 직접 연결한다.
void RawPlayerTemplateState::BindMinimumSize(std::function<PlayerTemplateStateStream&(std::uint32_t,std::int32_t)> ensure) {
    if (edition_!=OriginalEdition::Patch1078 || !ensure) throw std::invalid_argument("최소 State 확장의 PATCH 실제 부모 연결 누락");ensureMinimum_=std::move(ensure);
}
// 현재9개Template/State와선택진단·CD깊이효과를확인한다.
RawPlayerTemplateState::RawPlayerTemplateState(OriginalEdition edition,std::span<std::uint8_t> headers,
    std::array<PlayerTemplateStateStream,9>& states,std::array<std::string,9>& fileNames,
    std::uint32_t& loadingDepth,const bool& inMission,std::function<void(PlayerTemplateStateAssertion)> assertion,
    std::function<void(std::uint32_t)> depthChanged,std::function<std::span<std::uint8_t>(std::uint32_t,std::uint32_t)> allocate,std::function<void(std::uint32_t)> allocationComplete)
    :RawPlayerTemplateState(edition,headers,[&states](auto slot)->PlayerTemplateStateStream& { return states.at(slot); },[&fileNames](auto slot)->const std::string& { return fileNames.at(slot); },loadingDepth,inMission,std::move(assertion),std::move(depthChanged),std::move(allocate),std::move(allocationComplete)) {}
// 현재 manager 조회를 빌려 같은 State/이름을 사용하며 자체 배열 사본을 만들지 않는다.
RawPlayerTemplateState::RawPlayerTemplateState(OriginalEdition edition,std::span<std::uint8_t> headers,
    std::function<PlayerTemplateStateStream&(std::uint32_t)> state,std::function<const std::string&(std::uint32_t)> name,
    std::uint32_t& loadingDepth,const bool& inMission,std::function<void(PlayerTemplateStateAssertion)> assertion,
    std::function<void(std::uint32_t)> depthChanged,std::function<std::span<std::uint8_t>(std::uint32_t,std::uint32_t)> allocate,std::function<void(std::uint32_t)> allocationComplete)
    :edition_(edition),headers_(headers),states_(std::move(state)),names_(std::move(name)),depth_(loadingDepth),mission_(inMission),assertion_(std::move(assertion)),allocate_(std::move(allocate)),depthChanged_(std::move(depthChanged)),allocationComplete_(std::move(allocationComplete)) {
    if (headers.size()!=180 || !states_ || !names_ || !assertion_ || !depthChanged_) throw std::invalid_argument("Player Template실제9기록/State/진단/깊이효과누락");
}
// 호스트포인터로reinterpret하지않고매조회현재DWORD를읽는다.
std::uint32_t RawPlayerTemplateState::Header(std::uint32_t slot,std::size_t offset) const {
    if (slot>=9 || offset>16) throw std::out_of_range("Player Template실제9개/20BYTE기록범위");std::uint32_t value=0;
    // 실제네BYTE필드만낮은비트부터결합한다.
    for (std::size_t index=0;index<4;++index) value|=static_cast<std::uint32_t>(headers_[slot*20+offset+index])<<(8*index);
    return value;
}
// 실제State이름선택/조회는이9개전용뷰의동일Statecursor를사용하며데이터사본을만들지않는다.
std::span<std::uint8_t> RawPlayerTemplateState::Get(std::uint32_t slot) const {
    if (edition_==OriginalEdition::Cd1072) { ++depth_;depthChanged_(depth_);--depth_;depthChanged_(depth_); }
    if (!Header(slot,0)) assertion_(PlayerTemplateStateAssertion::MissingStateTemplate);
    if (!Header(slot,0)) throw std::logic_error("Player Stategetter실제manager0메모리경계");
    auto* current=&states_(slot);current->position=0; // 실제lookupState(writable1)의현재MemStream pos0쓰기다.
    if (!current->present || !current->end) {
        if (!initialization_.bytes || !initialization_.replace) throw std::logic_error("Player 빈 State의 미정 바이트/실제 섹션 생성 미연결");
        std::array<std::uint8_t,28> bytes;initialization_.bytes(slot,bytes);InitializePlayerTemplateStateBytes(bytes);
        current=&initialization_.replace(slot,bytes); // 이전 stream 참조를 새 현재 State에 계속 사용하지 않는다.
        if (!current->present || current->end!=28 || current->position!=28 || current->bytes.size()<28) throw std::logic_error("Player 실제 빈 State 생성/쓰기 계약 위반");
    }
    if (std::bit_cast<std::int32_t>(current->end)<28 && ensureMinimum_) {
        current=&ensureMinimum_(slot,28);
        if (!current->present || std::bit_cast<std::int32_t>(current->end)<28 || current->end>current->bytes.size()) throw std::logic_error("Player 최소 State 확장의 실제 현재 stream 계약 위반");
    }
    auto& stream=*current; // 생성/확장된 경우에는 소비자가 돌려준 같은 새 현재 stream을 사용한다.
    if (std::bit_cast<std::int32_t>(stream.end)<28) {
        if (!allocate_) throw std::logic_error("Player shortState 실제MemStream할당 미연결");
        auto replacement=allocate_(slot,28); //원본44d290은새record를할당한뒤현재원문의길이/포인터를다시읽는다.
        if (replacement.size()<28 || stream.end>28 || stream.end>stream.bytes.size()) throw std::out_of_range("Player shortState 현재원문/새할당 범위");
        std::copy_n(stream.bytes.begin(),stream.end,replacement.begin());std::fill(replacement.begin()+stream.end,replacement.begin()+28,std::uint8_t{});
        stream.bytes=replacement.first(28);stream.position=28;stream.end=28; //임시MemStream을현재State로바꾸며pos/end는둘다28이다.
        if (allocationComplete_) allocationComplete_(slot); // 원본을 모두 복사한 뒤 이전 실제 버퍼를 반납한다.
    }
    if (stream.bytes.size()<28 || stream.end>stream.bytes.size()) throw std::out_of_range("Player 현재State물리BYTE범위");return stream.bytes;
}
// 실제isMine후재검사에의한earlyreturn은빈callback대체가아니다.
void RawPlayerTemplateState::Changed(std::uint32_t slot) const {
    if (slot>=9 || names_(slot).empty()) throw std::out_of_range("Player Notify현재manager.fileName물리BYTE누락");
    if (names_(slot)[0]=='.') assertion_(PlayerTemplateStateAssertion::DotFileName);
    if (Header(slot,8)) {
        if (Header(slot,12)) assertion_(PlayerTemplateStateAssertion::MineTransientFlag);
        if (Header(slot,8)) return;
    }
    if (mission_) assertion_(PlayerTemplateStateAssertion::InMissionSave);
    if (!Header(slot,0)) assertion_(PlayerTemplateStateAssertion::MissingNotifyTemplate);
    if (Header(slot,16)) return;
    throw std::logic_error("Player Template실제DataManager파일저장44d8d0/CD44cc40미복원");
}
}
