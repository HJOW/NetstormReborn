// 원본 실제 finder/태그의259개 입력과 제품의 같은 현재 해시/삭제 순서를 비교한다.
#include "RawSceneSupport.h"
#include "o/RawFenceMarkerClear.h"
#include <cstdio>

using namespace netstorm::o;
using namespace netstorm::test::rawscene;
namespace {
constexpr std::array<std::uint16_t,6> kMarkerIds{50,51,60,61,62,63}; //두 끝점/세표식/다른타입의 원본 입력 SID다.
// 단일 raw hash/finder/중심과 명시 삭제 변경 입력을 구성한다.
struct MarkerClearScene {
    SidPool pool{OriginalEdition::Patch1078,kCapacity,false}; //실제 원본 할당 영역을 포함한 풀이다.
    SquidHash hash; //현재 여섯 입력 객체의 실제 공간 버킷이다.
    std::vector<RiftTypeRecord> types{188}; //finder와 중심이 함께 읽는 현재 발자국 표다.
    std::vector<GunTargetType> targetTypes{188}; //같은 중심 조회 객체의 현재 원본TYPE 부가 표다.
    std::vector<SquidDisplayShape> shapes{188}; //중심에는 사용하지 않는 명시 SHP 경계다.
    GunTargetState state; //같은 실제 표적 조회기의 현재 상태다.
    RawGunTarget target{pool,hash,types,targetTypes,shapes,state}; //004adea0의 실제 복원된 중심을 사용한다.
    std::uint32_t markerType{90}; //원본005411c0이며 포스 필드170과 구별한다.
    std::vector<std::uint32_t> input; //scene/두 태그 mask/삭제 변경의 원본 입력이다.
    std::string trace; //가상 삭제를 요청한 실제 순서다.
    std::size_t deletions{}; //명시 첫 삭제 변경을 구별하는 누계다.
    std::unique_ptr<RawFenceMarkerClear> body; //모든 현재 저장소를 빌리는 실제 제품 모듈이다.
    // 결과를 재계산하지 않고 동일한 입력 슬롯/버킷/태그만 설치한다.
    explicit MarkerClearScene(const std::string& text) {
        for (const auto& value:Split(text,',')) input.push_back(Number(value)); //원본의 네 입력만 읽는다.
        CHECK(input.size()==4);
        for (std::uint16_t sid=5;sid<=63;++sid) CHECK(pool.Allocate(2)==Sid{sid}); //필요한 실제 번호까지 확보한다.
        for (const auto type:{90U,91U,136U}) types[type].footX=types[type].footY=1; //같은 원본1×1 발자국을 공급한다.
        const std::array<float,2> first{50.25f,50.5f},second=input[0]==0 ? std::array<float,2>{56.25f,50.5f} : input[0]==1 ? std::array<float,2>{50.25f,56.5f} : input[0]==2 ? std::array<float,2>{56.25f,56.5f} : std::array<float,2>{44.25f,50.5f};
        for (auto item=kMarkerIds.rbegin();item!=kMarkerIds.rend();++item) { //native setup과 같은 역순 머리 삽입이다.
            const auto sid=*item;const auto point=sid==50 ? first : sid==51 ? second : std::array<float,2>{(first[0]+second[0])*.5f+(static_cast<int>(sid)-61)*.25f,(first[1]+second[1])*.5f};
            auto raw=pool.AllocatedBytes(Sid{sid});std::fill(raw.begin(),raw.end(),std::uint8_t{});Put(raw,0,0x11018000);raw[10]=static_cast<std::uint8_t>(sid<60 ? 136 : sid==63 ? 91 : 90);Put(raw,14,std::bit_cast<std::uint32_t>(point[0]));Put(raw,18,std::bit_cast<std::uint32_t>(point[1]));raw[33]=1;
            const bool packed=sid>=60 && (input[2]&(1U<<(sid-60))); //입력한 packed 형식의 일치 여부만 기록한다.
            Put(raw,12,sid==63 || (sid>=60 && (input[1]&(1U<<(sid-60)))) ? 50 : 77,2);Put(raw,26,packed ? 0 : 1);raw[30]=static_cast<std::uint8_t>(packed ? 50 : 77);
            auto& head=hash.Bucket(1,point[0],point[1]);Put(raw,4,head,2);head=sid;
        }
        body=std::make_unique<RawFenceMarkerClear>(pool,hash,types,target,markerType,[this](Sid sid,std::uint32_t flags) {
            CHECK(flags==64);if (!trace.empty()) trace+=';';trace+="D:"+std::to_string(sid.value)+':'+std::to_string(flags);++deletions;
            if (input[3]==1) Put(pool.AllocatedBytes(sid),12,77,2);
            if (input[3]==2 && deletions==1) Put(pool.AllocatedBytes(Sid{61}),12,50,2);
            if (input[3]==3 && deletions==1) Put(pool.AllocatedBytes(sid),4,0,2);
        });
    }
    std::string Objects() const { std::string value;for (const auto sid:kMarkerIds) value+=Hex(pool.Slot(Sid{sid}));return value; } //원본 여섯 슬롯의 전체300BYTE다.
};
}
// 실제 원본 중심/Rect/finder/태그/현재 삭제 효과를 전수 대조한다.
TEST_CASE(FenceMarkerClear_Native259) {
    const auto fixture=LoadFixture(NETSTORM_FENCE_MARKER_CLEAR_FIXTURE);CHECK(fixture.rows.size()==259);std::size_t count{}; //독립 고유 입력 수다.
    for (const auto& row:fixture.rows) { //결과는 원본 관찰로부터만 읽는다.
        CHECK(row.size()==4);MarkerClearScene scene(row[0]);const auto area=scene.body->Bounds(kSource,Sid{51});
        const auto bounds=std::to_string(area.left)+','+std::to_string(area.top)+','+std::to_string(area.right)+','+std::to_string(area.bottom);
        scene.body->Clear(kSource,Sid{51});const auto trace=scene.trace.empty() ? "-" : scene.trace;
        const bool same=bounds==row[1] && trace==row[2] && scene.Objects()==row[3];CHECK(same);++count;
        if (!same) { std::fprintf(stderr,"fencemark 불일치 입력%s 영역%s/%s 삭제%s/%s\n",row[0].c_str(),bounds.c_str(),row[1].c_str(),trace.c_str(),row[2].c_str());break; }
    }
    CHECK(count==259);
}
// 선택된 삭제만 필수이며 다른 타입은 현재 태그가 같아도 삭제하지 않는다.
TEST_CASE(FenceMarkerClear_RequiredDeletionAndCurrentType) {
    MarkerClearScene scene("0,0,0,0");RawFenceMarkerClear missing(scene.pool,scene.hash,scene.types,scene.target,scene.markerType,{});missing.Clear(kSource,Sid{51});CHECK(scene.trace.empty());
    Put(scene.pool.AllocatedBytes(Sid{60}),12,50,2);bool failed{};try { missing.Clear(kSource,Sid{51}); } catch (const std::logic_error&) { failed=true; }CHECK(failed);
    scene.markerType=91;scene.body->Clear(kSource,Sid{51});CHECK(scene.trace=="D:63:64");
}
