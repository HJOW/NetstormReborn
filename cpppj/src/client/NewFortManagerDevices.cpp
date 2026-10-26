// 원본004bfba0의 제어는 RawNewFort에 두고 실제 소유자와 파일 장치만 빌린다.
#include "client/NewFortManagerDevices.h"
#include "client/CampaignTemplateStorage.h"
#include "client/EditorFiles.h"
#include "o/RawNewFort.h"
#include "o/RawTemplateLoad.h"
#include <stdexcept>

namespace netstorm::client { // 독립 manager 사본이나 임시 Fort 파일을 만들지 않는다.
// 원문 부모가 선택한 하위는 같은 저장소를 사용한다. 실패 이전의 헤더/파일 효과를 되돌리지 않는다.
void CreateNewEditorFortManager(CampaignTemplateStorage& storage,const EditorFiles& files,
    std::uint32_t slot,std::string_view fileName,std::uint32_t argument,const NewFortManagerDiagnostics& diagnostics) {
    if (slot>=9) throw std::out_of_range("NewFort의 실제 Template9 슬롯 범위");
    o::NewFortManagerHooks hooks;
    hooks.assertion=diagnostics.templateAssertion;
    hooks.exists=[&files](auto name) { return files.Exists(name); };
    hooks.allocate=[&storage](auto bytes) { return storage.AllocateEmptyManager(bytes); };
    hooks.construct=[&storage](auto token,auto name) { return storage.ConstructEmptyManager(token,name); };
    hooks.fileName=[&storage](auto token) { return o::TemplateDataManagerFileName(storage.ManagerBytes(token)); };
    hooks.erase=[&files](auto name) { files.Delete(files.ResolvePath(name).string()); }; // 상대 이름도 존재 검사와 같은 작업 경로로 해석하며 manager에 저장한 원문 이름은 보존한다.
    const auto lifetime=storage.ManagerLifetime();hooks.destroy=lifetime.destroy;hooks.release=lifetime.release;
    hooks.initialize=[&storage,&diagnostics](auto token) { storage.InitializeEmptyManager(token,diagnostics.initializeAssertion); };
    o::CreateNewFortManager(storage.Headers().subspan(static_cast<std::size_t>(slot)*20,20),fileName,argument,hooks);
}
}
