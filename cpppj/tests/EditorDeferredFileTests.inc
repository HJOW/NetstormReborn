// 실제 Editor는 최초 Fort 파싱 없이 최종 작업 파일의 raw 설치와 늦은 객체 child를 사용한다.
#include "platform/Utf8Path.h" // 한글 작업 경로의 실제 UTF-8/W 장치 선언을 직접 사용한다.
// 저장 객체 수는 파일의 명시 레코드에서 읽으며 World의 생성 결과를 기대값으로 재사용하지 않는다.
std::size_t DeferredStoredObjects(const FortTemplate& fort) {
    std::size_t count{}; // 입력 파일이 저장한 레코드 수이며 지형 자동 생성 수와 구별한다.
    for (const auto& chunk:fort.chaff.chunks) count+=chunk.objects.size(); // Chaff의 실제 저장 순서다.
    for (const auto& region:fort.territories) for (const auto& chunk:region.chunks) count+=chunk.objects.size(); // 각 Terr의 저장 레코드를 합한다.
    return count;
}
// 빈 입력/다른 파일의 오래된 해석 자료 모두 최종 선택된 작업 파일에서 다시 해석한다.
TEST_CASE(EditorDeferredFile_EmptyAndStaleProjectionUseFinalRawFile) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);const auto input=assets.files.Read("d/TEST01.fort");const auto expected=FortTemplate::Parse(input,assets.game.TypeTable());const auto expectedObjects=DeferredStoredObjects(expected);CHECK(expectedObjects>0);
    for (const bool empty:{true,false}) { // 두 최초 투영은 같은 최종 TEST01 raw 파일을 독립 Session에 설치한다.
        std::shared_ptr<CampaignTemplateStorage> storage; // 실제 같은 manager owner를 생성 전부터 관측한다.
        WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
            const auto path=work.ResolvePath("d/Checkpoint385.fort");work.Write(netstorm::platform::PathToUtf8(path),input);std::size_t reads{};auto selected=checkpoint;const auto read=selected.load.read;
            selected.load.read=[&](auto name) { ++reads;CHECK(storage->OwnedImages()==1 && ExistingEditorTemplateValid(*storage,1) && name==netstorm::platform::PathToUtf8(path));return read(name); }; // manager/readonly를 공개한 뒤의 실제 reader만 센다.
            session.InstallExistingEditorCheckpoint(selected,client.ReadOnlyForts(),work);const auto& actual=storage->Template(1);
            CHECK(reads==1 && session.ExistingEditorCheckpointReady() && session.IslandCount()>0);std::size_t matched{}; // 완성 레코드 사본 대신 실제 저장 좌표/종류의 현재 월드를 확인한다.
            for (std::size_t chunk=0;chunk<expected.chaff.chunks.size();++chunk) for (const auto& object:expected.chaff.chunks[chunk].objects) { // 원문 Chaff의 y/x 순서와 nibble 좌표를 독립 입력에서 읽는다.
                if (!object.type || static_cast<std::uint32_t>(object.type)==session.Core().bridgeState.islandType) continue;
                const auto x=static_cast<float>((chunk%16)*16+object.CellX()),y=static_cast<float>((chunk/16)*16+object.CellY());const auto found=session.World().Find(x,y,static_cast<std::uint32_t>(object.type));CHECK(found.value!=0);++matched;
            }CHECK(matched>0);if (empty) CHECK(actual.chaff.chunks.empty()); // 원시 경로가 빈 최초 객체 사본을 보충해 우회하지 않는다.
            CHECK(session.Core().pool.FreeCount()+expectedObjects<session.Core().pool.Capacity());CHECK(!actual.Section("State").empty() && !(actual.Section("State")[0]&1U));CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); })); // checkpoint는 아직 최종 State 확정/loaded 공개가 아니다.
        },false,nullptr,{},[&](Client&,RawCampaignSessionServices& services) { storage=services.templateStorage;services.missionFortImage.reset(); },empty);
    }
}
// 잘못된 서명은 실제 거부/반납을, 잘못된 객체는 앞선 파일/Subscriber/지형의 부분 효과를 유지한다.
TEST_CASE(EditorDeferredFile_SignatureAndObjectErrorsKeepNativeBoundaries) {
    for (const bool signature:{true,false}) { // 실제 작업 파일에만 두 손상 입력을 만들고 보호 원문은 읽기만 한다.
        std::shared_ptr<CampaignTemplateStorage> storage;
        WithEditorInstallSession([&](RawCampaignSession& session,Client& client,const auto& checkpoint,EditorFiles& work) {
            const auto path=netstorm::platform::PathToUtf8(work.ResolvePath("d/Checkpoint385.fort"));auto input=work.TryRead(path).value();if (signature) input[0]=0;else { const auto at=CheckpointSection(input,4);CHECK(at+1<input.size());input[at+1]=0; }work.Write(path,input);
            auto selected=checkpoint;std::size_t reads{},rejects{},titles{},configs{};bool invalid{};const auto read=selected.load.read;const auto title=selected.file.title;const auto configure=selected.fort.setConfig;
            selected.load.read=[&](auto name) { ++reads;CHECK(storage->OwnedImages()==1 && ExistingEditorTemplateValid(*storage,1));return read(name); };
            selected.load.rejectFile=[&](auto name) { ++rejects;CHECK(signature && reads==1 && !titles && !configs);work.Delete(name); };
            selected.file.title=[&] { ++titles;title(); };selected.fort.setConfig=[&](auto key,auto value) { ++configs;configure(key,value); };
            selected.file.outside=[&](auto branch) { invalid=branch==ExistingEditorInstallOutside::InvalidFile;throw CheckpointStop("deferred-invalid-file"); };
            bool failed{};try { session.InstallExistingEditorCheckpoint(selected,client.ReadOnlyForts(),work); }catch (const CheckpointStop& error) { CHECK(signature && std::string_view(error.what())=="deferred-invalid-file");failed=true; }catch (const std::logic_error& error) { CHECK(!signature && std::string_view(error.what())=="원본 Objects Template.cpp 진단 890");failed=true; }
            CHECK(failed && reads==1 && !session.ExistingEditorCheckpointReady());
            if (signature) CHECK(invalid && rejects==1 && !titles && !configs && storage->OwnedImages()==0 && !work.TryRead(path));
            else CHECK(!invalid && !rejects && titles==2 && configs>=1 && storage->OwnedImages()==1 && session.CurrentSubscriber()==7 && session.IslandCount()>0 && session.Core().pool.FreeCount()<session.Core().pool.Capacity());
            CHECK(CheckpointRejects<std::logic_error>([&] { session.SnapshotFort(1); })); // 부분 실패로 입력/저장 관문을 열지 않는다.
        },false,nullptr,{},[&](Client&,RawCampaignSessionServices& services) { storage=services.templateStorage;services.missionFortImage.reset(); },true);
    }
}

