// 실제 OS 색 할당과 Renderer의 정렬/dirty 표면까지 입자 출력이 이어지는지 검사한다.
#include "DudeSpawnSupport.h"
#include "client/ParticlePaletteMemory.h"
#include "client/RawCampaignParticles.h"
#include "client/VFXLine.h"
#include <windows.h>

using namespace netstorm::client;
namespace {
// 앞11개는 원본 GIFCLOUD 생성자 결과이고 마지막은 재할당 전의 명시 검사 표식이다.
constexpr std::array<std::uint32_t,12> kPaletteSeed{217,226,62,56,51,200,221,218,218,197,196,0xcafebabe};
// SHP 없이 같은 Renderer 정렬 항목에 픽셀을 쓰는 명시 검사 소비자다.
RenderSprite Procedure(std::int16_t depth,std::uint8_t color,std::vector<std::uint8_t>& order) {
    RenderSprite sprite;sprite.clip={2,1,10,7};sprite.order={1,1,depth};
    sprite.procedure=RenderProcedure{{0,0,12,8},[color,&order](std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
        CHECK(height==8);order.push_back(color);
        // renderer가 전달한 실제 dirty/항목 clip 밖은 건드리지 않는다.
        for (int y=clip.top;y<clip.bottom;++y) {
            // 각 행의 유효 픽셀에만 명시 색을 쓴다.
            for (int x=clip.left;x<clip.right;++x) pixels[static_cast<std::size_t>(y*pitch+x)]=color;
        }
    }};return sprite;
}
// 원본 fixture의 원문 바이트를 읽으며 픽셀/색 기대값은 계산하지 않는다.
std::vector<std::uint8_t> Decode(std::string_view hex) {
    if (hex.size()%2) throw std::runtime_error("입자 픽셀 fixture hex 폭 오류");
    std::vector<std::uint8_t> result;result.reserve(hex.size()/2);
    // 원문의 두 글자마다 바이트 하나를 복원한다.
    for (std::size_t index=0;index<hex.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(hex.substr(index,2)),nullptr,16)));
    return result;
}
// 입력 입자의 비정렬 float DWORD를 호스트 별칭 규칙에 어긋나지 않게 읽는다.
float Float(std::span<const std::uint8_t> bytes,std::size_t offset) { return std::bit_cast<float>(Get(bytes,offset)); }
}
// 실제 Draw→wrapper→VFX 원문30행의 전체 화면을 C++ Draw→VFX 출력과 대조한다.
TEST_CASE(FieryTailPixels_Native027f) {
    std::ifstream input(NETSTORM_FIERYTAIL_PIXELS_FIXTURE);if (!input) throw std::runtime_error("Fiery Tail 픽셀 fixture 없음");
    SidPool pool(OriginalEdition::Patch1078,32768,false);GameRandom random;std::string line;std::size_t count=0,extended=0;
    // 비교64비트 정밀도의 원문은 보존하고 제품 기본53비트 결과만 대조한다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();if (line.empty() || line.front()=='#') continue;
        const auto row=Split(line,'\t');if (row.size()!=6) throw std::runtime_error("입자 픽셀 fixture 열 수 오류");
        const auto precision=std::stoul(row[1]);if (precision==0x037f) { ++extended;continue; }if (precision!=0x027f) throw std::runtime_error("입자 픽셀 fixture 정밀도 오류");
        const auto values=Split(row[2],',');if (values.size()!=9) throw std::runtime_error("입자 픽셀 fixture 좌표 폭 오류");
        const ScreenRect clip{std::stoi(values[0]),std::stoi(values[1]),std::stoi(values[2]),std::stoi(values[3])};
        FieryTailFrameState state;state.cameraX=std::stof(values[6]);state.cameraY=std::stof(values[7]);
        FieryTailFrameFields fields;fields.total=12;fields.particles.resize(12);fields.offset=std::stoi(values[8]);
        const auto particles=Decode(row[3]),rawColors=Decode(row[4]),expected=Decode(row[5]);
        if (particles.size()!=12*24 || rawColors.size()!=12*4 || expected.size()!=64*48) throw std::runtime_error("입자 픽셀 fixture 바이트 폭 오류");
        // 같은12개 입력 입자에는 죽은 슬롯과 마지막 인접 색을 읽는 수명4도 포함한다.
        for (std::size_t index=0;index<12;++index) {
            const auto offset=index*24;fields.particles[index]={Float(particles,offset),Float(particles,offset+4),Float(particles,offset+8),Float(particles,offset+12),Float(particles,offset+16),Get(particles,offset+20)};
        }
        std::array<std::uint32_t,12> colors{};
        // 원본의 색 표 원문을 바꾸거나 마지막 값을 별도 상수로 만들지 않는다.
        for (std::size_t index=0;index<12;++index) colors[index]=Get(rawColors,index*4);
        FieryTailFramePalette palette{11,colors};std::vector<std::uint8_t> pixels(64*48,199);FieryTailFrameHooks hooks;
        hooks.invalidate=[](FieryTailDirtyRect) { throw std::logic_error("Draw 전용 원본 픽셀 검사에서 예상하지 않은 dirty 요청"); };
        hooks.line=[&](FieryTailLine value) { DrawVfxLine(pixels,64,48,clip,{value.x1,value.y1},{value.x2,value.y2},value.color); };
        RawFieryTailFrame draw(pool,random,state,palette,std::move(hooks));draw.Draw(fields,std::stoi(values[4]),std::stoi(values[5]));
        const auto mismatch=std::mismatch(pixels.begin(),pixels.end(),expected.begin());
        if (mismatch.first!=pixels.end()) {
            const auto index=static_cast<std::size_t>(mismatch.first-pixels.begin());
            const auto message=row[0]+" 픽셀("+std::to_string(index%64)+","+std::to_string(index/64)+") 실제"+std::to_string(*mismatch.first)+" 기대"+std::to_string(*mismatch.second);
            netstorm::test::ReportFailure(__FILE__,__LINE__,message.c_str());
        }
        ++count;
    }
    CHECK(count==30 && extended==30);
}
// 색수만큼 요청한 실제 heap 배열과 그 다음 DWORD의 snapshot이 OS 원문과 같다.
TEST_CASE(ParticlePaletteMemory_ActualAllocationAndAdjacentRead) {
    ParticlePaletteMemory memory;CHECK(Throws([&] { memory.Snapshot(); }));
    memory.Assign(kPaletteSeed);memory.Assign(std::span(kPaletteSeed).first(11));
    CHECK(memory.Count()==11 && memory.Address()!=nullptr);const auto actual=memory.Snapshot();CHECK(actual.size()==12);
    std::array<std::uint32_t,12> raw{};SIZE_T received=0;
    CHECK(ReadProcessMemory(GetCurrentProcess(),memory.Address(),raw.data(),sizeof(raw),&received)!=0 && received==sizeof(raw));
    CHECK(std::equal(actual.begin(),actual.end(),raw.begin()));
    // 실제11색은 생성자의 입력과 같고, 마지막 DWORD에는 고정0/1/끝색 기대값을 두지 않는다.
    for (std::size_t index=0;index<11;++index) CHECK(actual[index]==kPaletteSeed[index]);
}
// 직접 표시도 같은 깊이 정렬/부분 갱신/SceneImage에 참여하며 행 padding을 덮지 않는다.
TEST_CASE(Renderer_ProceduresShareSortClipAndDirtyRemoval) {
    Renderer renderer(12,8);std::vector<std::uint8_t> order;
    renderer.SetScene({Procedure(0,10,order),Procedure(100,20,order),Procedure(-25,30,order)});
    const auto image=renderer.SceneImage();CHECK(order==std::vector<std::uint8_t>({20,10,30}));CHECK(image.indices[3*12+5]==30 && image.indices[0]==0);
    order.clear();std::vector<std::uint8_t> pixels(16*8,0x7e);renderer.Draw(pixels,16);renderer.Present([](ScreenRect) {});
    CHECK(order==std::vector<std::uint8_t>({20,10,30}));CHECK(pixels[3*16+5]==30 && pixels[0]==0);
    // 실제 pitch의 화면 밖 네 padding 바이트는 초기 원문을 유지한다.
    for (int y=0;y<8;++y) {
        // 각 행의 유효 폭 뒤 네 바이트만 별도로 확인한다.
        for (int x=12;x<16;++x) CHECK(pixels[static_cast<std::size_t>(y*16+x)]==0x7e);
    }
    renderer.SetScene({});renderer.Draw(pixels,16,9);renderer.Present([](ScreenRect) {});
    CHECK(pixels[3*16+5]==9 && pixels[0]==0);
}
// 실제 보호 팔레트·같은 raw Host/Kernel과 process11이 실제 VFX 선을 dirty 표면에 쓴다.
TEST_CASE(CampaignParticles_ActualHostPaletteAndVfxPixels) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078);
    // 보호 PATCH의 실제 다리/섬/연결/지형 번호를 같은 월드 초기화 입력으로 공급한다.
    BridgeConnectState links;links.bridgeType=82;links.islandType=94;links.stalagType=95;links.connectorType=155;links.noIslandType=157;
    RawSurfaceWorld world(OriginalEdition::Patch1078,assets.types,assets.frames,assets.shapes,links,83);auto core=world.Core();
    const auto parent=core.pool.Allocate(0);auto raw=core.pool.AllocatedBytes(parent);Put(raw,0,0x501dd0);raw[10]=80;raw[11]=0;
    Put(raw,14,std::bit_cast<std::uint32_t>(20.5F));Put(raw,18,std::bit_cast<std::uint32_t>(21.25F));
    Renderer renderer(1024,768);GameRandom random;RawCampaignParticles particles(core,random,assets.game.Palette());
    const SurfaceDisplayHooks display{[&] { return renderer.FullRedrawPending(); },[&](SquidDisplayRect rect,std::uint32_t flags) { renderer.Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }};
    particles.SetView({0,0,0,0,1024,768},display);particles.Add(11,parent);CHECK(particles.Rendering().Size()==1 && core.kernel.Size()==1);
    const auto form=Sid{static_cast<std::uint16_t>(Get(core.pool.Slot(parent),6,2))};auto* process=dynamic_cast<FieryTailVisualProcess*>(core.processes.FromForm(form));CHECK(process!=nullptr);
    particles.PrepareFrame(2);core.kernel.RunFrame();CHECK(process && process->Fields().total==10);
    const auto first=particles.Sprites({0,0,1024,768});CHECK(first.size()==1 && first[0].procedure.has_value() && !first[0].database);
    renderer.SetScene(first);std::vector<std::uint8_t> pixels(1024*768,199);renderer.Draw(pixels,1024,199);renderer.Present([](ScreenRect) {});
    particles.PrepareFrame(2.025);core.kernel.RunFrame();CHECK(process && process->Fields().total==12);
    renderer.SetScene(particles.Sprites({0,0,1024,768}));renderer.Draw(pixels,1024,199);renderer.Present([](ScreenRect) {});
    CHECK(pixels[234*1024+328]==196 && std::count(pixels.begin(),pixels.end(),196)>3);
    // 수명3.75의 원본 색 index10=196과 배경만 남아야 한다. 초기 인접 값은 이 검사 기대값으로 만들지 않는다.
    CHECK(std::all_of(pixels.begin(),pixels.end(),[](auto pixel) { return pixel==199 || pixel==196; }));
    particles.SetView({300,200,0,0,1024,768},display);auto shifted=particles.Sprites({0,0,1024,768});renderer.SetScene(shifted);
    const auto image=renderer.SceneImage();CHECK(image.indices[34*1024+28]==196);
    CHECK(Throws([&] { particles.Add(12,parent); }) && particles.Rendering().Size()==1);
    core.processes.Kill(*process,0x10);CHECK(particles.Rendering().Size()==0 && core.kernel.Size()==0);
    renderer.SetScene(std::move(shifted));const auto removed=renderer.SceneImage();CHECK(std::count(removed.indices.begin(),removed.indices.end(),196)==0);
}
