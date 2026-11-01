// 실제1000슬롯·현재 전역 재조회·부분 효과를 원문과 같은 순서로 유지한다.
#include "client/RawCaptureSequence.h"
#include <algorithm>
#include <bit>
#include <stdexcept>

namespace netstorm::client { // 창/파일을 직접 만들지 않는 원문 상태/복사 본체다.
namespace { // 선택 장치와 원문 고정 물리의 범위만 확인한다.
template<class F> const F& Need(const F& hook) { if (!hook) throw std::logic_error("연속 화면 저장의 선택된 실제 장치 미연결");return hook; } // 선택하지 않은 파일/화면 장치는 요구하지 않는다.
std::uint32_t& Slot(CaptureSequenceState& state,std::int32_t index) { // 음수/표밖 Native주소를 임의 확장된 호스트 표로 만들지 않는다.
    if (index<0 || static_cast<std::size_t>(index)>=state.frames.size()) throw std::out_of_range("연속 화면 저장의1000슬롯 물리 범위");return state.frames[static_cast<std::size_t>(index)];
}
std::span<std::uint8_t> Memory(const CaptureSequenceHooks& hooks,std::uint32_t token,std::uint32_t length) { // byte수는 Native의전체DWORD 곱셈 결과다.
    const auto bytes=Need(hooks.memory)(token,length);if (bytes.size()<length) throw std::out_of_range("연속 화면 저장의 실제 할당/표면 바이트 범위");return bytes.first(length);
}
std::string Name(std::string_view prefix,std::int32_t number) { // 원문%s%03d.bmp의최소숫자폭과첫NUL을보존한다.
    prefix=prefix.substr(0,prefix.find('\0'));auto digits=std::to_string(number);
    if (digits.size()<3) digits.insert(number<0 ? 1U:0U,3-digits.size(),'0');
    auto result=std::string(prefix)+digits+".bmp";if (result.size()>=256) throw std::length_error("연속 화면 저장의 원문 파일명256BYTE 범위");return result;
}
}
void StartCaptureSequence(CaptureSequenceState& state) { state.count=0;state.active=1;state.first=1; } // 기존포인터표/메모리에접근하거나반납하지않는다.
void AppendCaptureSequenceFrame(CaptureSequenceState& state,const CaptureSequenceScreen& screen,const CaptureSequenceHooks& hooks) {
    if (state.count>=static_cast<std::int32_t>(kCaptureSequenceCapacity)) return;
    if (state.count<0) throw std::out_of_range("연속 화면 저장의 음수 개수 물리");
    Need(hooks.lock)();const auto allocated=screen.height*screen.pitch;const auto token=Need(hooks.allocate)(allocated);
    const auto length=screen.height*screen.pitch;const auto source=screen.pixels; // 할당이끝난뒤현재복사길이/화면포인터를다시읽는다.
    auto destination=Memory(hooks,token,length);const auto pixels=Memory(hooks,source,length);std::copy(pixels.begin(),pixels.end(),destination.begin());
    Need(hooks.unlock)();Slot(state,state.count)=token;state.count=std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(state.count)+1U); // 원문도Unlock뒤현재개수의슬롯을선택한다.
}
void StopCaptureSequence(CaptureSequenceState& state,const CaptureSequenceScreen& screen,const CaptureSequenceHooks& hooks) {
    for (std::int32_t index=0;index<state.count;++index) { // 매번현재개수를읽으며첫null포인터에서파일순회를끝낸다.
        if (!Slot(state,index)) break;const auto name=Name(Need(hooks.prefix)(),index);
        const auto palette=screen.palette,width=screen.width,height=screen.height,frame=Slot(state,index); // prefix/formatter효과뒤현재표면전역과프레임포인터를고정한다.
        Need(hooks.bitmap)(name,frame,0,0,width,height,width,palette); // 수집pitch와달리내보내기는원문처럼현재width를stride로쓴다.
        Need(hooks.release)(Slot(state,index));Slot(state,index)=0; // writer가바꾼현재슬롯을반납하며예전포인터로고정하지않는다.
    }
    const auto first=Name(Need(hooks.prefix)(),0);const auto count=state.count;const auto last=Name(Need(hooks.prefix)(),count); // 마지막숫자는마지막prefix조회전에캡처한다.
    const auto message=Need(hooks.report)(first,last);Need(hooks.notice)(message,0,0);state.active=0; // 실패/중단앞선파일/반납/개수/active를롤백하지않는다.
}
}
