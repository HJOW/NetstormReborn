// 원문의 언어별 문자 표·현재 Renderer 중심·소리/쓰기 순서를 그대로 적용한다.
#include "client/SavedCameraKey.h"
#include <bit>
#include <stdexcept>
#include <string_view>

namespace netstorm::client { // Native 전역을 호스트 주소로 재해석하지 않는다.
namespace { // 보호 PE005434c0/cc/d8와00543504/1c/34의 실제 바이트다.
constexpr std::string_view kDigits="1234567890",kDefaultSymbols="!@#$%^&*()",kEnglishOneSymbols="!\"\xa3$%^&*()",kGermanSymbols="!\"\xa7$%&/()="; //표준/영어1/독일어3의 열 기호를 같은 인덱스로 대응한다.
std::string_view Symbols(std::int32_t language) { return language==3 ? kGermanSymbols : language==1 ? kEnglishOneSymbols : kDefaultSymbols; } //나머지 공개 번호는 원문 기본 표를 사용한다.
std::int32_t Add(std::int32_t first,std::int32_t second) { return std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(first)+std::bit_cast<std::uint32_t>(second)); } //원문의 signed ADD 하위DWORD 감김이며 C++ overflow는 피한다.
bool Candidate(std::uint32_t character) { //언어를 미리 읽지 않고 세 표에 속할 수 있는 문자만 좁은 키 몸체로 받는다.
    if (character>255) return false;const auto value=static_cast<char>(character);
    return kDigits.find(value)!=kDigits.npos || kDefaultSymbols.find(value)!=kDefaultSymbols.npos || kEnglishOneSymbols.find(value)!=kEnglishOneSymbols.npos || kGermanSymbols.find(value)!=kGermanSymbols.npos;
}
}
void InitializeSavedCameraPositions(SavedCameraPositions& saved) {
    for (auto& point:saved.points) point={-10000.0F,-10000.0F}; //최초 열 좌표를 모두 같은 원문 무효 Point로 쓴다.
}
o::ConstructionNoticePoint CaptureSavedCameraCenter(const RawCameraViewState& view,const std::array<std::int32_t,4>& viewport) {
    const auto x=Add(Add(viewport[0],viewport[2])/2,view.pixels[0]),y=Add(Add(viewport[1],viewport[3])/2,view.pixels[1]); //signed 절반은0 쪽 절삭이며 나머지 두 ADD는32비트로 감긴다.
    return {static_cast<float>(static_cast<double>(x)/std::bit_cast<float>(view.cellFloatBits[0])),static_cast<float>(static_cast<double>(y)/std::bit_cast<float>(view.cellFloatBits[1]))}; //FILD32→FDIV F32→FSTP F32이며 정수의 조기 float 변환을 하지 않는다.
}
bool IsSavedCameraPositionValid(o::ConstructionNoticePoint point) { return 0.0F<=point.x && 0.0F<=point.y && point.x<256.0F && point.y<256.0F; } //Tab의 strict 양수/현재 세계 크기 판정과 합치지 않는다.
bool DispatchSavedCameraKey(InputEvent event,SavedCameraPositions& saved,const SavedCameraKeyHooks& hooks) {
    if ((event.code&(InputCode::kRelease|InputCode::kOutside)) || !Candidate(event.code&0x03ffffffU)) return false;
    if (!hooks.paused) throw std::logic_error("숫자 저장 카메라의 실제 정지 판정 미연결");if (hooks.paused()) return true;
    if (!hooks.language) throw std::logic_error("숫자 저장 카메라의 현재 공개 언어 미연결");const auto symbols=Symbols(hooks.language());const auto character=static_cast<char>(event.code&255U);
    if (kDigits.find(character)==kDigits.npos && symbols.find(character)==symbols.npos) return true;
    const bool shift=(event.code&InputCode::kShift)!=0;if ((event.code&0x03ffffffU)=='*' && !shift) return true;
    const auto plain=event.code&~InputCode::kShift;std::size_t index{};
    if (shift) { if (plain>255) return true;index=symbols.find(static_cast<char>(plain)); }
    else index=plain=='0' ? 9U : plain-static_cast<std::uint32_t>('1'); //원문0만별도9이며 Ctrl/Alt/PressedSince는 제거하지 않는다.
    if (index>=saved.points.size()) return true;
    if (shift) {
        if (!hooks.sound) throw std::logic_error("숫자 저장 카메라의 실제 선택음 미연결");hooks.sound();
        if (!hooks.capture) throw std::logic_error("숫자 저장 카메라의 현재 Renderer 중심 미연결");saved.points[index]=hooks.capture(); //소리 이후의 현재 중심을 읽은 뒤에만 선택한 칸을 쓴다.
    }else if (IsSavedCameraPositionValid(saved.points[index])) {
        if (!hooks.center) throw std::logic_error("숫자 저장 카메라의 실제 즉시 중심 미연결");hooks.center(saved.points[index].x,saved.points[index].y);
    }
    return true;
}
}
