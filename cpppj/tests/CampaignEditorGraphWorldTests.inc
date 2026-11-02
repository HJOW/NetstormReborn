// 실제 다중 섬 편집 월드의 Graph 입력/출력을 원본 x86에 넘길 읽기 전용 검사 자료로 저장한다.
namespace { // 검사 출력은 명시 경로에만 쓰며 제품 저장 파일이나 기본 프로필에는 섞지 않는다.
// 원본 폭의 정수를 little endian으로 저장하며 호스트 구조체 패딩을 출력하지 않는다.
void GraphWorldNumber724(std::ostream& output,std::uint32_t value,std::size_t width) {
    for (std::size_t byte=0;byte<width;++byte) output.put(static_cast<char>(value>>(byte*8))); // 지정한 필드 폭만 순서대로 쓴다.
}
// 원본과 공유하는 실제 바이트 배열만 저장하고 파일 쓰기 실패를 검사 실패로 전달한다.
void GraphWorldBytes724(const std::filesystem::path& path,std::span<const std::uint8_t> bytes) {
    std::ofstream output(path,std::ios::binary);output.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));if (!output) throw std::runtime_error("Graph 실제 월드 바이트 출력 실패");
}
// 같은 풀/타입/프레임/전체 해시/점유/Graph 표·스택을 관측하며 게임 함수를 추가 호출하지 않는다.
void GraphWorldDump724(RawCampaignSession& session,const std::filesystem::path& path,std::uint32_t reset) {
    if (std::filesystem::exists(path)) throw std::runtime_error("Graph 최초 월드 관측 경로 중복");std::filesystem::create_directories(path);const auto core=session.Core();
    GraphWorldBytes724(path/"pool.bin",core.pool.Bytes());GraphWorldBytes724(path/"spots.bin",core.spots);
    std::ofstream heads(path/"heads.bin",std::ios::binary),records(path/"records.bin",std::ios::binary),stack(path/"stack.bin",std::ios::binary),types(path/"types.tsv",std::ios::binary),meta(path/"meta.tsv",std::ios::binary);
    for (int level=0;level<4;++level) for (const auto sid:core.hash.Entries(level)) GraphWorldNumber724(heads,sid,2); // 실제4단계 배열의 순서와 WORD 폭을 유지한다.
    for (const auto& record:core.graph.Records()) { // reserved WORD도 입력/출력으로 보존하며 의미를 새로 붙이지 않는다.
        GraphWorldNumber724(records,static_cast<std::uint16_t>(record.surfaces),2);GraphWorldNumber724(records,static_cast<std::uint16_t>(record.inUse),2);GraphWorldNumber724(records,static_cast<std::uint16_t>(record.reserved),2);
    }
    for (const auto value:core.graph.FloodStack()) GraphWorldNumber724(stack,value,4); // 비활성 흔적까지 실제4096DWORD를 남긴다.
    types<<"type\tflags1\tflags2\tfootX\tfootY\tcodes\n";constexpr char digits[]="0123456789abcdef"; // 프레임 코드의 실제4BYTE를 텍스트 hex로 옮길 표다.
    for (std::size_t number=0;number<core.types.size();++number) { // 빈 내장 타입도 포함해 같은188개 TYPE의 현재 입력을 빠짐없이 쓴다.
        const auto& type=core.types[number];types<<number<<'\t'<<type.flags1<<'\t'<<type.flags2<<'\t'<<type.footX<<'\t'<<type.footY<<'\t';
        const auto codes=core.frames[number].Codes();if (codes.empty()) types<<'-';
        for (const auto& code:codes) for (const auto value:std::array{code.side,code.variant,code.number,code.flags}) types<<digits[value>>4]<<digits[value&15]; // 코드 배열 순서를 정렬하거나 변환하지 않는다.
        types<<'\n';
    }
    meta<<"capacity\t"<<core.pool.Capacity()<<"\nreset\t"<<reset<<'\n';
    if (!heads || !records || !stack || !types || !meta) throw std::runtime_error("Graph 실제 월드 표 출력 실패");
}
}

// 서로 떨어진 두 실제 섬의 재구성·다리 연결/소유자 변경/절단을 독립 PE와 대조할 입력으로 제공한다.
TEST_CASE(CampaignEditorGraphWorld_TwoActualIslandsJoinOwnerCutAndRebuild) {
    WithEditorIslandWindowSession480(true,[&](RawCampaignSession& session,Client&) {
        ChunkMap preview(14,14);CHECK(session.BuildEditorIslandMap(preview));std::array<std::uint8_t,6> first{8,3,0x55,0x65,0,0},second{8,3,0,0x66,0,0}; // 첫 후보는723에서 원본과 대조한 입력이고 두 번째는 떨어진 위치/다른 성장seed다.
        CHECK(session.CanPlaceEditorIsland(preview,first) && session.DropEditorIsland(preview,first));CHECK(session.BuildEditorIslandMap(preview));
        CHECK(session.CanPlaceEditorIsland(preview,second) && session.DropEditorIsland(preview,second));CHECK(session.IslandCount()==2);
        const auto rebuild=[&](std::string_view name,std::uint32_t reset,std::size_t expected) { // 각 편집 직후의 현재 물리를 그대로 입력으로 보관하고 실제 재구성 결과의 성분 수도 확인한다.
            const auto core=session.Core();const auto count=[&] { // 재구성 전에 편집 자체의 병합/분할이 이미 적용됐는지도 별도로 확인한다.
                std::size_t active{};for (std::size_t number=1;number<Graph::kCount;++number) if (core.graph.Records()[number].inUse) ++active;return active; // 예약0번을 제외한 실제 활성 성분만 센다.
            };
            CHECK(count()==expected);
            const auto directory=std::getenv("NETSTORM_EDITOR_GRAPH_WORLD_DIR");
            if (directory) GraphWorldDump724(session,std::filesystem::path(directory)/name/"before",reset);
            core.graph.Rebuild(reset!=0);CHECK(count()==expected);
            if (directory) GraphWorldDump724(session,std::filesystem::path(directory)/name/"after",reset);
        };
        rebuild("incremental",0,2);rebuild("reset",1,2);
        const auto core=session.Core();std::map<std::uint32_t,std::vector<std::pair<int,int>>> islands;
        for (const auto& item:session.World().Objects()) { // 정상 Graph에 속한 실제 지면의 좌표만 다리 양 끝 후보로 삼는다.
            const auto raw=core.pool.Slot(item.sid);if (raw[10]==97 && raw[30]>0 && raw[30]<Graph::kCount) islands[raw[30]].emplace_back(static_cast<int>(item.object.x),static_cast<int>(item.object.y));
        }
        CHECK(islands.size()==2);if (islands.size()!=2) return;const auto a=islands.begin(),b=std::next(a);std::pair<int,int> start,end;int distance=1000;
        for (const auto& firstPoint:a->second) for (const auto& secondPoint:b->second) { // 검사 경로는 두 실제 섬 사이 최단 Manhattan 쌍이며 원본 UI의 자동 배치를 가장하지 않는다.
            const auto candidate=std::abs(firstPoint.first-secondPoint.first)+std::abs(firstPoint.second-secondPoint.second);
            if (candidate<distance) { distance=candidate;start=firstPoint;end=secondPoint; }
        }
        const auto frames=core.frames[82].Codes();CHECK(!frames.empty() && frames[0].side=='A' && frames[0].variant=='P' && frames[0].flags==0);std::vector<Sid> bridges;int x=start.first,y=start.second;
        while (x!=end.first || y!=end.second) { // 네 방향을 연결하는 실제 AP 정상 다리 프레임0으로 두 섬 사이 한 줄을 만든다.
            if (x!=end.first) x+=end.first>x ? 1 : -1;else y+=end.second>y ? 1 : -1;
            if (core.hash.Entries(0)[static_cast<std::size_t>(y*256+x)]) continue;
            const auto bridge=session.World().Create(82,2);core.frame.Set(bridge,0,0);core.owner.Set(bridge,1);session.World().Pop(bridge,static_cast<float>(x),static_cast<float>(y),0);bridges.push_back(bridge);
        }
        CHECK(bridges.size()>2);if (bridges.size()<=2) return;rebuild("joined",1,1);
        session.SetObjectOwner(bridges.front(),2);CHECK(core.pool.Slot(bridges.front())[34]==2);rebuild("owner",0,1);
        const auto middle=bridges[bridges.size()/2];CHECK(session.World().Destroy(middle));CHECK(core.pool.Slot(middle)[11]&1U);rebuild("cut",1,2);
    });
}
