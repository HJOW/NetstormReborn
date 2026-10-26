// 기존 이름 부모를 실제 Client/Template/설정/작업 파일 장치에 연결한다.
#pragma once
#include "client/ClientFortGlobals.h"
#include "client/CampaignTemplateStorage.h"
#include "client/EditorFiles.h"
#include "o/ConfigInterface.h"

namespace netstorm::client { // 게임 알고리즘은 기존 RawFortCurrentName 본체만 실행한다.
// 이름/경로 scratch와 동기 장치 대여를 소유한다. 네 외부 소유자는 이 객체보다 오래 살아야 한다.
class FortCurrentNameDevices {
public:
    FortCurrentNameDevices(ClientFortGlobals& names,CampaignTemplateStorage& storage,o::ConfigInterface& config,o::FortCurrentNameGlobals globals,o::FortCurrentNameHooks hooks,const EditorFiles* work=nullptr,std::function<std::filesystem::path(std::string_view)> resolve={}); // Notify/전체 snapshot/제목/State/진단과 선택적 명시 namespace resolver를 제공한다. 기존 standalone 기본 보호는 유지한다.
    FortCurrentNameDevices(const FortCurrentNameDevices&)=delete;FortCurrentNameDevices& operator=(const FortCurrentNameDevices&)=delete; // 자기 주소를 빌린 경로 callback을 복제하지 않는다.
    void Set(std::span<const std::uint8_t> name); // NUL 포함 같은 source와 실제 원문256BYTE 지역 scratch로 전체 이름 부모를 실행한다.
    void Set(std::span<const std::uint8_t> name,std::span<std::uint8_t,256> oldPathScratch); // Native 입력/진단 검사는 명시 지역 물리를 그대로 제공할 수 있다.
private:
    ClientFortGlobals& names_;CampaignTemplateStorage& storage_;o::ConfigInterface& config_; // 같은 장기 물리를 빌린다.
    o::FortCurrentNameGlobals globals_;o::FortCurrentNameHooks hooks_; // 현재 getter/dirty 참조와 실제 동기 경계를 소유한다.
    const EditorFiles* work_;std::string path_; // 실제 작업 namespace 선택과 다음 경로 호출 전까지 유효한 NUL 경로다.
};
}
