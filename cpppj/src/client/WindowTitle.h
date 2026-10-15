// PATCH00460730/CD004ca4e0의 창 제목 선택을 현재 설정·미션·Win32 창에 연결한다.
#pragma once
#include <functional>
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client {
// 원본 제목 함수가 읽는 현재 요새/모드/Template 상태다. 호출 중에도 원본 저장소의 현재 값을 사용한다.
struct WindowTitleState {
    std::optional<std::string> fortName; // 현재 요새 이름이며 null과 빈 문자열은 모두 빈 창 제목 경로다.
    bool editor{},mission{}; // 편집 모드가 미션보다 우선하고 그 밖에는 일반 요새 제목을 쓴다.
    bool templatePresent{},templateReadOnly{}; // 현재 Template의 +0과 +8이 모두 비영점일 때만 readonly 접두를 붙인다.
    bool demo{},testBattle{}; // 현재 미션의 +0x68/+0x64이며 demo가 testBattle보다 우선한다.
};
// 기존 ConfigInterface/MissionScript/Client::Title의 실제 소비자만 받는다.
struct WindowTitleHooks {
    std::function<std::string(std::string_view,std::optional<std::string_view>)> pathSpec; // 원본 설정 키와 첫 치환 인자다. null 인자와 빈 문자열을 구분한다.
    std::function<std::string()> missionTitle; // 미션 분기를 선택했을 때만 현재 title을 조회한다.
    std::function<void(std::string_view)> setTitle; // 완성된 제목을 현재 창에 적용한다.
};
// 원본 순서대로 설정을 조회하고 마지막에 한 번 창 제목을 바꾼다. 정의된 256BYTE 버퍼 범위만 지원한다.
void RefreshWindowTitle(const WindowTitleState& state,const WindowTitleHooks& hooks);
}
