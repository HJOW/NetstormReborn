// PATCH0043c4e0의 고정 소스/소유자 표/!color.dat 부모를 복원한다. 실제 File/TYPE표/9표 계산은 필수 하위다.
#pragma once
#include <array>
#include <cstdint>
#include <exception>
#include <functional>
#include <span>

namespace netstorm::o {
// Native Handler0052221c의 EX_UNABLE_TO_OPEN_FILE만 나타낸다. 일반 VFS 오류 전체를 이 형으로 바꾸지 않는다.
struct ColorCacheUnableToOpen final : std::exception {
    // 원문 형식 분류용이며 호스트 오류 메시지로 catch 대상을 추측하지 않는다.
    const char* what() const noexcept override { return "EX_UNABLE_TO_OPEN_FILE"; }
};
// 실제 장기 출력 배열과 현재 primary를 빌린다. 원문 전역 배열 주소는 설치 중 바뀌지 않는다.
struct ColorMapCacheGlobals {
    std::function<std::span<const std::uint8_t>()> primary; // PathSpec 복귀 뒤 현재00555d88의 raw 기록 물리를 한 번 빌린다.
    std::span<const std::uint32_t,8> ownerColors; //005319c8의 현재8DWORD이며 행마다 lowBYTE를 읽는다.
    std::span<std::uint8_t,2048> ownerHighlight; //00556538의8×256BYTE 출력이다.
    std::span<std::uint8_t,2048> ownerSolid; //00556e38의8×256BYTE 출력이다.
    std::array<std::span<std::uint8_t,256>,9> maps; //6038/7738/6438/6238/6138/6338/5f38/5e38/7838의 실제 파일 순서다.
};
// 빈 성공 장치를 자동 제공하지 않는다. 각 File 객체/경로 물리와 장치 소유자는 동기 호출 끝까지 살아 있어야 한다.
struct ColorMapCacheHooks {
    std::function<std::span<const std::uint8_t>()> path; //00459c60의 같은 NUL 경로 할당 전체 물리를 반환한다. 원문 두 경로 전역 인수는 외부에서 보존한다.
    std::function<void()> owners; //0043be30의 실제 TYPE별 소유자 표 등록이다.
    std::function<void()> buildMaps; //0043c010의 같은 고정 ColorSort/현재 빨강9표 계산이다.
    std::function<void(std::span<std::uint8_t,24>,std::span<const std::uint8_t>,std::uint32_t,std::uint32_t)> openRead; //0041b4c0(path,0,1)의 실제 reader다. 생성 반환값은 부모가 쓰지 않는다.
    std::function<void(std::span<std::uint8_t,24>,std::span<const std::uint8_t>,std::uint32_t,std::uint32_t)> openWrite; //0041b8d0(path,1,1)의 실제 writer다. writer 앞에 임의0 초기화를 넣지 않는다.
    std::function<std::uint32_t(std::span<std::uint8_t,24>,std::span<std::uint8_t,256>,std::uint32_t)> read; //0041a7a0(target,256,ffffffff)의 부분 읽기이며 반환 길이를 부모는 무시한다.
    std::function<std::uint32_t(std::span<std::uint8_t,24>,std::span<const std::uint8_t,256>,std::uint32_t)> write; //0041a910(source,256,ffffffff)의 부분 쓰기이며 반환 길이를 부모는 무시한다.
    std::function<void(std::span<std::uint8_t,24>)> close; //0041b410의 같은24BYTE 객체를 정리한다. 정상 close 예외는 지원하나 unwind 중 두 번째 throw는 지원 계약 밖이다.
};
// Path→현재 primary 한 번→두8행 표→TYPE표까지다. 반환 경로의 같은 주소/전체 물리는 이후 파일 단계도 빌린다.
std::span<const std::uint8_t> PrepareColorMapCache(const ColorMapCacheGlobals& globals,const ColorMapCacheHooks& hooks);
// 실제 독립 cleanup funclet004f9c20/004f9c28의 고정 this 전달과 같은 경계다. 별도 File 소유자를 만들지 않는다.
void CloseColorMapCacheFile(std::span<std::uint8_t,24> file,const ColorMapCacheHooks& hooks);
// 독립 catch0043c96b:9표 재계산→writer→9write→close다. 같은 reader try를 재진입하지 않는다.
void RegenerateColorMapCache(std::span<const std::uint8_t> path,const ColorMapCacheGlobals& globals,const ColorMapCacheHooks& hooks,std::span<std::uint8_t,24> writer);
// reader/writer는 caller가 명시한 서로 다른 지역 물리다. reader 마지막8BYTE만0으로 쓰고 미완성 ctor는 outer close하지 않는다.
// 정확 marker만 cache 재생성에 들어간다. MSVCRT typed dispatch 전체나 실제 OS 파일 장치를 복원한 API는 아니다.
// 장치는 unwind 중 close에서 두 번째 예외를 던지지 않아야 한다. 계약 밖 중첩 예외의 C++ 결과를 Native EH 정책으로 주장하지 않는다.
void InitializeColorMapCache(const ColorMapCacheGlobals& globals,const ColorMapCacheHooks& hooks,std::span<std::uint8_t,24> reader,std::span<std::uint8_t,24> writer);
}
