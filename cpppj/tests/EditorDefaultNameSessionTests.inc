// 같은 실제 Session checkpoint 앞에 기본 이름 선택을 연결하고 기존 파일/월드까지 이어지는지 확인한다.
#include "client/FortCurrentNameDevices.h"

TEST_CASE(EditorDefaultName_ActualSessionContinuesIntoExistingCheckpoint) {
    WithEditorInstallSession([](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        auto effects=checkpoint;std::string current,pending;std::size_t defaults{},paths{};client.Configuration().Set("newFort","Checkpoint385");
        effects.fort.currentName=[&]()->std::string_view { return current; };effects.fort.pendingName=[&]()->std::string_view { return pending; };effects.fort.setName=[&](auto name) { current=name; };
        effects.fort.defaultName=[&] { ++defaults;SelectConfiguredNewFort(client.Configuration(),effects.fort.setName); };
        const auto originalPath=effects.file.path;effects.file.path=[&] { CHECK(defaults==1 && current=="Checkpoint385");++paths;return originalPath(); };
        session.InstallExistingEditorCheckpoint(effects,client.ReadOnlyForts(),work);CHECK(defaults==1 && paths>=2 && session.ExistingEditorCheckpointReady() && session.IslandCount()>0); // 최종 loaded/State 공통 확정은 기존 후반부에 남긴다.
    });
}
