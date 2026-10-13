// 세 PE의 저장 관찰로 표시 영역·가시 목록·16바이트 출력·정렬 키를 대조한다. 원본 게임은 실행하지 않는다.
#include "ConstructionSupport.h"
#include "o/RawRenderCollection.h"
#include "client/Renderer.h"
#include <cstring>

namespace {
using namespace netstorm::test::construction;
// 독립 관찰의 공통 입력 열 순서다. 부동소수 값도 DWORD 비트로 읽는다.
enum Input:std::size_t { Sx,Sy,Left,Top,Right,Bottom,Vleft,Vtop,Vright,Vbottom,Slots,Pattern,Limit,Z,X,Y,Flags };
// 원본 함수의 반환값과 출력 바이트는 fixture에서만 읽는다.
const auto& Fixture() { static const auto rows=LoadFixture(NETSTORM_RENDERCOLLECTION_FIXTURE).rows;return rows; }
// 자료 구조의 실제 바이트를 직렬화하여 미사용 출력과 마지막 BYTE도 비교한다.
template<class T> std::string Bytes(const T& value) {
    return Hex({reinterpret_cast<const std::uint8_t*>(&value),sizeof(value)});
}
// 실제 raw 풀/등록 목록에 완성된 객체 입력을 공급한다. 기대 환산이나 가시 여부는 계산하지 않는다.
struct CollectionScene {
    // pool: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀. 입력 구성 또는 실행 결과 대조에 사용한다.
    SidPool pool;/* state: 검사 입력과 실행 뒤 결과를 보관할 상태. */ ConstructionVisualState state;/* fields: 원본 프로세스의 직접 읽기·쓰기 필드 이미지. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<RenderProcessFields> fields;
    // registry: 설정 객체나 등록 대상을 조회하는 등록부. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::unique_ptr<RawRenderRegistry> registry;/* input: 처리할 입력 또는 입력 분배기. 입력 구성 또는 실행 결과 대조에 사용한다. */ std::vector<std::uint32_t> input;
    // 판본에 따라 raw 슬롯의 폭만 바뀐다.
    explicit CollectionScene(OriginalEdition edition):pool(edition,kCapacity,false) {}
    // 등록/해제는 이 관찰에서 실행하지 않으므로 외부 효과를 요청하면 실패한다.
    RenderRegistryHooks Hooks() {
        RenderRegistryHooks hooks;hooks.reserve=[](std::uint32_t) { CHECK(false);return false; };
        hooks.releaseArray=[] { CHECK(false); };hooks.attach=[](RenderProcessFields&,std::uint32_t,Sid,std::uint32_t) { CHECK(false); };return hooks;
    }
    // 완성된 각 입력 기록을 읽어 슬롯·영역·부모를 연결한다. 실행기의 입력 생성 공식을 복제하지 않는다.
    void Prepare(const std::string& parameters,const std::string& objects) {
        input=Inputs(parameters);CHECK(input.size()==17);state.scaleX=Float(input[Sx]);state.scaleY=Float(input[Sy]);
        fields.assign(input[Slots],{});std::vector<std::uint32_t> slots(input[Slots]);
        if (objects!="-") {
            // 객체 기록은 slot,parent,z,left,top,right,bottom,x,y,flags 순서다.
            for (const auto& record:Split(objects,';')) {
                const auto values=Inputs(record);CHECK(values.size()==10);const auto slot=values[0];auto& field=fields.at(slot);
                slots.at(slot)=slot+1;field.token=slot+1;field.slot=static_cast<std::int32_t>(slot);field.parent=static_cast<std::int32_t>(values[1]);
                field.zOrder=std::bit_cast<std::int32_t>(values[2]);field.left=Float(values[3]);field.top=Float(values[4]);field.right=Float(values[5]);field.bottom=Float(values[6]);
                auto raw=InputRaw(pool,Sid{static_cast<std::uint16_t>(values[1])});Put(raw,6,7,2);raw[11]=static_cast<std::uint8_t>(values[9]);
                Put(raw,14,values[7]);Put(raw,18,values[8]);
            }
        }
        registry=std::make_unique<RawRenderRegistry>(pool,Hooks(),std::move(slots));
    }
    // 현재 필드와 목록의 수명은 반환한 collector보다 길다.
    RawRenderCollection Collector() {
        return RawRenderCollection(pool,state,*registry,[this](std::uint32_t token) { return &fields.at(token-1); });
    }
};
// 원본 함수의 반환값·환산 결과·전체 출력 버퍼를 세 판본에서 같은 C++ 경로로 비교한다.
void Replay(std::string_view edition) {
    CollectionScene scene(edition=="originals" ? OriginalEdition::Patch1078 : OriginalEdition::Cd1072);std::size_t count=0;
    // 각 행의 기대 출력은 독립 PE 실행으로 생성한 저장 관찰에서만 읽는다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;CHECK(row.size()==8);scene.Prepare(row[2],row[3]);auto collector=scene.Collector();const auto& input=scene.input;
        const RenderWorldRect view{Float(input[Vleft]),Float(input[Vtop]),Float(input[Vright]),Float(input[Vbottom])};
        std::array<RenderCollectionEntry,8> output;std::memset(output.data(),0xa5,sizeof(output));bool same=false;
        if (row[1]=="Bounds") same=Bytes(collector.Bounds(scene.fields.at(0)))==row[6] && Bytes(output)==row[5];
        else if (row[1]=="Intersect") {
            same=collector.Intersects(view,{Float(input[Left]),Float(input[Top]),Float(input[Right]),Float(input[Bottom])},scene.pool.Edition())==(Number(row[4])!=0) &&
                static_cast<std::uint32_t>(collector.Collect(view,output,1))==Number(row[4]);
        }
        else if (row[1]=="Collect") {
            const auto result=collector.Collect(view,output,std::bit_cast<std::int32_t>(input[Limit]));
            same=static_cast<std::uint32_t>(result)==Number(row[4]) && Bytes(output)==row[5];
        } else if (row[1]=="Order") {
            const auto result=netstorm::client::CompareDrawOrder({Float(input[X]),Float(input[Y]),std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(input[Z]))},{});
            same=static_cast<std::uint32_t>(result)==Number(row[4]);
        }
        CHECK(same);
        if (!same) { std::printf("표시 수집 %s 행 %zu %s 불일치\n",std::string(edition).c_str(),count,row[1].c_str());break; }++count;
    }
    CHECK(count==1435);
}
}
// 10.78의 역수 곱셈과 별도 교차 helper를 포함한 관찰이다.
TEST_CASE(RenderCollection_PatchNative) { Replay("originals"); }
// CD판의 직접 나눗셈과 collector 내부 교차를 포함한 관찰이다.
TEST_CASE(RenderCollection_CdNative) { Replay("originalCD"); }
// 추가 PE도 별도의 저장 관찰로 검사한다.
TEST_CASE(RenderCollection_AdditionalNative) { Replay("original1037"); }

// 호스트 연결 오류를 변경 전에 거부하고 용량을 채운 뒤 잘못된 후속 부모에 접근하지 않는다.
TEST_CASE(RenderCollection_ContractsAndEarlyLimit) {
    CollectionScene scene(OriginalEdition::Patch1078);scene.Prepare("1065353216,1065353216,0,0,1092616192,1092616192,0,0,1101004800,1101004800,2,0,1,0,0,0,0",
        "0,50,0,0,0,1092616192,1092616192,0,0,255;1,51,0,0,0,1092616192,1092616192,0,0,0");
    auto collector=scene.Collector();std::array<RenderCollectionEntry,1> output{};const RenderWorldRect view{0,0,20,20};
    Put(InputRaw(scene.pool,{51}),6,0,2);CHECK(collector.Collect(view,output,1)==1 && output[0].token==1);
    CHECK(Throws([&] { collector.Collect(view,output,2); }));CHECK(output[0].token==1);
    scene.state.scaleX=0;CHECK(collector.Collect(view,{},0)==0 && collector.Collect(view,{},-1)==0);
    CHECK(Throws([&] { collector.Bounds(scene.fields[0]); }));scene.state.scaleX=Float(0x7f800000);
    CHECK(Throws([&] { collector.Bounds(scene.fields[0]); }));scene.state.scaleX=Float(0x7fc00000);
    CHECK(Throws([&] { collector.Bounds(scene.fields[0]); }));scene.state.scaleX=1;
    scene.fields[0].parent=-1;CHECK(Throws([&] { collector.Bounds(scene.fields[0]); }));scene.fields[0].parent=static_cast<std::int32_t>(scene.pool.Capacity());
    CHECK(Throws([&] { collector.Bounds(scene.fields[0]); }));scene.fields[0].parent=51;CHECK(Throws([&] { collector.Bounds(scene.fields[0]); }));
    SidPool other(OriginalEdition::Patch1078,kCapacity,false);
    CHECK(Throws([&] { RawRenderCollection invalid(other,scene.state,*scene.registry,[](std::uint32_t) { return static_cast<const RenderProcessFields*>(nullptr); }); }));
    CHECK(Throws([&] { RawRenderCollection invalid(scene.pool,scene.state,*scene.registry,{}); }));
    RawRenderCollection incomplete(scene.pool,scene.state,*scene.registry,[](std::uint32_t) { return static_cast<const RenderProcessFields*>(nullptr); });
    CHECK(incomplete.Collect(view,output,1)==0 && output[0].token==1);
}
