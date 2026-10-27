// 실제10.78 TYPE129 포구·종점·130 생성/비행을 독립 원본 행과 전체 비트로 대조한다.
#include "RawSceneSupport.h"
#include "o/RawMissile.h"
#include "o/RawProjectileGeometry.h"
#include "o/RawGunMode.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
// 원본 도구의 KEYS 순서와 일치하는 독립 입력 열이다.
enum Column : std::size_t {
    Kind,X,Y,HotX,HotY,TypeHotX,TypeHotY,EndX,EndY,Fraction,TargetState,Previous,
    Corner,Range,FiringSound,DisplaySound,ImpactSound,Directions,Frames,Extra,Passed,SourceFrame,Side,SoundRoll,Output
};
// 실제 PE가 만든 기대값만 읽으며 C++로 관찰 결과를 재계산하지 않는다.
const std::vector<std::vector<std::string>>& Rows() {
    static const auto rows=LoadFixture(NETSTORM_ICE_MISSILE_INITIAL_FIXTURE).rows;return rows;
}
// 원본의 세 자산과 같은 raw 저장소·타입·공간 경계를 구성한다.
struct IceMissileInitialScene {
    SidPool pool{OriginalEdition::Patch1078,kCapacity,false}; // 같은 SID50/60/90 저장소다.
    std::vector<RiftTypeRecord> types{188}; // 실제 group1/타입 hotspot 입력이다.
    std::vector<MissileTypeData> missileTypes{188}; // 사거리·방향 유무·사운드 토큰이다.
    std::vector<ProjectileGeometryType> geometryTypes{188}; // 실제 포구/현재 SHP/방향 자료다.
    std::vector<RiftTypeFrames> frameCodes=std::vector<RiftTypeFrames>(188,RiftTypeFrames({})); // 원본 TYPE+124의 실제4BYTE 문자 입력이다.
    std::uint32_t windType{140}; // 원본 방향 getter의 크로스보우 전용 현재 타입이다.
    RawGunMode gunMode{pool,frameCodes,windType}; // 같은 raw 풀/현재 프레임을 읽는 실제 방향 몸체다.
    MissileState state; // 동일 현재 게임 시각과 기본 원본 전역이다.
    MissileFields fields; // 원본128BYTE 이미지와 padding을 유지한다.
    std::vector<std::string> events; // 명시 외부 경계의 호출 순서와 인자다.
    const std::vector<std::string>* input{}; // 현재 독립 입력 행이다.
    // 실제 입력 번호를 유지하도록 슬롯을 미리 확보한다.
    IceMissileInitialScene() {
        // fixture가 사용하는 세 번호를 포함한 풀을 한 번 준비한다.
        for (std::uint16_t id=5;id<=100;++id) CHECK(pool.Allocate(2)==Sid{id});
    }
    // free/dead 입력도 독립 행으로 재설정하는 검사 전용 raw 접근이다.
    std::span<std::uint8_t> Slot(Sid sid) {
        const auto raw=pool.Slot(sid);return {const_cast<std::uint8_t*>(raw.data()),raw.size()};
    }
    // binary32 독립 입력의 전체 비트를 그대로 읽는다.
    float F(Column column) const { return std::bit_cast<float>(Number(input->at(column))); }
    // native 실행기와 동일한 독립 이미지와 메타데이터를 구성한다.
    void Prepare(const std::vector<std::string>& row) {
        CHECK(row.size()==Output+4);input=&row;events.clear();state.now=Number(row[Kind])==0 ? 10.25 : 10+Number(row[Fraction])*0.5;
        // 원본 생성자가 쓰지 않는 바이트도 동일하게 비교한다.
        for (std::size_t i=0;i<fields.bytes.size();++i) fields.bytes[i]=static_cast<std::uint8_t>(i*13+7);
        // 원본의 source/target/display 세 슬롯을 동일하게 구성한다. 현재 source 문자는 별도 프레임 입력이다.
        for (const auto [sid,type]:{std::array<std::uint16_t,2>{50,129},{60,71},{90,130}}) {
            auto raw=Slot(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x11319000);raw[10]=static_cast<std::uint8_t>(type);raw[11]=static_cast<std::uint8_t>(sid==60 ? Number(row[TargetState]) : 0);raw[34]=1;raw[40]=static_cast<std::uint8_t>((sid==50 || sid==60) ? Number(row[Extra]) : 0);
            Put(raw,14,sid==50 ? Number(row[X]) : std::bit_cast<std::uint32_t>(50.0f));Put(raw,18,sid==50 ? Number(row[Y]) : std::bit_cast<std::uint32_t>(50.0f));
        }
        types[129].group=1;types[129].hotspotX=std::stoi(row[TypeHotX]);types[129].hotspotY=std::stoi(row[TypeHotY]);
        missileTypes[129].range=F(Range);missileTypes[129].firingSound=Number(row[FiringSound]);missileTypes[130].firingSound=Number(row[DisplaySound]);missileTypes[130].impactSound=Number(row[ImpactSound]);missileTypes[130].frameCount=std::stoi(row[Directions]);missileTypes[136].impactSound=Number(row[ImpactSound]);
        geometryTypes[129].defaultFrame=0;geometryTypes[129].frameCount=36;geometryTypes[129].shotOffsets.assign(36,{static_cast<std::int16_t>(std::stoi(row[HotX])),static_cast<std::int16_t>(std::stoi(row[HotY]))});geometryTypes[130].directions=std::stoi(row[Directions]);geometryTypes[130].frameCount=std::stoi(row[Frames]);
        Put(Slot(Sid{50}),36,Number(row[SourceFrame]));types[129].footX=types[129].footY=0;types[71].footX=2;types[71].footY=3;
        geometryTypes[129].range=F(Range);std::vector<FrameCode> codes(36);codes.at(Number(row[SourceFrame])).side=static_cast<std::uint8_t>(Number(row[Side]));frameCodes[129]=RiftTypeFrames(std::move(codes));
        if (Number(row[Kind])!=1) return;
        Put(fields.bytes,0,0x50d630);Put(fields.bytes,4,81);Put(fields.bytes,8,90);Put(fields.bytes,12,0);Put(fields.bytes,16,Number(row[Passed]));Put(fields.bytes,20,50);Put(fields.bytes,24,60);Put(fields.bytes,28,Number(row[Corner]));
        // 독립 frame 입력은 ctor 결과가 아닌 선언한 두 시각/좌표/상자다.
        for (std::size_t offset=32;offset<48;offset+=4) Put(fields.bytes,offset,0);
        Put(fields.bytes,48,std::bit_cast<std::uint32_t>(20.0f));Put(fields.bytes,52,0);Put(fields.bytes,56,Number(row[X]));Put(fields.bytes,60,Number(row[Y]));Put(fields.bytes,64,Number(row[EndX]));Put(fields.bytes,68,Number(row[EndY]));Put(fields.bytes,72,Number(row[EndX]));Put(fields.bytes,76,Number(row[EndY]));
        // 원본 무효 사각형과 이전 위치의 독립 입력이다.
        for (std::size_t offset=88;offset<104;offset+=4) Put(fields.bytes,offset,0xc61c4000);
        const auto end=std::bit_cast<std::uint64_t>(12.0),start=std::bit_cast<std::uint64_t>(10.0);Put(fields.bytes,104,static_cast<std::uint32_t>(end));Put(fields.bytes,108,static_cast<std::uint32_t>(end>>32));Put(fields.bytes,112,static_cast<std::uint32_t>(start));Put(fields.bytes,116,static_cast<std::uint32_t>(start>>32));Put(fields.bytes,120,std::bit_cast<std::uint32_t>(50.0f));Put(fields.bytes,124,Number(row[Previous]) ? std::bit_cast<std::uint32_t>(89.0f) : 0xc61c4000);
    }
    // 원본 외부 효과만 관찰하며 포구·SHP·방향 수치는 실제 제품 모듈을 사용한다.
    MissileHooks Hooks(const RawProjectileGeometry& geometry) {
        MissileHooks hooks;
        hooks.muzzle=[&geometry](Sid sid) { return geometry.Muzzle(sid); };
        hooks.hotspot=[this,&geometry](Sid sid) { events.push_back("H:50");return geometry.Hotspot(sid); };
        hooks.targetDimensions=[&geometry](Sid sid) { return geometry.Dimensions(sid); };
        hooks.rayEnd=[&geometry](std::array<float,2> from,std::array<float,2> to,Sid sid) { return geometry.CannonRayEnd(from,to,sid); };
        hooks.directionFrame=[&geometry](std::array<float,2> from,std::array<float,2> to,std::uint32_t type) {
            return geometry.DirectionFrame(from,to,type);
        };
        hooks.create=[this](std::uint32_t type,std::uint32_t flags) { events.push_back("A:"+std::to_string(type)+':'+std::to_string(flags));return Sid{90}; };
        hooks.owner=[this](Sid sid,std::uint8_t owner) { Slot(sid)[34]=owner;events.push_back("O:90:"+std::to_string(owner)); };
        hooks.pop=[this](Sid sid,float x,float y,std::uint32_t flags) { Put(Slot(sid),14,std::bit_cast<std::uint32_t>(x));Put(Slot(sid),18,std::bit_cast<std::uint32_t>(y));events.push_back("P:90:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(flags)); };
        hooks.unpop=[this](Sid,std::uint32_t flags) { events.push_back("U:90:"+std::to_string(flags)); };
        hooks.firingSound=[this](float x,float y,std::uint32_t token) { Sound(x,y,token); };
        hooks.impactSound=[this](float x,float y,std::uint32_t token) { Sound(x,y,token); };
        hooks.center=[this](Sid sid) { const auto raw=pool.Slot(sid);return std::array<float,2>{std::bit_cast<float>(Get(raw,14)),std::bit_cast<float>(Get(raw,18))}; };
        hooks.targetBounds=[this](Sid,std::array<float,2> point) { events.push_back("T:"+std::to_string(std::bit_cast<std::uint32_t>(point[0]))+':'+std::to_string(std::bit_cast<std::uint32_t>(point[1])));return std::array<float,4>{49,89,51,91}; };
        hooks.hit=[this](Sid firing,Sid target) { events.push_back("G:"+std::to_string(firing.value)+':'+std::to_string(target.value)); };
        hooks.hitEffect=[this](float x,float y) { events.push_back("H:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))); };
        hooks.refresh=[this](Sid) { events.push_back("Q:50"); };
        hooks.impact=[this](float x,float y,std::uint32_t mode,float payload) { events.push_back("I:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(mode)+':'+std::to_string(std::bit_cast<std::uint32_t>(payload))); };
        hooks.destroy=[this](Sid,std::uint32_t flags) { events.push_back("X:90:"+std::to_string(flags)); };
        hooks.randomInteger=[this](std::int32_t low,std::int32_t high) { events.push_back("N:"+std::to_string(low)+':'+std::to_string(high));return static_cast<std::int32_t>(Number(input->at(SoundRoll))); }; // 전용 정수 난수는 명시 외부 입력이다.
        hooks.impactName=[this](float x,float y,std::string_view name) { events.push_back("F:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::string(name)); }; // 파일명/위치 요청을 관찰하며 장치 재생으로 합산하지 않는다.
        hooks.ricochet=[this](const MissileFields& current) { events.push_back("R:"+std::to_string(current.Parent().value)+':'+std::to_string(current.Target().value)); }; // 실제 선택 위치만 대조하며 반사 생성 몸체는 이 fixture 밖이다.
        return hooks;
    }
    // 원본 cdecl 사운드 인자 다섯 개를 같은 순서로 기록한다.
    void Sound(float x,float y,std::uint32_t token) { events.push_back("S:"+std::to_string(std::bit_cast<std::uint32_t>(x))+':'+std::to_string(std::bit_cast<std::uint32_t>(y))+':'+std::to_string(token)+":0:0"); }
    // 실제 제품의 포구/생성/비행을 실행하고 전체 관찰 이미지를 직렬화한다.
    std::vector<std::string> Run() {
        RawProjectileGeometry geometry(pool,types,geometryTypes,16.0F,11.0F,&gunMode);RawMissile logic(pool,types,missileTypes,state,Hooks(geometry));std::array<std::uint32_t,2> point{};
        if (Number(input->at(Kind))==0) {
            GunProjectileRequest request;request.firing=Sid{50};request.target=Sid{60};request.speed=20;request.displayType=130;request.effectType=0;request.targetPosition={F(EndX),F(EndY)};request.targetCenter=Number(input->at(Corner)) ? request.targetPosition : std::array<float,2>{-10000,-10000};request.firingPosition={-10000,-10000};const auto asset=logic.Construct(fields,request);events.push_back("B:28:90:0:16");Put(fields.bytes,4,81);Put(fields.bytes,8,asset.value);
        } else if (Number(input->at(Kind))==1) logic.Frame(fields);
        else {
            const auto result=Number(input->at(Kind))==2 ? geometry.Muzzle(Sid{50}) : geometry.CannonRayEnd({F(X),F(Y)},{F(EndX),F(EndY)},Sid{50});
            point={std::bit_cast<std::uint32_t>(result[0]),std::bit_cast<std::uint32_t>(result[1])};
        }
        std::string actions,slots;
        // 명시 외부 효과의 순서를 그대로 보존한다.
        for (const auto& event:events) { if (!actions.empty()) actions+=';';actions+=event; }
        // 원본의 세 슬롯 전체150BYTE를 같은 순서로 비교한다.
        for (const auto sid:{Sid{50},Sid{60},Sid{90}}) slots+=Hex(pool.Slot(sid));
        std::array<std::uint8_t,8> output{};Put(output,0,point[0]);Put(output,4,point[1]);return {actions,Hex(fields.bytes),slots,Hex(output)};
    }
};
// 한 원본 몸체 종류의 모든 고유 행을 기대값 변경 없이 대조한다.
void Replay(std::uint32_t kind) {
    IceMissileInitialScene scene;std::size_t count=0;int failures=0;
    // 입력마다 전체 raw/효과/포구 결과를 검사하고 최초 차이를 출력한다.
    for (const auto& row:Rows()) {
        if (Number(row[Kind])!=kind) continue;scene.Prepare(row);const auto actual=scene.Run();const std::vector<std::string> expected(row.begin()+Output,row.end());CHECK(actual==expected);
        if (actual!=expected) {
            // 앞선 다섯 차이만 출력하고 원본 기대값은 그대로 둔다.
            for (std::size_t index=0;index<actual.size();++index) if (actual[index]!=expected[index]) std::printf("  IceMissileInitial kind%u 행%zu 결과%zu 기대%s / 실제%s\n",kind,count,index,expected[index].c_str(),actual[index].c_str());
            if (++failures>=5) break;
        }
        ++count;
    }
    constexpr std::array<std::size_t,4> kExpected{320,1029,8,336}; // 실제 원본 보고서의 종류별 고유 입력 수다.
    CHECK(count==kExpected.at(kind) && failures==0);
}
}
// 현재 SHP/type hotspot·실제 종점과130 방향/사운드 우선순위를 검사한다.
TEST_CASE(IceMissileInitial_NativeCtorAndDirection) { Replay(0); }
// 독립 비행 보간·일반/코너 충돌·사거리 종료·효과음·삭제를 검사한다.
TEST_CASE(IceMissileInitial_NativeFramesAndImpact) { Replay(1); }
// TYPE129의 실제 가상 포구가 원본 raw 좌표 두 DWORD를 복사함을 검사한다.
TEST_CASE(IceMissileInitial_NativeVirtualMuzzle) { Replay(2); }

// 모든 네 방향·signed 문자 fallback·불일치 사거리/클램프를 실제 종점과 비교한다.
TEST_CASE(IceMissileInitial_NativeRayEnd) { Replay(3); }
