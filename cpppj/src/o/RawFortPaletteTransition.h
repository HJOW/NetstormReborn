// 원본 004b6100의 팔레트 이름 전환과 세 하위 부모의 현재 전역·부분 효과를 복원한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::o {
// 고정 주소의 이름 버퍼와 같은 Client 수명의 실제 DWORD를 빌린다. 참조에 임시 bool을 연결하지 않는다.
struct FortPaletteTransitionGlobals {
    std::span<std::uint8_t> paletteName; // 005c8910의 물리 버퍼이다. 원문에는 의미상 255자 제한이 없다.
    std::span<std::uint8_t> cloudName; // 0056b7a0의 물리 버퍼이며 paletteName과 별개 주소이다.
    std::uint32_t& cache; // 00569ffc 현재 Kencloud 할당 주소/장치 토큰이다.
    const std::uint32_t& count; // 00540a80 현재 횟수를 build 뒤 한 번 읽는다.
    const std::uint32_t& renderer; // 0059a8bc 현재 렌더러 존재 DWORD를 maps 뒤에 읽는다.
    std::uint32_t& dirty; // 0059a8b0이며 renderer가 0이면 기존 값을 보존한다.
};
// 이미 복원된 계산·Screen과 아직 필요한 arg2=1 장치 경계를 필수 콜백으로 분리한다.
struct FortPaletteTransitionHooks {
    std::function<void(std::uint32_t,std::string_view)> log; // 004c2630(29,"init palette\n")이다. 첫 인자 포인터 캡처보다 먼저 호출한다.
    std::function<std::int32_t(std::span<const std::uint8_t>,std::span<const std::uint8_t>)> compare; // CRT 004e65ed의 현재 두 C 문자열 비교이다. 0일 때 자연 반환한다.
    std::function<void(std::uint32_t)> release; // 004e4354로 현재 캐시 주소를 해제한다. 복귀 뒤 부모가 반드시 0을 쓴다.
    std::function<void()> buildCloud; // 00471040의 실제 계산/할당 부모이다. cache가 0일 때만 호출하며 성공을 가장하지 않는다.
    std::function<void()> stepCloud; // 00470b20을 캡처한 count만큼 호출한다. callback 뒤 count를 다시 읽지 않는다.
    std::function<std::span<const std::uint8_t>(std::string_view,std::span<const std::uint8_t>,std::uint32_t,std::uint32_t)> pathSpec; // 004410f0("GamePalSpec",같은 입력 주소,0,0)의 현재 NUL 경로를 빌린다.
    std::function<void(std::span<const std::uint8_t>,std::uint32_t)> loadPalette; // 004a4850(path,1)의 필수 실제 장치 경계이다. 기존 arg2=0 경로만으로 대체하지 않는다.
    std::function<void()> buildMaps; // 0043c010의 실제 추가 표 계산·소비자 갱신 경계이다.
};
// 00470c30: 비영 현재 캐시만 free하고 정상 복귀한 뒤 무조건 같은 DWORD를 0으로 쓴다.
void ReleaseFortPaletteCloudCache(const FortPaletteTransitionGlobals& globals,const FortPaletteTransitionHooks& hooks);
// 00471190: 같은 source 주소에서 NUL까지 순방향 복사하고, 필요한 build 뒤 현재 count를 한 번 캡처한다.
void SetFortPaletteCloudName(std::span<const std::uint8_t> source,const FortPaletteTransitionGlobals& globals,const FortPaletteTransitionHooks& hooks);
// 004973c0: 현재 렌더러 DWORD가 비영인 경우에만 dirty=1로 만든다.
void MarkFortPaletteDirty(const FortPaletteTransitionGlobals& globals);
// 004b6100: log 뒤 입력 span 변수를 캡처한다. 이후 내용은 같은 물리를 계속 읽으며 외부 중단을 삼키지 않는다.
void TransitionFortPalette(const std::span<const std::uint8_t>& argument,const FortPaletteTransitionGlobals& globals,const FortPaletteTransitionHooks& hooks);
}
