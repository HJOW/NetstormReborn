// 연속 화면 저장의 원문 시작/프레임 복사/종료를 현재 장치와 분리한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>

namespace netstorm::client { // 화면 소유자는 현재 표면과 실제 할당/파일 장치를 빌려준다.
inline constexpr std::size_t kCaptureSequenceCapacity=1000; //005de880의 포인터 수와4db230의 수집 상한이다.
struct CaptureSequenceState { // Client가 소유할 현재 상태이며 Session 교체로 복제하지 않는다.
    std::int32_t count{}; //005df824의 현재 signed 개수다. Stop은 이 값을 초기화하지 않는다.
    std::uint32_t active{},first{}; //005df828/2c의 독립 DWORD이며 Start는둘다1을쓴다.
    std::array<std::uint32_t,kCaptureSequenceCapacity> frames{}; //005de880의 비소유 할당 토큰이며 실제 메모리는 장치가 소유한다.
};
struct CaptureSequenceScreen { // 각 원문 소비 위치에서 현재 표면 전역을 다시 읽는다.
    std::uint32_t width{},height{},pitch{}; //005c78f0/f4/ec의 논리 폭/높이/잠금 줄 간격이다.
    std::uint32_t pixels{},palette{}; //005c78f8/7954의 현재 물리 버퍼/색표를 식별하는 비소유 토큰이다.
};
struct CaptureSequenceHooks { // 실제 효과는 선택한 부모에서만 필수다. 메모리 조회는 순수 현재 span을 돌려준다.
    std::function<void()> lock,unlock; //4a14e0/4a1550의 현재 화면 잠금과 반납이다.
    std::function<std::uint32_t(std::uint32_t)> allocate; //43fa80의 전체DWORD 할당 크기이며 성공한 실제 메모리 토큰을 반환한다.
    std::function<std::span<std::uint8_t>(std::uint32_t,std::uint32_t)> memory; //주어진토큰/길이의 현재 실제 메모리를 빌린다. 물리 밖은 거부한다.
    std::function<void(std::uint32_t)> release; //43fa90의 현재 슬롯 토큰 반납이다. Native는 호출 뒤 슬롯을0으로 쓴다.
    std::function<std::string()> prefix; //4410f0의 인자 없는 현재 GifSequenceName 조회다. 숫자는 그 다음 formatter 인자다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> bitmap; //4ddc90의이름/픽셀/x/y/폭/높이/줄간격/색표8인자다.
    std::function<std::string(std::string_view,std::string_view)> report; //4b9390의style80·Wrote %s through %s. 개행 서식에 두 파일명을 전달한다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t)> notice; //4cf550의 실제 서식 결과와0/0 크기를 받는다.
};
void StartCaptureSequence(CaptureSequenceState& state); //4db210의개수0/active1/first1뿐이며기존1000슬롯을지우지않는다.
void AppendCaptureSequenceFrame(CaptureSequenceState& state,const CaptureSequenceScreen& screen,const CaptureSequenceHooks& hooks); //4db230의잠금/할당/전체pitch복사/현재개수등록이다. active관문은원문Frame호출자에있다.
void StopCaptureSequence(CaptureSequenceState& state,const CaptureSequenceScreen& screen,const CaptureSequenceHooks& hooks); //4db2a0의파일별현재조회/반납/범위안내뒤active0이며개수는보존한다.
}
