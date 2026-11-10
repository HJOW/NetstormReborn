// 현재 Client의 장기 자료와 기존 순수 저장 부모로 Fort/Battle snapshot을 수행한다.
#pragma once
#include "o/RawFortMissionSave.h"
#include <cstdint>
#include <memory>
#include <span>

namespace netstorm::client { // 모든 인자는 같은 실제 Client/Template/사용자 작업 파일의 소유자에 속해야 한다.
class Client;class CampaignTemplateStorage;class EditorFiles; // 동기 저장이 끝날 때까지 유효한 실제 장치를 빌린다.
// 004bf9a0의 TypeNames·기술·Objects·Territory·Deck·Money·Mission·Badges·Notify를 실행한다.
void SnapshotNativeFort(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates,std::span<std::uint8_t,87> options,std::span<std::uint32_t,256> restrictions,const o::FortMissionSaveHooks& mission,const EditorFiles& files,std::uint32_t slot);
// 004bf880의 기술·Deck·Objects·Territory·Money·Mission·CoreData·Notify를 실행한다.
void SnapshotNativeBattle(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates,std::span<std::uint8_t,87> options,std::span<std::uint32_t,256> restrictions,const o::FortMissionSaveHooks& mission,const EditorFiles& files,std::uint32_t slot);
// onlyIfPresent=true는004bfb50의 최초 manager0 자연 반환이며, false는004bfac0의 독립 두 if다.
void SnapshotNativeCurrent(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates,std::span<std::uint8_t,87> options,std::span<std::uint32_t,256> restrictions,const o::FortMissionSaveHooks& mission,const EditorFiles& files,bool onlyIfPresent);
// 004bd360의 현재 isMine/preventSave 관문과 실제 파일 저장만 실행하며 snapshot을 추가하지 않는다.
void NotifyNativeFortTemplate(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates,std::span<std::uint8_t,87> options,std::span<std::uint32_t,256> restrictions,const o::FortMissionSaveHooks& mission,const EditorFiles& files,std::uint32_t slot);
// 004bd550의 현재 State Get/최소28BYTE 확장만 수행한다. 반환 뷰의 실제 Template owner는 caller가 계속 보유한다.
std::span<std::uint8_t> GetNativeFortTemplateState(Client& client,const std::shared_ptr<CampaignTemplateStorage>& templates,std::uint32_t slot);
}
