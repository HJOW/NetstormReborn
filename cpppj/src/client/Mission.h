// 원본 Mission.cpp의 미션 시작(FUN_00482fb0 ↔ CD 00481020) 가운데 미션 스크립트와 요새 파일을 정하는 부분.
// UberGump가 원본 브리핑 본문을 표시한다. 미션 종류별 실제 객체·게임 명령은 후속이다.
#pragma once
#include "o/ConfigInterface.h"
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client {
class MissionScript {
public:
    // `mission` 이름의 설정 객체를 만들어 추가 설정 글(원본의 두 번째 인자)과 `missionSpec` 경로의 스크립트를 읽는다.
    // 이 객체가 살아 있는 동안 다른 설정에서 `{mission.키}`로 스크립트 값을 쓸 수 있다.
    MissionScript(o::ConfigInterface& configuration, std::string_view name, std::string_view extraSettings = {});
    // 스크립트의 값(치환 포함). 원본처럼 `mission.키`로 조회한다.
    std::optional<std::string> Get(std::string_view key);
    // 미션 이름(스크립트 파일 이름).
    const std::string& Name() const;
    // 스크립트 파일의 경로(`missionSpec`).
    const std::string& ScriptPath() const;
    // 스크립트를 실제로 읽었는가.
    bool Loaded() const;
    // 읽을 요새 이름: `mission.loadFort`가 있으면 그 값, 없으면 미션 이름.
    std::string FortName();
    // 요새 파일의 경로(`fortSpec`, 원본 FUN_00460650).
    std::string FortPath();
    // `mission.missionType`: 원본은 이 이름으로 등록된 미션 종류를 찾아 객체를 만든다.
    std::string MissionType();
    // 미션 파일의 원본 브리핑 본문을 설정 치환 전 상태로 읽는다.
    std::optional<std::string> Section(std::string_view name) const;
private:
    o::ConfigInterface& configuration_;
    std::string name_;
    std::string scriptPath_;
    o::Config script_;
};
}
