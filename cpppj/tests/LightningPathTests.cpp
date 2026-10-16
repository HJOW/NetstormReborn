// 실제 빔 경로의 모든 중간64BYTE/난수/꼬리와120입자를 독립 원본 비트로 비교한다.
#include "RawSceneSupport.h"
#include "o/RawLightningPath.h"
#include <cstdio>

using namespace netstorm::test::rawscene;
namespace {
// 입력 hex의 원문 바이트만 복원하며 경로/난수 기대값은 계산하지 않는다.
std::vector<std::uint8_t> Decode(std::string_view text) {
    if (text.size()%2) throw std::runtime_error("빔 경로 fixture hex 폭 오류");std::vector<std::uint8_t> result;
    // 두 글자마다 바이트 하나를 복원한다.
    for (std::size_t index=0;index<text.size();index+=2) result.push_back(static_cast<std::uint8_t>(std::stoul(std::string(text.substr(index,2)),nullptr,16)));
    return result;
}
// 원본 little endian DWORD를 출력 자료의 끝에 추가한다.
void Append(std::vector<std::uint8_t>& bytes,std::uint32_t word) {
    // 낮은 바이트부터 정확히 네 바이트만 쓴다.
    for (unsigned index=0;index<4;++index) bytes.push_back(static_cast<std::uint8_t>(word>>(8*index)));
}
// 같은 시점의 전체 경로64BYTE·현재 total·실제 난수 상태를 붙인다.
void Snapshot(std::vector<std::uint8_t>& bytes,const LightningPathFields& fields,const LightningParticles& particles,const GameRandom& random) {
    bytes.insert(bytes.end(),fields.bytes.begin(),fields.bytes.end());Append(bytes,static_cast<std::uint32_t>(particles.total));Append(bytes,random.State());
}
// 용량 전체의 실제24BYTE 의미 필드를 원본 순서로 직렬화한다.
std::vector<std::uint8_t> Particles(const LightningParticles& particles) {
    std::vector<std::uint8_t> bytes;
    // 미사용 꼬리/NULL 이전 포인터도 비교 범위에 포함한다.
    for (const auto& particle:particles.values) {
        Append(bytes,std::bit_cast<std::uint32_t>(particle.x));Append(bytes,std::bit_cast<std::uint32_t>(particle.y));Append(bytes,std::bit_cast<std::uint32_t>(particle.strength));
        Append(bytes,std::bit_cast<std::uint32_t>(particle.jitterWeight));Append(bytes,std::bit_cast<std::uint32_t>(particle.life));Append(bytes,particle.previous);
    }
    return bytes;
}
// 최초 차이의 바이트 위치와 실제/원본 값을 입력 번호와 함께 보고한다.
void Compare(std::span<const std::uint8_t> actual,std::span<const std::uint8_t> expected,std::size_t row,const char* field) {
    if (actual.size()!=expected.size()) { const auto text="빔 경로 행"+std::to_string(row)+" "+field+" 폭 불일치";netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str());return; }
    const auto mismatch=std::mismatch(actual.begin(),actual.end(),expected.begin());
    if (mismatch.first!=actual.end()) {
        const auto offset=static_cast<std::size_t>(mismatch.first-actual.begin());const auto text="빔 경로 행"+std::to_string(row)+" "+field+" 바이트"+std::to_string(offset)+" 실제"+std::to_string(*mismatch.first)+" 원본"+std::to_string(*mismatch.second);
        netstorm::test::ReportFailure(__FILE__,__LINE__,text.c_str());
    }
}
// 실제 ctor/Seed/Step을 같은 입력 저장소로 실행한다. 전체 화면/피해 연결과 구별한다.
void Replay(std::uint32_t kind) {
    SidPool pool(OriginalEdition::Patch1078,24000,false);
    // 원문이 읽는 두 SID50/51을 포함하는 실제 저장소를 준비한다.
    for (std::uint16_t sid=5;sid<=51;++sid) CHECK(pool.Allocate(2)==Sid{sid});
    std::size_t count=0,rowIndex=0;
    // 현재 종류의 기본53비트 원문을 사용하며 기대값은 재생성하지 않는다.
    for (const auto& row:LoadFixture(NETSTORM_LIGHTNING_PATH_FIXTURE).rows) {
        const auto index=rowIndex++;if (row.size()!=13) throw std::runtime_error("빔 경로 fixture 열 수 오류");if (Number(row[0])!=kind) continue;
        LightningPathState state;state.scaleX=std::bit_cast<float>(Number(row[7]));state.scaleY=std::bit_cast<float>(Number(row[8]));GameRandom random(Number(row[9]));
        // 실제 두 raw 슬롯의 타입/좌표만 구성한다. 중심의 발자국은 타입별로 동일하다.
        for (const auto [sid,type,x,y]:{std::array<std::uint32_t,4>{50,Number(row[1]),Number(row[3]),Number(row[4])},{51,Number(row[2]),Number(row[5]),Number(row[6])}}) {
            auto raw=pool.AllocatedBytes(Sid{static_cast<std::uint16_t>(sid)});std::fill(raw.begin(),raw.end(),std::uint8_t{});raw[10]=static_cast<std::uint8_t>(type);Put(raw,14,x);Put(raw,18,y);
        }
        RawLightningPath path(pool,random,state,[&](Sid sid) {
            const auto raw=pool.Slot(sid);const int foot=raw[10]==148 || raw[10]==149 ? 3 : raw[10]==144 ? 2 : 1;
            return std::array<float,2>{static_cast<float>(static_cast<double>(std::bit_cast<float>(Get(raw,14)))-foot*.5),static_cast<float>(static_cast<double>(std::bit_cast<float>(Get(raw,18)))-foot*.5)};
        });
        LightningPathFields fields;fields.bytes.fill(0xa5);LightningParticles particles;particles.values.resize(120);particles.owner=1;std::vector<std::uint8_t> trace;
        path.Initialize(fields,Sid{50},Sid{51},1);Snapshot(trace,fields,particles,random);
        if (kind) {
            path.Seed(fields,particles);Snapshot(trace,fields,particles,random);
            // 원본의 진행/후퇴/완료 후 호출 순서를 보존하도록 각 입력 쌍을 그대로 전달한다.
            for (const auto& step:Split(row[10],',')) {
                const auto pair=Split(step,':');if (pair.size()!=2) throw std::runtime_error("빔 경로 단계 인자 오류");
                path.Step(fields,particles,std::bit_cast<float>(Number(pair[0])),std::bit_cast<float>(Number(pair[1])));Snapshot(trace,fields,particles,random);
            }
        }
        Compare(trace,Decode(row[11]),index,"중간 상태");Compare(Particles(particles),Decode(row[12]),index,"120입자");++count;
    }
    CHECK(count==(kind==0 ? 72 : 144));
}
}
// source/target의 다른 높이 보정·배율·퇴화 점·미사용 바이트를 실제 생성자와 비교한다.
TEST_CASE(LightningPath_NativeConstructor) { Replay(0); }
// 재귀 가지/난수 두 번 소비·완료 후 재호출/전체120입자를 원본 그대로 대조한다.
TEST_CASE(LightningPath_NativeGeneration) { Replay(1); }
