// 실제 선택·강조의 아홉 DWORD와 다중 선택 배열을 Runtime보다 오래 보관한다.
#pragma once
#include "client/RawUserSelection.h"
#include "client/RawUserHighlight.h"
#include "client/Renderer.h"
#include <bit>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace netstorm::client { // raw 처리기는 이 owner의 같은 현재 값을 빌린다.
class ClientUserSelectionGlobals {
public:
    // 실제 주소를 섞지 않고 원문 DWORD의 읽기/쓰기 대상을 명시한다.
    enum class Field { Enabled,Mode,Selected,MouseType,MouseX,MouseY,RememberedX,RememberedY,MultipleCount };
    ClientUserSelectionGlobals()=default; // PE의 enabled1과 나머지 BSS0만 구성한다.
    ClientUserSelectionGlobals(const ClientUserSelectionGlobals&)=delete;ClientUserSelectionGlobals& operator=(const ClientUserSelectionGlobals&)=delete; // 현재 ref/getter가 빌린 owner를 복제하지 않는다.
    ClientUserSelectionGlobals(ClientUserSelectionGlobals&&)=delete;ClientUserSelectionGlobals& operator=(ClientUserSelectionGlobals&&)=delete; // 현재 ref/getter의 주소를 옮기지 않는다.
    std::uint32_t enabled{1}; //005434e4: bool로 좁히지 않는 전체 DWORD다.
    std::uint32_t mode{},selected{},mouseType{}; //005cae90/005caea0/005caea4: 선택 SID도 소비 전까지 전체 DWORD다.
    ScreenPoint mouse{}; //005cae94/98: 원시 화면 정수 좌표이며 커서의 보정55a2f8/fc와 별개다.
    std::array<std::uint32_t,2> remembered{}; //005caf38/3c: 기억한 두 float의 원시 DWORD 비트다.
    std::int32_t multipleCount{}; //005caf34: 반복 조건이 매번 다시 읽는 signed DWORD다.
    std::vector<std::uint32_t> multiple; //005caf2c가 가리키는 실제 DWORD 요소들이다. 호스트 vector header를 Native pointer/header와 같다고 하지 않는다.
    // 기존 일반 World의 새 입력 상태만 같은 주소에서 준비한다. Native Clear나 prepared 대여에 사용하지 않는다.
    void ResetForGeneralWorld() { enabled=1;mode=selected=mouseType=0;mouse={};remembered={};multipleCount=0;multiple.clear(); }
    // 실제 다중 배열은 callback의 재할당 뒤에도 다음 반복에서 현재 pointer/size를 다시 읽는다.
    UserSelectionState SelectionState() { return {selected,mouseType,remembered,{},multipleCount,[this] { return std::span<const std::uint32_t>(multiple); }}; }
    // 기존 강조 모듈도 같은 선택·마우스 필드를 직접 빌린다.
    UserHighlightState HighlightState() { return {mode,selected,mouseType,mouse.x,mouse.y}; }
    // signed 좌표/count를 reinterpret 참조로 위장하지 않고 전체 DWORD 비트를 반환한다.
    std::uint32_t ReadDword(Field field) const {
        static_assert(std::is_same_v<decltype(ScreenPoint::x),std::int32_t> && sizeof(multipleCount)==4); // 현재 호스트 표현도 각 signed DWORD 폭이어야 한다.
        switch (field) {
        case Field::Enabled:return enabled;case Field::Mode:return mode;case Field::Selected:return selected;case Field::MouseType:return mouseType;
        case Field::MouseX:return std::bit_cast<std::uint32_t>(mouse.x);case Field::MouseY:return std::bit_cast<std::uint32_t>(mouse.y);
        case Field::RememberedX:return remembered[0];case Field::RememberedY:return remembered[1];case Field::MultipleCount:return std::bit_cast<std::uint32_t>(multipleCount);
        }
        throw std::invalid_argument("선택 전역의 알려지지 않은 DWORD 조회");
    }
    // 실제 필드 하나에만 쓰며 bool 정규화·SID16 절삭·이웃 필드 reset을 추가하지 않는다.
    void WriteDword(Field field,std::uint32_t value) {
        switch (field) {
        case Field::Enabled:enabled=value;return;case Field::Mode:mode=value;return;case Field::Selected:selected=value;return;case Field::MouseType:mouseType=value;return;
        case Field::MouseX:mouse.x=std::bit_cast<std::int32_t>(value);return;case Field::MouseY:mouse.y=std::bit_cast<std::int32_t>(value);return;
        case Field::RememberedX:remembered[0]=value;return;case Field::RememberedY:remembered[1]=value;return;case Field::MultipleCount:multipleCount=std::bit_cast<std::int32_t>(value);return;
        }
        throw std::invalid_argument("선택 전역의 알려지지 않은 DWORD 쓰기");
    }
};
}
