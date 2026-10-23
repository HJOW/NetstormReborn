// 원문v2의618관측과부분중단을신규제품의현재Pool/표/trace에독립대조한다.
#include "TestSupport.h"
#include "o/RawEditorWorldFinish.h"
#include <algorithm>
#include <array>
#include <bit>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
using namespace netstorm::o;
constexpr std::array<std::uint32_t,2> kPools{0x12001000,0x12001000+16000}; // 원문격리입력과같은두독립Pool기준값이다.
constexpr std::uint32_t kPoolBytes=32*50,kMapBytes=256*256*2,kSpotBytes=256*256; // 전체관측하는실제물리입력폭이다.
// 마지막빈필드도보존하여원문전체trace를읽는다.
std::vector<std::string> Split(std::string_view text,char delimiter) {
    std::vector<std::string> result;std::size_t start{}; // 현재열/필드누적위치다.
    for (;;) { const auto end=text.find(delimiter,start);result.emplace_back(text.substr(start,end==std::string_view::npos ? end : end-start));if (end==std::string_view::npos) return result;start=end+1; } // 구분자가없으면남은필드까지반환한다.
}
// fixture의전체DWORD숫자를읽는다.
std::uint32_t Number(const std::string& value) { return static_cast<std::uint32_t>(std::stoul(value)); }
// 입력/관측필드의감긴비트를signed로읽는다.
std::int32_t Signed(std::uint32_t value) { return std::bit_cast<std::int32_t>(value); }
// 명시입력의little endian필드만쓴다.
void Put(std::span<std::uint8_t> bytes,std::size_t at,std::uint32_t value,std::size_t width=4) {
    for (std::size_t byte=0;byte<width;++byte) bytes[at+byte]=static_cast<std::uint8_t>(value>>(byte*8)); // 실제폭밖바이트는보존한다.
}
// 실제부분효과의WORD/BYTE관측값을읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width) {
    std::uint32_t result{};for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(bytes[at+byte])<<(byte*8);return result; // 작은실제표필드만조합한다.
}
// 전체물리입력바이트를원문과같은16진표현으로만든다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    constexpr char digits[]="0123456789abcdef";std::string result;result.reserve(bytes.size()*2); // 각BYTE의두자리표현이다.
    for (const auto byte:bytes) { result+=digits[byte>>4];result+=digits[byte&15]; }return result; // 모든비변경BYTE도전수대조한다.
}
// zlib adler32와같은전체표체크섬이다.
std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    std::uint32_t first=1,second{};for (const auto byte:bytes) { first=(first+byte)%65521U;second=(second+first)%65521U; }return (second<<16)|first; // 두누적값은각BYTE마다감긴다.
}
// 원문콜백과같은명시중단을부분효과뒤전달한다.
struct BoundaryStop:std::runtime_error { explicit BoundaryStop(const std::string& cause):std::runtime_error(cause) {} };
// 입력만구성하고결과는원문fixture에서읽어비교하는독립효과장치다.
struct FinishFixture {
    std::array<std::vector<std::uint8_t>,2> pools,maps,spots; // 원문과같은서로다른물리입력저장소들이다.
    std::uint32_t pool{},map{},spot{},capacity{16},noIsland{157},island{94};std::int32_t board{256}; // 현재원문전역의명시입력이다.
    std::uint32_t neighbors{};std::string mutation;EditorWorldFinishForm form; // Finder결과입력과콜백효과대상이다.
    std::vector<std::string> events,expectedEvents; // 실제하위trace와원문관측trace이며제품에기대상태를주입하지않는다.
    bool pending{};char pendingKind{};std::uint32_t pendingTable{},pendingIndex{};std::vector<std::uint8_t> pendingBefore; // 직접표쓰기의대여/반납사이실제바이트관측이다.

    // 각원문행의입력열만사용해Pool/표를초기화한다.
    FinishFixture(const std::vector<std::string>& input,std::string_view expectedTrace) {
        const auto profile=Number(input[0]),x=Number(input[1]),y=Number(input[2]);neighbors=Number(input[3]);mutation=input[4]; // 원문입력열이다.
        capacity=profile==5 ? 2U : 16U;noIsland=profile==4 ? 413U : 157U;
        expectedEvents=expectedTrace.empty() ? std::vector<std::string>{} : Split(expectedTrace,';');
        for (std::uint32_t table=0;table<2;++table) { // 모든비변경입력BYTE도원문패턴과같게한다.
            pools[table].resize(kPoolBytes);maps[table].resize(kMapBytes);spots[table].assign(kSpotBytes,static_cast<std::uint8_t>(0xB3U+table));
            for (std::size_t at=0;at<kPoolBytes;++at) pools[table][at]=static_cast<std::uint8_t>(at*13+0xA5U+table*7); // 원문의비0raw패턴이다.
            for (std::size_t at=0;at<kMapBytes;++at) maps[table][at]=static_cast<std::uint8_t>(at%2 ? 0xA5U : 0x1DU+table); // WORD표의전체초기패턴이다.
            for (std::uint32_t sid=0;sid<32;++sid) { // 예약/활성/비활성슬롯을명시하며순회결과는계산하지않는다.
                Put(pools[table],sid*50,0x14000000);pools[table][sid*50+10]=85;pools[table][sid*50+11]=static_cast<std::uint8_t>(sid<16 ? 0 : 1);Put(pools[table],sid*50+14,x);Put(pools[table],sid*50+18,y);
            }
            if (profile!=0) {
                pools[table][2*50+10]=157;
                if (profile==2 || profile==3) { pools[table][3*50+10]=157;pools[table][4*50+10]=157;pools[table][3*50+11]=1; }
                if (profile==2) pools[table][1*50+10]=157;
                if (profile==3) pools[table][16*50+10]=157;
            }
            pools[table][5*50+10]=static_cast<std::uint8_t>(table ? 42 : 94);pools[table][6*50+10]=42;pools[table][7*50+10]=94;
        }
    }
    // 예상원문주소에서실제쓴값을읽고그외전체바이트가보존됐는지확인한다.
    void Flush() {
        if (!pending) return;auto& bytes=pendingKind=='S' ? maps[pendingTable] : spots[pendingTable];const std::size_t width=pendingKind=='S' ? 2U : 1U,at=std::size_t(pendingIndex)*width; // 고정관측위치/폭이다.
        for (std::size_t byte=0;byte<bytes.size();++byte) if (byte<at || byte>=at+width) CHECK(bytes[byte]==pendingBefore[byte]); // 원문관측주소밖에추가쓰기가없어야한다.
        events.push_back(std::string(1,pendingKind)+':'+std::to_string(pendingTable)+':'+std::to_string(pendingIndex)+':'+std::to_string(Read(bytes,at,width)));pending=false;
    }
    // 표getter대여뒤다음경계에서실제바이트를읽는다.원문관측주소는읽기위치선택에만쓰고메모리에쓰지않는다.
    std::span<std::uint8_t> Table(char kind) {
        Flush();auto& tables=kind=='S' ? maps : spots;const auto selected=kind=='S' ? map : spot; // 현재실제표주소선택이다.
        if (events.size()>=expectedEvents.size() || expectedEvents[events.size()].front()!=kind) return tables[selected]; // 원문물리범위중단은쓰기전표대여만도달한다.
        const auto fields=Split(expectedEvents[events.size()],':');pending=true;pendingKind=kind;pendingTable=selected;pendingIndex=Number(fields[2]);pendingBefore=tables[selected];
        CHECK(selected==Number(fields[1]));return tables[selected];
    }
    // 직접표쓰기의부분효과를먼저관측한뒤실제하위호출을기록한다.
    void Record(std::string event) { Flush();events.push_back(std::move(event)); }
    // 원문과같은실제경계에서현재전역또는캡처form을변경한다.
    void Mutate(const std::string& boundary) {
        if (mutation==boundary+"-stop") throw BoundaryStop(boundary);
        if (mutation==boundary+"-pool1") pool=1;if (mutation==boundary+"-map1") map=1;if (mutation==boundary+"-spots1") spot=1;
        if (mutation==boundary+"-island") island=42;if (mutation==boundary+"-board0") board=0;
        if (mutation==boundary+"-coordinate") {
            for (std::uint32_t table=0;table<2;++table) if (form.address>=kPools[table] && form.address<kPools[table]+kPoolBytes) { const auto at=form.address-kPools[table];Put(pools[table],at+14,0);Put(pools[table],at+18,std::bit_cast<std::uint32_t>(2.75F)); } // 같은캡처물리form만바꾼다.
        }
        if (mutation==boundary+"-cap3") capacity=3;if (mutation==boundary+"-cap20") capacity=20;
        if (mutation==boundary+"-next-free") pools[0][3*50+11]=0;
    }
    // 실제부모가읽을현재저장소/하위경계만바인딩한다.
    EditorWorldFinishHooks Hooks() {
        EditorWorldFinishHooks hooks; // 제품에별도기대값/상태분기를공급하지않는실제효과장치다.
        hooks.pool=[this] { return EditorWorldFinishPool{pools[pool],kPools[pool],capacity}; };hooks.noIslandType=[this] { return noIsland; };hooks.islandType=[this] { return island; };hooks.boardSize=[this] { return board; };
        hooks.surfaces=[this] { return Table('S'); };hooks.spots=[this] { return Table('P'); };
        hooks.begin=[this](const EditorWorldFinishForm& captured,std::int32_t left,std::int32_t top,std::int32_t right,std::int32_t bottom) {
            form=captured;Record("B:"+std::to_string(left)+':'+std::to_string(top)+':'+std::to_string(right)+':'+std::to_string(bottom));Mutate("begin");
            struct Cursor { std::vector<std::uint32_t> values; /* values: 명시Finder결과순서다. */ std::size_t index{}; /* index: 현재후보위치다. */ std::uint32_t calls{}; /* calls: 이독립Finder의Next횟수다. */ }; // 독립Finder의명시현재/다음입력수명이다.
            auto cursor=std::make_shared<Cursor>();cursor->values=neighbors==0 ? std::vector<std::uint32_t>{} : neighbors==1 ? std::vector<std::uint32_t>{5} : std::vector<std::uint32_t>{6,5,7};
            return EditorWorldFinishCursor{[cursor] { return cursor->index<cursor->values.size() ? cursor->values[cursor->index] : 0U; },[this,cursor] {
                Record("N:"+std::to_string(cursor->values[cursor->index]));Mutate("next"+std::to_string(++cursor->calls));++cursor->index;
            }};
        };
        hooks.unpop=[this](const EditorWorldFinishForm& captured,std::uint32_t flags) { form=captured;Record("U:"+std::to_string(Signed(captured.address-kPools[0]))+':'+std::to_string(flags));Mutate("unpop"); };
        hooks.release=[this](std::int32_t sid) { Record("R:"+std::to_string(sid));Mutate("release"); };
        hooks.graph=[this](std::uint32_t flags) { Record("G:"+std::to_string(flags));Mutate("graph"); };return hooks;
    }
};
}

// 입력열만신규제품에공급하고완료/부분효과/전역/전체Pool/전체표checksum/모든하위순서를대조한다.
TEST_CASE(EditorWorldFinish_IndependentNative618) {
    std::ifstream stream(std::filesystem::path(__FILE__).parent_path()/"fixtures/editor-world-finish-v2-x86.tsv");CHECK(stream.good());std::string line;std::size_t rows{},stops{}; // 최초v2원문관측의읽기상태다.
    while (std::getline(stream,line)) { // 두정밀도의모든관측을생략하지않는다.
        if (line.empty() || line.front()=='#') continue;const auto columns=Split(line,'\t');CHECK(columns.size()==7);const auto input=Split(columns[1],',');FinishFixture scene(input,columns[6]);auto hooks=scene.Hooks();std::string completed="1";
        try { FinishEditorWorld(hooks);scene.Flush(); }
        catch (const BoundaryStop& stop) { completed=stop.what(); }
        catch (const std::out_of_range&) { completed="surface-range"; }
        CHECK(completed==columns[2]);if (completed!="1") ++stops;
        const std::array<std::uint32_t,7> globals{scene.pool,scene.map,scene.spot,scene.capacity,scene.noIsland,scene.island,std::bit_cast<std::uint32_t>(scene.board)};const auto expectedGlobals=Split(columns[3],',');
        for (std::size_t field=0;field<globals.size();++field) CHECK(globals[field]==Number(expectedGlobals[field])); // 모든현재전역을독립비교한다.
        CHECK(Hex(scene.pools[0])+':'+Hex(scene.pools[1])==columns[4]);const auto expectedChecksums=Split(columns[5],',');
        CHECK(Adler(scene.maps[0])==Number(expectedChecksums[0]) && Adler(scene.maps[1])==Number(expectedChecksums[1]) && Adler(scene.spots[0])==Number(expectedChecksums[2]) && Adler(scene.spots[1])==Number(expectedChecksums[3]));
        CHECK(scene.events==scene.expectedEvents);++rows;
    }
    CHECK(rows==618 && stops==46);
}

// 빈Pool도마지막Graph는필수이며선택하지않은Finder/가상/Release를가짜성공으로강제하지않는다.
TEST_CASE(EditorWorldFinish_SelectedBoundariesAndRequiredGraph) {
    EditorWorldFinishHooks hooks;std::uint32_t graphs{}; // 외부효과가없는빈실제물리뷰다.
    hooks.pool=[] { return EditorWorldFinishPool{{},0x12001000,0}; };hooks.graph=[&](std::uint32_t flags) { CHECK(flags==0);++graphs; };
    FinishEditorWorld(hooks);CHECK(graphs==1);hooks.graph={};bool rejected=false;
    try { FinishEditorWorld(hooks); }catch (const std::logic_error&) { rejected=true; }CHECK(rejected);
}

// 원문fixture주소를관측위치로도사용하지않고실제하위효과에서선행쓰기와현재표전환을직접확인한다.
TEST_CASE(EditorWorldFinish_ActualPartialWritesAndCurrentAddresses) {
    std::vector<std::uint8_t> raw(3*50),surface0(kMapBytes,0xA5),surface1(kMapBytes,0xA5),spot0(kSpotBytes,0xA5),spot1(kSpotBytes,0xA5); // 순회할한noIsland와현재표들의명시입력이다.
    raw[2*50+10]=157;Put(raw,2*50+14,std::bit_cast<std::uint32_t>(3.75F));Put(raw,2*50+18,std::bit_cast<std::uint32_t>(4.25F));
    constexpr std::size_t kCell=4*256+3;std::uint32_t base=0x12001000,map{},spot{};std::vector<std::string> order; // 두정수좌표의관측칸과변경되는실제기준선택이다.
    EditorWorldFinishHooks hooks; // 본문이도달하는실제외부효과만공급한다.
    hooks.pool=[&] { return EditorWorldFinishPool{raw,base,3}; };hooks.noIslandType=[] { return 157U; };
    hooks.surfaces=[&]() ->std::span<std::uint8_t> { return map ? surface1 : surface0; };hooks.spots=[&]() ->std::span<std::uint8_t> { return spot ? spot1 : spot0; };
    hooks.begin=[&](const EditorWorldFinishForm& form,std::int32_t left,std::int32_t top,std::int32_t right,std::int32_t bottom) {
        CHECK(form.bytes.data()==raw.data()+2*50 && form.address==0x12001000U+2*50 && left==3 && top==4 && right==3 && bottom==4);order.push_back("begin");return EditorWorldFinishCursor{[] { return 0U; },{}};
    };
    hooks.unpop=[&](const EditorWorldFinishForm& form,std::uint32_t flags) {
        CHECK(flags==0 && form.bytes.data()==raw.data()+2*50);CHECK(Read(surface0,kCell*2,2)==2 && spot0[kCell]==0xA5 && spot1[kCell]==0xA5);
        order.push_back("unpop");base+=50;map=1;spot=1; // 현재기준이바뀌어도캡처form과최초삭제칸을유지한다.
    };
    hooks.release=[&](std::int32_t sid) {
        CHECK(sid==1 && spot1[kCell]==0 && spot0[kCell]==0xA5 && Read(surface0,kCell*2,2)==2 && Read(surface1,kCell*2,2)==0xA5A5);
        order.push_back("release");map=0; // 최종0쓰기가Release뒤현재surface를다시선택해야한다.
    };
    hooks.graph=[&](std::uint32_t flags) {
        CHECK(flags==0 && Read(surface0,kCell*2,2)==0 && Read(surface1,kCell*2,2)==0xA5A5);order.push_back("graph");
    };
    FinishEditorWorld(hooks);CHECK(order==std::vector<std::string>({"begin","unpop","release","graph"}));
}
