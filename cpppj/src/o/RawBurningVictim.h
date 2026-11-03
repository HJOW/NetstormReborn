//연소 캡처/연소점/프레임의 같은 현재 SID 재조회와 선택 진단 앞단이다.
#pragma once
#include "o/RawCheckingFlag.h"
#include "o/SidPool.h"
#include <functional>
#include <optional>
#include <string_view>

namespace netstorm::o { //세 부모의 이후 캡처/HP 계산/입자 이동 몸체는 호출자가 이어서 실행한다.
enum class BurningVictimEntry { Capture,Sources,Frame }; //423d50·424110·424530의 진단 줄/조건을 선택한다.
struct BurningVictimHooks { //도달한 실제 조회/진단만 요구한다.
    std::function<bool(Sid,bool)> robust; //같은 현재 SID를 live1로 진단하며 checking 선택 뒤에도 다시 호출한다.
    std::function<bool(Sid)> vtable; //캡처 부모만 선택한 슬롯의 가상 표 비영 여부를 읽는다.
    std::function<bool(Sid)> hasHitPoints; //두 번째 robust 통과 뒤 고정한 같은 슬롯을 읽는다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; //원문 조건/파일/줄이며 정상 복귀 뒤 기존 앞단을 계속한다.
};
std::optional<Sid> RunBurningVictimParent(const Sid& current,BurningVictimEntry entry,RawCheckingFlag checking,const BurningVictimHooks& hooks); //빈 반환은 두 번째 실제 robust0이며 예외/진단 중단과 구별한다.
}
