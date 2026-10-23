// 원본004bd450/004bcb60/004b38d0의실제Subscriber ID·이름·등록마감순서를복원한다.
#pragma once
#include "o/RawMemStream.h"
#include <string_view>

namespace netstorm::o {
// 정상파일꼬리와등록helper가동일하게읽고쓰는실제현재DWORD다.
struct FortSubscriberLoadGlobals {
    std::uint32_t& subscriber; //00540c74이며파일ID를직접쓴뒤등록helper가현재값을다시읽는다.
};
// 원문진단은복귀하면현재상태로계속한다.물리접근실패와별개다.
enum class FortSubscriberLoadAssertion : std::uint32_t {
    MissingManager=0x1d8, //고정this의첫DWORD0이며진단뒤현재manager를다시읽는다.
    PositionPastEnd=0x2e //ID/길이/복사각현재pos쓰기뒤signed pos>end인memstream진단이다.
};
// 원문parent/string/등록판정외실제저장소와Config/새ID생성만분리한다.
struct FortSubscriberLoadHooks {
    std::function<std::uint32_t(std::int32_t)> manager; //같은고정Template의현재manager를각원문조회위치에서읽는다.
    std::function<std::uint32_t(std::uint32_t,std::string_view,std::int32_t)> select; //0044ceb0(currentManager,"Subscriber",0)의반환stream토큰이다.
    std::function<void(std::uint32_t,const std::function<void(std::span<std::uint8_t>,const MemStreamHooks&)>&)> access; //같은선택노드fields/실제버퍼를ID와문자열전체동안동기빌린다.
    std::function<void(FortSubscriberLoadAssertion)> assertion; //진단후바뀐현재data/pos/end/ID를그다음원문위치에서읽는다.
    std::function<bool(std::string_view,std::uint32_t&)> readRegistered; //00441270("registeredSubId",현재ID참조)의값변경여부다.GetInt/존재여부로대체하지않는다.
    std::function<std::uint32_t()> generate; //004bc950의실제새ID생성이다.선택시에만요구하며반환후현재ID에쓴다.
    std::function<void(std::string_view)> configure; //00441580에원문오탈자registerdSubId와unsigned십진수를포함한설정식을넘긴다.
};
//004bd450:현재manager진단/Select→실제ID쓰기→pos진단→signed길이/직접복사/NUL→등록마감이다.
//handle은진입때전달받은실제출력버퍼다.논리end나임의256자제한으로선행복사/부분효과를생략하지않는다.
void LoadFortSubscriber(std::int32_t slot,std::span<std::uint8_t> handle,const FortSubscriberLoadGlobals& globals,const FortSubscriberLoadHooks& hooks);
//004b38d0:현재ID==-1때만ReadInt→변경없으면생성/ID쓰기/설정식순서를실행한다.
void FinishFortSubscriberRegistration(const FortSubscriberLoadGlobals& globals,const FortSubscriberLoadHooks& hooks);
}
