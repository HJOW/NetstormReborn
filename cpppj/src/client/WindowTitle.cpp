#include "client/WindowTitle.h"
#include <stdexcept>

namespace netstorm::client {
// 편집→일반 요새→demo/test/일반 미션 순서와 readonly의 별도 설정 조회를 보존한다.
void RefreshWindowTitle(const WindowTitleState& state,const WindowTitleHooks& hooks) {
    if (!hooks.pathSpec || !hooks.missionTitle || !hooks.setTitle) throw std::invalid_argument("창 제목의 실제 설정/미션/창 소비자 누락");
    std::string title; // 원본의 0으로 채운 256BYTE 임시 제목 버퍼다.
    if (!state.fortName || state.fortName->empty()) title=hooks.pathSpec("blankWindowName",std::nullopt);
    else if (state.editor) title=hooks.pathSpec("EditorWindowName",*state.fortName);
    else if (!state.mission) {
        if (state.templatePresent && state.templateReadOnly) title=hooks.pathSpec("readonly",std::nullopt)+" ";
        title+=hooks.pathSpec("windowName",*state.fortName);
    } else {
        const auto key=state.demo ? "demoWindowName" : state.testBattle ? "testBattleWindowName" : "missionWindowName"; // 실제 미션 두 플래그의 우선순위다.
        const auto mission=hooks.missionTitle(); // 설정 치환보다 먼저 현재 미션 title을 읽으며 파일 이름으로 대신하지 않는다.
        title=hooks.pathSpec(key,mission);
    }
    if (title.size()>=256 || title.find('\0')!=std::string::npos) throw std::length_error("원본 창 제목의 NUL/256BYTE 물리 범위 초과");
    hooks.setTitle(title);
}
}
