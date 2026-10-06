// 원본 함수의 바이트 쓰기를 보존하며 미복원 파생/공간 효과는 변경 전에 거부한다.
#include "o/SquidFactory.h"
#include <stdexcept>

namespace netstorm::o {
namespace {
// 원본의 공통 raw 필드 위치와 상태 비트다.
constexpr std::size_t kType=10,kState=11,kHp=26;
constexpr std::uint8_t kFree=1,kVoid=4,kContained=8;
// 원본 타입 번호와 dais의 HP 필요성 플래그다. 두 판본의 PE 초기값/로딩 순서로 대조했다.
constexpr std::uint32_t kManaType=158,kFakeSurface=162,kDaisFlag=0x400000;
// 호스트 포인터로 사용하지 않는 실제 32비트 base vtable 기록값이다.
constexpr std::uint32_t kPatchVtable=0x501dd0,kCdVtable=0x5058d8;
#include "TypeConstructors.inc"
// 원본 포인터/HP를 little endian raw 필드로 쓴다. 정렬되지 않은 +26 쓰기도 안전하다.
void Put(std::span<std::uint8_t> bytes,std::size_t offset,std::uint32_t value,std::size_t width) {
    // CD HP는 2바이트이고 패치 HP는 4바이트다.
    for (std::size_t i=0;i<width;++i) bytes[offset+i]=static_cast<std::uint8_t>(value>>(i*8));
}
// 이전 vtable이 실제 base 객체인지 raw 기록값만 읽는다.
std::uint32_t Vtable(std::span<const std::uint8_t> bytes) {
    std::uint32_t value=0;
    // 네 바이트를 호스트 엔디언과 무관하게 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(bytes[i])<<(i*8);
    return value;
}
}
// 타입 번호 밖 조회를 거부하고 원본 주소 메타데이터를 반환한다.
std::uint32_t TypeConstructorAddress(OriginalEdition edition,std::size_t number) {
    const auto values=edition==OriginalEdition::Patch1078 ? std::span<const std::uint32_t>(kPatchConstructors) :
        std::span<const std::uint32_t>(kCdConstructors);
    if (number>=values.size()) throw std::out_of_range("생성자 타입 번호 오류");
    return values[number];
}
// pool 판본에 맞는 타입 배열을 소유한다. 실제 타입 로더 또는 합성 검증 입력을 사용할 수 있다.
SquidFactory::SquidFactory(SidPool& pool,std::span<const RiftTypeRecord> types,bool weakenedMana)
    :pool_(pool),types_(types.begin(),types.end()),weakenedMana_(weakenedMana) {
    const auto count=pool_.Edition()==OriginalEdition::Patch1078 ? kPatchConstructors.size() : kCdConstructors.size();
    if (types_.size()!=count) throw std::invalid_argument("SID 판본과 타입 수가 다릅니다");
}
// 미복원 생성자 호출이나 form에 base 가상 메서드를 적용하는 것을 막는다.
const RiftTypeRecord& SquidFactory::Type(std::uint32_t number,bool requireBase) const {
    if (number<kFirstAssetTypeNumber || number>=types_.size()) throw std::out_of_range("base Squid 타입 번호 오류");
    const auto& type=types_[number];
    if (requireBase && type.constructorAddress) throw std::logic_error("파생 Squid 생성자는 아직 복원하지 않았습니다");
    return type;
}
// Take로 받은 예측 머리도 원본 범위 안이다. 일반 Allocate의 예약 검사와 분리한다.
std::span<std::uint8_t> SquidFactory::LiveBytes(Sid sid) {
    const auto offset=pool_.Offset(sid);
    auto bytes=std::span(pool_.bytes_).subspan(offset,pool_.layout_.stride);
    if (sid.value<5 || (bytes[kState]&(kFree|kContained))) throw std::logic_error("base 초기화의 SID 상태 오류");
    Type(bytes[kType],false);
    return bytes;
}
// 판본별 실제 base vtable의 기록값이다.
std::uint32_t SquidFactory::BaseVtable() const { return pool_.Edition()==OriginalEdition::Patch1078 ? kPatchVtable : kCdVtable; }
// 기존 SID 할당→base vtable→type→base postCreate를 원본 순서로 적용한다.
Sid SquidFactory::Create(std::uint32_t type,std::uint32_t flags) {
    Type(type,true);
    if (pool_.Edition()==OriginalEdition::Patch1078 && type==kFakeSurface)
        throw std::logic_error("패치판 fakeThreeByThreeSurface는 생성할 수 없습니다");
    const auto sid=pool_.Allocate(flags);
    auto bytes=pool_.AllocatedBytes(sid);
    Put(bytes,0,BaseVtable(),4);
    bytes[kType]=static_cast<std::uint8_t>(type);
    PostCreate(sid);
    return sid;
}
// free/dead만 지우고 payload와 풀 카운터는 유지한다. 공간 해제 필요 여부를 먼저 검사한다.
Sid SquidFactory::Take(std::uint32_t type,Sid sid) {
    Type(type,true);
    if (sid.value<pool_.layout_.serverFirst) throw std::out_of_range("Take는 서버 영역 SID만 받습니다");
    const auto offset=pool_.Offset(sid);
    auto bytes=std::span(pool_.bytes_).subspan(offset,pool_.layout_.stride);
    if (bytes[kState]&kContained) throw std::logic_error("contained 객체의 base Take는 지원하지 않습니다");
    if (!(bytes[kState]&kFree)) {
        if (bytes[kType]!=type) throw std::logic_error("Take 타입이 기존 객체와 다릅니다");
        if (!(bytes[kState]&kVoid) || Vtable(bytes)!=BaseVtable())
            throw std::logic_error("Take의 파생/공간 해제 효과는 아직 복원하지 않았습니다");
        // 실제 base Unpop은 이미 void이면 쓰기 없이 반환한다.
    }
    bytes[kState]=static_cast<std::uint8_t>((bytes[kState]&0xfc)|kVoid);
    Put(bytes,0,BaseVtable(),4);
    bytes[kType]=static_cast<std::uint8_t>(type);
    PostTake(sid);
    return sid;
}
// base 서버 초기화의 owner와 HP만 갱신하고 나머지 payload는 보존한다.
void SquidFactory::PostCreate(Sid sid) {
    auto bytes=LiveBytes(sid);
    const auto& type=Type(bytes[kType],false);
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    bytes[patch ? 35 : 33]=static_cast<std::uint8_t>(type.zOrder);
    if (!pool_.IsServer()) return;
    bytes[patch ? 34 : 32]=0;
    const bool needsHp=(type.flags2&kDaisFlag) ? (bytes[patch ? 40 : 35]&1)!=0 : (type.flags1&TypeFlag1::kHasHitPoints)!=0;
    if (needsHp) {
        const auto hp=weakenedMana_ && bytes[kType]==kManaType ? type.maxHitPoints/4 : type.maxHitPoints;
        Put(bytes,kHp,static_cast<std::uint32_t>(hp),patch ? 4 : 2);
    }
}
// Take는 네트워크에서 받은 owner/HP를 초기값으로 덮지 않는다.
void SquidFactory::PostTake(Sid sid) {
    auto bytes=LiveBytes(sid);
    bytes[pool_.Edition()==OriginalEdition::Patch1078 ? 35 : 33]=static_cast<std::uint8_t>(Type(bytes[kType],false).zOrder);
}
}
