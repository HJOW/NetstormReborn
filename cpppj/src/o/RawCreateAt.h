// Damageable.cpp의 CreateAt 생성 부모를 기존 위치 parser/실제 월드 경계에 조립한다.
#pragma once
#include "o/RawDestroyAt.h"
#include "o/RiftType.h"

namespace netstorm::o { //일반 건설/임의 배치나 UI 선택 성공으로 대체하지 않는다.
//실제 부모가 도달한 현재 권한/TYPE/생성/가상 효과만 공급한다.
struct CreateAtHooks {
    std::function<bool()> boss; //00540bc4의 현재 권한이다. 진단 복귀 뒤 다시 읽는다.
    std::function<void(std::uint32_t)> assertion; //Damageable.cpp803의 원문 진단이다.
    std::function<DestroyAtLocation(std::string_view)> parse; //같은0044c540의 다섯 정수 출력이다. owner를 float로 해석하지 않는다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> create; //같은004af530(type,0)의 실제 현재 SID다.
    std::function<std::uint32_t(std::int32_t,std::int32_t)> surface; //owner0일 때만 원래 정수좌표의0040da00을 조회한다.
    std::function<std::span<const std::uint8_t>(std::uint32_t)> slot; //현재 지면+22의 owner BYTE를 읽는다. free/dead를 새로 거르지 않는다.
    std::function<void(std::uint32_t,std::uint32_t)> owner; //같은 생성 객체의 실제 가상+74다. 전체DWORD 인자를 전달한다.
    std::function<bool(std::uint32_t)> hasHitPoints; //004adc60이며 owner 효과 뒤 현재 생성 객체를 조회한다.
    std::function<std::int32_t(std::uint32_t)> maximumHitPoints; //004adcb0의 현재 최대 HP다.
    std::function<void(std::uint32_t,std::int32_t)> hitPoints; //004adca0의 공통 raw setter다. 사제 가상 HP 전환을 호출하지 않는다.
    std::function<bool(std::uint32_t)> directional; //00425830의 현재 네 TYPE 전역과 원래 parser type을 비교한다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> frameField; //현재 생성 객체 TYPE의 offset=15c+signed(radius/2)*4 DWORD다.
    std::function<void(std::uint32_t,std::uint32_t)> frame; //공간 등록 전의 raw+24 직접쓰기다. 가상 SetFrame/재등록을 더하지 않는다.
    std::function<void(std::uint32_t,float,float,std::uint32_t)> pop; //실제 가상+90에 float32(x),float32(y),flags0을 전달한다.
};
//0044c660 전체 부모다. 미도달 하위는 요구하지 않고 앞선 생성/소유자/HP 효과를 롤백하지 않는다.
void CreateObjectAt(std::string_view source,const CreateAtHooks& hooks);
//현재 TYPE의 복원된12c/first[16]/count[16] 물리 DWORD를 읽는다. 이웃 미복원 필드/포인터를 frame0으로 합성하지 않는다.
std::uint32_t CreateAtFrameField(const RiftTypeFrames& frames,std::uint32_t offset);
}
