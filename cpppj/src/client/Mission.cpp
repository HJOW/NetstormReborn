// 원본: FUN_00482fb0 @ 00482fb0 (패치판 10.78, Mission.cpp) [신뢰도 A] ↔ CD판 FUN_00481020 (string/strong)
// 범위: `mission` 설정 객체 만들기, 추가 설정과 스크립트 읽기, `mission.loadFort`·`mission.missionType` 조회,
//       `fortSpec` 경로 계산까지. 그 뒤의 미션 종류 표 검색과 미션 객체 생성(+4 이름, +0x5c 설정)은 옮기지 않았다.
#include "client/Mission.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
// 원본이 조회하는 키의 접두어. 설정 객체 이름 "mission"에 구분자 '.'을 붙인 것이다.
constexpr std::string_view kMissionPrefix = "mission.";
//이미 선택한 같은 경로의 명시 부재/예외를 다른 자산 reader로 우회하지 않는다.
std::vector<std::uint8_t> ReadFortPath(const o::ConfigInterface::FileReader& source,std::string_view path) {
    auto bytes=source ? source(path) : std::nullopt;
    if (!bytes) throw std::runtime_error("Game file not found: "+std::string(path));return std::move(*bytes);
}
//최초 설치와 기존 로그인 reader도 같은 현재 fortSpec을 한 번 선택한다.
std::vector<std::uint8_t> ReadFortSource(o::ConfigInterface& configuration,const o::ConfigInterface::FileReader& source,std::string_view selectedFort) {
    return ReadFortPath(source,configuration.PathSpec("fortSpec",selectedFort));
}
}

// 원본 순서: 객체 생성 → 추가 설정(FUN_00440410) → `missionSpec` 경로의 파일(FUN_00440380).
MissionScript::MissionScript(o::ConfigInterface& configuration, std::string_view name, std::string_view extraSettings,o::ConfigInterface::FileReader source)
    : configuration_(configuration), name_(name),explicitSource_(static_cast<bool>(source)),source_(source ? std::move(source) : configuration.Reader()), script_(configuration.Registry(), "mission") {
    script_.AppendArguments(extraSettings, source_);
    scriptPath_ = configuration_.PathSpec("missionSpec", name_);
    if (source_) if (const auto bytes = source_(scriptPath_)) script_.LoadBytes(*bytes);
    if (script_.IsValid()) script_.Set("fileName", name_);
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
//같은 source의 명시 부재/예외를 다른 자산 reader로 우회하지 않는다. 기존 BaseFileSystem::Read의 부재 진단을 유지한다.
std::vector<std::uint8_t> MissionScript::ReadFortImage(std::string_view selectedFort) {
    return ReadFortSource(configuration_,source_,selectedFort);
}
//Config는 실제 Client 수명을 빌리고, 파일 namespace는 미션 객체 자체와 독립된 소유 복사로 유지한다.
std::function<std::vector<std::uint8_t>(std::string_view)> MissionScript::FortReader() const {
    return [configuration=&configuration_,source=source_](auto selectedFort) { return ReadFortSource(*configuration,source,selectedFort); };
}
//I/O 경계가 Config를 바꾸더라도 이미 선택한 경로와 그 파일의 바이트를 같은 결과에 유지한다.
MissionFortFileReader MissionScript::FortFileReader(std::function<std::string(std::string_view)> fileName) const {
    return [configuration=&configuration_,source=source_,fileName=std::move(fileName)](auto selectedFort) {
        const auto path=configuration->PathSpec("fortSpec",selectedFort);auto bytes=ReadFortPath(source,path);
        return MissionFortSourceImage{std::move(bytes),fileName ? fileName(path) : path};
    };
}
//Guide 교체는 명시 작업 namespace를 유지하고 기본 자산 경계는 새 미션의 현재 Config에 맡긴다.
o::ConfigInterface::FileReader MissionScript::FileSource() const { return explicitSource_ ? source_ : o::ConfigInterface::FileReader{}; }
// 없으면 빈 문자열이다.
std::string MissionScript::MissionType() { return Get("missionType").value_or(std::string()); }
// Section은 등록 객체의 접두어를 붙이지 않는 원본 본문 조회다.
std::optional<std::string> MissionScript::Section(std::string_view name) const {
    const auto section = script_.Section(name);
    return section ? std::optional<std::string>(std::string(*section)) : std::nullopt;
}
// 스크립트 본문의 원래 섹션 순서와 끝 세미콜론을 그대로 반환한다.
std::string MissionScript::SectionNames(std::string_view prefix) const { return script_.SectionNames(prefix); }
// 객체 접두어를 더하지 않고 현재 미션 Config에 직접 기록하여 Start의 실제 설정 쓰기를 연결한다.
bool MissionScript::Set(std::string_view key,std::string_view value) { return script_.Set(key,value); }
// 원문00482bf0의00594fa0/Config 쓰기를 현재 Guide가 소유한 실제 미션 생성에 연결한다.
std::unique_ptr<MissionScript> CreateGuideMissionScript(o::ConfigInterface& configuration,std::string_view name,bool& missionMode,o::ConfigInterface::FileReader source) {
    auto mission=std::make_unique<MissionScript>(configuration,name,std::string_view{},std::move(source));missionMode=true;configuration.Set("global.inMission","1");return mission;
}
// 원문00483210/00483260은 보유 Config 해제 뒤 Native0,그 뒤 global.inMission0을 공개한다.
void ReleaseGuideMissionScript(std::unique_ptr<MissionScript>& mission,bool& missionMode,o::ConfigInterface& configuration) {
    if (!mission) return;mission.reset();missionMode=false;configuration.Set("global.inMission","0"); //전체 State cleanup이나 Editor prefix에서 원문에 없는 모드 초기화를 추가하지 않는다.
}
}
