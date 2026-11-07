// 원문00478d10의 선택/부분 효과와 DWORD 감김·float32 저장 지점을 보존한다.
#include "client/NativeMetaSpriteDraw.h"
#include "client/RawCameraView.h"
#include <bit>
#include <stdexcept>

namespace netstorm::client { // TYPE 자산과 동적 Image는 실제 장치가 각각 같은 표면으로 그린다.
namespace { // 비정렬 원문 조회와 선택된 하위 호출만 공통 처리한다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("메타 섬 추가Draw의 선택된 실제 장치 미연결");return value; } //조기 반환에서 미사용 장치를 요구하지 않는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타 섬 Draw의 현재 필드 물리 범위");std::uint32_t result{};for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(bytes[at+byte])<<(8*byte);return result; } //선택한 폭만 낮은BYTE부터 읽는다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); } //원문 signed DWORD 해석이다.
std::int32_t Add(std::int32_t left,std::int32_t right) { return Signed(std::bit_cast<std::uint32_t>(left)+std::bit_cast<std::uint32_t>(right)); } //x86 덧셈 하위DWORD를 보존한다.
std::int32_t Sub(std::int32_t left,std::int32_t right) { return Signed(std::bit_cast<std::uint32_t>(left)-std::bit_cast<std::uint32_t>(right)); } //좌표 뺄셈에 signed overflow를 만들지 않는다.
std::int32_t Scale(std::span<const std::uint8_t> object) { const auto fixed=((Read(object,0x1a)&0x3fffU)<<16)+Read(object,8,2);return Signed(fixed ? fixed:0x10000U); } //상위14비트와 현재분수WORD를 합친 뒤0만1배로 대체한다.
std::string_view Description(std::span<const std::uint8_t> battle) { const auto tail=battle.subspan(0xf9);std::size_t length{};while (length<tail.size() && tail[length]) ++length;if (length==tail.size()) throw std::out_of_range("전투섬 설명의 실제 NUL 물리 범위");return {reinterpret_cast<const char*>(tail.data()),length}; } //Native C문자열을 같은 기록에서 빌린다.
std::array<std::uint32_t,8> Context(std::uint32_t font,std::uint32_t color,std::uint32_t shadow) { return {2,font,color,0,0,shadow,1,1}; } //004248a0(font,color,0,2,0,textColor)의 실제32BYTE다.
void Center(std::string_view text,ScreenPoint point,const std::array<std::uint32_t,8>& context,std::int32_t offset,const NativeMetaSpriteDrawHooks& hooks) { const auto size=Need(hooks.measure)(text,context);point.x=Sub(point.x,size[0]/2);point.y=Sub(Sub(point.y,size[1]/2),offset);Need(hooks.text)(text,point,context); } //측정 후 signed 나눗셈0방향과 DWORD 뺄셈을 유지한다.
}
// 숨김 비트는 다른 장치/CS를 조회하기 전에 검사한다. QA는 kill일 때 Rig Form으로 해석한다.
void DrawNativeMetaSprite(std::span<const std::uint8_t> object,ScreenPoint point,NativeMetaSpriteDrawGlobals globals,const NativeMetaSpriteDrawHooks& hooks) {
    const auto entryFlags=Read(object,0x1a);if (entryFlags&0x4000U) return;
    if (Read(object,10,1)!=globals.challenge.BattleType()) {
        std::span<const std::uint8_t> image;
        if (entryFlags&0x8000U) { const auto qa=Read(object,0xc,2);if (!qa) throw std::out_of_range("메타 참가자 kill Image의 Native NULL+78 물리 접근");image=Need(hooks.rigImage)(qa); }
        else { const auto qa=Read(object,0xc,2);if (qa>=40) Need(hooks.assertion)("q >= 0 && q < CL_NUM_CHAL_PLAYERS",".\\Metadisplay.cpp",0x2fd);image=globals.challenge.Participant(Read(object,0xc,2)).subspan(0xbc,20); }
        if (!Read(image,4)) Need(hooks.assertion)("imgPtr->shape != NULL",".\\Metadisplay.cpp",0x305);
        const auto fixed=Scale(object),product=Signed(std::bit_cast<std::uint32_t>(fixed)*11U);Need(hooks.image)(image,point,fixed,product/16);return;
    }
    std::span<const std::uint8_t> battle;std::int32_t maximum=-1,minimum=256;bool hasDescription{};
    if (entryFlags&0x8000U) { const auto qa=Read(object,0xc,2);if (qa) static_cast<void>(Need(hooks.rigImage)(qa)); } //전투 kill도 원문의 비영 QA 변환을 수행하지만 Image는 사용하지 않는다.
    else { const auto qa=Read(object,0xc,2);if (qa>=10) Need(hooks.assertion)("q >= 0 && q < CL_NUM_CHAL_BATTLES",".\\Metadisplay.cpp",0x27a);battle=globals.challenge.Battle(Read(object,0xc,2));minimum=Signed(Read(battle,0x1c));maximum=Signed(Read(battle,0x18));hasDescription=Read(battle,0xf9,1)!=0; }
    const auto fixed=Scale(object);const auto frame=Read(object,0x24);const auto type=Need(hooks.type)();Need(hooks.shape)(type,frame,point,fixed,fixed);
    const auto extendedScale=static_cast<double>(fixed)*0x1p-16;const auto storedScale=static_cast<float>(extendedScale);const bool outside=!((extendedScale>=0.9 && storedScale<=1.1) || storedScale==0.0F); //첫 비교는 레지스터 값, 둘째/0 비교와 포구 좌표는 FST32 값을 쓴다.
    if (maximum>=0 && minimum<256 && !outside) {
        if (maximum<=78 && minimum>=78) { if (hasDescription) Center(Description(battle),point,Context(6,globals.normalColor,globals.textColor),0,hooks); }
        else {
            const auto context=Context(0,globals.ownColor,globals.textColor);std::string_view key="Incompatible";
            switch (maximum) { case 37:key="Netstorm 37";break;case 62:key="Netstorm 62 Patch";break;case 63:key="Netstorm 63 Alpha Patch";break;case 64:key="Netstorm 64";break;case 65:key="Dark Skies Expansion 65";break;default:break; } //원문 문자열 선택은 minimum이 아닌 maximum을 사용한다.
            const auto version=Need(hooks.translate)(key);if (hasDescription) Center(Description(battle),point,Context(6,globals.normalColor,globals.textColor),0,hooks);Center(version,point,context,15,hooks);
        }
    }
    if (battle.empty()) return;const auto overlayType=Need(hooks.type)();
    for (std::size_t seat=0;seat<kMetaBattleSeatOrder.size();++seat) { //현재8DWORD 중 비영만 원문의 고정표 순서로 표시하며 참가자active를 검사하지 않는다.
        if (!Read(battle,0x38+seat*4)) continue;const auto offset=kMetaBattleSeatOrder[seat];const auto source=Add(Need(hooks.firstFrame)(overlayType,'C'),offset);const auto muzzle=Need(hooks.muzzle)(overlayType,source);
        const auto y=CameraFtolLow(static_cast<double>(muzzle[1])*storedScale+point.y),x=CameraFtolLow(static_cast<double>(muzzle[0])*storedScale+point.x);const auto target=Add(Need(hooks.firstFrame)(overlayType,'B'),offset);Need(hooks.shape)(overlayType,std::bit_cast<std::uint32_t>(target),{x,y},fixed,fixed);
    }
}
}
