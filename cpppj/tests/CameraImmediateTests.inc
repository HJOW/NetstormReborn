// 즉시 부모/목표와 실제497960을 결합해 Native 전체 현재 상태·효과·순서를 대조한다.
TEST_CASE(CameraImmediate_Native168ParentTargetCommitAndCurrentEffects) {
    const auto path=std::filesystem::path(NETSTORM_CAMERA_COMMIT_FIXTURE).parent_path()/"camera-immediate-x86.tsv";std::ifstream file(path);CHECK(file.good());std::string line;std::size_t count{}; // 같은 원문 입력의 읽기 전용 fixture다.
    while (std::getline(file,line)) { // 각 행의 독립 초기 Renderer/snapshot/target/dirty/전역을 그대로 준비한다.
        if (line.empty() || line.front()=='#') continue;std::istringstream row(line);CommitInput input{};
        for (auto& value:input) row>>value; // 입력28개에 기대값을 섞지 않는다.
        std::uint32_t x{},y{},revision{},expectedRevision{};std::string renderer,snapshot,target,dirty,globals,events,entry;row>>x>>y>>revision>>renderer>>snapshot>>target>>dirty>>globals>>events>>expectedRevision>>entry;CHECK(!row.fail());if (row.fail()) continue;
        CommitScene scene(input);std::string actualEntry;scene.camera.CenterImmediately(std::bit_cast<float>(x),std::bit_cast<float>(y),scene.width,scene.height,scene.worldRedraw,revision,[&] {
            actualEntry=Words(std::array{static_cast<std::uint32_t>(scene.camera.Target().pixels[0]),static_cast<std::uint32_t>(scene.camera.Target().pixels[1]),scene.worldRedraw,revision});
            CommitCameraTarget(scene.State(),scene.Globals(),scene.Hooks()); // 원문 부모의 현재 전역 쓰기가 끝난 뒤 실제 적용 본체를 실행한다.
        });
        const auto& goal=scene.camera.Target();CHECK(actualEntry==entry && revision==expectedRevision);
        CHECK(Words(std::bit_cast<std::array<std::uint32_t,16>>(scene.camera.RendererView()))==renderer && Words(std::bit_cast<std::array<std::uint32_t,16>>(scene.camera.Snapshot()))==snapshot);
        CHECK(Words(std::array{static_cast<std::uint32_t>(goal.limits[0]),static_cast<std::uint32_t>(goal.limits[1]),static_cast<std::uint32_t>(goal.pixels[0]),static_cast<std::uint32_t>(goal.pixels[1]),goal.zoom,scene.redraw})==target && scene.DirtyWords()==dirty);
        CHECK(Words(std::array{scene.flags,static_cast<std::uint32_t>(scene.denominator),static_cast<std::uint32_t>(scene.viewport.x),static_cast<std::uint32_t>(scene.viewport.y),static_cast<std::uint32_t>(scene.viewport.width),static_cast<std::uint32_t>(scene.viewport.height),static_cast<std::uint32_t>(scene.width),static_cast<std::uint32_t>(scene.height),static_cast<std::uint32_t>(scene.mouse.x),static_cast<std::uint32_t>(scene.mouse.y),scene.worldRedraw})==globals);
        if (scene.events!=events) std::printf("즉시 카메라 행 %zu 기대=%s 실제=%s\n",count,events.c_str(),scene.events.c_str());CHECK(scene.events==events);++count;
    }
    CHECK(count==168);
}

// 실제 Client 장치가 빠지면 원문 선행 쓰기를 보존한 채 명확히 실패하며 두setter로 성공을 만들지 않는다.
TEST_CASE(CameraImmediate_MissingCommitPreservesNativePrefixAndRejectsFallback) {
    CommitScene scene(DefaultInput());const auto before=scene.camera.RendererView(),snapshot=scene.camera.Snapshot();scene.worldRedraw=0xffffffffU;std::uint32_t revision=0xffffffffU;bool failed{};
    try { scene.camera.CenterImmediately(100.25F,101.75F,scene.width,scene.height,scene.worldRedraw,revision,{}); }catch (const std::logic_error&) { failed=true; }
    CHECK(failed && scene.worldRedraw==0 && revision==0 && (scene.camera.Target().pixels==std::array<std::int32_t,2>{1092,735}));
    CHECK(Words(std::bit_cast<std::array<std::uint32_t,16>>(scene.camera.RendererView()))==Words(std::bit_cast<std::array<std::uint32_t,16>>(before)) && Words(std::bit_cast<std::array<std::uint32_t,16>>(scene.camera.Snapshot()))==Words(std::bit_cast<std::array<std::uint32_t,16>>(snapshot)));
}
