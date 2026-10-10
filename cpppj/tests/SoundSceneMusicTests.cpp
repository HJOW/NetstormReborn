// 독립 원본 장면 음악 관찰을 재생하고, 실제 곡 선택·기본 채널·효과음 재생 계층에 이은 장면 전환의 수명을 검사한다.
#include "RawSceneSupport.h"
#include "client/SoundSceneMusic.h"
#include <algorithm>
#include <cstdio>
#include <limits>
#include <optional>
#include <set>

using namespace netstorm::o;
using namespace netstorm::client;
using namespace netstorm::test::rawscene;
namespace {
// 판본별 입력 수(잠금 96 + 요청 1,377 + 다음 504 + 프레임 288 + 시작 576 + 날씨 32 + 흐름 240)와 한 행의 칸 수다.
constexpr std::size_t kCases=3113,kColumns=14;
// 날씨 색 표의 시험 값이다(원본은 실행 중에 채운다). 대조 도구 DEFAULT_TINTS와 같아야 한다.
constexpr std::array<std::uint32_t,4> kTints{0x1111,0x2222,0x3333,0x4444};
// 일반 CTest는 PE/Python을 실행하지 않고 저장한 독립 관찰을 읽는다.
const std::vector<std::vector<std::string>>& Fixture() {
    static const auto data=LoadFixture(NETSTORM_SCENEMUSIC_FIXTURE);return data.rows;
}
// 입력 문자열의 16진수 두 글자를 한 바이트로 되돌린다. '-'는 빈 이름이다.
std::string Decode(const std::string& hex) {
    std::string text;
    if (hex=="-") return text;
    // 한 바이트씩 읽는다.
    for (std::size_t i=0;i<hex.size();i+=2) text+=static_cast<char>(std::stoul(hex.substr(i,2),nullptr,16));
    return text;
}
// 바이트열을 사건에 적는 16진수로 만든다. 빈 이름은 '-'다.
std::string Encode(std::string_view text) {
    if (text.empty()) return "-";
    return Hex({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()});
}
// 배정도 비트를 16진수 16자리로 적는다(대조 도구의 표기와 같다).
std::string DoubleBits(double value) {
    char text[17]{};std::snprintf(text,sizeof(text),"%016llx",static_cast<unsigned long long>(std::bit_cast<std::uint64_t>(value)));return text;
}
// 16진수 16자리 비트를 배정도로 읽는다.
double ParseDouble(const std::string& hex) { return std::bit_cast<double>(static_cast<std::uint64_t>(std::stoull(hex,nullptr,16))); }
// 소비하는 입력 목록이다. 끝나면 마지막 값을 반복하고 비어 있으면 기본값을 쓴다.
struct Feed {
    std::vector<std::string> values;std::size_t used{};
    // 하나를 꺼낸다.
    std::string Take(const std::string& fallback) {
        const auto value=values.empty() ? fallback : values[std::min(used,values.size()-1)];++used;return value;
    }
};
// 원본 실행기의 대체 경계와 같은 사건을 만드는 시험 월드다. 시계/선택 결과/판정은 입력 목록이 정한다.
struct SceneWorld {
    SceneMusicState state;GameRandom random;MusicSelectionState names;MusicChannelState primary;
    std::vector<std::string> events;Feed clock,durations,sacrifice,waiting,option;
    // 원본 초기값과 시험 색 표로 시작한다.
    SceneWorld() { state.tints=kTints; }
    // 곡 길이를 기본 채널 +0x18에 설치한다(곡을 열면 채널이 쓰는 값).
    void InstallDuration(const std::string& bitsHex) {
        const auto value=static_cast<std::uint64_t>(std::stoull(bitsHex,nullptr,16));auto raw=primary.Raw();
        // 낮은 바이트부터 여덟 바이트를 쓴다.
        for (std::size_t i=0;i<8;++i) raw[static_cast<std::size_t>(MusicField::Duration)+i]=static_cast<std::uint8_t>(value>>(8*i));
    }
    // 장면 음악 감독에 넘길 경계다. 사건 문자는 대조 도구와 같다.
    SceneMusicHooks Hooks() {
        SceneMusicHooks hooks;
        hooks.wallSeconds=[this] { events.push_back("T");return ParseDouble(clock.Take(DoubleBits(1000.0))); };
        hooks.currentName=[this] { return names.Current(); };
        hooks.select=[this](std::string_view name) {
            events.push_back("S:"+Encode(name));names.Assign(name);
            const auto length=durations.Take("keep");if (length!="keep") InstallDuration(length);
        };
        hooks.duration=[this] { return MusicChannelDuration(primary); };
        hooks.sacrificing=[this](std::uint32_t player) { events.push_back("C:"+std::to_string(player));return sacrifice.Take("0")=="1"; };
        hooks.waitingRoom=[this] { events.push_back("W");return waiting.Take("0")=="1"; };
        hooks.refresh=[this] { events.push_back("R"); };
        hooks.ascendancyPalette=[this] { events.push_back("O");return option.Take("0")=="1"; };
        hooks.loadPalette=[this](std::string_view name) { events.push_back("J:"+Encode(name));events.push_back("A"); };
        hooks.thunderFlash=[this] { events.push_back("K"); };
        hooks.weatherSound=[this](std::string_view name) { events.push_back("N:"+Encode(name)); };
        return hooks;
    }
    // 한 호출 동안의 사건을 꺼내고 비운다.
    std::string TakeEvents() {
        std::string text;
        // 같은 호출 순서를 보존한다.
        for (const auto& event:events) { if (!text.empty()) text+=';';text+=event; }
        events.clear();return text.empty() ? "-" : text;
    }
};
// 한 행의 연산 목록을 실제 장면 음악 감독에 같은 순서로 적용하고 호출별 사건·반환과 마지막 전역을 비교한다.
bool ReplayRow(const std::vector<std::string>& row) {
    CHECK(row.size()==kColumns);SceneWorld world;SceneMusic scene(world.state,world.random,world.Hooks());std::string tokens;
    // 목록 연산은 비어 있지 않은 값만 나눈다.
    const auto feed=[](const std::string& value) { Feed result;if (!value.empty()) result.values=Split(value,';');return result; };
    // 연산마다 입력 설정 한 번 또는 실제 호출 한 번이다.
    for (const auto& operation:Split(row[2],'|')) {
        const auto op=Split(operation,':');std::string result="-";bool called=true;
        if (op[0]=="i") {
            called=false;const auto value=static_cast<std::uint32_t>(std::stoull(op[2]));
            if (op[1]=="index") world.state.index=static_cast<std::int32_t>(value);
            else if (op[1]=="result") world.state.resultState=value;
            else if (op[1]=="battle") world.state.battle=value;
            else if (op[1]=="players") world.state.playersReady=value;
            else if (op[1]=="local") world.state.localPlayer=value;
            else if (op[1]=="tint") world.state.tint=value;
            else if (op[1]=="dirty") world.state.paletteDirty=value;
            else throw std::runtime_error("알 수 없는 전역");
        }
        else if (op[0]=="t") { called=false;world.state.tints.at(std::stoul(op[1]))=static_cast<std::uint32_t>(std::stoull(op[2])); }
        else if (op[0]=="f") { called=false;(op[1]=="fanfare" ? world.state.fanfareEnd : op[1]=="defeat" ? world.state.defeatEnd : world.state.songEnd)=ParseDouble(op[2]); }
        else if (op[0]=="cur") {
            called=false;auto raw=world.names.Raw();std::fill(raw.begin(),raw.end(),std::uint8_t{});const auto text=Decode(op[1]);
            std::copy(text.begin(),text.end(),raw.begin());
        }
        else if (op[0]=="rng") { called=false;world.random.SetState(static_cast<std::uint32_t>(std::stoull(op[1]))); }
        else if (op[0]=="clk") { called=false;world.clock=feed(op[1]); }
        else if (op[0]=="durs") { called=false;world.durations=feed(op[1]); }
        else if (op[0]=="sac") { called=false;world.sacrifice=feed(op[1]); }
        else if (op[0]=="wait") { called=false;world.waiting=feed(op[1]); }
        else if (op[0]=="opt") { called=false;world.option=feed(op[1]); }
        else if (op[0]=="lock") result=scene.ResultMusicLocked() ? "1" : "0";
        else if (op[0]=="req") scene.Request(Decode(op[1]));
        else if (op[0]=="next") scene.Next();
        else if (op[0]=="frame") scene.Frame();
        else if (op[0]=="start") scene.Start();
        else if (op[0]=="weather") scene.ApplyWeather();
        else throw std::runtime_error("알 수 없는 연산");
        if (!called) continue;
        if (!tokens.empty()) tokens+='|';
        tokens+=world.TakeEvents()+'='+result;
    }
    const auto name=world.names.Current();
    const std::vector<std::string> snapshot{std::to_string(static_cast<std::uint32_t>(world.state.index)),DoubleBits(world.state.fanfareEnd),DoubleBits(world.state.defeatEnd),
        DoubleBits(world.state.songEnd),std::to_string(world.state.tint),std::to_string(world.state.paletteDirty),std::to_string(world.random.State()),Encode(name),
        DoubleBits(MusicChannelDuration(world.primary)),std::to_string(world.state.resultState)};
    bool same=tokens==row[3];
    // 마지막 전역 열 열 개를 기대값과 하나씩 비교한다.
    for (std::size_t i=0;i<snapshot.size();++i) same=same && snapshot[i]==row[4+i];
    if (!same) {
        std::printf("%s 장면 음악 불일치: %s\n  기대 %s\n  실제 %s\n",row[0].c_str(),row[2].c_str(),row[3].c_str(),tokens.c_str());
        // 어느 전역이 다른지 보인다.
        for (std::size_t i=0;i<snapshot.size();++i) if (snapshot[i]!=row[4+i]) std::printf("  전역 %zu: 기대 %s 실제 %s\n",i,row[4+i].c_str(),snapshot[i].c_str());
    }
    return same;
}
// 한 판본의 모든 원본 관찰을 실제 C++ 모듈에 재생한다. 원본 출력값을 다시 계산하지 않는다.
void Replay(const char* edition) {
    std::size_t count=0;CHECK(Fixture().size()==kCases*3);
    // 첫 불일치에서 멈춰 출력이 넘치지 않게 한다.
    for (const auto& row:Fixture()) {
        if (row[0]!=edition) continue;
        const bool same=ReplayRow(row);CHECK(same);++count;
        if (!same) break;
    }
    CHECK(count==kCases);
}
}
// 패치판의 실제 요청/다음 곡/프레임/시작/날씨 관찰을 재생한다.
TEST_CASE(SceneMusic_ReplaysOriginals) { Replay("originals"); }
// CD판(시계 가산이 인라인인 구조)의 관찰도 같은 C++ 모듈에 대조한다.
TEST_CASE(SceneMusic_ReplaysCd) { Replay("originalCD"); }
// 추가 10.37 PE도 독립 관찰로 대조한다.
TEST_CASE(SceneMusic_ReplaysExtra1037) { Replay("original1037"); }

namespace {
// 실제 곡 선택·기본 채널·효과음 재생 계층에 이은 시험 스택이다. 파일 열기/버퍼 생성/장치 호출만 모의한다.
struct RealStack {
    SceneMusicState state;GameRandom random;MusicSelectionState names;MusicChannelState primary;SoundState sound;SoundList list{OriginalEdition::Patch1078};
    SoundDeviceHooks device;SoundPlayer player;MusicChannel channel;SoundMusic music;MusicSelection selection;
    std::vector<std::string> opened,sounds;std::set<std::string> missing;double wall=1000.0;std::uint32_t token=0,live=0;bool thunder=false;
    // 곡 길이를 정하는 파일 길이다: 22050Hz 16비트 스테레오에서 214초에 가까운 바이트 수.
    static constexpr std::uint32_t kLength=18874800;
    // 장치 경계와 모의 파일을 연결해 한 스택을 만든다.
    RealStack():device(MakeDevice()),player(list,sound,device),channel(OriginalEdition::Patch1078,primary,MakeChannel()),music(OriginalEdition::Patch1078,sound,player,channel),
        selection(names,music,{{"music"},{"backup"}},MakeOpen(),MakeBuffers()) {
        sound.initialized=sound.device=sound.enabled=sound.musicInitialized=true;state.tints=kTints;list.Initialize();
    }
    // 효과음 장치: 적재/재생/정지를 기록하고 항상 성공한다.
    SoundDeviceHooks MakeDevice() {
        SoundDeviceHooks h;
        h.status=[](SoundBuffer)->std::uint32_t { return 0; };
        h.play=[](SoundBuffer,std::uint32_t) {};
        h.setPosition=[](SoundBuffer,std::uint32_t) {};
        h.setVolume=[](SoundBuffer,std::int32_t)->std::int32_t { return 0; };
        h.setPan=[](SoundBuffer,std::int32_t)->std::int32_t { return 0; };
        h.stop=[](SoundBuffer) {};
        h.duplicate=[](SoundBuffer,SoundBuffer& copy)->std::int32_t { copy=0x16001100;return 0; };
        h.load=[this](std::string_view name)->SoundLoad { sounds.push_back(std::string(name));return {0x16001000,0}; };
        return h;
    }
    // 음악 채널: 열린 파일 수를 세고 읽기/되감기는 항상 성공한다.
    MusicChannelHooks MakeChannel() {
        MusicChannelHooks h;
        h.setVolume=[](SoundBuffer,std::int32_t)->std::int32_t { return 0; };
        h.stop=[](SoundBuffer) {};
        h.release=[](SoundBuffer)->std::int32_t { return 0; };
        h.seek=[](std::uint32_t,std::uint32_t offset)->std::int32_t { return static_cast<std::int32_t>(offset); };
        h.read=[](std::uint32_t,std::span<std::uint8_t> target)->std::int32_t { std::fill(target.begin(),target.end(),std::uint8_t{});return static_cast<std::int32_t>(target.size()); };
        h.close=[this](std::uint32_t) { if (live>0) --live; };
        return h;
    }
    // 곡 조회/열기: missing에 있는 이름은 찾지 못하고, 나머지는 같은 길이의 스테레오 16비트 22050Hz 파일로 연다.
    MusicOpenHooks MakeOpen() {
        MusicOpenHooks h;
        h.find=[this](std::string_view query)->std::optional<std::string> {
            // 마지막 경로 요소를 곡 이름으로 본다.
            const auto name=std::string(query.substr(query.rfind('\\')==std::string_view::npos ? 0 : query.rfind('\\')+1));
            return missing.count(name) ? std::nullopt : std::optional<std::string>(std::string(query));
        };
        h.open=[this](std::string_view found,MusicFileHeader& header) {
            opened.push_back(std::string(found));header.file=++token;++live;header.length=kLength;header.dataOffset=44;
            const std::array<std::uint8_t,18> format{1,0,2,0,0x22,0x56,0,0,0x88,0x58,1,0,4,0,16,0,0,0};
            header.format=format;return true;
        };
        return h;
    }
    // 음악 버퍼 생성만 성공시킨다(재생 갱신은 이 검사에서 부르지 않는다).
    MusicBufferHooks MakeBuffers() {
        MusicBufferHooks h;
        h.create=[](std::span<const std::uint8_t>,std::uint32_t,std::uint32_t,SoundBuffer& buffer)->std::int32_t { buffer=0x16001010;return 0; };
        return h;
    }
    // 화면·월드·설정 경계를 시험값으로 채우고 실제 모듈 경계를 잇는다. 번개는 기록만 한다.
    SceneMusicHooks Hooks(bool& sacrificing,bool& waiting,bool& palette) {
        SceneMusicHooks external;
        external.wallSeconds=[this] { return wall; };
        external.sacrificing=[&sacrificing](std::uint32_t) { return sacrificing; };
        external.waitingRoom=[&waiting] { return waiting; };
        external.refresh=[] {};
        external.ascendancyPalette=[&palette] { return palette; };
        external.loadPalette=[this](std::string_view) {};
        external.thunderFlash=[this] { thunder=true; };
        return MakeSceneMusicHooks(selection,names,primary,player,std::move(external));
    }
};
}
// 실제 곡 선택/기본 채널/효과음 재생 계층 위에서 전투 순환·희생 전환·결과 곡 잠금·메뉴 복귀의 수명을 검사한다.
TEST_CASE(SceneMusic_DrivesRealSelectionChannelAndSounds) {
    RealStack stack;bool sacrificing=false,waiting=false,palette=false;stack.state.battle=1;stack.state.playersReady=1;
    SceneMusic scene(stack.state,stack.random,stack.Hooks(sacrificing,waiting,palette));
    // 시작: 전투 중이면 난수 색인의 다음 곡이 열린다. 곡 길이는 실제 채널이 파일 헤더에서 계산한 값이다.
    stack.random.SetState(0);GameRandom expected(0);const auto first=static_cast<std::int32_t>(expected.Next(4000)/1000);
    scene.Start();
    const auto index=(first+1)%4;
    CHECK(stack.state.index==index && stack.names.Current()==kElementMusic[static_cast<std::size_t>(index)] && stack.random.State()==expected.State());
    const double length=MusicChannelDuration(stack.primary);
    CHECK(length>213.9 && length<214.1 && stack.state.songEnd==length+1000.0 && stack.state.tint==kTints[static_cast<std::size_t>(index)] && stack.channel.Active() && stack.live==1);
    // 곡 끝 전에는 아무 일도 없고, 끝 시각에 이르면 다음 곡으로 바뀌며 이전 파일은 닫힌다. 천둥 곡은 번개와 효과음을 함께 낸다.
    const auto opened=stack.opened.size();stack.wall=1100.0;scene.Frame();CHECK(stack.opened.size()==opened);
    std::vector<std::string> order{std::string(stack.names.Current())};
    // 한 바퀴를 돌며 네 곡의 순서와 천둥 곡의 효과음을 확인한다.
    for (int i=0;i<4;++i) {
        stack.wall=stack.state.songEnd;scene.Frame();order.push_back(std::string(stack.names.Current()));
        CHECK(stack.live==1 && stack.state.songEnd==stack.wall+MusicChannelDuration(stack.primary));
    }
    CHECK(order[1]==kElementMusic[static_cast<std::size_t>((index+1)%4)] && order[4]==order[0]);
    CHECK(stack.thunder && std::count(stack.sounds.begin(),stack.sounds.end(),std::string("thunderCrack.wav"))>=1);
    // 희생 의식이 진행 중이면 곡 끝에서 희생 곡이 열리고 날씨 효과(번개)는 적용하지 않는다.
    stack.thunder=false;sacrificing=true;stack.wall=stack.state.songEnd;scene.Frame();
    CHECK(stack.names.Current()==kSacrificeMusic && !stack.thunder && stack.live==1);
    // 희생이 끝난 뒤의 곡 끝에서는 다음 원소 곡으로 돌아간다(끊긴 곡을 이어 틀지 않는다).
    sacrificing=false;stack.wall=stack.state.songEnd;scene.Frame();CHECK(stack.names.Current()==kElementMusic[static_cast<std::size_t>(stack.state.index)]);
    // 결과 화면: 승리 곡은 결과 상태 1에서만 열리고, 끝나기 전에는 다른 곡 요청이 막히지만 대기실 곡은 허용된다.
    stack.wall+=10.0;scene.Request(kFanfareMusic);CHECK(stack.names.Current()!=kFanfareMusic);
    stack.state.resultState=1;scene.Request(kFanfareMusic);CHECK(stack.names.Current()==kFanfareMusic);
    const double fanfareEnd=stack.state.fanfareEnd;CHECK(fanfareEnd==MusicChannelDuration(stack.primary)+stack.wall);
    const auto opening=stack.opened.size();scene.Request(kMenuMusic);CHECK(stack.names.Current()==kFanfareMusic && stack.opened.size()==opening);
    scene.Request(kAnticipationMusic);CHECK(stack.names.Current()==kAnticipationMusic);
    // 승리 곡이 끝난 뒤에는 메뉴 곡을 요청할 수 있다. 전투가 끝났으면 곡 끝에서 메뉴 곡으로 복귀한다.
    stack.wall=fanfareEnd+1.0;stack.state.battle=0;stack.state.songEnd=0.0;scene.Frame();CHECK(stack.names.Current()==kMenuMusic);
    // 찾을 수 없는 곡은 이름만 바뀌고 열린 곡(메뉴 곡)과 곡 길이는 유지된다. 곡 끝은 남은 채널의 길이로 정한다.
    stack.missing.insert("thu22.mus");const auto kept=stack.opened.size();scene.Request("thu22.mus");
    CHECK(stack.names.Current()=="thu22.mus" && stack.opened.size()==kept && stack.channel.Active() && stack.live==1);
    // 날씨 설정이 켜지면 현재 색인의 색과 팔레트 갱신 표시를 쓴다.
    palette=true;stack.state.index=1;scene.ApplyWeather();CHECK(stack.state.tint==kTints[1] && stack.state.paletteDirty==1);
}
// 필수 경계 누락과 범위를 벗어난 색인을 거부하고, 비교 불가 시각·감김 색인·빈 이름의 원본 동작을 확인한다.
TEST_CASE(SceneMusic_RejectsInvalidUseAndKeepsOriginalEdges) {
    SceneWorld world;auto hooks=world.Hooks();auto broken=hooks;broken.thunderFlash={};
    CHECK(Throws([&] { SceneMusic(world.state,world.random,broken); }));
    SceneMusic scene(world.state,world.random,world.Hooks());
    // 색인이 0~3을 벗어나면 원본은 표 앞 메모리를 읽는다. 여기서는 진단 예외다.
    world.state.index=-2;CHECK(Throws([&] { scene.Next(); }));CHECK(world.state.index==-1);
    world.state.index=-1;CHECK(Throws([&] { scene.ApplyWeather(); }));
    // 전투 밖 시작은 색인을 3으로 되돌리므로 처음 색인이 범위 밖이어도 읽지 않는다.
    world.state.index=9;scene.Start();CHECK(world.state.index==kMenuMusicIndex && world.state.tint==kTints[3] && world.names.Current()==kMenuMusic);
    // 아주 큰 색인은 32비트로 감겨 0이 된다.
    world.state.index=0x7fffffff;world.state.battle=1;world.durations.values={DoubleBits(214.1)};scene.Next();CHECK(world.state.index==0 && world.names.Current()==kElementMusic[0]);
    // 시계가 비교 불가이면 곡 끝에 이르지 않은 것으로 보고 결과 곡은 잠긴 것으로 본다.
    world.clock.values={DoubleBits(std::numeric_limits<double>::quiet_NaN())};world.clock.used=0;world.state.songEnd=0.0;world.events.clear();scene.Frame();CHECK(world.TakeEvents()=="T");
    world.names.Assign(kFanfareMusic);world.state.fanfareEnd=1.0;world.clock.used=0;CHECK(scene.ResultMusicLocked());
}
