// Squid 프레임 지정을 원본 기계어 기대값으로 검사하고 실제 표시 없는 Unpop/Pop에 이어 본다.
#include "TestSupport.h"
#include "o/SquidFrame.h"
#include "o/SquidFactory.h"
#include "o/SquidPop.h"
#include "o/SquidUnpop.h"
#include <bit>
#include <cstdio>
#include <fstream>
#include <map>
#include <stdexcept>
#include <string>

using namespace netstorm::o;
namespace {
// 기계어 실행기의 입력 번호·타입 번호·가상 표 기록값이다. 호스트 함수 주소로 해석하지 않는다.
constexpr Sid kObject{50};
constexpr std::uint32_t kType=82,kVtable=0x11010000,kCapacity=24000;
// 실행기가 쓰는 객체 위치(소수 좌표)다.
constexpr float kX=20.75f,kY=21.9f;
// 패치판 00419850이 프레임 수를 두 배로 보는 타입 플래그 1 비트다.
constexpr std::uint32_t kDoubleFrames=0x440000;
// 판본마다 기대하는 관찰 행 수다.
constexpr std::size_t kSetRows=1920,kThunkRows=4;
// 마지막 빈 필드도 보존한다.
std::vector<std::string> Split(std::string_view text,char separator) {
    std::vector<std::string> result;std::size_t start=0;
    // 구분자가 없을 때 남은 마지막 필드까지 추가한다.
    for (;;) {
        const auto end=text.find(separator,start);
        result.emplace_back(text.substr(start,end==std::string_view::npos ? end : end-start));
        if (end==std::string_view::npos) return result;
        start=end+1;
    }
}
// fixture 숫자 필드를 DWORD로 읽는다.
std::uint32_t Number(const std::string& value) { return static_cast<std::uint32_t>(std::stoul(value)); }
// little endian raw 필드를 쓰되 주변 바이트는 보존한다.
void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 원본 필드의 폭만큼 낮은 바이트부터 쓴다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// 슬롯 전체를 실행기와 같은 소문자 16진수로 만든다.
std::string Hex(std::span<const std::uint8_t> bytes) {
    static const char digits[]="0123456789abcdef";std::string text;
    // 바이트마다 두 글자를 쓴다.
    for (const auto byte:bytes) { text+=digits[byte>>4];text+=digits[byte&15]; }
    return text;
}
// 타입 종류 한 줄: flags1, flags2, 프레임 수 필드.
struct Kind { std::uint32_t flags1{},flags2{};int count{}; };
// 저장된 독립 기계어 출력: 프레임 크기 표, 타입 종류, 관찰 행이다. 원본 PE/Python 없이 읽는다.
struct FixtureData {
    std::vector<std::array<float,2>> sizes;
    std::vector<Kind> kinds;
    std::vector<std::vector<std::string>> rows;
};
const FixtureData& Fixture() {
    static const auto data=[] {
        std::ifstream input(NETSTORM_SETFRAME_FIXTURE);
        if (!input) throw std::runtime_error("프레임 지정 fixture 없음");
        FixtureData result;std::string line;
        // 주석을 제외하고 탭으로 입력/관찰 칸을 나눈다. 표 행은 따로 읽어 둔다.
        while (std::getline(input,line)) {
            if (!line.empty() && line.back()=='\r') line.pop_back();
            if (line.empty() || line.front()=='#') continue;
            auto row=Split(line,'\t');
            if (row[0]=="Sizes") {
                // 프레임마다 폭, 높이의 float 비트다.
                for (const auto& entry:Split(row.at(1),';')) {
                    const auto pair=Split(entry,',');
                    result.sizes.push_back({std::bit_cast<float>(Number(pair.at(0))),std::bit_cast<float>(Number(pair.at(1)))});
                }
            } else if (row[0]=="Kind") {
                result.kinds.push_back({Number(row.at(2)),Number(row.at(3)),std::stoi(row.at(4))});
            } else {
                result.rows.push_back(std::move(row));
            }
        }
        return result;
    }();return data;
}
// 명시적인 보호 경로를 확인한다.
template<class F> bool Throws(F operation) { try { operation(); } catch (const std::exception&) { return true; } return false; }
struct Scene {
    SidPool pool;
    std::vector<RiftTypeRecord> types;
    std::vector<std::string> events;
    bool patch{};
    Kind kind;
    // 실행기와 같은 풀 번호 5~74를 확보한다.
    explicit Scene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        patch(edition==OriginalEdition::Patch1078) {
        // 예측/서버 목록과 무관한 client 번호를 입력 슬롯으로 확보한다.
        for (std::uint16_t id=5;id<=74;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 판본별 필드 위치.
    std::size_t FrameOffset() const { return patch ? 0x24 : 0x22; }
    std::size_t FrameWidth() const { return patch ? 4 : 1; }
    std::size_t LevelOffset() const { return patch ? 0x21 : 0x1f; }
    // 실행기와 같은 객체 슬롯과 타입을 준비한다: 가상 표, 타입, 위치, 프레임, 단계 바이트.
    void Prepare(const Kind& input,std::uint32_t level,std::uint32_t frame) {
        kind=input;events.clear();
        types.at(kType).flags1=kind.flags1;types.at(kType).flags2=kind.flags2;
        auto raw=pool.AllocatedBytes(kObject);std::fill(raw.begin(),raw.end(),std::uint8_t{});
        Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(kType);
        Put(raw,14,std::bit_cast<std::uint32_t>(kX));Put(raw,18,std::bit_cast<std::uint32_t>(kY));
        Put(raw,FrameOffset(),frame,FrameWidth());raw[LevelOffset()]=static_cast<std::uint8_t>(level);
    }
    // 원본 실행기의 대체와 같은 경계다: 호출 사실과 인자만 기록한다.
    // 프레임 크기는 실행기의 합성 SHP 표에서 읽고, 패치판 00419850의 범위 검사(assert)는 예외로 옮긴다.
    SquidFrameHooks Hooks() {
        SquidFrameHooks hooks;
        hooks.frameSize=[this](Sid,std::int32_t frame) {
            const int limit=kind.count*(patch && (kind.flags1&kDoubleFrames)!=0 ? 2 : 1);
            if (patch && (frame<0 || frame>=limit)) throw std::out_of_range("프레임이 표 밖이다(00419850 assert)");
            const auto& sizes=Fixture().sizes;
            return static_cast<std::size_t>(frame)<sizes.size() ? sizes[static_cast<std::size_t>(frame)] : std::array<float,2>{};
        };
        // 사건의 마지막 칸은 호출 시점의 프레임 필드다. 프레임을 쓰는 시점과 가상 호출의 순서를 함께 비교한다.
        hooks.update=[this](Sid sid,std::uint32_t flags) {
            events.push_back(std::string((flags&kFrameUpdateAlternate)!=0 ? "E:" : "D:")+std::to_string(sid.value)+':'+FrameNow());
        };
        hooks.unpop=[this](Sid sid,std::uint32_t flags) {
            events.push_back("U:"+std::to_string(sid.value)+':'+std::to_string(flags)+':'+FrameNow());
        };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) {
            events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+
                std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(flags)+':'+FrameNow());
        };
        return hooks;
    }
    // 객체의 현재 프레임 필드를 판본의 폭으로 읽어 십진수로 만든다.
    std::string FrameNow() const {
        const auto raw=pool.Slot(kObject);std::uint32_t value=0;
        // 낮은 바이트부터 합친다.
        for (std::size_t i=0;i<FrameWidth();++i) value|=static_cast<std::uint32_t>(raw[FrameOffset()+i])<<(8*i);
        return std::to_string(value);
    }
    // 모든 원본 효과의 순서와 인자를 한 문자열로 만든다.
    std::string Events() const {
        std::string text;
        // 사건 사이에 세미콜론을 둔다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        return text.empty() ? "-" : text;
    }
};
// 한 판본의 모든 관찰 행을 재생한다.
void Replay(const char* edition) {
    Scene scene(std::string_view(edition)=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);
    const auto& fixture=Fixture();
    CHECK(fixture.sizes.size()==8 && fixture.kinds.size()==4);
    std::size_t sets=0,thunks=0;int failures=0;
    // 같은 입력으로 C++을 실행해 사건 순서와 슬롯 전체를 원본 관찰과 비교한다.
    for (const auto& row:fixture.rows) {
        if (row[1]!=edition) continue;
        if (row[0]=="Thunk") {
            // base의 +0x8c는 +0x88로 넘어간다. C++의 표시 갱신 훅이 두 경로를 하나로 받는 근거다.
            CHECK(row.size()==4 && row[3]=="D:50:0");++thunks;continue;
        }
        CHECK(row.size()==9);
        std::string actual;
        try {
            scene.Prepare(fixture.kinds.at(Number(row[2])),Number(row[3]),Number(row[4]));
            SquidFrame frames(scene.pool,scene.types,scene.Hooks());
            frames.Set(kObject,std::bit_cast<std::int32_t>(Number(row[5])),Number(row[6]));
            actual=scene.Events()+'|'+Hex(scene.pool.Slot(kObject));
        } catch (const std::exception& error) { actual=std::string("예외: ")+error.what(); }
        const bool same=actual==row[7]+'|'+row[8];
        CHECK(same);
        if (!same) {
            std::printf("  %s 행 %zu: 기대 %s / 실제 %.200s\n",edition,sets,row[7].c_str(),actual.c_str());
            if (++failures>=6) break;
        }
        ++sets;
    }
    CHECK(failures==0 && sets==kSetRows && thunks==kThunkRows);
}
}
// 세 PE는 같은 입력이라도 각 실행 파일에서 직접 얻은 관찰을 사용한다.
TEST_CASE(set_frame_patch_x86_levels_and_effects) { Replay("originals"); }
// CD 판본은 프레임이 바이트이고 단계 바이트 위치가 다르다.
TEST_CASE(set_frame_cd_x86_levels_and_effects) { Replay("originalCD"); }
// 추가 10.37은 별도 PE 입력 SHA를 가진 관찰이다.
TEST_CASE(set_frame_1037_x86_levels_and_effects) { Replay("original1037"); }

// 훅 누락과 원본 assert에 해당하는 flags는 상태를 바꾸기 전에 거부한다.
TEST_CASE(set_frame_rejects_missing_hooks_and_invalid_flags) {
    Scene scene(OriginalEdition::Patch1078);
    scene.Prepare(Fixture().kinds.at(2),2,3);
    auto hooks=scene.Hooks();
    CHECK(Throws([&] { auto broken=hooks;broken.pop={};SquidFrame invalid(scene.pool,scene.types,broken); }));
    CHECK(Throws([&] { auto broken=hooks;broken.frameSize={};SquidFrame invalid(scene.pool,scene.types,broken); }));
    SquidFrame frames(scene.pool,scene.types,hooks);
    const std::vector<std::uint8_t> before(scene.pool.Bytes().begin(),scene.pool.Bytes().end());
    CHECK(Throws([&] { frames.Set(kObject,1,1); }));
    CHECK(Throws([&] { frames.Set(kObject,1,0x2010); }));
    CHECK(scene.events.empty() && std::equal(before.begin(),before.end(),scene.pool.Bytes().begin()));
    // 현재 프레임이 표 밖이면 패치판 00419850의 assert다. 훅이 거부하고 단계 바이트·프레임은 그대로다.
    scene.Prepare(Fixture().kinds.at(2),2,9);
    const std::vector<std::uint8_t> invalid(scene.pool.Bytes().begin(),scene.pool.Bytes().end());
    CHECK(Throws([&] { frames.Set(kObject,1,0); }));
    CHECK(scene.events.empty() && std::equal(invalid.begin(),invalid.end(),scene.pool.Bytes().begin()));
}

// 기록 훅 대신 실제 Unpop/Pop에 잇는다: 프레임 크기가 다른 단계에 속하면 한 번 늦게(다음 지정 때) 해시 단계를 옮긴다.
// 표시 갱신은 연결하지 않은 비전투·표시 억제 공간에서의 통합 검사다.
TEST_CASE(set_frame_real_unpop_and_pop_move_hash_level_on_next_change) {
    // 같은 생성자/가상 표 배치를 쓰는 두 판본에서 확인한다.
    for (const auto edition:{OriginalEdition::Patch1078,OriginalEdition::Cd1072}) {
        // 실제 raw 생성자가 요구하는 원본 전체 슬롯 수와 연결 객체 타입 번호(공통 가상 표를 쓰는 비표면 타입)다.
        constexpr std::uint32_t capacity=32768,type=155;
        const bool patch=edition==OriginalEdition::Patch1078;
        const std::size_t frameOffset=patch ? 0x24 : 0x22,levelOffset=patch ? 0x21 : 0x1f;
        SidPool pool(edition,capacity,true);SquidHash hash;std::vector<std::uint8_t> spots(65536);
        std::vector<RiftTypeRecord> types(patch ? 188 : 171);
        types[type].constructorAddress=TypeConstructorAddress(edition,type);
        types[type].flags1=0x28000002;types[type].flags2=0x02000000;types[type].footX=types[type].footY=1;
        // 프레임 0·1은 1단계 크기, 2는 3단계 크기다.
        const std::array<std::array<float,2>,3> sizes{{{1.0f,1.0f},{2.0f,1.5f},{5.0f,5.0f}}};
        SquidUnpop unpop(pool,hash,spots);SquidPop pop(pool,hash,spots);
        SquidFactory factory(pool,types,false,&unpop);
        std::vector<std::string> order;
        SquidFrameHooks hooks;
        hooks.frameSize=[&](Sid,std::int32_t frame) { return sizes.at(static_cast<std::size_t>(frame)); };
        hooks.update=[&](Sid,std::uint32_t) { order.push_back("update"); };
        hooks.unpop=[&](Sid sid,std::uint32_t flags) { order.push_back("unpop");unpop.Unpop(sid,types[type],flags); };
        // 다시 등록할 때의 단계는 그때의 (새) 프레임 크기로 정한다.
        hooks.pop=[&](Sid sid,float x,float y,std::uint32_t flags) {
            order.push_back("pop");
            const auto size=sizes.at(pool.Slot(sid)[frameOffset]);
            CHECK(pop.Pop(sid,types[type],size[0],size[1],x,y,flags)==RawPopResult::Registered);
        };
        SquidFrame frames(pool,types,std::move(hooks));
        const Sid object=factory.Create(type);
        CHECK(pop.Pop(object,types[type],sizes[0][0],sizes[0][1],40.5f,41.25f)==RawPopResult::Registered);
        CHECK(pool.Slot(object)[levelOffset]==1 && hash.Bucket(1,40.5f,41.25f)==object.value);
        // 1. 같은 단계의 프레임 1로: 표시 갱신 두 번, 등록은 그대로다.
        frames.Set(object,1,0);
        CHECK(order==std::vector<std::string>({"update","update"}) && pool.Slot(object)[frameOffset]==1);
        // 2. 큰 프레임 2로: 현재 프레임(1)의 단계가 저장값과 같으므로 여전히 제자리에서 바뀐다. 1단계 버킷에 남는다.
        order.clear();frames.Set(object,2,0);
        CHECK(order==std::vector<std::string>({"update","update"}) && pool.Slot(object)[frameOffset]==2);
        CHECK(pool.Slot(object)[levelOffset]==1 && hash.Bucket(1,40.5f,41.25f)==object.value && hash.Bucket(3,40.5f,41.25f)==0);
        // 3. 같은 프레임을 다시 지정: 현재 프레임(2)의 단계 3이 저장값 1과 달라 단계 바이트만 3이 되고 끝난다.
        order.clear();frames.Set(object,2,0);
        CHECK(order.empty() && pool.Slot(object)[levelOffset]==3 && hash.Bucket(1,40.5f,41.25f)==object.value);
        // 4. 단계 바이트가 맞춰졌으므로 다음 지정은 다시 제자리 경로다(등록은 여전히 1단계 버킷).
        order.clear();frames.Set(object,1,0);
        CHECK(order==std::vector<std::string>({"update","update"}) && hash.Bucket(1,40.5f,41.25f)==object.value);
        // 5. 현재 프레임(1)의 단계 1이 저장값 3과 다르고 새 프레임이 다르면 Unpop → Pop(flags | 0x50)으로 옮긴다.
        order.clear();frames.Set(object,2,0);
        CHECK(order==std::vector<std::string>({"unpop","pop"}) && pool.Slot(object)[frameOffset]==2);
        CHECK(pool.Slot(object)[levelOffset]==3 && hash.Bucket(1,40.5f,41.25f)==0 && hash.Bucket(3,40.5f,41.25f)==object.value);
        CHECK((pool.Slot(object)[11]&4)==0);
    }
}
