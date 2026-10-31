// 게임/창/실제 통신 없이 같은 작업본과 Session에서 손상 파일의 원문 반복을 확인한다.
#include "platform/Utf8Path.h"

TEST_CASE(EditorInvalidFile_RepeatedRejectsReachCurrentValidFile) { // 손상된 선택 파일과 기본 파일을 순서대로 거부하고 세 번째 실제 파일을 설치한다.
    std::shared_ptr<CampaignTemplateStorage> storage; // 실제 Session과 같은 헤더/manager를 외부 진단 위치에서 조회한다.
    WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
        const auto first=netstorm::platform::PathToUtf8(work.ResolvePath("d/Checkpoint385.fort")),second=netstorm::platform::PathToUtf8(work.ResolvePath("d/Default665.fort")),last=netstorm::platform::PathToUtf8(work.ResolvePath("d/Recovered665.fort"));
        const auto good=work.TryRead(first).value();auto bad=good;bad[0]=0;work.Write(first,bad);work.Write(second,bad);work.Write(last,good);
        auto selected=checkpoint;std::string path=first,current="Checkpoint385";std::size_t reads{},rejects{},selections{},erases{};std::vector<std::string> logs; // 원문 경계의 현재 이름/경로만 명시 입력한다.
        selected.file.path=[&]()->std::string_view { return path; };selected.fort.currentName=[&]()->std::string_view { return current; };const auto read=selected.load.read;
        selected.load.read=[&](auto name) { ++reads;return read(name); };selected.load.rejectFile=[&](auto name) { ++rejects;work.Delete(name); };
        selected.invalid.log=[&](auto channel,auto text) { CHECK(channel==29 && storage->OwnedImages()==0);const auto header=storage->Headers().subspan(20,20);CHECK(std::all_of(header.begin(),header.end(),[](auto byte) { return byte==0; }));logs.emplace_back(text); };
        selected.invalid.isDefaultName=[&] { return path==second ? 1 : 0; }; // 첫 손상 파일과 현재 기본 파일을 구별하는 외부 장치 입력이다.
        selected.invalid.erase=[&](auto name) { CHECK(name==second && logs.size()==3);++erases;work.Delete(name); }; // 삭제 뒤 재조회는 실제 파일의 대소문자 표기 대신 정규화된 별칭일 수 있다.
        selected.fort.defaultName=[&] { ++selections;if (selections==1) { path=second;current="Default665"; }else if (selections==2) { path=last;current="Recovered665"; }else throw std::logic_error("예상 밖 Editor 파일 재시도"); };
        session.InstallExistingEditorCheckpoint(selected,client.ReadOnlyForts(),work);
        CHECK(reads==3 && rejects==2 && selections==2 && erases==1 && logs==std::vector<std::string>({"Game file was invalid.\n","Game file was invalid.\n","Killing game file.\n"}));
        CHECK(session.ExistingEditorCheckpointReady() && session.IslandCount()>0 && session.CurrentSubscriber()==7 && storage->OwnedImages()==1 && storage->Name(1)==last);
        CHECK(!work.Exists(first) && !work.Exists(second) && work.Exists(last)==5 && client.Configuration().Configuration().Get("lastEditFort")==std::optional<std::string>("Recovered665")); // 최종 이름/현재 manager가 실제 저장 꼬리까지 이어진다.
    },false,nullptr,{},[&](Client&,RawCampaignSessionServices& services) { storage=services.templateStorage;services.missionFortImage.reset(); },true);
}

TEST_CASE(EditorInvalidFile_DefaultRejectThenMissingCreatesActualNewFort) { // 기본 파일도 잘못됐으면 제거 뒤 같은 반복의 부재 분기로 실제 새 자료를 만든다.
    WithNewFortSession([](auto& session,Client& client,const auto& checkpoint,EditorFiles& work,CampaignTemplateStorage& storage,auto& devices) {
        const auto path=netstorm::platform::PathToUtf8(work.ResolvePath("d/Fresh472.fort"));const std::array<std::uint8_t,2> bad{0,0};work.Write(path,bad);client.Configuration().Set("newFort","Fresh472");
        auto selected=checkpoint;std::size_t rejects{},selections{},erases{};std::vector<std::string> logs; // 새 파일 생성 결과는 실제 NewFort/Template 본체에서만 얻는다.
        selected.load.rejectFile=[&](auto name) { ++rejects;CHECK(NormalizeGamePath(name)==NormalizeGamePath(path));work.Delete(name); };
        selected.invalid.log=[&](auto channel,auto text) { CHECK(channel==29 && storage.OwnedImages()==0);logs.emplace_back(text); };
        selected.invalid.isDefaultName=[&] { CHECK(client.Configuration().PathSpec("newFort")==devices.current);return 1; };
        selected.invalid.erase=[&](auto name) { CHECK(NormalizeGamePath(name)==NormalizeGamePath(path));++erases;work.Delete(name); }; // 삭제 뒤 재조회는 실제 파일의 대소문자 표기 대신 정규화된 별칭일 수 있다.
        selected.fort.defaultName=[&] { SelectConfiguredNewFort(client.Configuration(),[&](auto name) { ++selections;devices.current=name; }); };
        session.InstallExistingEditorCheckpoint(selected,client.ReadOnlyForts(),work);
        CHECK(rejects==1 && selections==1 && erases==1 && logs==std::vector<std::string>({"Game file was invalid.\n","Killing game file.\n"}));
        CHECK(session.ExistingEditorCheckpointReady() && session.IslandCount()==0 && storage.OwnedImages()==1 && storage.State(1).bytes[17]==2 && work.Exists(path)==5 && session.CurrentSubscriber()==7);
        CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); })); // checkpoint 정상 복귀와 최종 State/UI 공개를 구별한다.
    });
}

TEST_CASE(EditorInvalidFile_ClearFailureKeepsSuppressionAndDoesNotSelect) { // 반납이 실패하면 원문의 선행+8 DWORD 쓰기만 남고 후속 로그/이름 선택은 실행하지 않는다.
    std::array<std::uint8_t,20> header;header.fill(0x6d);std::uint32_t alternate=77;std::size_t loads{},clears{},logs{},selections{}; // 미선택 경계는 성공 장치로 채우지 않는다.
    NewFortMissingFileHooks missing;missing.path=[] { return std::string_view("bad.fort"); };missing.exists=[](auto) { return 5; };
    ExistingEditorFileHooks present;present.path=missing.path;present.owner=[] { return 1U; };present.load=[&](auto slot,auto name) { CHECK(slot==1 && name=="bad.fort");++loads; };present.valid=[](auto) { return 0; };
    InvalidEditorFileHooks invalid;invalid.header=[&](auto slot) { CHECK(slot==1);return std::span(header); };invalid.clear=[&](auto slot) { CHECK(slot==1 && header[8]==1 && header[9]==0 && header[10]==0 && header[11]==0);++clears;throw CheckpointStop("invalid-clear-failure"); };invalid.log=[&](auto,auto) { ++logs; };invalid.defaultName=[&] { ++selections; };
    CHECK(CheckpointRejects<CheckpointStop>([&] { InstallEditorFortFile(alternate,missing,present,invalid); }));CHECK(alternate==77 && loads==1 && clears==1 && !logs && !selections);
    for (std::size_t byte=0;byte<header.size();++byte) CHECK(header[byte]==(byte==8 ? 1 : byte>8 && byte<12 ? 0 : 0x6d)); // 주변 stale BYTE를 자동으로 초기화하거나 복원하지 않는다.
}
