// 기계어 실행기(decomp_bridgeconnect_oracle.py 계열)의 장면 입력을 raw SID 풀·해시·spot·타입 표로 준비하는 검사 공용 도구.
// 장면 형식: 객체 | spot | 프레임 변형 | 소유자 | 전역(다리 타입, 전투, 미션, 색 표 10칸). 판단 결과는 계산하지 않는다.
#pragma once
#include "TestSupport.h"
#include "o/RawBridgeConnect.h"
#include <algorithm>
#include <bit>
#include <fstream>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

namespace netstorm::test::rawscene {
using namespace netstorm::o;
// 기계어 실행기의 입력 번호·가상 표 기록값이다. 호스트 함수 주소로 해석하지 않는다.
inline constexpr Sid kSource{50};
// 대체 생성이 차례로 돌려주는 번호의 시작과 개수, 실행기 풀의 마지막 번호.
inline constexpr std::uint16_t kFirstBorn=100,kBornCount=12,kLastSid=127;
inline constexpr std::uint32_t kVtable=0x11010000,kCapacity=24000;
// 기계어 실행기가 쓰는 합성 타입 번호. 플래그·발자국은 실제 10.78 타입 표의 값이다.
inline constexpr std::uint32_t kBridgeType=82,kConnectorType=90,kIslandType=91,kStalagType=92,kNoIslandType=93;
struct TypeInput { std::uint32_t number,flags1,flags2; int footX,footY; };
// bridge, isle, isleBig, priest(비표면 walker), bridgeConnector, island, islandStalag, noIsland 순서다.
inline constexpr std::array<TypeInput,8> kTypeInputs{{{82,0x00000802,0x00000004,1,1},{83,0x28000803,0x00000002,1,1},
    {84,0x08000803,0x01000002,3,3},{85,0x00069012,0x00210000,1,1},{90,0x28000002,0x02000000,1,1},
    {91,0x28000003,0x01000000,3,3},{92,0x28000002,0x00000000,3,3},{93,0x08000803,0x01000002,1,1}}};
// 마지막 빈 필드도 보존한다.
inline std::vector<std::string> Split(std::string_view text,char separator) {
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
inline std::uint32_t Number(const std::string& value) { return static_cast<std::uint32_t>(std::stoul(value)); }
// little endian raw 필드를 쓰되 주변 바이트는 보존한다.
inline void Put(std::span<std::uint8_t> raw,std::size_t offset,std::uint32_t value,std::size_t width=4) {
    // 원본 필드의 폭만큼 낮은 바이트부터 쓴다.
    for (std::size_t i=0;i<width;++i) raw[offset+i]=static_cast<std::uint8_t>(value>>(8*i));
}
// little endian raw 필드를 읽는다.
inline std::uint32_t Get(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 낮은 바이트부터 합친다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 슬롯 전체를 실행기와 같은 소문자 16진수로 만든다.
inline std::string Hex(std::span<const std::uint8_t> bytes) {
    static const char digits[]="0123456789abcdef";std::string text;
    // 바이트마다 두 글자를 쓴다.
    for (const auto byte:bytes) { text+=digits[byte>>4];text+=digits[byte&15]; }
    return text;
}
// zlib과 같은 Adler-32다.
inline std::uint32_t Adler(std::span<const std::uint8_t> bytes) {
    std::uint32_t a=1,b=0;
    // 두 합을 65521로 나눈 나머지로 유지한다.
    for (const auto byte:bytes) { a=(a+byte)%65521U;b=(b+a)%65521U; }
    return (b<<16)|a;
}
// 입력된 합성 코드 표를 bridgeevent 도구의 세 변형과 같게 만든다. 판단 결과는 계산하지 않는다.
inline RiftTypeFrames Frames(int variant) {
    std::vector<FrameCode> codes;
    const std::string letters="JKALMNOJKLBJPPCD";
    // variant별 hard 비트는 bridgeevent 도구와 같은 입력이다.
    for (std::size_t i=0;i<letters.size();++i) {
        const bool hard=variant==1 ? i==0 || i==1 || i==7 || i==8 || i==11 : variant==2 && (i==0 || i==8);
        const auto flags=static_cast<std::uint8_t>(hard ? 0x40 : letters[i]=='L' || letters[i]=='M' ? 0x20 : 0);
        codes.push_back({static_cast<std::uint8_t>(letters[i]),'P',static_cast<std::uint8_t>(i+1),flags});
    }
    return RiftTypeFrames(std::move(codes));
}
// 저장된 독립 기계어 출력: 장면 입력(번호 → 칸)과 관찰 행이다. 원본 PE/Python 없이 읽는다.
struct FixtureData {
    std::map<std::string,std::vector<std::string>> scenes;
    std::vector<std::vector<std::string>> rows;
};
// 주석을 제외하고 탭으로 입력/관찰 칸을 나눈다. 장면 행은 번호로 찾을 수 있게 따로 둔다.
inline FixtureData LoadFixture(const char* path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error(std::string("fixture 없음: ")+path);
    FixtureData result;std::string line;
    // 파일 끝까지 한 줄씩 읽는다.
    while (std::getline(input,line)) {
        if (!line.empty() && line.back()=='\r') line.pop_back();
        if (line.empty() || line.front()=='#') continue;
        auto row=Split(line,'\t');
        if (row[0]=="Scene") result.scenes.emplace(row.at(1),std::vector<std::string>(row.begin()+2,row.end()));
        else result.rows.push_back(std::move(row));
    }
    return result;
}
// 명시적인 보호 경로를 확인한다.
template<class F> bool Throws(F operation) { try { operation(); } catch (const std::exception&) { return true; } return false; }
struct Scene {
    SidPool pool;
    SquidHash hash;
    std::vector<std::uint8_t> spots=std::vector<std::uint8_t>(65536);
    std::vector<RiftTypeRecord> types;
    std::vector<RiftTypeFrames> frames;
    std::unique_ptr<RawSquidNeighbors> neighbors;
    BridgeConnectState state;
    std::vector<std::string> events;
    std::uint16_t born{};
    bool patch{};
    // 실행기와 같은 풀 번호 5~127을 확보한다. 타입/프레임 입력은 행별로 준비한다.
    explicit Scene(OriginalEdition edition):pool(edition,kCapacity,false),types(edition==OriginalEdition::Patch1078 ? 188 : 171),
        frames(types.size(),RiftTypeFrames({})),patch(edition==OriginalEdition::Patch1078) {
        // 예측/서버 목록과 무관한 client 번호를 입력 슬롯으로 확보한다.
        for (std::uint16_t id=5;id<=kLastSid;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // 판본별 필드 위치.
    std::size_t OwnerOffset() const { return patch ? 34 : 32; }
    std::size_t ExtraOffset() const { return patch ? 40 : 35; }
    std::size_t FrameOffset() const { return patch ? 36 : 34; }
    std::size_t FrameWidth() const { return patch ? 4 : 1; }
    // 장면 입력 칸(객체, spot, 프레임 변형, 소유자, 전역)으로 raw 슬롯·타입·실제 네 단계 체인·전역을 준비한다.
    void Prepare(const std::vector<std::string>& columns) {
        CHECK(columns.size()==5);
        const int variant=std::stoi(columns[2]);
        hash.Reset();std::fill(spots.begin(),spots.end(),std::uint8_t{});events.clear();born=0;
        // 앞선 행의 슬롯이 남지 않도록 실행기 풀 전체를 지운다.
        for (std::uint16_t id=5;id<=kLastSid;++id) { auto raw=pool.AllocatedBytes(Sid{id});std::fill(raw.begin(),raw.end(),std::uint8_t{}); }
        // 객체가 없는 타입도 조회 대상이므로 합성 타입 표 전체를 넣는다.
        for (const auto& input:kTypeInputs) {
            auto& type=types.at(input.number);type.flags1=input.flags1;type.flags2=input.flags2;type.footX=input.footX;type.footY=input.footY;
            frames.at(input.number)=Frames(variant);
        }
        // 각 node는 분석 도구에 넣은 12개 정수 필드다: 번호, 타입, 상태, extra, x, y, 폭, 높이, flags1, flags2, 프레임, 해시 단계.
        for (const auto& entry:Split(columns[0],';')) {
            const auto n=Split(entry,',');CHECK(n.size()==12);
            const Sid sid{static_cast<std::uint16_t>(Number(n[0]))};const auto typ=Number(n[1]);
            auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});
            Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(typ);raw[11]=static_cast<std::uint8_t>(Number(n[2]));
            raw[ExtraOffset()]=static_cast<std::uint8_t>(Number(n[3]));
            Put(raw,14,Number(n[4]));Put(raw,18,Number(n[5]));
            Put(raw,FrameOffset(),Number(n[10]),FrameWidth());
            auto& type=types.at(typ);type.footX=std::stoi(n[6]);type.footY=std::stoi(n[7]);type.flags1=Number(n[8]);type.flags2=Number(n[9]);
            frames.at(typ)=Frames(variant);
            auto& head=hash.Bucket(std::stoi(n[11]),std::bit_cast<float>(Number(n[4])),std::bit_cast<float>(Number(n[5])));
            Put(raw,4,head,2);head=sid.value;
        }
        // 기준 객체의 두 단어는 실행기의 고정 입력(+8 = 0x2a2b, +12 = 0x1234)이다.
        Put(pool.AllocatedBytes(kSource),8,0x2a2b,2);Put(pool.AllocatedBytes(kSource),12,0x1234,2);
        if (columns[1]!="-") {
            // spot 입력은 x, y, 값이다.
            for (const auto& entry:Split(columns[1],';')) {
                const auto n=Split(entry,',');spots.at(static_cast<std::size_t>(std::stoi(n[1])*256+std::stoi(n[0])))=static_cast<std::uint8_t>(Number(n[2]));
            }
        }
        if (columns[3]!="-") {
            // 소유자 입력은 번호, 소유자다.
            for (const auto& entry:Split(columns[3],';')) {
                const auto n=Split(entry,',');pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(Number(n[0]))})[OwnerOffset()]=static_cast<std::uint8_t>(Number(n[1]));
            }
        }
        // 전역: 다리 타입, 전투, 미션, 색 표 10칸(표는 앞의 9칸이다).
        const auto globals=Split(columns[4],',');CHECK(globals.size()==13);
        state=BridgeConnectState{};
        state.connectorType=kConnectorType;state.islandType=kIslandType;state.stalagType=kStalagType;state.noIslandType=kNoIslandType;
        state.bridgeType=Number(globals[0]);state.battle=globals[1]=="1";state.mission=globals[2]=="1";
        // 색 표의 아홉 칸을 넣는다.
        for (std::size_t i=0;i<state.ownerColors.size();++i) state.ownerColors[i]=std::stoi(globals[3+i]);
        neighbors=std::make_unique<RawSquidNeighbors>(pool,hash,spots,types,frames);
    }
    // 프레임 필드를 판본의 폭으로 쓴다.
    void PutFrame(Sid sid,std::uint32_t frame) { Put(pool.AllocatedBytes(sid),FrameOffset(),frame,FrameWidth()); }
    // 원본 실행기의 대체와 같은 경계다: 호출 사실과 인자를 기록하고, 소유자 지정은 소유자 바이트만, 프레임 지정은 프레임 필드만 쓴다.
    BridgeConnectHooks Hooks() {
        BridgeConnectHooks hooks;
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) {
            if (born>=kBornCount) throw std::runtime_error("새 객체 번호 소진");
            const Sid sid{static_cast<std::uint16_t>(kFirstBorn+born++)};
            auto raw=pool.AllocatedBytes(sid);std::fill(raw.begin(),raw.end(),std::uint8_t{});
            Put(raw,0,kVtable);raw[10]=static_cast<std::uint8_t>(type);raw[11]=0;
            events.push_back("C:"+std::to_string(type)+':'+std::to_string(flags));return sid;
        };
        hooks.setOwner=[this](Sid sid,std::uint32_t owner) {
            pool.AllocatedBytes(sid)[OwnerOffset()]=static_cast<std::uint8_t>(owner);
            events.push_back("O:"+std::to_string(sid.value)+':'+std::to_string(owner));
        };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) {
            events.push_back("P:"+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+
                std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(flags));
        };
        hooks.setFrame=[this](Sid sid,std::int32_t frame,std::uint32_t flags) {
            PutFrame(sid,std::bit_cast<std::uint32_t>(frame));
            events.push_back("S:"+std::to_string(sid.value)+':'+std::to_string(std::bit_cast<std::uint32_t>(frame))+':'+std::to_string(flags));
        };
        hooks.notifySurface=[this](Sid sid) { events.push_back("N:"+std::to_string(sid.value)); };
        return hooks;
    }
    // 모든 원본 효과의 순서와 인자를 한 문자열로 만든다.
    std::string Events() const {
        std::string text;
        // 사건 사이에 세미콜론을 둔다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        return text.empty() ? "-" : text;
    }
    // 만든 객체 슬롯 전체의 16진수 목록이다.
    std::string Born() const {
        std::string text;
        // 만든 순서대로 쉼표로 잇는다.
        for (std::uint16_t i=0;i<born;++i) { if (i) text+=',';text+=Hex(pool.Slot(Sid{static_cast<std::uint16_t>(kFirstBorn+i)})); }
        return text.empty() ? "-" : text;
    }
    // 실행기 풀(번호 0~127)의 Adler-32다. 번호 0~4는 실행기에서 항상 0이다.
    std::uint32_t PoolAdler() const {
        const auto stride=pool.Layout().stride;
        std::vector<std::uint8_t> image((kLastSid+1U)*stride);
        // 확보한 슬롯만 실제 바이트를 복사한다.
        for (std::uint16_t id=5;id<=kLastSid;++id) {
            const auto raw=pool.Slot(Sid{id});std::copy(raw.begin(),raw.end(),image.begin()+static_cast<std::ptrdiff_t>(id*stride));
        }
        return Adler(image);
    }
};
}
