// 원문 메타 update88의 signed DWORD 감김과 next 선저장·현재 Form 판정을 유지한다.
#include "client/NativeMetaSpriteRefresh.h"
#include <bit>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client { // 일반 표시 억제/선택표식/그림자를 이 파생에 추가하지 않는다.
namespace { // 원문 비정렬 필드와 하위DWORD 연산만 공통 처리한다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("메타 섬 표시 갱신의 선택된 실제 장치 미연결");return value; } //도달하지 않은 장치는 요구하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t at,std::size_t width=4) { if (at>raw.size() || width>raw.size()-at) throw std::out_of_range("메타 섬 표시 갱신의 현재 필드 물리 범위");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(raw[at+byte])<<(8*byte);return value; } //필요한 BYTE만 낮은 자리부터 조립한다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } //원문 signed DWORD 해석이다.
std::int32_t Add(std::int32_t left,std::int32_t right) { return Signed(std::bit_cast<std::uint32_t>(left)+std::bit_cast<std::uint32_t>(right)); } //좌표 덧셈 하위DWORD를 보존한다.
std::int32_t Sub(std::int32_t left,std::int32_t right) { return Signed(std::bit_cast<std::uint32_t>(left)-std::bit_cast<std::uint32_t>(right)); } //좌표 뺄셈 하위DWORD를 보존한다.
std::int32_t Product(std::int32_t value,std::uint32_t scale) { return Signed(std::bit_cast<std::uint32_t>(value)*scale); } //IMUL의 하위DWORD만 다음 signed 나눗셈에 전달한다.
void RefreshSpriteGumps(std::span<const std::uint8_t> object,NativeMetaSpriteRefreshGlobals globals,const NativeMetaSpriteRefreshHooks& hooks,bool allowDestroy) { //두 원문88의 동일한 기본 Form finder이며 Sprite의yesIknow=1과 메타의0을 구별한다.
    if (globals.checking && !globals.metamap && (Read(object,0xb,1)&2U) && !globals.boss && !allowDestroy) Need(hooks.assertion)("inMetamapMode || !cont.isDead() || boss || yesIKnowThatIShouldntLookInsideMyselfDuringADestroy","\\Ns\\O\\Squidfinder.cpp",0x158);
    auto current=Read(object,6,2);std::unordered_set<std::uint32_t> visited;
    while (current) { //가상 효과 전에 next를 저장하고 복귀 뒤 저장한 순서에서 현재 Form TYPE를 읽는다.
        if (!visited.insert(current).second) throw std::logic_error("메타 섬 표시 갱신의 Form finder 순환");const auto raw=Need(hooks.raw)(current);const auto next=Read(raw,4,2);const auto formType=Read(raw,10,1);
        if (formType<70 && formType!=globals.excludedType && !(formType>9 && static_cast<std::int32_t>(formType)<Signed(globals.processTypeEnd))) Need(hooks.redrawGump)(Read(raw,0x12));current=next; //free/dead/별도active 필터를 추가하지 않는다.
    }
}
}
// 8c의004ad670 thunk도 같은88로 분배된다. 숨김/표시 억제 관문을 만들지 않는다.
void RefreshNativeMetaSprite(std::span<const std::uint8_t> object,NativeMetaSpriteRefreshGlobals globals,const NativeMetaSpriteRefreshHooks& hooks) {
    auto rect=Need(hooks.bounds)(object);auto fixed=((Read(object,0x1a)&0x3fffU)<<16)+Read(object,8,2);if (!fixed) fixed=0x10000U;
    const auto type=Read(object,10,1);
    if (type==globals.challenge.BattleType()) {
        const auto width=Sub(rect.right,rect.left),height=Sub(rect.bottom,rect.top),x=Add(rect.right,rect.left)/2,y=Add(rect.top,rect.bottom)/2;
        if (Signed(fixed)>0x10000) { const auto halfWidth=Product(width,fixed)/0x20000,halfHeight=Product(height,fixed)/0x20000;rect={Sub(x,halfWidth),Sub(y,halfHeight),Add(x,halfWidth),Add(y,halfHeight)}; } //Native JLE는 signed 비교이며 확대 때 중심을 유지한다.
    } else if (type==globals.challenge.ParticipantType() && Signed(fixed)>0x10000) { rect.left=Sub(rect.right,Product(Sub(rect.right,rect.left),fixed)/0x10000);rect.top=Sub(rect.bottom,Product(Sub(rect.bottom,rect.top),fixed)/0x10000); } //참가자는 오른쪽/아래 고정이며 축소는 보정하지 않는다.
    Need(hooks.dirty)(rect);
    RefreshSpriteGumps(object,globals,hooks,false);
}
//일반 Sprite는 TYPE 역할 분기 없이 큰 배율에서 중심을 유지하며 dirty 뒤 실제BYTE35를 초기화한다.
void RefreshNativeSprite(std::span<std::uint8_t> object,NativeMetaSpriteRefreshGlobals globals,const NativeMetaSpriteRefreshHooks& hooks) {
    if (object.size()!=50) throw std::invalid_argument("Sprite 표시 갱신의 현재50BYTE 계약 오류");
    auto rect=Need(hooks.bounds)(object);auto fixed=((Read(object,0x1a)&0x3fffU)<<16)+Read(object,8,2);if (!fixed) fixed=0x10000U;
    const auto x=Add(rect.right,rect.left)/2,y=Add(rect.top,rect.bottom)/2;
    if (Signed(fixed)>0x10000) { const auto halfWidth=Product(Sub(rect.right,rect.left),fixed)/0x20000,halfHeight=Product(Sub(rect.bottom,rect.top),fixed)/0x20000;rect={Sub(x,halfWidth),Sub(y,halfHeight),Add(x,halfWidth),Add(y,halfHeight)}; }
    Need(hooks.dirty)(rect);object[0x23]=0x14;RefreshSpriteGumps(object,globals,hooks,true);
}
}
