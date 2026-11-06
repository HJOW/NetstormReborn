// 원본 두 판본의 슬롯 초기화·번호 할당·FIFO 반납·서버 free list 재구성을 옮긴다.
#include "o/SidPool.h"
#include <algorithm>
#include <bit>
#include <stdexcept>
#include <utility>

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
SidPool::SidPool(OriginalEdition edition,std::uint32_t capacity,bool server,const bool* sharedServer)
    :layout_(edition==OriginalEdition::Patch1078 ? SidLayout{50,15000,23001} : SidLayout{36,6000,14000}),
     capacity_(capacity),server_(server),sharedServer_(sharedServer) {
    // 원본의 고정 free list가 풀 밖을 쓰는 잘못된 크기는 새 C++에서 거부한다.
    if (capacity_<=layout_.predictableFirst+1 || capacity_>65535) throw std::invalid_argument("SID 풀 크기 오류");
    bytes_.resize(static_cast<std::size_t>(capacity_)*layout_.stride);
    Reset();
}
// 기존 메모리 전체를 지우고 세 영역을 잇는다. predictableHead 자체는 할당하지 않는다.
void SidPool::Reset(std::int32_t capacity) {
    if (capacity>0) {
        const auto requested=static_cast<std::uint32_t>(capacity);
        if (requested<=layout_.predictableFirst+1 || requested>65535) throw std::invalid_argument("SID 풀 재확보 크기 오류");
        // 확보 실패 시 기존 풀/목록/삭제 기록은 그대로 유지한다. 원본 malloc 실패의 잘못된 접근은 재현하지 않는다.
        std::vector<std::uint8_t> replacement(static_cast<std::size_t>(requested)*layout_.stride);
        bytes_.swap(replacement);capacity_=requested;
    }
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
// 각 SID/type DWORD만 순서대로 지우며 Pool 바이트·목록·권한을 다시 초기화하지 않는다.
void SidPool::ClearDeletionHistory() {
    for (auto& record:deletions_[1]) { record.sid=0;record.type=0; } //원문 첫40DWORD인 other 기록의 실제20쌍이다.
    for (auto& record:deletions_[0]) { record.sid=0;record.type=0; } //다음40DWORD인 client 기록의 실제20쌍이다.
}
// 장치연결은기존목록/카운터/현재원문값을바꾸지않는다.
void SidPool::BindAllocation(SidAllocationBindings bindings) { allocation_=std::move(bindings); }
// 진단의정상복귀/예외를그대로전달하며자가재연결중에도선택한callable을유지한다.
void SidPool::AssertAllocation(std::string_view expression,std::uint32_t line,std::string_view file) const {
    const auto assertion=allocation_.assertion;if (!assertion) throw std::logic_error("SID 할당의 선택된 원문 단언 미연결");assertion(expression,file.empty() ? (Edition()==OriginalEdition::Cd1072 ? "\\ns\\o\\Squid.cpp":"\\Ns\\O\\Squid.cpp"):file,line);
}
// PATCH004af1d0의진단/현재값/소진회수/머리갱신/슬롯초기화순서를실제Pool에연결한다.
Sid SidPool::Allocate(std::uint32_t flags) {
    if (Edition()==OriginalEdition::Cd1072) return AllocateCd(flags); //CD의 다른 dump/상세/고정 후보 루프를 PATCH로 대체하지 않는다.
    if (allocation_.disallow && *allocation_.disallow) AssertAllocation("disallowSquidAllocation == 0",0xe8);
    const auto raw=[this](std::uint32_t sid) { //DWORD를WORD로좁히기전에실제물리범위만검사한다.
        if (sid>=capacity_) throw std::out_of_range("SID 할당의 실제 물리 범위");return std::span(bytes_).subspan(static_cast<std::size_t>(sid)*layout_.stride,layout_.stride);
    };
    std::uint32_t sid{};
    if (flags&1U) {
        const auto head=layout_.predictableFirst*layout_.stride;
        if (!(bytes_[head+kState]&kFree)) AssertAllocation("predictableHead->isFree()",0xf0);
        sid=layout_.predictableFirst+predictableCursor_;
        if (std::bit_cast<std::int32_t>(sid)>=static_cast<std::int32_t>(capacity_)) AssertAllocation("sid < numSquids",0xf2); //원문signed비교뒤호스트물리범위를따로검사한다.
        if (!(raw(sid)[kState]&kFree)) AssertAllocation("me->isFree()",0xf4);
        if (IsServer()) {
            if (ReadNext(static_cast<std::size_t>(sid)*layout_.stride)!=sid+1U) AssertAllocation("me->getNext() == sid+1",0xf7);
            const auto next=ReadNext(static_cast<std::size_t>(sid)*layout_.stride); //진단이바꾼현재next를다시읽는다.0도실제Pool안의포인터다.
            if (!(raw(next)[kState]&kFree)) AssertAllocation("checkMe->isFree()",0xfa);
            const auto selected=ReadNext(static_cast<std::size_t>(sid)*layout_.stride);
            if (selected==layout_.predictableFirst) AssertAllocation("n != getSid()",0x123,"c:\\ns\\o\\squid.h");
            WriteNext(head,selected);
        }
    } else {
        const bool client=(flags&2U)!=0;
        if (!client && !IsServer()) AssertAllocation("server",0x10a);
        const auto list=client ? 0U:1U;
        if (ReadNext(kHeads[list])==tails_[list]) {
            const auto boss=allocation_.boss;if (!boss) throw std::out_of_range("SID 소진 복구의 현재 boss 미연결");
            if (*boss) {
                const auto end=capacity_;std::uint32_t candidate=2,remaining=50; //원문iterator는1에서먼저전진하고끝번호를한번보관한다.
                while (candidate<end && (raw(candidate)[kState]&kFree)) ++candidate; //첫현재live슬롯을원문과같이찾는다.
                while (candidate>=1 && candidate<capacity_ && remaining) { //회수콜백뒤현재capacity/TYPE를다시읽고free만건너뛴다.
                    const auto bridge=allocation_.bridgeType;if (!bridge) throw std::logic_error("SID 소진 복구의 현재 Bridge TYPE 미연결");
                    if (raw(candidate)[kType]==*bridge) {
                        const auto reclaim=allocation_.reclaim;if (!reclaim) throw std::logic_error("SID 소진 복구의 현재 가상 삭제 미연결");reclaim(Sid{static_cast<std::uint16_t>(candidate)});--remaining;
                    }
                    ++candidate;
                    while (candidate<end && (raw(candidate)[kState]&kFree)) ++candidate; //현재배열/최초끝을사용하며삭제목록을사본으로고정하지않는다.
                }
            }
            if (!client && !IsServer()) AssertAllocation("server",0x139); //회수뒤재선택하는원문권한관문이다.
        }
        sid=ReadNext(kHeads[list]);
        if (!(raw(sid)[kState]&kFree)) AssertAllocation("p->isFree()",0x149);
        if (sid==tails_[list]) AssertAllocation("p != *freeTail",0x14a);
        const auto next=ReadNext(static_cast<std::size_t>(sid)*layout_.stride);
        if (!next) AssertAllocation("n != getSid()",0x123,"c:\\ns\\o\\squid.h"); //두head객체의원문번호는모두0이다.정상복귀뒤0도그대로쓴다.
        WriteNext(kHeads[list],next);
    }
    auto selected=raw(sid);std::fill(selected.begin(),selected.end(),std::uint8_t{});selected[kState]=kVoid;
    if (flags&1U) ++predictableCursor_;
    --freeCount_;return Sid{static_cast<std::uint16_t>(sid)};
}
// CD004aae40의 정상 복귀 진단과 고정 후보50회 호출을 보존하고 무효과 무한 반복은 명시 실패한다.
Sid SidPool::AllocateCd(std::uint32_t flags) {
    if (allocation_.disallow && *allocation_.disallow) AssertAllocation("disallowSquidAllocation == 0",0xe8);
    const auto raw=[this](std::uint32_t sid) { //DWORD 번호의 원문 진단 뒤 실제 호스트 저장소 밖 접근만 거부한다.
        if (sid>=capacity_) throw std::out_of_range("CD SID 할당의 실제 물리 범위");return std::span(bytes_).subspan(static_cast<std::size_t>(sid)*layout_.stride,layout_.stride);
    };
    std::uint32_t sid{};
    if (flags&1U) {
        const auto head=layout_.predictableFirst*layout_.stride;
        if (!(bytes_[head+kState]&kFree)) AssertAllocation("predictableHead->isFree()",0xf0);
        sid=layout_.predictableFirst+predictableCursor_;
        if (std::bit_cast<std::int32_t>(sid)>=static_cast<std::int32_t>(capacity_)) AssertAllocation("sid < numSquids",0xf2);
        if (!(raw(sid)[kState]&kFree)) AssertAllocation("me->isFree()",0xf4);
        if (IsServer()) {
            if (ReadNext(static_cast<std::size_t>(sid)*layout_.stride)!=sid+1U) AssertAllocation("me->getNext() == sid+1",0xf7);
            const auto next=ReadNext(static_cast<std::size_t>(sid)*layout_.stride);
            if (!(raw(next)[kState]&kFree)) AssertAllocation("checkMe->isFree()",0xfa);
            const auto selected=ReadNext(static_cast<std::size_t>(sid)*layout_.stride);
            if (selected==layout_.predictableFirst) AssertAllocation("n != getSid()",0x10c,"\\ns\\o\\squid.h");
            WriteNext(head,selected);
        }
    } else {
        const bool client=(flags&2U)!=0;
        if (!client && !IsServer()) AssertAllocation("server",0x10a);
        const auto list=client ? 0U : 1U;
        if (ReadNext(kHeads[list])==tails_[list]) {
            const auto diagnose=[this](std::string_view text,std::uint32_t line) { //원문의 dump→상세→단언을 각 선택 위치에서 조회한다.
                const auto dump=allocation_.dump;if (!dump) throw std::logic_error("CD SID 소진 객체 덤프 미연결");dump();
                const auto detail=allocation_.detail;if (!detail) throw std::logic_error("CD SID 소진 상세 미연결");detail(text);AssertAllocation("0",line);
            };
            if (client) diagnose("alCLIENT: Send in \\NetStorm\\squids.txt with this bug!!",0x118);
            const auto boss=allocation_.boss;if (!boss) throw std::logic_error("CD SID 소진 복구의 현재 boss 미연결");
            if (!*boss) diagnose("!Server: Send in \\NetStorm\\squids.txt with this bug!",0x12c);
            else {
                std::uint32_t candidate=2; //CD는 첫 live 후보를 찾은 뒤 번호를 증가시키지 않는다.
                while (candidate<capacity_ && (raw(candidate)[kState]&kFree)) ++candidate; //최초 free 탐색만 현재 물리 배열을 전진한다.
                std::uint32_t remaining=50; //원문 ESI의 같은 후보 가상 삭제 호출 상한이다.
                while (candidate>=1 && candidate<capacity_ && remaining) { //현재 TYPE/Bridge를 다시 읽되 후보는 고정한다.
                    const auto bridge=allocation_.bridgeType;if (!bridge) throw std::logic_error("CD SID 소진 복구의 현재 Bridge TYPE 미연결");
                    if (raw(candidate)[kType]!=*bridge) throw std::logic_error("CD SID 회수 원문의 고정 후보 무효과 무한 반복"); //004aaf45..82의 동일 상태 반복에는 호스트를 멈추는 대신 명시 오류를 낸다.
                    --remaining;const auto reclaim=allocation_.reclaim;if (!reclaim) throw std::logic_error("CD SID 소진 복구의 현재 가상 삭제 미연결");reclaim(Sid{static_cast<std::uint16_t>(candidate)});
                }
                const auto tell=allocation_.tell;if (!tell) throw std::logic_error("CD SID 회수 뒤 Tell 미연결");tell("BridgesKilled");
            }
            if (!client && !IsServer()) AssertAllocation("server",0x130);
        }
        sid=ReadNext(kHeads[list]);
        if (!(raw(sid)[kState]&kFree)) AssertAllocation("p->isFree()",0x140);
        if (sid==tails_[list]) AssertAllocation("p != *freeTail",0x141);
        const auto next=ReadNext(static_cast<std::size_t>(sid)*layout_.stride);
        if (!next) AssertAllocation("n != getSid()",0x10c,"\\ns\\o\\squid.h");
        WriteNext(kHeads[list],next);
    }
    auto selected=raw(sid);std::fill(selected.begin(),selected.end(),std::uint8_t{});selected[kState]=kVoid;
    if (flags&1U) ++predictableCursor_;
    --freeCount_;return Sid{static_cast<std::uint16_t>(sid)};
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
    if (!IsServer() && sid.value>=layout_.serverFirst) {
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
bool SidPool::IsServer() const { return sharedServer_ ? *sharedServer_ : server_; }
// 이미 확정된 raw 슬롯 크기에서 판본을 반환한다.
OriginalEdition SidPool::Edition() const { return layout_.stride==50 ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072; }
// 확보한 raw 슬롯 수를 반환한다.
std::uint32_t SidPool::Capacity() const { return capacity_; }
const std::uint32_t& SidPool::CapacityValue() const { return capacity_; } // Pool 소유권보다 오래 보관하지 않는다.
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
