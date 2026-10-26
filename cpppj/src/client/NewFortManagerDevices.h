// 새 맵 manager 부모에 실제 저장소와 보호된 작업본 파일 장치를 연결한다.
#pragma once
#include "o/RawDataManagerInitialize.h"
#include <cstdint>
#include <functional>
#include <string_view>

namespace netstorm::client { // 원본 클라이언트의 파일/할당 경계만 제공한다.
class CampaignTemplateStorage; // 실제 manager 이미지와 섹션 수명을 소유하는 장기 저장소다.
class EditorFiles; // 원본 밖의 작업 namespace만 쓰는 파일 장치다.

// 원문 진단에서 복귀할지 중단할지는 호출자가 실제 장치로 결정한다.
struct NewFortManagerDiagnostics {
    std::function<void(std::uint32_t)> templateAssertion; // Template.cpp:0x172의 기존 manager 진단이다.
    std::function<void(o::DataManagerInitializeAssertion)> initializeAssertion; // Datamanager.cpp/Snart.cpp의 선택된 초기화 진단이다.
};

// 지정한 같은20BYTE 헤더에 원문004bfba0을 실행한다. 존재 재조회/삭제/임시 소멸/최종 공개/35개 초기화를 분리하지 않는다.
// 호출 중 저장소와 파일 장치는 살아 있어야 하며 fileName은 같은 작업 namespace의 경로여야 한다.
void CreateNewEditorFortManager(CampaignTemplateStorage& storage,const EditorFiles& files,
    std::uint32_t slot,std::string_view fileName,std::uint32_t argument,const NewFortManagerDiagnostics& diagnostics={});
}
