// 같은 객체/SID의 비석·주문 전환을 실제 공간/타입 변경 하위와 조립한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::o { // 원본 객체의 가상 Input과 타입 변경을 GUI 수명에서 분리한다.
class SquidFactory; // 동일 풀의 실제 타입 변경/생성자 몸체를 제공하는 장기 소유자다.
// 실제 Client 진단에 원문의 식·파일·행을 전달한다. callback이 복귀하면 전환을 계속한다.
using ObjectConversionAssertion=std::function<void(std::string_view,std::string_view,std::uint32_t)>;
// 객체 필드 쓰기도 원문의 개별 위치에서 수행하며 현재 슬롯은 각 효과 뒤 다시 읽는다.
struct ObjectConversionHooks {
    std::function<void(Sid,std::uint32_t)> unpop,repop; // 현재 가상48/4c의 flags0 공간 해제/재등록이다.
    std::function<void(Sid,std::uint32_t)> retype; // 실제4b1620이며 재할당/초기화를 대신하지 않는다.
    std::function<std::uint32_t(std::uint32_t)> defaultFrame; // 현재 TYPE+118의 전체 DWORD다.
    std::function<void(Sid,std::uint32_t)> frame,qa; // raw+36 직접 DWORD와4ac220의 QA WORD 쓰기다.
    std::function<std::uint32_t(Sid,std::uint32_t)> base; // 미처리 명령의 실제4ae010 부모 입력이다.
};
// 같은 풀의 실제 Retype·frame DWORD·QA WORD를 연결한다. 공간/부모/defaultFrame은 입력 장치를 보존한다.
// pool/factory와 그 진단 callback의 참조 대상은 반환 hooks보다 오래 살아야 한다.
ObjectConversionHooks MakeObjectConversionHooks(SidPool& pool,SquidFactory& factory,ObjectConversionHooks hooks,
    ObjectConversionAssertion assertion);
// 실제 가상6c가41e400/462290인 표만 반환하며 다른 클래스는0이다.
std::uint32_t ObjectConversionBody(std::uint32_t vtable);
// 원문 Command의 상위 입력 비트 제거와 Unpop 뒤 snapshot 위치를 유지한다.
class RawObjectConversion {
public:
    RawObjectConversion(const SidPool& pool,const std::uint32_t& buriedType,ObjectConversionHooks hooks); // 현재10.78 풀과00541218의 장기 주소를 빌린다.
    std::uint32_t Command(Sid sid,std::uint32_t code,std::uint32_t body) const; // 선택한 실제6c 몸체를 실행하고 그 부모 반환을 보존한다.
private:
    std::span<const std::uint8_t> Raw(Sid sid) const; // 원문의 물리 범위만 확인하고 상태/소유자 필터를 추가하지 않는다.
    const SidPool& pool_; // 실제 현재 TYPE/QA를 읽는 유일한 저장소다.
    const std::uint32_t& buriedType_; // Spell Unpop 뒤 읽는 현재 비석 타입이다.
    ObjectConversionHooks hooks_; // 선택된 실제 하위만 요구하는 장치 경계다.
};
}
