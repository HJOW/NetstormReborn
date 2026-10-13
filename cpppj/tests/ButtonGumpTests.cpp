// 실제 세 PE의 버튼 관찰을 같은 생성/입력/수명 저장소에서 재생하고 가상 분배 연결을 검사한다.
#include "GumpRawSupport.h"
#include "client/ButtonGump.h"
#include "client/GumpBackground.h"
#include <fstream>
#include <map>
#include <memory>

namespace {
using namespace netstorm::test::gumpraw;
// 기계어 관찰의 raw 크기·토큰·가상 표 주소다. 호스트 객체의 물리 배치와 무관하다.
constexpr std::size_t kRawSize=0x170;
constexpr std::uint32_t kData=0x19000000,kTable=0x19008000;
constexpr std::array<std::uint32_t,6> kTokens{kData+16,kData+16+0x190,kData+16+2*0x190,
    kData+16+3*0x190,kData+16+4*0x190,kData+16+5*0x190};
// raw QWORD는 double 연산 없이 비트 그대로 읽고 쓴다.
std::uint64_t Qword(std::span<const std::uint8_t> raw,std::size_t offset) {
    return Get(raw,offset)|(std::uint64_t(Get(raw,offset+4))<<32);
}
// 전체 표를 길이:값 구간으로 기록한다. 기대 호출 대상은 계산하지 않는다.
std::string Table(std::span<const std::uint32_t> values) {
    std::string result;
    // 비활성 칸도 포함하여 불필요한 초기화/쓰기까지 대조한다.
    for (std::size_t i=0;i<values.size();) {
        const auto value=values[i];const auto start=i;
        // 같은 DWORD의 연속 구간만 줄인다.
        while (i<values.size() && values[i]==value) ++i;
        if (!result.empty()) result+=';';result+=std::to_string(i-start)+':'+std::to_string(value);
    }
    return result;
}
// 원본 표 입력의 구간을 정확한 크기에 펼친다.
void ReadTable(const std::string& text,std::span<std::uint32_t> output) {
    std::size_t cursor=0;
    // 모든 구간을 입력 순서로 복사한다.
    for (const auto& item:Split(text,';')) {
        const auto parts=Split(item,':');const auto count=Number(parts.at(0));
        if (count>output.size()-cursor) throw std::runtime_error("버튼 fixture 표 범위 오류");
        std::fill_n(output.begin()+cursor,count,Number(parts.at(1)));cursor+=count;
    }
    if (cursor!=output.size()) throw std::runtime_error("버튼 fixture 표 폭 오류");
}
// 전체 raw 미사용 바이트와 의미별 버튼 상태를 함께 보관한다. 배경 +b4는 별도 의미별 저장소다.
struct ButtonRecord {
    // node: 기본 0xa0 필드는 기존 저장소와 공유한다. 아래 필드는 +a0/+a4/+a8/+ac/+b0/+b8/+100에 대응한다 (ButtonGumpNode).
    ButtonGumpNode node;
    // original: 원래. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::uint8_t> original;
    // backgroundFlags: 배경 타일·누름 표시의 출력 플래그. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::uint32_t backgroundFlags{};
    // background: 배경 출력 자료 또는 배경 Gump 토큰. 입력 구성 또는 실행 결과 대조에 사용한다.
    bool background{};
    // 전체 원본 바이트를 의미별 노드로 읽고 파생 분배 종류는 보존한다.
    void Read(std::vector<std::uint8_t> raw) {
        if (raw.size()!=kRawSize) throw std::runtime_error("버튼 fixture 노드 폭 오류");
        original=std::move(raw);static_cast<GumpCreateNode&>(node)=Decode(original);
        node.value=Get(original,160);node.pressed=Get(original,164);node.blink=Get(original,168);node.buttonFlags=Get(original,172);
        node.firstDeadline=std::bit_cast<double>(Qword(original,176));node.repeatDeadline=std::bit_cast<double>(Qword(original,184));
        node.background=Get(original,256);backgroundFlags=Get(original,180);
    }
    // 호스트 필드의 현재 값만 원래 위치에 쓰고 지원하지 않는 파생/글자 바이트는 보존한다.
    std::vector<std::uint8_t> Raw() const {
        auto raw=original;const auto base=Bytes(Encode(node));std::copy(base.begin(),base.end(),raw.begin());
        Put(raw,160,node.value);Put(raw,164,node.pressed);Put(raw,168,node.blink);Put(raw,172,node.buttonFlags);
        const auto first=std::bit_cast<std::uint64_t>(node.firstDeadline),repeat=std::bit_cast<std::uint64_t>(node.repeatDeadline);
        Put(raw,176,static_cast<std::uint32_t>(first));Put(raw,180,static_cast<std::uint32_t>(first>>32));
        Put(raw,184,static_cast<std::uint32_t>(repeat));Put(raw,188,static_cast<std::uint32_t>(repeat>>32));Put(raw,256,node.background);
        if (background) Put(raw,180,backgroundFlags);return raw;
    }
};
// 안정된 노드/공통 표와 명시 응답·벽시계·투영을 보관하며 콜백 당시 전체 상태를 기록한다.
struct ButtonScene {
    // create: 등록 표 +0이 실제 루트다. 외부에서 표를 바꿀 때 collection.root도 같은 토큰으로 맞춰야 한다 (GumpCreateState).
    GumpCreateState create;
    // lifetime: 등록된 객체의 삭제·해제를 처리하는 수명 관리자. 입력 구성 또는 실행 결과 대조에 사용한다.
    GumpLifetimeState lifetime;
    // event: 대상 처리기에 전달할 이벤트 ID. 입력 구성 또는 실행 결과 대조에 사용한다.
    GumpEventState event;
    // input: 처리할 입력 또는 입력 분배기. 입력 구성 또는 실행 결과 대조에 사용한다.
    GumpInputState input;
    // state: 검사 입력과 실행 뒤 결과를 보관할 상태.
    ButtonGumpState state{0x12345,0x12346,0x12347,0x12348,56};
    // records: 기록. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::map<std::uint32_t,ButtonRecord> records;
    // responses: 응답. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::map<std::string,std::vector<std::array<std::uint32_t,3>>> responses;
    // anchors: 앵커. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::map<std::uint32_t,ScreenPoint> anchors;
    // times: 시각. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::vector<std::uint64_t> times;
    // poll: 사건 하나: 코드와 화면 좌표(창 테두리를 뺀 커서 위치) (InputEvent).
    InputEvent poll{0x10000,999,888};
    // parentReturns: 부모 반환. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::array<std::uint32_t,6> parentReturns{1,1,1,1,1,1};
    // baseReturn: 기본 반환. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::uint32_t baseReturn{0x81234567};
    // timeIndex: 시각 인덱스. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::size_t timeIndex{};
    // calls: 호출 횟수 또는 호출 인자를 대조하는 기록. 입력 구성 또는 실행 결과 대조에 사용한다.
    std::string calls;
    // eventHooks: 실제 가상 +0x34 이벤트 필터와 +0x28 처리기를 공급한다. 콜백은 현재 링크/전역을 바꿀 수 있다 (GumpEventHooks).
    GumpEventHooks eventHooks;
    // inputHooks: 가상 +0x28과 dialog 한도 로그의 경계다. 콜백은 현재 캡처/표/노드를 바꿀 수 있다 (GumpInputHooks).
    GumpInputHooks inputHooks;
    // frameHooks: node는 처리 동안 안정된 노드를 제공한다. 세 가상 콜백은 같은 표/링크/플래그를 변경할 수 있다 (GumpFrameHooks).
    GumpFrameHooks frameHooks;
    // dispatch: 돌 버튼 생성/텍스트/부착은 StoneButtonGump에 맡기는 상태 처리기다. GumpEvents/입력/프레임 훅에 연결한다 (ButtonGump).
    const ButtonGump* dispatch{};
    // CD는 idle 앞 80칸만 활성이다.
    std::size_t IdleSize() const { return create.collection.edition==netstorm::o::OriginalEdition::Patch1078 ? 500U : 80U; }
    // 도달한 기본/버튼 저장소의 동일 객체 주소를 제공한다.
    GumpCreateNode* Find(std::uint32_t token) { auto i=records.find(token);return i==records.end() ? nullptr : &i->second.node; }
    // 입력/응답의 일곱 스칼라를 같은 전역에 연결한다.
    std::uint32_t& Scalar(std::size_t i) {
        switch (i) {
        case 0:return create.count;case 1:return create.serial;case 2:return lifetime.focus;case 3:return lifetime.killCount;
        case 4:return lifetime.dialogCount;case 5:return lifetime.currentDialog;case 6:return create.collection.overrideHead;
        default:throw std::runtime_error("버튼 스칼라 index 오류");
        }
    }
    // 실제 일곱 현재 스칼라와 다섯 전체 표를 기록한다.
    std::string State() const {
        std::string result;
        // 관찰 도구와 같은 스칼라 순서를 사용한다.
        for (auto value:{create.count,create.serial,lifetime.focus,lifetime.killCount,lifetime.dialogCount,lifetime.currentDialog,create.collection.overrideHead}) {
            if (!result.empty()) result+=',';result+=std::to_string(value);
        }
        return result+'#'+Table(create.entries)+'#'+Table(std::span(lifetime.idle).first(IdleSize()))+'#'+Table(lifetime.overlay)+'#'+Table(lifetime.kill)+'#'+Table(lifetime.dialogs);
    }
    // 모든 0x170 노드 바이트를 주소 순으로 보존한다.
    std::string Nodes() const {
        std::string result;
        // 콜백에 관계없는 노드도 포함해 뜻하지 않은 쓰기를 검출한다.
        for (const auto& [token,record]:records) { if (!result.empty()) result+=';';result+=std::to_string(token)+'='+Hex(record.Raw()); }
        return result;
    }
    // 실제 callback 인자/현재 전체 상태를 기록한 뒤 입력의 명시 쓰기만 적용한다.
    void Respond(const std::string& kind,std::uint32_t token,std::initializer_list<std::uint64_t> args={}) {
        const auto key=kind+','+std::to_string(token);if (!calls.empty()) calls+='|';calls+=key;
        // 벽시계만 QWORD이며 나머지 가상 인자는 전체 DWORD다.
        for (auto value:args) calls+=','+std::to_string(value);
        calls+='@'+State()+'@'+Nodes();const auto found=responses.find(key);if (found==responses.end()) return;
        // 입력에서 지정한 상태/노드 변경만 적용한다. 기대 전파/표 결과는 계산하지 않는다.
        for (const auto& patch:found->second) {
            if (!patch[0]) Scalar(patch[1])=patch[2];
            else if (patch[0]==1) lifetime.idle.at(patch[1])=patch[2];
            else { auto& record=records.at(patch[0]);auto raw=record.Raw();Put(raw,patch[1],patch[2]);record.Read(std::move(raw)); }
        }
        create.collection.root=create.entries[0];
    }
    // 실제 부모 이벤트 경계는 전체 반환 DWORD를 보존한다.
    std::uint32_t Parent(std::uint32_t token,std::uint32_t eventCode,std::uint32_t first,std::uint32_t second) {
        Respond("Parent",token,{eventCode,first,second});
        const auto i=std::find(kTokens.begin(),kTokens.end(),token);if (i==kTokens.end()) throw std::runtime_error("버튼 부모 토큰 오류");
        return parentReturns.at(static_cast<std::size_t>(i-kTokens.begin()));
    }
    // 실행 전에 가상 분배를 연결한다. 실제 버튼 포인터는 모든 참조 처리기를 만든 직후 설정한다.
    void Prepare() {
        eventHooks.node=[this](auto token) { return Find(token); };
        eventHooks.handle=[this](auto token,auto e,auto first,auto second) { return Parent(token,e,first,second); };
        inputHooks.node=eventHooks.node;inputHooks.handle=eventHooks.handle;
        frameHooks.node=eventHooks.node;
        frameHooks.idle=[this](auto token) { Respond("OtherIdle",token); };
        frameHooks.broadcast=[this](auto token,auto e,auto first,auto second) { Respond("OtherBroadcast",token,{e,first,second}); };
        frameHooks.redraw=[this](auto token,bool flag) { Respond("Redraw",token,{flag ? 1U : 0U}); };
        // 현재 버튼 토큰만 실제 복원 처리기로 분배한다.
        BindButtonGump(eventHooks,inputHooks,frameHooks,[this](auto token) { return token==kTokens[1] ? dispatch : nullptr; });
    }
    // 가상 장치·시계·폴링/기본 도움말을 같은 입력 응답 경계에 연결한다.
    ButtonGumpHooks Hooks() {
        ButtonGumpHooks hooks;
        hooks.node=[this](auto token)->ButtonGumpNode* { auto* n=Find(token);return n ? &records.at(token).node : nullptr; };
        hooks.baseNode=[this](auto token) { return Find(token); };
        hooks.backgroundFlags=[this](auto token)->std::uint32_t* { return &records.at(token).backgroundFlags; };
        // 시각 조회마다 입력 다음 QWORD와 조회 직전 상태를 기록한다.
        hooks.now=[this] { const auto value=times.at(timeIndex);Respond("Now",static_cast<std::uint32_t>(timeIndex++),{value});return std::bit_cast<double>(value); };
        hooks.poll=[this](auto code) { Respond("Poll",0,{code});return poll; };
        hooks.redraw=[this](auto token,bool value) { Respond("Redraw",token,{value ? 1U : 0U}); };
        hooks.sound=[this] { Respond("Sound",0); };
        hooks.baseEvent=[this](auto token,auto e,auto first,auto second) { Respond("Base",token,{e,first,second});return baseReturn; };
        return hooks;
    }
    // 링크·시간·외부 경계의 독립 C++ 연결 검사를 위한 작은 실제 저장소다.
    void Basic() {
        create.count=6;create.serial=91;
        // 모든 원본 크기 노드를 준비하며 의미별 기본 필드는 아래에서 직접 작성한다.
        for (std::size_t i=0;i<kTokens.size();++i) {
            auto& record=records[kTokens[i]];record.Read(std::vector<std::uint8_t>(kRawSize));record.background=i>=4;
            auto& n=record.node;n.vtable=kTable+static_cast<std::uint32_t>(i)*0x100;n.flags=1;n.id=static_cast<std::uint32_t>(i);
            n.serial=75+static_cast<std::uint32_t>(i);n.parent=i ? kTokens[0] : 0;n.width=n.baseWidth=75;n.height=n.baseHeight=19;n.scale=1;
            create.entries[i]=kTokens[i];anchors[static_cast<std::uint32_t>(i)]={17,-19};
        }
        create.collection.root=kTokens[0];records[kTokens[0]].node.child=kTokens[1];
        auto& button=records[kTokens[1]].node;button.flags=0xa01;button.background=kTokens[5];button.kind=ButtonGumpKind::Stone;
        button.child=kTokens[5];records[kTokens[5]].node.parent=kTokens[1];
        times.assign(32,std::bit_cast<std::uint64_t>(1.0));Prepare();
    }
};
// 모든 모듈이 같은 저장소를 참조하고 실제 캡처/idle/부모 전달/기본 생성/팝업 Kill을 사용한다.
struct ButtonRig {
    // scene: 안정된 노드/공통 표와 명시 응답·벽시계·투영을 보관하며 콜백 당시 전체 상태를 기록한다 (ButtonScene).
    ButtonScene& scene;
    // factory: 타입에 맞는 객체를 생성하는 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    GumpCreate factory;
    // lifetime: 등록된 객체의 삭제·해제를 처리하는 수명 관리자. 입력 구성 또는 실행 결과 대조에 사용한다.
    GumpLifetime lifetime;
    // collection: 화면 밖 자식 통과, 최상위 삭제 필터, 원본의 루트별 제한 개수를 보존하는 UI 수집기다 (GumpCollection).
    GumpCollection collection;
    // events: 발생한 이벤트 또는 이벤트 처리기. 입력 구성 또는 실행 결과 대조에 사용한다.
    GumpEvents events;
    // input: 처리할 입력 또는 입력 분배기. 입력 구성 또는 실행 결과 대조에 사용한다.
    GumpInput input;
    // button: 돌 버튼 생성/텍스트/부착은 StoneButtonGump에 맡기는 상태 처리기다. GumpEvents/입력/프레임 훅에 연결한다 (ButtonGump).
    ButtonGump button;
    // frame: 사용할 그림 프레임 또는 프레임 처리 자료. 입력 구성 또는 실행 결과 대조에 사용한다.
    GumpFrame frame;
    // 처리기 생성 순서와 공통 상태를 유지하며 순환 가상 분배 포인터는 마지막에 연결한다.
    explicit ButtonRig(ButtonScene& s):scene(s),factory(s.create,{[&s](auto token) { return s.Find(token); },{}}),
        lifetime(s.create,s.lifetime,{[&s](auto token) { return s.Find(token); },[&s](auto token) { s.Respond("Push",token); },{},{},{}}),
        collection(s.create.collection,factory.CollectionHooks([&s](auto sid) { s.Respond("Project",sid);return s.anchors.at(sid); })),
        events(s.create,s.lifetime,s.event,collection,s.eventHooks),input(s.create,s.lifetime,s.input,collection,events,s.inputHooks),
        button(s.create,s.state,factory,collection,events,input,lifetime,s.Hooks()),frame(s.create,s.lifetime,s.frameHooks) { s.dispatch=&button; }
};
// 각 판본의 완성 입력을 재생하고 출력/콜백 상태는 저장된 실제 PE 관찰에서만 얻는다.
void Replay(std::string_view edition) {
    std::ifstream stream(NETSTORM_BUTTONGUMP_FIXTURE);if (!stream) throw std::runtime_error("버튼 fixture 없음");
    std::string line;std::size_t count=0;
    // 각 행은 새 저장소를 쓰고 행 안의 연속 action 사이에만 상태를 유지한다.
    while (std::getline(stream,line)) {
        if (line.empty() || line[0]=='#') continue;const auto row=Split(line,'\t');CHECK(row.size()==13);if (row.size()!=13) return;
        if (row[0]!=edition) continue;CHECK(Number(row[1])==count++);ButtonScene scene;
        scene.create.collection.edition=edition=="originals" ? netstorm::o::OriginalEdition::Patch1078 : netstorm::o::OriginalEdition::Cd1072;
        // 전체 raw 노드를 의미별 저장소로 읽는다.
        for (const auto& item:Split(row[3],';')) { const auto parts=Split(item,'=');const auto token=Number(parts[0]);auto& record=scene.records[token];record.Read(Bytes(parts[1]));record.background=token>=kTokens[4]; }
        const auto globals=Split(row[4],'#');const auto scalar=Inputs(globals.at(0));
        for (std::size_t i=0;i<scalar.size();++i) scene.Scalar(i)=scalar[i];
        ReadTable(globals.at(1),scene.create.entries);ReadTable(globals.at(2),std::span(scene.lifetime.idle).first(scene.IdleSize()));
        ReadTable(globals.at(3),scene.lifetime.overlay);ReadTable(globals.at(4),scene.lifetime.kill);ReadTable(globals.at(5),scene.lifetime.dialogs);
        scene.create.collection.root=scene.create.entries[0];const auto extras=Split(row[5],'#');const auto kinds=Inputs(extras.at(0));
        // 원본 가상 표의 종류와 공급한 시각 비트/폴링 응답을 읽는다.
        for (std::size_t i=0;i<kTokens.size();++i) scene.records.at(kTokens[i]).node.kind=kinds.at(i) ? ButtonGumpKind::Stone : ButtonGumpKind::Base;
        for (const auto& value:Split(extras.at(1),',')) scene.times.push_back(std::stoull(value));
        const auto poll=Inputs(extras.at(2)),parents=Inputs(extras.at(3));scene.poll={poll.at(0),std::bit_cast<std::int32_t>(poll.at(1)),std::bit_cast<std::int32_t>(poll.at(2))};
        std::copy(parents.begin(),parents.end(),scene.parentReturns.begin());scene.baseReturn=Number(extras.at(4));scene.state.baseColor=std::bit_cast<std::int8_t>(static_cast<std::uint8_t>(Number(extras.at(5))));
        // 투영 출력은 명시한 정수점이며 기대 위치는 계산하지 않는다.
        for (const auto& value:Split(extras.at(6),';')) { const auto fields=Split(value,',');scene.anchors[Number(fields.at(0))]={std::stoi(fields.at(1)),std::stoi(fields.at(2))}; }
        if (row[6]!="-") {
            // 콜백의 raw 응답 쓰기만 공급한다.
            for (const auto& item:Split(row[6],';')) {
                const auto parts=Split(item,':');auto& patches=scene.responses[parts.at(0)];
                for (const auto& value:Split(parts.at(1),'/')) { const auto fields=Inputs(value);patches.push_back({fields.at(0),fields.at(1),fields.at(2)}); }
            }
        }
        CHECK(scene.Nodes()==row[3] && scene.State()==row[4]);scene.Prepare();ButtonRig rig(scene);std::string results;
        // 입력 action과 전체 인자를 공개 복원 함수에 그대로 전달한다.
        for (const auto& action:Split(row[2],';')) {
            const auto split=action.find(',');const auto op=action.substr(0,split);const auto args=Inputs(action.substr(split+1));const auto token=args.at(0);std::string value="-";
            if (op=="Construct") value=std::to_string(rig.button.ConstructBase(token,std::bit_cast<std::int32_t>(args.at(1)),std::bit_cast<std::int32_t>(args.at(2)),args.at(3)));
            else if (op=="Value") rig.button.SetValue(token,args.at(1));else if (op=="Pressed") rig.button.SetPressed(token,args.at(1));
            else if (op=="Enabled") rig.button.SetEnabled(token,args.at(1)!=0);else if (op=="Pulse") rig.button.SetPulse(token,args.at(1)!=0);else if (op=="Blink") rig.button.SetBlink(token,args.at(1));
            else if (op=="Handle") value=std::to_string(rig.button.Handle(token,args.at(1),args.at(2),args.at(3)));
            else if (op=="Broadcast") rig.button.Broadcast(token,args.at(1),args.at(2),args.at(3));else if (op=="Idle") rig.button.Idle(token);
            else if (op=="Contains") value=std::to_string(rig.button.Contains(token,{Float(args.at(1)),Float(args.at(2))}));
            else if (op=="Local") { const auto p=rig.collection.ScreenToLocal(token,{std::bit_cast<std::int32_t>(args.at(2)),std::bit_cast<std::int32_t>(args.at(3))});value=std::to_string(Bits(p[0]))+','+std::to_string(Bits(p[1])); }
            else throw std::runtime_error("버튼 fixture action 오류");
            if (!results.empty()) results+=';';results+=op+':'+value;
        }
        const bool same=results==row[7] && scene.Nodes()==row[8] && scene.State()==row[9] && (scene.calls.empty() ? "-" : scene.calls)==row[10];
        if (!same) {
            std::printf("ButtonGump %s/%s: result=%d nodes=%d state=%d calls=%d\n",row[0].c_str(),row[1].c_str(),results==row[7],scene.Nodes()==row[8],scene.State()==row[9],scene.calls==row[10]);
            CHECK(same);return;
        }
    }
    CHECK(count==(edition=="originals" ? NETSTORM_BUTTONGUMP_PATCH_CASES : NETSTORM_BUTTONGUMP_CD_CASES));
}
// 세 실제 PE의 기본 53비트 모드 결과를 독립적으로 대조한다.
TEST_CASE(ButtonGump_X86_Patch1078) { Replay("originals"); }
TEST_CASE(ButtonGump_X86_Cd1072) { Replay("originalCD"); }
TEST_CASE(ButtonGump_X86_Original1037) { Replay("original1037"); }
// 실제 트리 전달→캡처→프레임 통지/idle→같은 객체 뗌·캡처 해제와 출력 입력을 연결한다.
TEST_CASE(ButtonGump_BindCaptureFrameAndDraw) {
    ButtonScene scene;scene.Basic();ButtonRig rig(scene);const auto token=kTokens[1];auto& n=scene.records.at(token).node;
    CHECK(rig.input.Route(0x10000,1,1)==token);CHECK(scene.lifetime.focus==token && n.pressed==1 && (n.flags&0x180)==0x180);
    CHECK(scene.records.at(kTokens[5]).backgroundFlags&GumpBackgroundFlags::kPressed);
    const auto visual=rig.button.VisualState(token,37,14);CHECK(visual.pressed==1 && visual.value==0 && visual.rect.left==0);
    rig.frame.Broadcast(0,100,100);CHECK(n.pressed==0 && scene.lifetime.focus==token);
    rig.frame.Broadcast(0,1,1);CHECK(n.pressed==1);
    CHECK(rig.input.Route(0x80010000,1,1)==token);CHECK(!scene.lifetime.focus && !n.pressed && !(n.flags&0x180));
    CHECK(!(scene.records.at(kTokens[5]).backgroundFlags&GumpBackgroundFlags::kPressed));
    const auto previous=scene.calls;rig.frame.Idle();CHECK(scene.calls==previous);
}
// pulse 해제는 현재 idle 표에서 제거해도 이미 실행 중인 idle의 반복 통지 후반을 취소하지 않는다.
TEST_CASE(ButtonGump_PulseAndRepeatCurrentState) {
    ButtonScene scene;scene.Basic();ButtonRig rig(scene);const auto token=kTokens[1];auto& n=scene.records.at(token).node;
    n.pressed=1;rig.button.SetPulse(token,true);rig.frame.Idle();
    CHECK(!(n.buttonFlags&0x10) && !(n.flags&0x80));CHECK(scene.calls.find("Parent,")!=std::string::npos);CHECK_NEAR(n.repeatDeadline,1.05,1e-15);
    scene.calls.clear();n.buttonFlags=0;n.pressed=1;n.firstDeadline=0;n.repeatDeadline=0;rig.button.Idle(token);
    CHECK(scene.calls.find("Parent,")!=std::string::npos);CHECK_NEAR(n.repeatDeadline,1.05,1e-15);
}
// 실제 팝업 순회는 visible/void와 무관하고 dead를 제외하며 같은 지연 삭제 큐를 사용한다.
TEST_CASE(ButtonGump_PopupKillSharedStorage) {
    ButtonScene scene;scene.Basic();ButtonRig rig(scene);const auto token=kTokens[1],popup=kTokens[2];
    scene.records.at(token).node.next=popup;auto& p=scene.records.at(popup).node;p.flags=0x4008;scene.lifetime.focus=popup;
    rig.button.Handle(token,0x10000,1,1);CHECK((p.flags&0x10000)!=0 && scene.lifetime.killCount==1 && scene.lifetime.kill[0]==popup);
    CHECK(scene.lifetime.focus==token && scene.records.at(token).node.pressed==1);
}
// 원본의 로컬 변환은 루트 위치를 빼지 않고 월드 부모의 기준점 뒤 일반 위치도 생략한다.
TEST_CASE(ButtonGump_LocalRootAndWorldParent) {
    ButtonScene scene;scene.Basic();ButtonRig rig(scene);const auto token=kTokens[1],parent=kTokens[2];
    scene.records.at(kTokens[0]).node.x=100;scene.records.at(token).node.x=10.25F;
    const auto local=rig.collection.ScreenToLocal(token,{11,1});CHECK(local[0]==.75F && local[1]==1);
    auto& p=scene.records.at(parent).node;p.flags|=0x2000;p.object=2;p.x=-.75F;p.y=.25F;scene.records.at(token).node.parent=parent;
    scene.anchors[2]={17,-19};const auto world=rig.collection.ScreenToLocal(token,{18,-18});CHECK(world[0]==1 && world[1]==1);
}
// 누락 경계/잘못된 저장소/순환은 호스트 예외이고 콜백이 이미 쓴 값은 복구하지 않는다.
TEST_CASE(ButtonGump_HostContractsAndCallbackExceptions) {
    ButtonScene scene;scene.Basic();ButtonRig rig(scene);const auto token=kTokens[1];bool threw=false;
    auto hooks=scene.Hooks();hooks.redraw=[&scene](auto t,bool) { scene.records.at(t).node.value=7;throw std::runtime_error("의도한 콜백 예외"); };
    ButtonGump throwing(scene.create,scene.state,rig.factory,rig.collection,rig.events,rig.input,rig.lifetime,hooks);
    try { throwing.SetValue(token,1); } catch (const std::runtime_error&) { threw=true; }CHECK(threw && scene.records.at(token).node.value==7);
    threw=false;scene.records.at(token).node.parent=token;try { rig.collection.ScreenToLocal(token,{1,1}); } catch (const std::runtime_error&) { threw=true; }CHECK(threw);
    threw=false;try { rig.button.Handle(0,0x10000,1,1); } catch (const std::runtime_error&) { threw=true; }CHECK(threw);
}
}
