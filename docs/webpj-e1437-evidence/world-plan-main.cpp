// 창을열지않고기존cpppj의씨앗1/AI4명시작계획을출력하는읽기전용비교도구다.
#include "client/SkirmishSetup.h"
#include <iostream>
// 원래생성함수의결과만TSV로기록한다. 게임/플레이어상태를만들지않는다.
int main() {
    netstorm::client::SkirmishSettings settings;settings.aiCount=4;
    const auto launch=netstorm::client::BuildSkirmishLaunch(settings,1U);
    std::cout<<"mapSize\t"<<launch.mapSize<<"\ncentral";
    // 원래6바이트중앙섬기록을순서대로출력한다.
    for(const auto byte:launch.central) std::cout<<'\t'<<static_cast<unsigned>(byte);
    std::cout<<'\n';
    // 참가자순서의fort/영역을바꾸지않고출력한다.
    for(const auto& island:launch.islands) std::cout<<"island\t"<<island.fort<<'\t'<<island.territory<<'\n';
    return 0;
}
