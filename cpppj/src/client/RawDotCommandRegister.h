// 원본 Winmain의 점 명령 등록0043f660을 실제 인접 표의 바이트에 실행한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>

namespace netstorm::client {
inline constexpr std::uint32_t kDotCommandStateBytes=0xa08; // 이름2048/함수256/표시260/count4BYTE의 원문 인접 물리 폭이다.
inline constexpr std::uint32_t kDotCommandNamesToken=0x00531b30; // 실제 host 포인터가 아닌 원문 이름표의 주소 식별자다.
inline constexpr std::uint32_t kDotCommandEmptyToken=0x00503c8b; // 숨김 명령의 원문 빈 문자열 주소 식별자다.

// CRT strncpy의 정상 NUL padding을 물리 span 안에서 수행한다. 입력 alias의 CRT 전체 동작은 별도 계약이다.
void CopyDotCommandName(std::span<std::uint8_t> destination,std::span<const std::uint8_t> source,std::uint32_t count);
// 등록 부모는 현재count를 복사 callback 뒤에 다시 읽는다. 같은 state를 변경하는 장치를 그대로 빌린다.
struct DotCommandRegisterHooks {
    std::function<void(std::span<std::uint8_t>,std::span<const std::uint8_t>,std::uint32_t)> copy{CopyDotCommandName}; //31BYTE CRT 경계이며 미선택 상한return에서는 요구하지 않는다.
};
// state는 실제2568BYTE 인접 표다. count/함수/표시포인터를 별도 사본으로 분리하지 않는다.
// callback은 원문 함수 식별자이며 flag비영은이름표주소,0은빈문자열주소를 표시표에 쓴다.
// 원문진입관문은signed count<64다. 음수초기count의원시주소감김은host지원밖이므로out_of_range로명시거부한다.
// 복사뒤count는한번캡처하며직접쓰기중count자체와겹쳐도같은캡처값으로증가/종단효과를계속한다.
std::uint32_t RegisterDotCommand(std::span<std::uint8_t> state,std::span<const std::uint8_t> name,std::uint32_t callback,std::uint32_t flag,const DotCommandRegisterHooks& hooks={});
}
