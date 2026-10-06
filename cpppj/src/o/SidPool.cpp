// 원본 두 판본의 슬롯 초기화·번호 할당·FIFO 반납·서버 free list 재구성을 옮긴다.
#include "o/SidPool.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본 raw 슬롯의 next·타입·상태 바이트 위치는 두 판본에서 같다.
constexpr std::size_t kNext=4,kType=10,kState=11;
// 일반 객체 첫 번호와 free/dead/void 비트다. 0~4는 일반 할당 대상이 아니다.
constexpr std::uint16_t kFirst=5;
constexpr std::uint8_t kFree=1,kDead=2,kVoid=4;
// 일반 free list의 두 머리는 pool+0와 pool+14에 놓인다.
constexpr std::array<std::size_t,2> kHeads{0,14};
}
// raw 슬롯을 저장하는 호스트 벡터와 원본 판본 경계를 준비한다.
SidPool::SidPool(OriginalEdition edition,std::uint32_t capacity,bool server)
    :layout_(edition==OriginalEdition::Patch1078 ? SidLayout{50,15000,23001} : SidLayout{36,6000,14000}),
     capacity_(capacity),server_(server) {
    // 원본의 고정 free list가 풀 밖을 쓰는 잘못된 크기는 새 C++에서 거부한다.
    if (capacity_<=layout_.predictableFirst+1 || capacity_>65535) throw std::invalid_argument("SID 풀 크기 오류");
    bytes_.resize(static_cast<std::size_t>(capacity_)*layout_.stride);
    Reset();
}
// 기존 메모리 전체를 지우고 세 영역을 잇는다. predictableHead 자체는 할당하지 않는다.
void SidPool::Reset() {
    std::fill(bytes_.begin(),bytes_.end(),std::uint8_t{0});
    // 원본에서 2번부터 free 비트가 켜진다. 0/1번은 초기 상태 0을 유지한다.
    for (std::uint32_t sid=2;sid<capacity_;++sid) bytes_[sid*layout_.stride+kState]=kFree;
    // 일반 클라이언트·서버·예측 영역의 끝은 각각 0으로 닫힌다.
    for (std::uint32_t sid=kFirst;sid<capacity_;++sid) {
        const auto end=sid<layout_.serverFirst ? layout_.serverFirst :
            (sid<layout_.predictableFirst ? layout_.predictableFirst : capacity_);
        WriteNext(sid*layout_.stride,static_cast<std::uint16_t>(sid+1==end ? 0 : sid+1));
    }
    WriteNext(kHeads[0],kFirst);
    WriteNext(kHeads[1],static_cast<std::uint16_t>(layout_.serverFirst));
    tails_={static_cast<std::uint16_t>(layout_.serverFirst-1),static_cast<std::uint16_t>(layout_.predictableFirst-1)};
    freeCount_=capacity_-kFirst;
    predictableCursor_=1;
}
// 실제 next 필드를 사용한다. 마지막 일반 free 슬롯은 원본처럼 꼬리로 남긴다.
Sid SidPool::Allocate(std::uint32_t flags) {
    std::uint32_t sid{};
    if (flags&1) {
        const auto head=layout_.predictableFirst*layout_.stride;
        sid=layout_.predictableFirst+predictableCursor_;
        if (sid>=capacity_ || !(bytes_[head+kState]&kFree)) throw std::out_of_range("예측 SID 소진");
        const auto offset=sid*layout_.stride;
        if (!(bytes_[offset+kState]&kFree)) throw std::logic_error("예측 SID가 사용 중입니다");
        if (server_) {
            const auto next=ReadNext(offset);
            if (next!=sid+1 || next>=capacity_ || !(bytes_[next*layout_.stride+kState]&kFree))
                throw std::out_of_range("예측 SID 서버 꼬리 소진");
            WriteNext(head,next);
        }
        ++predictableCursor_;
    } else {
        const bool client=(flags&2)!=0;
        if (!client && !server_) throw std::logic_error("서버 SID 할당 권한이 없습니다");
        const auto list=client ? 0U : 1U;
        sid=ReadNext(kHeads[list]);
        // 원본의 소진 시 UI/다리 파괴 분기는 후속이다. 이 보호 경로에서는 풀을 바꾸지 않는다.
        if (!sid || sid==tails_[list]) throw std::out_of_range("일반 SID 소진: 예약 꼬리를 유지합니다");
        const auto offset=Offset(Sid{static_cast<std::uint16_t>(sid)});
        if (!(bytes_[offset+kState]&kFree)) throw std::logic_error("free list SID가 사용 중입니다");
        const auto next=ReadNext(offset);
        if (!next || next>=capacity_) throw std::logic_error("free list next 오류");
        WriteNext(kHeads[list],next);
    }
    const auto offset=sid*layout_.stride;
    std::fill_n(bytes_.begin()+offset,layout_.stride,std::uint8_t{0});
    bytes_[offset+kState]=kVoid;
    --freeCount_;
    return Sid{static_cast<std::uint16_t>(sid)};
}
// 삭제 기록→슬롯 초기화/타입 보존→필요한 FIFO 꼬리 연결→상태를 원본 순서로 적용한다.
void SidPool::Release(Sid sid) {
    const auto offset=Offset(sid);
    if (sid.value<kFirst || sid.value==layout_.predictableFirst ||
        !(bytes_[offset+kState]&kVoid) || (bytes_[offset+kState]&kFree))
        throw std::logic_error("반납할 SID가 void 할당 객체가 아닙니다");
    const auto type=bytes_[offset+kType];
    Record(sid,type);
    std::fill_n(bytes_.begin()+offset,layout_.stride,std::uint8_t{0});
    bytes_[offset+kType]=type;
    if (!server_ && sid.value>=layout_.serverFirst) {
        bytes_[offset+kState]=kFree;
        return;
    }
    if (sid.value<layout_.predictableFirst) {
        const auto list=sid.value<layout_.serverFirst ? 0U : 1U;
        WriteNext(tails_[list]*layout_.stride,sid.value);
        tails_[list]=sid.value;
        ++freeCount_;
    }
    bytes_[offset+kState]=kFree|kDead;
}
// free 서버 슬롯을 번호순으로 재연결한다. 카운터 누적은 원본 의미 그대로다.
void SidPool::RebuildServer() {
    std::vector<std::uint16_t> free;
    // 파생 객체 효과 없이 raw free 비트만 확인한다.
    for (std::uint32_t sid=layout_.serverFirst;sid<layout_.predictableFirst;++sid)
        if (bytes_[sid*layout_.stride+kState]&kFree) free.push_back(static_cast<std::uint16_t>(sid));
    if (free.empty()) throw std::logic_error("서버 free SID가 없습니다");
    WriteNext(kHeads[1],free.front());
    // 각 next는 뒤 번호를 가리키고 마지막 항목은 0으로 닫힌다.
    for (std::size_t i=0;i<free.size();++i) WriteNext(free[i]*layout_.stride,i+1<free.size() ? free[i+1] : 0);
    tails_[1]=free.back();
    freeCount_+=static_cast<std::uint32_t>(free.size());
}
// 파생 클래스가 생성자를 연결할 때만 사용할 수 있는 할당 슬롯이다.
std::span<std::uint8_t> SidPool::AllocatedBytes(Sid sid) {
    const auto offset=Offset(sid);
    if (sid.value<kFirst || sid.value==layout_.predictableFirst || (bytes_[offset+kState]&kFree))
        throw std::logic_error("할당되지 않은 SID 쓰기");
    return std::span(bytes_).subspan(offset,layout_.stride);
}
// 원본 raw 필드와 검증용 풀 상태를 읽는다.
std::span<const std::uint8_t> SidPool::Slot(Sid sid) const { return std::span(bytes_).subspan(Offset(sid),layout_.stride); }
// 풀 전체의 raw 바이트를 읽기 전용으로 반환한다.
std::span<const std::uint8_t> SidPool::Bytes() const { return bytes_; }
// 최근 20개 삭제 기록을 클라이언트/서버 영역별로 반환한다.
std::span<const SidDeletion> SidPool::Deletions(bool client) const { return deletions_[client ? 0 : 1]; }
// 원본 판본의 raw 슬롯 크기와 영역 경계를 반환한다.
SidLayout SidPool::Layout() const { return layout_; }
// owner/HP 초기화의 원본 서버 분기를 선택한다.
bool SidPool::IsServer() const { return server_; }
// 이미 확정된 raw 슬롯 크기에서 판본을 반환한다.
OriginalEdition SidPool::Edition() const { return layout_.stride==50 ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072; }
// 확보한 raw 슬롯 수를 반환한다.
std::uint32_t SidPool::Capacity() const { return capacity_; }
// 원본 카운터를 반환한다. 재구성의 누적과 예측 반납의 비증가도 보존한다.
std::uint32_t SidPool::FreeCount() const { return freeCount_; }
// 다음 예측 번호에 더하는 원본 커서를 반환한다.
std::uint32_t SidPool::PredictableCursor() const { return predictableCursor_; }
// 지정 영역의 머리 객체가 가리키는 첫 빈 슬롯을 반환한다.
Sid SidPool::FirstFree(bool client) const { return Sid{ReadNext(kHeads[client ? 0 : 1])}; }
// 할당 때 예약하는 지정 영역의 마지막 빈 슬롯을 반환한다.
Sid SidPool::Tail(bool client) const { return Sid{tails_[client ? 0 : 1]}; }
// 원본 raw next를 16비트 정수로 읽는다. 호스트 정렬과 엔디언에 의존하지 않는다.
std::uint16_t SidPool::ReadNext(std::size_t offset) const {
    return static_cast<std::uint16_t>(bytes_[offset+kNext]|(bytes_[offset+kNext+1]<<8));
}
// raw next를 little endian으로 쓴다.
void SidPool::WriteNext(std::size_t offset,std::uint16_t next) {
    bytes_[offset+kNext]=static_cast<std::uint8_t>(next);
    bytes_[offset+kNext+1]=static_cast<std::uint8_t>(next>>8);
}
// 범위를 벗어난 번호는 배열 밖 접근 전에 거부한다.
std::size_t SidPool::Offset(Sid sid) const {
    if (sid.value>=capacity_) throw std::out_of_range("SID 번호 범위 오류");
    return static_cast<std::size_t>(sid.value)*layout_.stride;
}
// 원본 memmove/memcpy의 겹친 152바이트 이동 결과를 항목 단위로 보존한다.
void SidPool::Record(Sid sid,std::uint8_t type) {
    auto& records=deletions_[sid.value>=kFirst && sid.value<layout_.serverFirst ? 0 : 1];
    std::move_backward(records.begin(),records.end()-1,records.end());
    records[0]={sid.value,type};
}
}
