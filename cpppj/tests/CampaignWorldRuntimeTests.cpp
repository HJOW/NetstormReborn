// 동일 보호 미션 설치 장면을 공유하며 기존172객체 검사를 유지한다.
#include "CampaignInstallSupport.h"
// 실제 최초 미션의172 저장 객체와 정상 생성 지면을 한 번 설치한다.
TEST_CASE(CampaignWorldRuntime_ProtectedWarBeginsInstallation) {
    InstallScene scene;CHECK(&scene.runtime->Pool()==&scene.core.pool);scene.runtime->Load();
    CHECK(scene.objectState.loadingDepth==0 && scene.core.bookkeeping.depth==0);
    CHECK(scene.islands->Count()==2 && scene.priestState.priests.priests.count==2);
    CHECK(scene.assetState.temples.count==2 && scene.assetState.geysers.count==14);
    CHECK(scene.player->Technology(1)!=0 && Get(scene.players,0xb4+0x78)!=127 && Get(scene.players,2*0xb4+0x78)!=127);
    CHECK(scene.streams[1].position==120 && scene.external.back()=="카메라");
    CHECK(scene.handle[0]=='Y' && scene.players[0xb4+0xc]=='Y');
    std::array<std::uint32_t,188> expected{},actual{}; // 저장파일과 실제 활성 자산의 타입별 수를 별도로 센다.
    std::uint32_t savedCount=0,contentCount=0; // 저장172객체와 실제 Form6을 자산 수에 혼합하지 않는다.
    // 원본 Terr00..19의 저장 순서로 입력 레코드를 센다. 이 수는 Native 재관찰 수가 아니다.
    for (const auto& section:scene.templates[1].territories)
        // 원본 해석 자료의 청크와 내부 저장 객체를 그대로 순회한다.
        for (const auto& chunk:section.chunks) for (const auto& object:chunk.objects) { ++expected[object.type];++savedCount; }
    // Chaff에 저장된 다리/간헐천/noIsland를 같은 입력 수에 포함한다.
    for (const auto& chunk:scene.templates[1].chaff.chunks) for (const auto& object:chunk.objects) { ++expected[object.type];++savedCount; }
    // 실제 현재 월드의 활성 자산을 세며 생성한 지면/받침도 별도 번호로 남긴다.
    for (const auto& object:scene.world.Objects()) ++actual[object.object.type];
    // 저장 타입은 최초 설치 후 모두 존재해야 한다. generated97/121/94/95/155와 Player152는 저장수에 넣지 않는다.
    for (std::size_t type=0;type<expected.size();++type) if (expected[type]) CHECK(actual[type]==expected[type]);
    // 실제 contained6의 할당과 살아 있는 부모 링크를 검사한다. 내용물을 살아 있는 워커로 세지 않는다.
    for (std::uint32_t id=5;id<scene.core.pool.Capacity();++id) {
        const auto sid=Sid{static_cast<std::uint16_t>(id)};const auto raw=scene.core.pool.Slot(sid);
        if (!(raw[11]&1) && raw[10]==6) { ++contentCount;CHECK(Get(raw,18,2)==133 && scene.contentForms->Linked(sid)); }
    }
    CHECK(savedCount==172 && contentCount==2 && actual[133]==0 && actual[152]==1);
    CHECK(Throws([&] { scene.runtime->Load(); }));
    std::printf("캠페인 실제 설치: 활성 자산 %zu, Kernel %zu, 외부 요청 %zu\n",scene.world.Objects().size(),scene.core.kernel.Size(),scene.external.size());
}
// 실제 Player 생성 뒤 Territory 부족을 보고하며 부분 설치의 재실행을 거부한다.
TEST_CASE(CampaignWorldRuntime_RejectsMissingTerritoryAfterPlayer) {
    InstallScene scene;scene.runtime.reset();const auto free=scene.core.pool.FreeCount();
    scene.streams[1].bytes={};CHECK(Throws([&] { scene.runtime=scene.MakeRuntime();scene.runtime->Load(); }));
    CHECK(scene.core.pool.FreeCount()<free); // 실제Player 초기 생성 이후120BYTE 부족을 보고했으며 로드 완료로 바꾸지 않았다.
    CHECK(Throws([&] { scene.runtime->Load(); }));
}
// 현재 Template9 배열의 잘못된 연결은 실제 초기Player 생성 전 거부한다.
TEST_CASE(CampaignWorldRuntime_RejectsWrongTemplateCountBeforeCreation) {
    InstallScene scene;const auto free=scene.core.pool.FreeCount();CHECK(Throws([&] { scene.MakeRuntime(8); }));
    CHECK(scene.core.pool.FreeCount()==free && scene.core.kernel.Size()==0);
}
