// 실제497f20 전후 바이트를 현재 Client owner에 공급하며 계산한 기대 좌표를 만들지 않는다.
TEST_CASE(CameraCenter_Native140PreservesCurrentViewsAndOnlyWritesTarget) {
    std::size_t count{}; // 서로 다른 원문 입력 행의 실제 대조 수다.
    const auto fixture=std::filesystem::path(NETSTORM_CAMERA_VIEW_FIXTURE).parent_path()/"camera-center-x86.tsv"; // 기존 fixture 기준 디렉토리의 새 독립 관측이다.
    for (const auto& row:LoadFixture(fixture.string().c_str()).rows) { // 전체 Renderer160BYTE 중 현재view64·target20을 같은 자료형으로 복원한다.
        CHECK(row.size()==10);ClientCameraGlobals camera;camera.RendererView()=CameraImage(std::string_view(row[5]).substr(0,128));camera.Snapshot()=CameraImage(row[6]);
        std::array<std::uint8_t,20> target{}; // 원문+48..5b의 한계/목표/zoom20BYTE다.
        for (std::size_t index=0;index<target.size();++index) target[index]=static_cast<std::uint8_t>(std::stoul(row[5].substr((0x48+index)*2,2),nullptr,16)); // 관측 바이트를 계산 없이 옮긴다.
        static_assert(sizeof(RawCameraTargetState)==20);std::memcpy(&camera.Target(),target.data(),target.size());camera.Denominator().current=23;camera.Denominator().defaultValue=17;
        auto revision=Number(row[4]);camera.RequestCenter(std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(row[2],nullptr,16))),std::bit_cast<float>(static_cast<std::uint32_t>(std::stoul(row[3],nullptr,16))),std::stoi(row[0]),std::stoi(row[1]),revision);
        CHECK(CameraBytes(camera.RendererView())==row[7].substr(0,128) && CameraBytes(camera.Snapshot())==row[8]);
        CHECK(Hex({reinterpret_cast<const std::uint8_t*>(&camera.Target()),sizeof(RawCameraTargetState)})==row[7].substr(0x48*2,40));
        CHECK(revision==Number(row[9]) && camera.Denominator().current==23 && camera.Denominator().defaultValue==17);++count;
    }
    CHECK(count==140);
}
