// 원본 Mission.cpp의 미션 시작(FUN_00482fb0 ↔ CD 00481020) 가운데 미션 스크립트와 요새 파일을 정하는 부분.
// UberGump가 원본 브리핑 본문을 표시한다. 미션 종류별 실제 객체·게임 명령은 후속이다.
#pragma once
#include "o/ConfigInterface.h"
#include "client/MissionFortSource.h"
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace netstorm::client {
// 미션 이름에 맞는 스크립트를 읽고 섹션을 선택하는 처리기이다.
class MissionScript {
public:
    // `mission` 이름의 설정 객체를 만들어 추가 설정 글(원본의 두 번째 인자)과 `missionSpec` 경로의 스크립트를 읽는다.
    // 이 객체가 살아 있는 동안 다른 설정에서 `{mission.키}`로 스크립트 값을 쓸 수 있다.
    MissionScript(o::ConfigInterface& configuration, std::string_view name, std::string_view extraSettings = {},o::ConfigInterface::FileReader source = {}); //명시 source가 있으면 추가 설정 파일/언어 script/Fort에 같은 수명으로 사용하며 전역 Config reader를 바꾸지 않는다.
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
    std::vector<std::uint8_t> ReadFortImage(std::string_view selectedFort); //Init 요청이 고른 이름의 fortSpec을 현재 Config로 해석해 같은 미션 source에서 읽는다. loadFort를 다시 선택하지 않는다.
    std::function<std::vector<std::uint8_t>(std::string_view)> FortReader() const; //같은 source를 소유 복사해 Session 로그인 재로드에 넘긴다. Script/Editor 반납 뒤에도 현재 Client Config로 같은 fortSpec을 해석한다.
    MissionFortFileSelector FortFileSelector(std::function<std::string(std::string_view)> fileName={}) const; // 현재 fortSpec과 실제 이름만 선택한다. 반환한 read는 그 경로/source를 소유해 Script 반납과 이후 Config 변경에도 같은 파일을 읽는다.
    MissionFortFileReader FortFileReader(std::function<std::string(std::string_view)> fileName={}) const; //현재 fortSpec을 한 번 선택하여 같은 이미지와 실제 파일 이름을 반환한다. 경로 mapper도 reader가 소유한다.
    o::ConfigInterface::FileReader FileSource() const; //명시 source만 소유 복사해 Guide 교체에 넘긴다. 기본 자산 reader는 빈 함수로 표현하여 다음 미션도 기본 경계를 유지한다.
    // `mission.missionType`: 원본은 이 이름으로 등록된 미션 종류를 찾아 객체를 만든다.
    std::string MissionType();
    // 미션 파일의 원본 브리핑 본문을 설정 치환 전 상태로 읽는다.
    std::optional<std::string> Section(std::string_view name) const;
    // 실제 미션 Config의00440640(prefix)를 실행한다. Start의 tellAtList 작성과 같은 객체를 사용한다.
    std::string SectionNames(std::string_view prefix) const;
    // 실제 미션 Config의004404e0(key,value)다. 전역 설정이 아닌 현재 mission 객체에 값을 기록한다.
    bool Set(std::string_view key,std::string_view value);
private:
    // configuration_: 현재 언어와 실행 설정을 조회하는 인터페이스.
    o::ConfigInterface& configuration_;
    // name_: 이름으로 조회·표시할 때 사용하는 문자열.
    std::string name_;
    // scriptPath_: 스크립트 경로.
    std::string scriptPath_;
    bool explicitSource_{}; //호출자가 직접 선택한 namespace인지다. 기본 자산 reader를 복사해 작업본으로 오인하지 않는다.
    o::ConfigInterface::FileReader source_; //선택한 작업 namespace/자산 reader의 소유 복사다. Editor World 반납 뒤에도 같은 경로 장치가 살아 있다.
    // script_: 안내·명령·섹션을 조회할 미션 스크립트.
    o::Config script_;
};
// 실제 Guide 미션 생성의 Native 모드와 global.inMission=1을 같은 스크립트 수명 시작에 공개한다.
std::unique_ptr<MissionScript> CreateGuideMissionScript(o::ConfigInterface& configuration,std::string_view name,bool& missionMode,o::ConfigInterface::FileReader source={}); //선택 source를 새 미션에 넘기고 생성 정상 반환 뒤 기존 Native/Config 공개 순서를 유지한다.
// 실제 Guide 해제처럼 스크립트 등록을 먼저 반납한 뒤 Native 모드와 global.inMission을0으로 쓴다. 현재가 없으면 효과가 없다.
void ReleaseGuideMissionScript(std::unique_ptr<MissionScript>& mission,bool& missionMode,o::ConfigInterface& configuration);
}
