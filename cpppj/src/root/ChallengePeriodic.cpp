// 현재 CCh의 주기 생성/배치와 원문 전역 난수 소비 순서를 복원한다.
#include "root/ChallengeDetail.h"
#include "root/ParticipantState.h"
#include "root/Clock.h"
#include "root/Random.h"

namespace netstorm::root { // 같은 raw 전투/참가자·현재 시계/공유 scratch·실제 송신만 사용한다.
using namespace challenge_detail; // 실제 비정렬 필드의 폭과 packet 본문 보존을 공유한다.
namespace { // 원문의 float/double 필드와 PE의 가상 참가자 이름이다.
constexpr std::array<std::string_view,25> names{{"JiMBoB","killer","torpedo","Sgt Bilko","zippy","uppity","psi-master","fanTom","WillyW","JQuest","CaptX","DollarDog","MasterMind","Oinker","PKarpas","Sammy","BB_Ghali","KJohnson","Scratcher","SMeyer","CheeseBoy","ZeroDog","StickMyEye","Mariah","gypsy"}}; //004cd538의 실제25개 포인터를 ASCII 값 그대로 복사했다.
float Float(std::span<const std::uint8_t> bytes,std::size_t at) { return std::bit_cast<float>(Read(bytes,at)); } // 현재 raw float32의 비트를 조회한다.
double Double(std::span<const std::uint8_t> bytes,std::size_t at) { if (at>bytes.size() || 8>bytes.size()-at) throw std::out_of_range("R CCh 현재 FullTime8BYTE 범위");double value;std::memcpy(&value,bytes.data()+at,8);return value; } // 현재 전투의 삭제/생성 시각을 별도 캡처하지 않는다.
float Distance(float x,float y,const ChallengePoint& point) { const float dx=x-point[0],dy=y-point[1];return static_cast<float>(double(dx)*double(dx)+double(dy)*double(dy)); } // 원문 dx/dy와 최종 거리의 float32 저장 경계를 유지한다.
}
std::array<std::uint8_t,6> ChallengeRuntime::RandomIdentity() { std::array<std::uint8_t,6> identity{};identity[0]=static_cast<std::uint8_t>(random_.Next()%21U);identity[3]=static_cast<std::uint8_t>(random_.Next());identity[1]|=1U;return identity; } //44860의 초기0 뒤 실제 두 난수만 소비한다.
std::array<std::uint8_t,10> ChallengeRuntime::RandomCore() {
    std::array<std::uint8_t,10> core{};Put(core,4,random_.Next()%10U,1);
    for (std::size_t bit=0;bit<22;++bit) if (random_.Next()%1000U<500U) Put(core,6,Read(core,6)|(1U<<bit)); // 현재 능력DWORD의 해당22비트만 OR한다.
    Put(core,5,random_.Next()%3U,1);Put(core,2,random_.Next()%1000U,2);Put(core,0,Read(core,2,2)+random_.Next()%100U,2);return core;
}
void ChallengeRuntime::InitializeBattle(std::uint32_t challenge,std::uint32_t battle,std::span<const std::uint8_t,6> identity,const ChallengePoint& position) {
    auto record=Battle(challenge,battle);if (Read(record,0)&1U) Assert("!isValid()",455);Put(record,0,1);Put(record,4,battle);Copy(record,8,identity);Put(record,0x10,std::bit_cast<std::uint32_t>(position[0]));Put(record,0x14,std::bit_cast<std::uint32_t>(position[1]));std::fill_n(record.begin()+0x20,0x20,std::uint8_t{});Put(record,0x18,0xffffffffU);Put(record,0x1c,0x100);std::memcpy(record.data()+0x150,&clock_.game,8);
}
ChallengePoint ChallengeRuntime::FindPosition(std::uint32_t challenge,const ChallengePoint& center,std::int32_t distance,std::int32_t range) {
    const auto threshold=distance ? std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(distance)*static_cast<std::uint32_t>(distance)):225;const auto span=range ? std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(range)*2U):50;float bestDistance{};std::optional<ChallengePoint> best;
    for (std::size_t attempt=0;attempt<5;++attempt) { // 원문은 먼저 y,그다음 x 난수를 소비하며 후보5개까지만 조회한다.
        const auto first=random_.Next();if (!span) throw std::domain_error("R 배치 rand idiv의 원문0 제수");const auto y=static_cast<std::int32_t>(first)%span-span/2;const auto x=static_cast<std::int32_t>(random_.Next())%span-span/2;const ChallengePoint point{{center[0]+static_cast<float>(x),center[1]+static_cast<float>(y)}};float nearest=100000000.0f;
        for (std::uint32_t battle=0;battle<10;++battle) { const auto record=Battle(challenge,battle);const auto squared=Distance(Float(record,0x10),Float(record,0x14),point);nearest=nearest<squared ? nearest:squared; } // 유효 여부를 거르지 않는 원문의 현재10개 전투 거리다.
        for (std::uint32_t participant=0;participant<40;++participant) { const auto record=Player(challenge,0);const auto squared=Distance(Float(record,0x10),Float(record,0x14),point);nearest=nearest<squared ? nearest:squared; } //457ea6/eb2는 실제로 전투 순회가 끝난 stale 포인터를 계속 읽는다. 별도 참가자 포인터 증가를 거리 참조로 바꾸지 않는다.
        if (static_cast<float>(threshold)<nearest) return point;if (bestDistance<nearest) { bestDistance=nearest;best=point; }
    }
    if (!best) throw std::out_of_range("R 배치의 원문 미초기화 best 위치 읽기 거부");return *best;
}
std::int32_t ChallengeRuntime::Tick(std::uint32_t challenge) {
    std::int32_t firstEmpty{},activeBattles{},activePlayers{};
    for (std::uint32_t battle=1;battle<10;++battle) { if (Read(Battle(challenge,battle),0)&1U) ++activeBattles;else if (!firstEmpty) firstEmpty=static_cast<std::int32_t>(battle); } // 원문 선두 빈 슬롯 지역도 계산하되 후속 루프의 검색을 생략하지 않는다.
    for (std::uint32_t participant=1;participant<40;++participant) if (Read(Player(challenge,participant),0x64)&1U) ++activePlayers; // 현재 실제 활성 참가자 수다.
    static_cast<void>(firstEmpty);
    for (auto count=activeBattles;count<10;++count) { // 이 부모의 목표 전투 수는 원문처럼 옵션이 아닌 상수10이다.
        for (std::uint32_t battle=1;battle<10;++battle) { // 매회 첫 빈1..9를 새로 검색하며 현재 삭제 시각/deadTime 관문을 적용한다.
            const auto record=Battle(challenge,battle);if ((Read(record,0)&1U) || !(double(options_.battleDeadTime)<clock_.game-Double(record,0x150))) continue;const auto position=challenges_.Geometry().ring[battle];const auto identity=RandomIdentity();InitializeBattle(challenge,battle,identity,position);Broadcast(challenge,BattlePacket(challenge,battle,false));break;
        }
    }
    if (!options_.fakePlayers || activePlayers>=options_.fakePlayers) return 1;
    for (auto count=activePlayers;count<options_.fakePlayers;++count) { // 현재 옵션을 매회 읽으며 원문 count>39 관문 위치도 보존한다.
        std::uint32_t selected{};
        for (std::uint32_t participant=1;participant<40;++participant) { // 빈칸이 없으면 마지막39번 포인터가 남는 원문을 유지한다.
            selected=participant;auto player=Player(challenge,participant);if (Read(player,0x64)&1U) continue;const auto core=RandomCore();const auto point=FindPosition(challenge,{{128.0f,128.0f}});const auto identity=RandomIdentity();const auto name=names[random_.Next()%names.size()];std::array<std::uint8_t,8> position{};Put(position,0,std::bit_cast<std::uint32_t>(point[0]));Put(position,4,std::bit_cast<std::uint32_t>(point[1]));InitializeParticipant(player,participant,0,name,identity,core,position,0);
            if (random_.Next()%4U<3U) {
                const auto roll=random_.Next();const auto divisor=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(options_.fakeBattles)-1U);if (!divisor) throw std::domain_error("R fake battle rand idiv의 원문0 제수");const auto chosen=static_cast<std::uint32_t>(static_cast<std::int32_t>(roll)%divisor+1);auto battle=Battle(challenge,chosen);
                if (Read(battle,0)&1U) {
                    std::uint32_t free{};for (std::size_t slot=0;slot<8;++slot) if (!Read(battle,0x20+slot*4)) ++free; // 유효 전투의 실제 빈8칸 개수이며4보다 커야 배정한다.
                    if (free>4) {
                    Put(player,0xa4,chosen);std::int32_t member=-1;
                    for (std::uint32_t slot=0;slot<8;++slot) if (!Read(battle,0x20+slot*4)) { Put(battle,0x20+slot*4,participant);member=static_cast<std::int32_t>(slot);break; } //0044c300의 첫 빈칸/진단 뒤-1 반환이다.
                    if (member<0) Assert("false",441);Put(player,0xa8,static_cast<std::uint32_t>(member));if (free==8) Put(player,0x64,Read(player,0x64)|4U);
                    }
                }
            }
            break;
        }
        if (count>39) return 1;if (!selected) { Assert("c",5516);throw std::out_of_range("R 가상 참가자의 원문 NULL 포인터 읽기 거부"); }Broadcast(challenge,PlayerPacket(challenge,selected,false));
    }
    return 1;
}
}
