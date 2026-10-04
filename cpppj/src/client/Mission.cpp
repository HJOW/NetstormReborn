// 원본: FUN_00482fb0 @ 00482fb0 (패치판 10.78, Mission.cpp) [신뢰도 A] ↔ CD판 FUN_00481020 (string/strong)
// 범위: `mission` 설정 객체 만들기, 추가 설정과 스크립트 읽기, `mission.loadFort`·`mission.missionType` 조회,
//       `fortSpec` 경로 계산까지. 그 뒤의 미션 종류 표 검색과 미션 객체 생성(+4 이름, +0x5c 설정)은 옮기지 않았다.
#include "client/Mission.h"

namespace netstorm::client {
namespace {
// 원본이 조회하는 키의 접두어. 설정 객체 이름 "mission"에 구분자 '.'을 붙인 것이다.
constexpr std::string_view kMissionPrefix = "mission.";
}

// 원본 순서: 객체 생성 → 추가 설정(FUN_00440410) → `missionSpec` 경로의 파일(FUN_00440380).
MissionScript::MissionScript(o::ConfigInterface& configuration, std::string_view name, std::string_view extraSettings)
    : configuration_(configuration), name_(name), script_(configuration.Registry(), "mission") {
    script_.AppendArguments(extraSettings, configuration_.Reader());
    scriptPath_ = configuration_.PathSpec("missionSpec", name_);
    if (configuration_.Reader()) if (const auto bytes = configuration_.Reader()(scriptPath_)) script_.LoadBytes(*bytes);
}
// 원본은 미션 설정 객체를 this로 FUN_004409d0을 부른다. 스크립트를 읽지 못했으면 찾지 못한다.
std::optional<std::string> MissionScript::Get(std::string_view key) {
    return script_.Get(std::string(kMissionPrefix) + std::string(key));
}
// 미션 이름을 돌려준다.
const std::string& MissionScript::Name() const { return name_; }
// 스크립트 경로를 돌려준다.
const std::string& MissionScript::ScriptPath() const { return scriptPath_; }
// 버퍼가 만들어졌는지로 판단한다.
bool MissionScript::Loaded() const { return script_.IsValid(); }
// 값이 비어 있으면(없거나 빈 문자열) 미션 이름을 쓴다(원본은 출력 버퍼의 첫 글자로 판단한다).
std::string MissionScript::FortName() {
    const auto fort = Get("loadFort").value_or(std::string());
    return fort.empty() ? name_ : fort;
}
// `fortSpec`에 요새 이름을 넣어 경로를 만든다.
std::string MissionScript::FortPath() { return configuration_.PathSpec("fortSpec", FortName()); }
// 없으면 빈 문자열이다.
std::string MissionScript::MissionType() { return Get("missionType").value_or(std::string()); }
}
