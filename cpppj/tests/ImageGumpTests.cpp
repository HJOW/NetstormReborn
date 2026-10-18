// 독립 원본221입력의 전체 노드·호출과 실제 썬 캐논/골렘 SHP의 출력·적중을 검사한다.
#include "GumpRawSupport.h"
#include "DudeSpawnSupport.h"
#include "client/ImageGump.h"
#include "client/PaletteShade.h"

namespace {
using namespace netstorm::test::gumpraw;
constexpr std::uint32_t kNode=0x12401000,kShape=0x16000000,kOwnerTable=0x12408000; // 원본 격리 관측과 같은 명시 토큰이다.
// 생성자가 쓰지 않는 필드도 같은 임의 원문 바이트를 공급한다.
ImageGumpNode Initial() {
    ImageGumpNode node;static_cast<GumpCreateNode&>(node)=Decode(std::vector<std::uint8_t>(160,0xa5));node.type=node.frame=node.mode=0xa5a5a5a5;node.hotspotX=node.hotspotY=Float(0xa5a5a5a5);return node;
}
// 현재 파생 필드를 실제 원본0xb4바이트 위치로 기록한다.
std::string EncodeImage(const ImageGumpNode& node) {
    auto raw=Bytes(Encode(node));raw.resize(0xb4);Put(raw,160,node.type);Put(raw,164,node.frame);Put(raw,168,node.mode);Put(raw,172,Bits(node.hotspotX));Put(raw,176,Bits(node.hotspotY));return Hex(raw);
}
// 독립 입력/현재 전역과 같은 실제 기본 생성자를 보유한다.
struct ImageScene {
    GumpCreateState create;ImageGumpNode node=Initial(); // 하나의 실제 등록표와 안정 노드다.
    std::array<std::uint32_t,188> ownerMaps{};std::uint32_t owner{},answer{}; // 현재 원본 색표·owner·명시 적중 응답이다.
    ScreenPoint shadow{5,5},point{17,-9}; // 현재 선행 오프셋과 위치 공급이다.
    ImageGumpHeader header;ImageGumpType type{kShape,0,0}; // 명시한 원문 TYPE/헤더 입력이다.
    std::string trace;ImageGumpHooks hooks;GumpCreate factory; // 같은 기본 생성자/외부 효과 관측이다.
    // 계산 결과를 공급하지 않고 원본과 같은 조회·효과만 기록한다.
    ImageScene():factory(create,{[this](auto token)->GumpCreateNode* { return token==kNode ? &node : nullptr; },{}}) {
        create.serial=23;hooks.node=[this](auto token) { return token==kNode ? &node : nullptr; };
        hooks.type=[this](auto number) { CHECK(number==128);return type; };
        hooks.header=[this](auto number,auto frame) { Event("Header",{number,0,frame});return header; };
        hooks.position=[this](auto token) { CHECK(token==kNode);Event("Position");return point; };hooks.color=[this](auto token) { Event("Color",{token}); };
        hooks.attach=[this](auto token,float x,float y,auto parent,auto mode) { CHECK(token==kNode);Event("BaseAttach",{Bits(x),Bits(y),parent,mode}); };
        hooks.draw=[this](ImageGumpDrawRequest r) {
            const auto name=r.scaled ? (r.mapped ? "0x4dab40" : "0x4daaf0") : (r.mapped ? "0x4daab0" : "0x4daa70");
            if (r.scaled) Event(name,{r.shape,r.frame,static_cast<std::uint32_t>(r.position.x),static_cast<std::uint32_t>(r.position.y),0,static_cast<std::uint32_t>(r.scale),static_cast<std::uint32_t>(r.scale)});
            else Event(name,{r.shape,r.frame,static_cast<std::uint32_t>(r.position.x),static_cast<std::uint32_t>(r.position.y)});
        };
        hooks.hit=[this](ImageGumpHitRequest r) { Event("Hit",{static_cast<std::uint32_t>(r.point.x),static_cast<std::uint32_t>(r.point.y),r.shape,r.frame,static_cast<std::uint32_t>(r.origin.x),static_cast<std::uint32_t>(r.origin.y),0,static_cast<std::uint32_t>(r.scale),static_cast<std::uint32_t>(r.scale)});return answer; };
    }
    // 실제 인자를 원문의 구분자로 이어 기록한다.
    void Event(std::string text,std::initializer_list<std::uint32_t> values={}) {
        if (!trace.empty()) trace+=';';trace+=text;
        // 전체 DWORD 비트를 부호 해석 없이 보존한다.
        for (const auto value:values) trace+=','+std::to_string(value);
    }
};
}
// 전체 생성/재설정/모드/owner/배율/적중의 원문 바이트와 효과를 비교한다.
TEST_CASE(ImageGump_Native221) {
    std::size_t count{};
    // 각각의 독립 입력에서 동일 기본 저장소와 생성자를 사용한다.
    for (const auto& row:LoadFixture(NETSTORM_INVENTORY_IMAGE_FIXTURE).rows) {
        CHECK(row.size()==5);ImageScene s;const auto a=Inputs(row[1]);std::string result="-";
        ImageGump image(s.create,s.factory,{s.owner,s.ownerMaps,s.shadow},s.hooks);
        if (row[0]=="Construct" || row[0]=="Reset") {
            s.header={std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(a[0])),std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(a[1])),std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(a[2])),std::bit_cast<std::int16_t>(static_cast<std::uint16_t>(a[3]))};
            s.type.hotspotX=std::bit_cast<std::int32_t>(a[4]);s.type.hotspotY=std::bit_cast<std::int32_t>(a[5]);s.node.scale=Float(a[6]);
            if (row[0]=="Construct") result=std::to_string(image.Construct(kNode,128,7,a[7]));else image.SetFrame(kNode,128,7,a[7]);
        } else {
            s.node.vtable=0x501bb0;s.node.type=128;s.node.frame=7;s.node.hotspotX=12.25F;s.node.hotspotY=-3.75F;s.node.x=2.5F;s.node.y=-4.5F;
            if (row[0]=="Draw") {
                s.node.mode=a[0];s.node.scale=Float(a[1]);s.owner=a[2];s.ownerMaps[128]=a[3] ? kOwnerTable : 0;s.node.hotspotX=Float(a[4]);s.node.hotspotY=Float(a[5]);s.point={std::bit_cast<std::int32_t>(a[6]),std::bit_cast<std::int32_t>(a[7])};s.shadow={std::bit_cast<std::int32_t>(a[8]),std::bit_cast<std::int32_t>(a[9])};image.Draw(kNode);
            } else if (row[0]=="Hit") { s.node.scale=Float(a[0]);s.answer=a[3];result=std::to_string(image.Hit(kNode,Float(a[1]),Float(a[2]))); }
            else { CHECK(row[0]=="Attach");image.Attach(kNode,4,5,0x1234,7); }
        }
        const auto actual=EncodeImage(s.node);if (result!=row[2] || actual!=row[3] || s.trace!=row[4]) std::printf("이미지Gump 원문%zu %s result%d node%d trace%d\n실제:%s\n원본:%s\n",count,row[0].c_str(),result==row[2],actual==row[3],s.trace==row[4],s.trace.c_str(),row[4].c_str());
        CHECK(result==row[2]);CHECK(actual==row[3]);CHECK(s.trace==row[4]);++count;
    }
    CHECK(count==221);
}

// 실제 썬 캐논/골렘의 같은 VFX 출력과 투명 클릭을 연결하고 클립·선택색도 확인한다.
TEST_CASE(ImageGump_ActualCannonGolemPixelsAndHit) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game;ImageScene s;BindImageGumpAssets(s.hooks,assets);
    GumpDrawState clip;SetGumpDrawClip(clip,{0,0,200,200});std::vector<std::uint8_t> pixels(200*200+32,255);GumpPixelTarget target(pixels,200,200,clip);ColorMap current{};
    std::array<std::uint32_t,256> logical{};
    // 원본 파일 팔레트의 실제 RGB를 같은 명암표에 공급한다.
    for (std::size_t i=0;i<logical.size();++i) { const auto c=assets.Palette().Color(static_cast<std::uint8_t>(i));logical[i]=c.red|(std::uint32_t(c.green)<<8)|(std::uint32_t(c.blue)<<16); }
    const auto maps=BuildGumpShadeMaps(logical);ImageGumpPixelTarget raster(target,assets,current,[&](auto token) { CHECK(token==0x556438);if (token!=0x556438) throw std::logic_error("검사 미복원 색표");return maps.dark; });
    auto hooks=raster.Hooks(s.hooks);ImageGump image(s.create,s.factory,{s.owner,s.ownerMaps,s.shadow},hooks);
    // 실제128은 썬 캐논,133은 골렘이며 원본 기본 프레임을 선택한다.
    for (const std::uint32_t type:{128U,133U}) {
        const auto block=type-kFirstAssetTypeNumber;const auto frame=static_cast<std::uint32_t>(assets.Types()[block].definition.specialFrames.defaultFrame);
        image.Construct(kNode,type,frame,0);s.node.x=70;s.node.y=70;s.point={70,70};
        // 실제 원본 VFX의 단위/축소/반전/0배율을 모두 같은 픽셀·클릭 경로로 사용한다.
        for (const float scale:{1.0F,0.63F,-1.0F,0.0F}) {
            s.node.scale=scale;std::fill(pixels.begin(),pixels.end(),std::uint8_t{255});image.Draw(kNode);
            const auto painted=std::count_if(pixels.begin(),pixels.begin()+40000,[](auto color) { return color!=255; });if (scale>0) CHECK(painted>0);
            // 현재 화면 점과 같은 항목 로컬 점의 판정은 실제 출력 색255와 일치해야 한다.
            for (int y=0;y<196;y+=7) {
                // 양 축의 투명/불투명 구간을 고르게 검사한다.
                for (int x=0;x<196;x+=7) CHECK(image.Hit(kNode,static_cast<float>(x),static_cast<float>(y))==(pixels[static_cast<std::size_t>(y*200+x)]!=255 ? 1U : 0U));
            }
            CHECK(std::all_of(pixels.begin()+40000,pixels.end(),[](auto color) { return color==255; }));
        }
        s.node.scale=1;s.node.mode=0;std::fill(pixels.begin(),pixels.end(),std::uint8_t{255});image.Draw(kNode);const auto plain=pixels;s.node.mode=1;std::fill(pixels.begin(),pixels.end(),std::uint8_t{255});image.Draw(kNode);CHECK(plain!=pixels);
        SetGumpDrawClip(clip,{60,60,90,90});std::fill(pixels.begin(),pixels.end(),std::uint8_t{255});image.Draw(kNode);
        // 그림의 현재 clip 밖과 실제 표면 끝 padding을 보존한다.
        for (int y=0;y<200;++y) for (int x=0;x<200;++x) if (x<60 || x>=90 || y<60 || y>=90) CHECK(pixels[static_cast<std::size_t>(y*200+x)]==255);
        SetGumpDrawClip(clip,{0,0,200,200});
    }
}
