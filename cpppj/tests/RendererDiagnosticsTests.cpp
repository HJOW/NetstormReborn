// 네 원문 후단의 전체 픽셀/clip/호출과 같은 거리표 소유를 창 없이 검사한다.
#include "GumpRawSupport.h"
#include "client/RendererDiagnostics.h"
#include "client/ClientGameplayGlobals.h"
#include "client/VFXLine.h"
#include "o/RawPathSearch.h"
#include <algorithm>
#include <bit>
#include <filesystem>

using namespace netstorm::client; // 실제제품의같은좌표/VFX/진단부모다.
using namespace netstorm::o; // 원문50BYTE/네해시/현재TYPE를입력한다.
using namespace netstorm::test::gumpraw; // 동결fixture의해독만공유한다.
namespace { // 실제기계어기대와분리한명시입력/메모리화면이다.
constexpr int Width=96,Height=80,Pitch=100; // 화면폭과행padding을구분한다.
std::string RectText(ScreenRect rect) { return std::to_string(rect.left)+':'+std::to_string(rect.top)+':'+std::to_string(rect.right)+':'+std::to_string(rect.bottom); } // 원문동일형식의사각형직렬화다.
std::vector<std::uint8_t> ExpectedPixels(std::string_view changed) {
    std::vector<std::uint8_t> result(Pitch*Height,199);if (changed=="-") return result;
    for (const auto& run:Split(changed,';')) { const auto fields=Split(run,':');CHECK(fields.size()==2);if (fields.size()!=2) continue;const auto offset=Number(fields[0]);const auto bytes=Bytes(fields[1]);CHECK(offset+bytes.size()<=result.size());if (offset+bytes.size()<=result.size()) std::copy(bytes.begin(),bytes.end(),result.begin()+offset); } // 원문이관측한변경BYTE만원위치에놓는다.
    return result;
}
// 실제풀/해시와메모리화면만소유하며기대좌표/픽셀은계산하지않는다.
struct Drawing {
    SidPool pool{OriginalEdition::Patch1078,24000,false};SquidHash hash;std::vector<RiftTypeRecord> types=std::vector<RiftTypeRecord>(256); // 원문과같은합성입력배열이다.
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);std::vector<std::int16_t> distances; // 명시거리표입력이며새BFS를실행하지않는다.
    RawCameraViewState renderer,snapshot;RendererDiagnosticKind kind{};std::uint32_t flag{},mode{},background{73};std::int32_t worldCells{256}; // 현재진단/카메라의독립입력이다.
    ScreenRect clip{5,7,91,73};std::vector<std::uint8_t> pixels=std::vector<std::uint8_t>(Pitch*Height,199);std::array<std::uint32_t,9> colors{10,11,12,13,14,15,16,17,18}; // 현재색과clip을장치마다읽는다.
    std::string trace;RendererDiagnosticHooks hooks; // 관측한호출만직렬화한다.
    void Add(std::string text) { if (!trace.empty()) trace+=';';trace+=text; } // 원문과동일순서의효과열이다.
    // 원문에서분리한입력6열만현재물리/장치에설치한다.
    explicit Drawing(const std::vector<std::string>& input) {
        kind=static_cast<RendererDiagnosticKind>(Number(input[0]));flag=Number(input[1]);mode=Number(input[2]);std::fill(spots.begin(),spots.end(),static_cast<std::uint8_t>(Number(input[3])));if (Number(input[4])) distances.assign(65536,static_cast<std::int16_t>(Number(input[4])));
        renderer.worldBits={std::bit_cast<std::uint32_t>(19.25F),std::bit_cast<std::uint32_t>(19.5F)};renderer.cellPixels=Number(input[5]) ? std::array<std::int32_t,2>{32,22}:std::array<std::int32_t,2>{16,11};snapshot.cellPixels=renderer.cellPixels;snapshot.pixels={304,209};
        constexpr std::array<std::uint16_t,6> ids{50,60,61,62,63,64};constexpr std::array<std::uint8_t,6> states{0,1,2,4,8,6},graphs{2,0,254,1,0,3}; // 원문입력의종류/상태/graphBYTE다.
        for (std::size_t i=0;i<ids.size();++i) { // 입력순서의실제해시머리만만들고순회는제품에맡긴다.
            const auto x=static_cast<float>(20+i%3)+.25F,y=static_cast<float>(20+i/3)+.5F;auto raw=InputRaw(pool,Sid{ids[i]});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(100+i);raw[11]=states[i];raw[12]=static_cast<std::uint8_t>(i&1);raw[30]=graphs[i];Put(raw,14,std::bit_cast<std::uint32_t>(x));Put(raw,18,std::bit_cast<std::uint32_t>(y));types[100+i].footX=static_cast<int>(1+i%3);types[100+i].footY=static_cast<int>(1+i%2);types[100+i].flags1=i%2 ? 0U:0x800U;
            auto& head=hash.Bucket(static_cast<int>(i%4),x,y);Put(raw,4,head,2);head=ids[i];
        }
        hooks.screen.clip=[this](auto rect) { Add("Clip:"+RectText(rect));clip=rect; };
        hooks.screen.rectangle=[this](auto rect,auto ink) { Add("Rect:"+RectText(rect)+':'+std::to_string(ink));const auto saved=clip;clip={0,0,Width,Height};Add("Lock:"+RectText(clip));DrawWaterRectangle(pixels,Pitch,Height,clip,rect,ink);Add("Unlock:"+RectText(clip));clip=saved; };
        hooks.line=[this](auto first,auto second,auto ink) { Add("Line:"+std::to_string(first[0])+':'+std::to_string(first[1])+':'+std::to_string(second[0])+':'+std::to_string(second[1])+':'+std::to_string(ink));const auto saved=clip;clip={0,0,Width,Height};Add("Lock:"+RectText(clip));DrawVfxLine(pixels,Pitch,Height,clip,{first[0],first[1]},{second[0],second[1]},ink);Add("Unlock:"+RectText(clip));clip=saved; };
        hooks.screen.present=[this] { Add("Present:0:0:96:80"); };hooks.colors=[this]()->std::span<const std::uint32_t> { return colors; };hooks.invalidGraph=[] { return 254U; };hooks.distances=[this]()->std::span<const std::int16_t> { return distances; };hooks.assertion=[this](auto line) { Add("Assert:"+std::to_string(line)); };
    }
    // 실제부모의입력trace만추가하고모든픽셀은제품VFX로그린다.
    void Draw() {
        if (flag && kind==RendererDiagnosticKind::Fire) { const auto area=DrawWaterVisibleArea(renderer,Width,Height);Add("Begin:"+std::to_string(std::bit_cast<std::uint32_t>(static_cast<float>(area.left)))+':'+std::to_string(std::bit_cast<std::uint32_t>(static_cast<float>(area.top)))+':'+std::to_string(std::bit_cast<std::uint32_t>(static_cast<float>(area.right)))+':'+std::to_string(std::bit_cast<std::uint32_t>(static_cast<float>(area.bottom)))+":0"); } // 부수없는실제finder입력값만기록하고후보순회는같은제품본문이수행한다.
        RawRendererDiagnostics(pool,hash,types,spots).Draw(kind,renderer,snapshot,Width,Height,flag,mode,worldCells,background,hooks);
    }
};
}

// 원문252개/504회관측의전체표면/padding/clip/전역쓰기/효과순서를읽는다.
TEST_CASE(RendererDiagnostics_Native252WholePixelsAndCurrentBodies) {
    const auto fixture=std::filesystem::path(__FILE__).parent_path()/"fixtures/renderer-diagnostics-x86.tsv";const auto rows=LoadFixture(fixture.string().c_str()).rows;CHECK(rows.size()==252);
    for (const auto& row:rows) { // Hash/Fire모드0·1/Spot/Board와0/비영전체DWORD를각독립입력으로재생한다.
        CHECK(row.size()==7);if (row.size()!=7) continue;const auto input=Split(row[0].substr(1,row[0].size()-2),',');CHECK(input.size()==6);if (input.size()!=6) continue;Drawing draw(input);draw.Draw();CHECK(draw.pixels==ExpectedPixels(row[1]));CHECK(RectText(draw.clip)==row[2] && draw.background==Number(row[6]));CHECK((draw.trace.empty()?"-":draw.trace)==row[5]);
    }
}

// 모든진단의flag0은카메라나실제장치를요구하지않으며장기저장소도변경하지않는다.
TEST_CASE(RendererDiagnostics_DisabledAndLongOwnerNoSyntheticActivation) {
    ClientGameplayGlobals globals;CHECK(!globals.DrawHash() && !globals.DrawFire() && !globals.DrawSpot() && !globals.DrawBoard() && !globals.FireMode());const auto* identity=&globals.DrawBoard();globals.DrawBoard()=29;
    const std::array<std::uint32_t,5> addresses{0x54d91cU,0x54d920U,0x54d924U,0x54d92cU,0x595144U}; //명시 주소 해석기가 같은 다섯 실제 저장소를 사용하는지 확인한다.
    const std::array<std::uint32_t*,5> words{&globals.DrawHash(),&globals.DrawFire(),&globals.DrawSpot(),&globals.DrawBoard(),&globals.FireMode()}; //별도 진단 사본을 만들지 않고 getter의 현재 주소를 보관한다.
    for (std::size_t index=0;index<addresses.size();++index) { //각 주소의 쓰기와 직접 getter의 쓰기가 서로 즉시 보이는지 검사한다.
        globals.WriteDebugWord(addresses[index],99);CHECK(*words[index]==99);*words[index]=29;CHECK(globals.ReadDebugWord(addresses[index])==29);
    }
    { auto lease=globals.AcquirePrepared();CHECK(globals.DrawBoard()==29 && &globals.DrawBoard()==identity); }CHECK(globals.DrawBoard()==29 && &globals.DrawBoard()==identity);
    SidPool pool(OriginalEdition::Patch1078,24000,false);SquidHash hash;std::uint32_t disabled{},mode{},background{71};const std::int32_t cells=256;RawRendererDiagnostics drawing(pool,hash,{},{});
    for (const auto kind:{RendererDiagnosticKind::Hash,RendererDiagnosticKind::Fire,RendererDiagnosticKind::Spot,RendererDiagnosticKind::Board}) drawing.Draw(kind,{}, {},0,0,disabled,mode,cells,background,{}); // viewport나눗셈/물리배열에0관문이선행한다.
    CHECK(background==71);
}

// 기존거리표를두탐색기가이어빌리되생성만으로원문비NULL진단상태를합성하지않는다.
TEST_CASE(RendererDiagnostics_SharedPathWorkSurvivesSearchOwnerReplacement) {
    SidPool pool(OriginalEdition::Patch1078,24000,false);SquidHash hash;std::vector<std::uint8_t> spots(65536);std::vector<RiftTypeRecord> types(1);std::vector<RiftTypeFrames> frames;frames.emplace_back(std::vector<FrameCode>{});RawPathSearchState state;RawPathSearchStorage storage;const RawPathSearchWork* address{};
    { RawPathSearch first(pool,hash,spots,types,frames,state,&storage);address=&first.Work();CHECK(storage.work && !storage.initialized);RawPathRuntime output(OriginalEdition::Patch1078);CHECK(!first.SearchCells(output,0,0,0,0,false));CHECK(storage.initialized && first.Work().distance[0]==1);storage.work->distance[37]=123; }
    { RawPathSearch next(pool,hash,spots,types,frames,state,&storage);CHECK(&next.Work()==address && next.Work().distance[37]==123 && storage.initialized); }CHECK(storage.work && storage.work.get()==address);
}

// 같은상위관문뒤의7/10/13키효과를576개원문입력과대조한다.
TEST_CASE(RendererDiagnostics_Native576KeyGatesFullDwordAndPositionLog) {
    const auto fixture=std::filesystem::path(__FILE__).parent_path()/"fixtures/renderer-diagnostic-keys-x86.tsv";const auto rows=LoadFixture(fixture.string().c_str()).rows;CHECK(rows.size()==576);
    for (const auto& row:rows) { //정지와Console은서로다르며수식/outside는원문마스크만적용한다.
        CHECK(row.size()==12);if (row.size()!=12) continue;auto fire=Number(row[3]),spot=fire,board=fire,redraw=Number(row[4]);std::uint32_t logs{};
        if (!Number(row[6])) CHECK(DispatchRendererDiagnosticKey({Number(row[0]),0,0},Number(row[1]),Number(row[2]),fire,spot,board,redraw,[&](auto) { ++logs; }));
        CHECK(fire==Number(row[7]) && spot==Number(row[8]) && board==Number(row[9]) && redraw==Number(row[10]));CHECK(logs==(row[11].find("position-log")!=row[11].npos ? 1U:0U));
    }
}
