// 원본의 두 local 난수·현재 option/type 재조회·부동소수 예산 절삭·부분 효과를 보존한다.
#include "o/RawWorldGeyser.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o { // 세계 생성 순서만 소유하며 객체/Player/옵션은 외부의 같은 물리다.
namespace { // 실제 정수 변환과 선택된 하위 호출의 내부 도구다.
constexpr std::uint32_t kPredictableSeed=0xfed,kSpecialLimit=10; //원본 세계 시드와 Monument 최대 요청 수다.
constexpr std::int32_t kDecorationsPerOption=50; //현재 옵션9의 반복 배율이다.
// 실제 도달한 하위가 없으면 앞선 효과를 유지한 채 실패한다.
template<class Function> const Function& Need(const Function& function) {
    if (!function) throw std::logic_error("세계 간헐천 생성의 실제 하위 미연결");return function;
}
// 원본 DWORD의 감긴 signed 비트 해석이다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// x87 CRT의 signed64 절삭 하위 DWORD를 반환한다.
std::int32_t Truncate(double value) {
    if (!std::isfinite(value) || value< -9223372036854775808.0 || value>=9223372036854775808.0) return 0;
    return Signed(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
}
// 밀도와 간격은 서로 다른 옵션이며 현재 BYTE를 각각 조회한다.
std::int32_t WorldGeyserDensity(std::uint8_t selection) {
    constexpr std::array<std::int32_t,4> values{-1,0,1,2}; //원문0041ca40의 실제 스택 표다.
    if (selection>=values.size()) throw std::out_of_range("세계 간헐천 밀도의 원문 표 범위");return values[selection];
}
// 고정 판본/물리 폭만 확인하고 하위는 선택된 지점에서 요구한다.
RawWorldGeyser::RawWorldGeyser(OriginalEdition edition,WorldGeyserState state,WorldGeyserHooks hooks):state_(state),hooks_(std::move(hooks)) {
    if (edition!=OriginalEdition::Patch1078 || state.options.size()!=87) throw std::invalid_argument("세계 간헐천 부모의PATCH87BYTE 계약 오류");
}
// 전체 부모는 boss/fort 관문을 새로 추가하지 않는다. 각 실제 Populate가 현재 권한을 판정한다.
void RawWorldGeyser::Generate() const {
    Need(hooks_.log)("Make Local Geysers\n");Need(hooks_.local)();GameRandom random(kPredictableSeed);
    if (state_.battle && state_.options[9]!=0) {
        const std::array<std::uint32_t,6> types{state_.decorationTypes[0],state_.decorationTypes[0],state_.decorationTypes[0],state_.decorationTypes[1],state_.decorationTypes[2],state_.decorationTypes[3]}; //Local 뒤 여섯 선택 항목만 한 번 고정한다.
        GameRandom decoration(0);decoration.SetState(RawFortFinishOwner::TimeSeed(Need(hooks_.wallSeconds)()));std::uint32_t special{};
        for (std::int32_t index=0;index<state_.options[9]*kDecorationsPerOption;++index) { //배치 하위가 바꾼 현재 옵션9로 다음 반복 상한을 읽는다.
            const auto type=types[decoration.Next(6)];
            if (type==state_.decorationTypes[3] && ++special>kSpecialLimit) continue;
            const auto rectangle=Need(hooks_.whole)();Need(hooks_.decorate)(decoration,rectangle,0,1,type,1);
        }
    }
    if (Need(hooks_.configInt)("predictableGeysers",1)==0) random.SetState(RawFortFinishOwner::TimeSeed(Need(hooks_.wallSeconds)()));
    const auto base=Need(hooks_.budget)(),density=Need(hooks_.density)();const double original=base;
    const double densityAddition=density==1 ? 0.1 : density==2 ? 0.2 : 0.0; //ROM00501960/00505de0/00500460이다.
    const auto adjusted=Truncate((densityAddition+1.0)*original);const auto spacing=Need(hooks_.spacing)();
    const double sizeAddition=spacing==3 ? -0.2 : spacing==4 ? -0.1 : spacing==6 ? 0.1 : 0.0; //ROM005066b8/005066d0/00501960/00500460이다.
    const auto corner=Truncate(sizeAddition*original+adjusted)/2/4,edge=Signed(static_cast<std::uint32_t>(corner)+1U),center=edge/2;
    if (state_.options[18]==2) {
        for (std::int32_t cell=1;cell<=9;++cell) { //원본은 분할 시작을 선택한 뒤 옵션18이 바뀌어도 아홉 요청을 끝낸다.
            const bool middle=cell==5,cornerCell=cell==1 || cell==3 || cell==7 || cell==9;
            const auto rectangle=Need(hooks_.partition)(cell,3);
            Need(hooks_.populate)(middle ? center : cornerCell ? corner : edge,random,rectangle,{1,middle ? 0U : 1U,1});
        }
    }
    if (state_.options[18]!=1) return;
    static_cast<void>(Need(hooks_.density)());std::int32_t quantity{}; //원문의 결과 미사용 getter도 한 번 호출한다.
    if (Need(hooks_.density)()==-1) quantity=Need(hooks_.budget)()/2;
    else if (Need(hooks_.density)()==0) quantity=Need(hooks_.budget)();
    else if (Need(hooks_.density)()==1) quantity=Signed(static_cast<std::uint32_t>(Need(hooks_.budget)()/2)*3U);
    else if (Need(hooks_.density)()==2) quantity=Signed(static_cast<std::uint32_t>(Need(hooks_.budget)())<<1);
    else quantity=Need(hooks_.budget)();
    const auto half=quantity/2;auto rectangle=Need(hooks_.whole)();
    const auto allowVoid=Need(hooks_.demo)(),allowLand=Need(hooks_.demo)()==0 ? 1U : 0U; //두 getter는 각각 허공/지형 인수다. 첫 결과를 상수1로 대신하지 않는다.
    Need(hooks_.populate)(half,random,rectangle,{1,allowLand,allowVoid});
    rectangle=Need(hooks_.whole)();const auto remaining=Signed(static_cast<std::uint32_t>(Need(hooks_.budget)())-static_cast<std::uint32_t>(half));
    Need(hooks_.populate)(remaining,random,rectangle,{1,0,1});
}
}
