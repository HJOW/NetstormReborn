// 같은 실제 메뉴/트리/비트맵을 사용해 미니맵의 기본 영역과 부분 출력을 확인한다.
namespace { // 장치의 명시 카메라 입력과 현재 실제 UI 노드를 함께 보관한다.
struct MinimapDirtyScene {
    MenuClient client; // 실제 보호 자산/폰트/공통 Gump와 외부 영역 수신기다.
    std::shared_ptr<RawSurfaceStorage> storage; // Session을 만들지 않는 같은 실제 Pool/TYPE/Hash 물리다.
    MinimapGeometryView view{{std::bit_cast<std::uint32_t>(120.0F),std::bit_cast<std::uint32_t>(80.0F)},{84,0,320,240},16,11}; // 원문 현재 카메라/지도/칸 배율의 명시 입력이다.
    RawMinimapControls* mini{};std::uint32_t token{},mode{},centers{},moves{}; // 실제 현재 처리기와 입력/장치 호출 수다.
    InputEvent pointer{InputCode::kLeftButton,50,187}; // 실제 폴링/드래그의 현재 화면 좌표다.
    bool hideOnCenter{}; // 카메라 장치가 반환하기 전에 표시를 바꾸는 경계 입력이다.
    std::vector<std::array<std::size_t,2>> centerOrder; // 카메라 호출 당시 영역 수와 실제 캡처 존재 여부다.
    MinimapDirtyScene() { // 원본 생성/부착/최초 전체 bitmap을 실제 제품 경계로 준비한다.
        client.Resize(320,240);storage=MakeRawSurfaceStorage(client.assets);RawMinimapHooks hooks;hooks.view=[this] { return view; };hooks.inputMode=[this] { return mode; };hooks.poll=[this] { return pointer; };
        hooks.center=[this](float x,float y) { centerOrder.push_back({client.redrawRectangles.size(),client.buttons.HasCapture() ? 1U : 0U});++centers;view.camera={std::bit_cast<std::uint32_t>(x),std::bit_cast<std::uint32_t>(y)};if (hideOnCenter) mini->Node(token)->flags&=~1U; }; // 게임 카메라 계산을 흉내 내지 않고 반환 후 현재 표시를 읽는 순서만 관측한다.
        hooks.move=[this](float,float) { ++moves; };mini=&client.buttons.ConstructMinimap(storage,std::move(hooks));token=mini->RootToken();auto& node=*mini->Node(token);
        node.baseWidth=node.width=80;node.baseHeight=node.height=75;mini->Attach(token,10,150,client.buttons.RootToken(),0);mini->SetPalette(client.Palette());mini->Refresh();Clear();
    }
    ~MinimapDirtyScene() { client.buttons.DisableMinimap(); } // 카메라/장치 관측 필드보다 실제 연결 UI를 먼저 반납한다.
    void Clear() { client.redrawRectangles.clear();client.fullRequests=0; } // 구조 변경과 실제 영역 요청을 각 동작마다 분리한다.
    void CheckOne() { // 480270/4803f0/480440의 실제 기본+8(0)은 동일 영역 한 번이다.
        CHECK(client.fullRequests==0 && client.redrawRectangles.size()==1);if (client.redrawRectangles.size()!=1) return;
        const auto& [rect,flags]=client.redrawRectangles[0];CHECK(flags==0 && rect.left==10 && rect.top==150 && rect.right==90 && rect.bottom==225);
    }
};
}

TEST_CASE(MinimapDirty_ActualRefreshClickAndDragPreserveFullPixels) { // 카메라 outline/crop 변경을 받은 영역만 다시 그린다.
    MinimapDirtyScene scene;constexpr int width=320,height=240,pitch=327;constexpr std::uint8_t background=0x5a; // 행 padding을 포함하는 현재 논리 화면이다.
    std::array<std::uint32_t,256> logical{};
    for (std::size_t index=0;index<logical.size();++index) { const auto color=scene.client.assets.Palette().Color(static_cast<std::uint8_t>(index));logical[index]=color.red|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16); } // 원본 파일의 실제 RGB만 명암표에 공급한다.
    const auto palette=scene.client.Palette();const auto maps=BuildGumpShadeMaps(logical);std::vector<std::uint8_t> partial(pitch*height,background),whole=partial;scene.client.menus->Draw(partial,pitch,height,palette,maps);
    const auto compare=[&] { // 전달한 영역만 지우고 그린 결과를 독립 전체 출력과 대조한다.
        scene.CheckOne();
        for (const auto& [rect,flags]:scene.client.redrawRectangles) { // 영역 밖과 행 padding은 이전 바이트를 유지한다.
            CHECK(flags==0);const auto left=std::max(0,rect.left),right=std::min(width,rect.right);
            if (left<right) for (int y=std::max(0,rect.top);y<std::min(height,rect.bottom);++y) std::fill(partial.begin()+y*pitch+left,partial.begin()+y*pitch+right,background); // half-open 영역과 실제 표면의 교차만 비운다.
            scene.client.menus->Draw(partial,pitch,height,palette,maps,rect);
        }
        std::fill(whole.begin(),whole.end(),background);scene.client.menus->Draw(whole,pitch,height,palette,maps);CHECK(partial==whole);
    };
    scene.view.camera[0]=std::bit_cast<std::uint32_t>(140.0F);CHECK(scene.mini->Refresh());compare(); // 카메라 자동 추적의 실제480270 요청이다.
    scene.Clear();scene.client.buttons.RouteInput(scene.pointer);CHECK(scene.client.buttons.HasCapture() && (scene.mini->Node(scene.token)->flags&0x80U));CHECK(scene.centers==1 && (scene.centerOrder[0]==std::array<std::size_t,2>{0,0}));compare(); // 카메라 중심 뒤 Redraw/캡처/Idle 순서다.
    scene.Clear();scene.view.camera[0]=std::bit_cast<std::uint32_t>(160.0F);CHECK(!scene.mini->Refresh());CHECK(scene.client.fullRequests==0 && scene.client.redrawRectangles.empty()); // 드래그 동안 자동 crop 추적은 요청하지 않는다.
    for (const auto position:{ScreenPoint{105,187},ScreenPoint{50,145}}) { // 오른쪽/위쪽 바깥 드래그의 crop과 outline을 실제 UI에서 갱신한다.
        scene.Clear();scene.pointer.x=position.x;scene.pointer.y=position.y;scene.mini->Idle(scene.token);CHECK((scene.centerOrder.back()==std::array<std::size_t,2>{0,1}));compare();
    }
    scene.Clear();scene.pointer.code|=InputCode::kRelease;scene.client.buttons.RouteInput(scene.pointer);CHECK(!scene.client.buttons.HasCapture() && !(scene.mini->Node(scene.token)->flags&0x80U));CHECK(scene.client.fullRequests==0 && scene.client.redrawRectangles.empty()); // 해제는 원문처럼 새 그림 요청 없이 capture/Idle을 반납한다.
}

TEST_CASE(MinimapDirty_HiddenAndMoveModesUseCurrentVisibility) { // 숨김/선택 이동 분기의 불필요한 전체 갱신과 영역 요청을 막는다.
    MinimapDirtyScene scene;auto& node=*scene.mini->Node(scene.token);node.flags&=~1U;scene.view.camera[0]=std::bit_cast<std::uint32_t>(144.0F);CHECK(!scene.mini->Refresh());CHECK(scene.client.fullRequests==0 && scene.client.redrawRectangles.empty());
    node.flags|=1U;
    for (const auto mode:{2U,5U}) { // 원문 두 선택 이동 모드의 입력은 카메라/Redraw/캡처를 수행하지 않는다.
        scene.mode=mode;scene.Clear();CHECK(scene.mini->Handle(scene.token,InputCode::kLeftButton,50,187)==1);CHECK(scene.centers==0 && !scene.client.buttons.HasCapture() && !(node.flags&0x80U));CHECK(scene.client.fullRequests==0 && scene.client.redrawRectangles.empty());
    }
    CHECK(scene.moves==2);scene.mode=0;scene.hideOnCenter=true;scene.Clear();scene.client.buttons.RouteInput(scene.pointer);CHECK(scene.centers==1 && !(node.flags&1U) && scene.client.buttons.HasCapture());CHECK(scene.client.fullRequests==0 && scene.client.redrawRectangles.empty()); // 중심 장치 반환 뒤의 현재visible을 기본Redraw가 읽는다.
    scene.pointer.code|=InputCode::kRelease;scene.client.buttons.RouteInput(scene.pointer);CHECK(!scene.client.buttons.HasCapture() && !(node.flags&0x80U));
}
