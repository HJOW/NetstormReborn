// 실제 Console/TextEntry가 선택하는 벽시계·clipboard·Chat 콜백 경계를 제공한다.
#pragma once
#include "client/TextEntryGump.h"
namespace netstorm::client { // 공통 Client 편집 UI의 연결 문맥이다.
struct RawTextEntryContext { // 선택한 기능만 해당 실제 장치를 요구한다.
    std::function<double()> now; // 원본460d70의 벽시계 초이며 게임 시각과 구별한다.
    std::function<void(std::span<char>)> clipboardRead; // 원문 최대256BYTE 읽기다. 선택 시 실제 장치로 연결한다.
    std::function<void(std::span<const char>)> clipboardWrite; // 명시 길이의 원문 선택/전체 복사다.
    std::function<bool(std::string_view)> recipient; // 원문42dc70의 수신 대상 검사다.
    std::function<std::optional<std::string>(std::uint32_t,std::optional<std::string_view>,std::int32_t,bool)> complete; // 실제 Console callback이다.
    std::function<void(std::uint32_t,std::string_view)> submit; // 실제 Console 제출 callback이다.
};
}
