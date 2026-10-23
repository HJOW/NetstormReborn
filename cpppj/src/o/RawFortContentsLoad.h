// 원본004bd130의원시stream/현재변환표·TYPE/즉시ContainedCreate와중첩순서를복원한다.
#pragma once
#include "o/RawMemStream.h"

namespace netstorm::o {
// 같은Client현재변환표와checking조건만빌리며별도parsed타입표를만들지않는다.
struct FortContentsLoadGlobals {
    std::uint32_t& conversion; //005c89bc의현재255BYTE할당토큰이다.변환진단복귀뒤다시읽는다.
    std::function<std::uint32_t()> checking; //005e4794의현재DWORD순수조회다.relaxed0/현재표비영일때만필요하다.
};
// 각실제TYPEgetter뒤읽는필드다.첫조회flags1과두번째contentListFlags를같은사본으로합치지않는다.
struct FortContentsLoadType {
    std::uint32_t flags1; //TYPE+e8이며QA/QB/container조건을그때캡처한다.
    std::uint32_t contentListFlags; //TYPE+100이며저장BYTE/부모fallback을그때결정한다.
};
// 원본이진단후계속하는두위치다.반환0과물리거부/명시중단은구별한다.
enum class FortContentsLoadAssertion : std::uint32_t {
    MissingTypeConversion=0x79, //checking/relaxed0이며현재변환BYTE가0인Template.cpp진단이다.
    WordPastEnd=0x2e //QB WORD를먼저읽고pos+=2뒤signed pos>end인memstream진단이다.
};
// 큰하위몸체와실제물리저장소만분리한다.직접읽기/현재타입/flags순회는본문이소유한다.
struct FortContentsLoadHooks {
    std::function<void(std::uint32_t,const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>&)> access; //같은반환stream의실제16BYTE/버퍼를전체재귀동안동기빌린다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> conversionBuffer; //현재비영토큰의실제255BYTE를순수조회한다.0/만료/물리부족은성공0으로바꾸지않는다.
    std::function<FortContentsLoadType(std::uint32_t)> type; //0049a840(kind)의현재두필드다.한항목에서도원문위치에서두번호출한다.
    std::function<std::uint32_t(std::uint32_t)> parentListFlags; //004ac550(진입부모의raw주소)+fc의그때현재그릇flags다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> create; //실제004afa70(parent,type,quantity,listFlags,attachFlags,0)의반환SID다.생략/parsed목록으로대체하지않는다.
    std::function<void(FortContentsLoadAssertion)> assertion; //진단복귀뒤현재전역/cursor/TYPE를원문위치에서재조회한다.
};
//004bd130:rawcount/현재conversion/type/Q/listFlags→즉시Create→같은stream재귀다.본문은loadingDepth를쓰지않는다.
//false는빈data/end·변환0·중첩false의원문반환이며장치누락/물리범위는앞선효과를유지한예외다.
bool LoadFortContents(std::uint32_t parent,std::uint32_t stream,std::uint32_t flags,std::uint32_t relaxedConversion,const FortContentsLoadGlobals& globals,const FortContentsLoadHooks& hooks);
}
