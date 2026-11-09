// 현재 소유권 효과 뒤 대상의 종류/좌표/가상 표와 전역을 원문의 위치에서 다시 읽는다.
#include "o/RawBombTreason.h"
#include "o/RawPathAnimation.h"
#include "o/TerrainBuilder.h"
#include <bit>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace netstorm::o { //독립 세계/소유자/검색 결과 사본을 만들지 않는다.
namespace { //선택 하위 진단과 CRT 절삭의 낮은 DWORD를 이 몸체에 한정한다.
template<class F> F TreasonNeed(const F& function) { if (!function) throw std::logic_error("트리즌 실제 하위 미연결");return function; } //실행 중 함수 사본을 정상/예외 복귀까지 유지한다.
std::int32_t TreasonCell(double value) { //원문_ftol의 signed64 절삭/하위DWORD와 범위 밖0을 보존한다.
    constexpr double limit=9223372036854775808.0; //signed64로 변환할 수 있는 배정도 경계다.
    if (!(value>=-limit && value<limit)) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
}
RawBombTreason::RawBombTreason(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,BombTreasonHooks hooks)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),hooks_(std::move(hooks)) { //현재 풀/해시/행의 주소만 빌린다.
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=188) throw std::invalid_argument("트리즌 같은PATCH/188TYPE 계약 오류");
}
std::uint32_t RawBombTreason::Read(Sid sid,std::size_t offset,std::size_t width) const { //현재 물리 슬롯의 지정 폭을 낮은 BYTE부터 조합한다.
    const auto raw=pool_.Slot(sid);std::uint32_t value{};
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(raw[offset+byte])<<(8*byte); //인접 필드/상태를 추가로 읽지 않는다.
    return value;
}
const RiftTypeRecord& RawBombTreason::Type(Sid sid) const { //효과마다 다시 읽은 TYPE의 실제 행을 선택한다.
    const auto type=Read(sid,10,1);if (type>=types_.size()) throw std::out_of_range("트리즌 현재TYPE 범위 오류");return types_[type];
}
bool RawBombTreason::Is(Sid sid,BombTreasonRole role) const { const auto type=Read(sid,10,1);return type==TreasonNeed(hooks_.role)(role); } //역할 DWORD를 미리 BYTE로 줄이거나 캐시하지 않는다.
int RawBombTreason::Side(Sid sid) const { const auto type=Read(sid,10,1);if (type>=frames_.size()) throw std::out_of_range("트리즌 현재 방향TYPE 범위 오류");return RawPathAnimation::Side(frames_[type],std::bit_cast<std::int32_t>(Read(sid,36))); } //같은 실제004ad680을 사용한다.
std::array<float,2> RawBombTreason::Position(Sid sid) const { return {std::bit_cast<float>(Read(sid,14)),std::bit_cast<float>(Read(sid,18))}; } //대상 원래 좌표를 중심으로 변환하지 않는다.
bool RawBombTreason::MayTarget(Sid parent,Sid target) const { //원문 첫 owner 관문 뒤에만 종류/방향/genus를 조회한다.
    if (Read(target,34,1)==Read(parent,34,1)) return false;
    if (!(Is(target,BombTreasonRole::NoIsland) && Side(target)=='H') && !Is(target,BombTreasonRole::EmptyGeyser) && !(Type(target).flags2&0x10170004U)) return false;
    return !(Type(target).flags2&0x200000U);
}
void RawBombTreason::Set(Sid parent,Sid target) const { //소유자 호출이 바꾼 가상 표를 표시 호출 직전에 다시 읽는다.
    const auto table=Read(target,0);const auto owner=static_cast<std::uint8_t>(Read(parent,34,1));TreasonNeed(hooks_.owner)(target,table,owner);
    TreasonNeed(hooks_.refresh)(target,Read(target,0));
}
void RawBombTreason::At(Sid parent,float x,float y,bool connectors) const { //일반 지점 finder는 flags0과 보정 없는 절삭을 사용한다.
    const auto cx=TreasonCell(x),cy=TreasonCell(y);RawSquidFinder finder(pool_,hash_,types_);
    for (auto candidate=finder.Begin({cx,cy,cx,cy},0);candidate.value;candidate=finder.Next()) { //외부 효과 전 저장한 next와 다음 후보의 현재 자료를 그대로 사용한다.
        if (connectors ? Is(candidate,BombTreasonRole::Connector):(Type(candidate).flags2&0x10040000U)!=0) Set(parent,candidate);
    }
}
void RawBombTreason::Apply(Sid parent,Sid target) const { //효과 진입은 자격을 반복하거나 현재 권한을 추가 검사하지 않는다.
    if (target.value==Read(parent,12,2)) return;
    const auto position=Position(target);const auto oldOwner=Read(target,34,1),owner=Read(parent,34,1);std::ostringstream message;
    message<<"Set "<<Type(target).name.c_str()<<" at ("<<TreasonCell(position[0])<<","<<TreasonCell(position[1])<<") from "<<oldOwner<<" to "<<owner<<'\n';TreasonNeed(hooks_.trace)(4,message.str());
    if (Is(target,BombTreasonRole::Fence)) TreasonNeed(hooks_.fence)(target,false);
    Set(parent,target);
    if (Is(target,BombTreasonRole::Fence)) TreasonNeed(hooks_.fence)(target,true);
    if (Type(target).flags2&0x10040000U) {
        const auto point=Position(target);const auto beneath=TreasonNeed(hooks_.find)(point[0],point[1],TreasonNeed(hooks_.role)(BombTreasonRole::NoIsland));
        if (beneath.value) {
            TreasonNeed(hooks_.trace)(4,"A - Also setting noIsland under it.");
            if (Side(beneath)!='H') TreasonNeed(hooks_.assertion)("anIsle.getOrientation() == 'H'","\\Ns\\O\\Bomb.cpp",0x5e9);
            const auto at=Position(beneath);TreasonNeed(hooks_.support)(at[0],at[1],static_cast<std::uint8_t>(Read(parent,34,1)));
        }
    }
    if (Is(target,BombTreasonRole::NoIsland) || Is(target,BombTreasonRole::EmptyGeyser) || (Type(target).flags2&0x10000000U)) {
        if (Is(target,BombTreasonRole::NoIsland) && Side(target)!='H') TreasonNeed(hooks_.assertion)("s.getOrientation() == 'H'","\\Ns\\O\\Bomb.cpp",0x5f0);
        TreasonNeed(hooks_.trace)(4,"B - Also setting noIsland under it.");
        const auto point=Position(target);TreasonNeed(hooks_.support)(point[0],point[1],static_cast<std::uint8_t>(Read(parent,34,1)));
        const auto current=Position(target);At(parent,current[0],current[1],false);
    }
    if (Is(target,BombTreasonRole::Bridge)) {
        for (std::size_t direction=0;direction<8;direction+=2) { //원본 ROM의 북/동/남/서 순서이며 매 방향마다 현재 대상 좌표를 다시 읽는다.
            const auto point=Position(target);const auto [dx,dy]=kNeighborCells[direction];
            At(parent,static_cast<float>(TreasonCell(static_cast<double>(point[0])+dx)),static_cast<float>(TreasonCell(static_cast<double>(point[1])+dy)),true);
        }
    }
    TreasonNeed(hooks_.flare)(target,1,3,TreasonNeed(hooks_.role)(BombTreasonRole::Flare),3);
}
const SidPool& RawBombTreason::Pool() const { return pool_; } //같은 현재 Runtime 자료의 대여 여부를 확인한다.
}
