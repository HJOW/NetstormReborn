// 원본 목록과 실제 MetaImage 자식의 전체 tail/기하/호출 순서를 재생한다.
#include "RawSceneSupport.h"
#include "client/MetaImageList.h"
#include "client/GumpLifetime.h"
#include <cstdio>

using namespace netstorm::client;
using namespace netstorm::test::rawscene;

// 명시 기본 장치 효과만 공급하고 제품 목록/자식 산술을 Native 전체 바이트와 비교한다.
TEST_CASE(MetaImageList_ReplaysNativeWholeListAndActualMetaChildren) {
    const auto fixture=LoadFixture(NETSTORM_META_IMAGE_LIST_FIXTURE);std::uint32_t checked{};
    for (const auto& row:fixture.rows) { // 두 x87 관측을 각각 실제 입력/기대 행으로 재생한다.
        if (row[0]=="op") continue;
        MetaImageListNode node;node.count=0x5a5a5a5a;node.ids.fill(0x5a5a5a5a);node.cellHeight=node.cellWidth=std::bit_cast<float>(0x5a5a5a5aU);
        std::array<MetaImageGumpNode,22> children{};std::array<RawImage,22> images;std::vector<std::string> trace;GumpCreateState state;state.collection.edition=OriginalEdition::Patch1078;
        const auto base=[&](std::uint32_t token)->GumpCreateNode* { if (token==1) return &node;return token>=100 && token<122 ? &children[token-100] : nullptr; };
        GumpCreate factory(state,{base,{}});GumpLifetimeState life;GumpLifetimeHooks lifetimeHooks;lifetimeHooks.node=base;GumpLifetime lifetime(state,life,std::move(lifetimeHooks));
        const auto prepare=[&](std::uint32_t index) { const auto width=std::array{224,6,112}[index%3],height=std::array{112,4,224}[index%3];if (images[index].Width()==0) images[index].Convert({width,height,std::vector<std::uint8_t>(static_cast<std::size_t>(width)*height,7)},false); };
        const auto attachBase=[&](std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t) {
            trace.push_back("attach:"+(token==1 ? std::string("list") : std::to_string(token-100))+":"+std::to_string(std::bit_cast<std::uint32_t>(x))+":"+std::to_string(std::bit_cast<std::uint32_t>(y)));
            auto& current=*base(token);current.x=x;current.y=y;current.parent=parent;if (token==1 && Number(row[13])==1) { node.baseWidth=131.75F;node.baseHeight=99.5F; }
        };
        MetaImageGumpHooks metaHooks;metaHooks.node=[&](auto token) { return token>=100 && token<122 ? &children[token-100] : nullptr; };metaHooks.attach=attachBase;MetaImageGump meta(state,factory,std::move(metaHooks));
        std::unique_ptr<MetaImageList> body;MetaImageListHooks hooks;hooks.node=[&](auto token) { return token==1 ? &node : nullptr; };hooks.image=[&](auto token) { return token>=100 && token<122 ? &children[token-100] : nullptr; };
        hooks.createImage=[&](RawImage& image) { trace.push_back("allocate");const auto token=100+node.count;return meta.Construct(token,image); };
        hooks.baseAttach=attachBase;hooks.attach=[&](auto token,float x,float y,auto parent,auto mode) { if (token==1) body->Attach(token,x,y,parent,mode);else meta.Attach(token,x,y,parent,mode); };
        hooks.push=[&](auto token) {
            trace.push_back("push:"+(token==1 ? std::string("list") : std::to_string(token-100)));base(token)->flags|=8;
            if (token==100 && Number(row[13])==2) { node.count=std::max(1U,node.count-1);node.cellWidth=17.25F;node.cellHeight=13.5F; }
        };
        hooks.kill=[&](auto token) { trace.push_back("kill:"+std::to_string(token-100)); };hooks.destructBase=[&](auto token) { trace.push_back("destruct");lifetime.DestructBase(token); };
        hooks.bounds=[&](auto) { trace.push_back("bounds");return ScreenRect{10,20,210,140}; };hooks.redraw=[&](auto,bool erase) { trace.push_back("redraw:"+std::to_string(erase)); };
        hooks.notify=[&](auto,auto event,auto first,auto second) { trace.push_back("notify:"+std::to_string(event)+":"+std::to_string(first)+":"+std::to_string(second)); };hooks.green=[] { return 43U; };
        hooks.fill=[&](auto rect,auto color) { trace.push_back("fill:"+std::to_string(static_cast<std::uint32_t>(rect.left))+":"+std::to_string(static_cast<std::uint32_t>(rect.top))+":"+std::to_string(static_cast<std::uint32_t>(rect.right))+":"+std::to_string(static_cast<std::uint32_t>(rect.bottom))+":"+std::to_string(color)); };
        body=std::make_unique<MetaImageList>(state,factory,std::move(hooks));std::uint32_t returned=body->Construct(1,std::stoi(row[2]));
        if (row[0]!="Construct") {
            node.count=Number(row[1]);node.selection=std::stoi(row[8]);node.baseWidth=std::bit_cast<float>(Number(row[3]));node.baseHeight=std::bit_cast<float>(Number(row[4]));node.scale=std::bit_cast<float>(Number(row[5]));
            node.cellWidth=std::bit_cast<float>(Number(row[6]));node.cellHeight=std::bit_cast<float>(Number(row[7]));node.x=2.25F;node.y=3.5F;
            for (std::uint32_t index=0;index<node.count;++index) { prepare(index);meta.Construct(100+index,images[index]);node.images[index]=100+index;node.ids[index]=4096+index*7; } // 이미 존재하는 실제 자식/ID를 명시 입력으로 준비한다.
            if (row[0]=="Destruct") node.flags|=0x40000;
            trace.clear();returned=0;
            if (row[0]=="Attach") body->Attach(1,2.25F,3.5F,0,0);
            else if (row[0]=="Hit") returned=static_cast<std::uint32_t>(body->Hit(1,std::stoi(row[9]),std::stoi(row[10])));
            else if (row[0]=="Handle") returned=body->Handle(1,Number(row[11]),static_cast<std::uint32_t>(std::stoi(row[9])),static_cast<std::uint32_t>(std::stoi(row[10])));
            else if (row[0]=="Draw") body->Draw(1);
            else if (row[0]=="Id") returned=body->Id(1,std::stoi(row[12]));
            else if (row[0]=="Image") { const auto* image=body->Image(1,std::stoi(row[12]));returned=image ? static_cast<std::uint32_t>(image-images.data()+1) : 0; }
            else if (row[0]=="Remove") returned=body->Remove(1,std::stoi(row[12]));
            else if (row[0]=="Add") { prepare(node.count);returned=body->Add(1,images[node.count],0x81234567); }
            else body->Destruct(1);
        }
        std::vector<std::uint8_t> tail(0xbc),geometry(22*28);Put(tail,0,node.count);Put(tail,0xac,static_cast<std::uint32_t>(node.selection));Put(tail,0xb0,static_cast<std::uint32_t>(node.columns));Put(tail,0xb4,std::bit_cast<std::uint32_t>(node.cellHeight));Put(tail,0xb8,std::bit_cast<std::uint32_t>(node.cellWidth));
        for (std::size_t index=0;index<21;++index) { Put(tail,4+index*4,node.images[index] ? node.images[index]-99 : 0);Put(tail,0x58+index*4,node.ids[index]); } // 실제 포인터만 Native와 같은 논리 자식 순번으로 정규화한다.
        for (std::size_t index=0;index<22;++index) { const auto& child=children[index];const std::array values{child.scale,child.baseWidth,child.baseHeight,child.width,child.height,child.x,child.y};for (std::size_t field=0;field<values.size();++field) Put(geometry,index*28+field*4,std::bit_cast<std::uint32_t>(values[field])); } // 현재 자식 기하의 저장 비트를 비교한다.
        std::string joined;for (const auto& item:trace) { if (!joined.empty()) joined+=';';joined+=item; } // 실제 가상 효과의 순서를 그대로 비교한다.
        if (joined.empty()) joined="-";const bool same=returned==Number(row[15]) && Hex(tail)==row[22] && Hex(geometry)==row[23] && joined==row[24];
        if (!same) std::fprintf(stderr,"ImageList Native행%u %s 저장소/순서 불일치\n",checked,row[0].c_str());CHECK(same);
        CHECK(std::bit_cast<std::uint32_t>(node.baseWidth)==Number(row[16]) && std::bit_cast<std::uint32_t>(node.baseHeight)==Number(row[17]) && node.flags==Number(row[18]) && node.vtable==Number(row[19]));CHECK(state.count==Number(row[20]) && state.serial==Number(row[21]));++checked;
    }
    CHECK(checked==514);
}
