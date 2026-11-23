// 안내·메뉴 그림에 한해 원본10.78 자료에서 빠진 마나 아이콘을 보충한다.
#pragma once
#include "client/GameAssets.h"

namespace netstorm::client { // 게임 월드 자산을 바꾸지 않는 읽기 전용 UI 그림 경계다.
struct TextPictureResource { // 같은 그림의 측정·제한 검사·출력에 공유할 저장소와 논리 프레임 수다.
    const ShapeDatabase* shapes{}; // 반환 뒤에도 GameAssets 또는 정적 보충 자산이 소유하는 SHP다.
    std::size_t block{}; // 해당 저장소 안의 그림 블록 번호다.
    std::int32_t frames{}; // 다중 레이어를 포함하지 않는 UI의 사용 가능한 프레임 수다.
};
TextPictureResource ResolveTextPictureResource(const GameAssets& assets,const TypeAsset& type); // PATCH의 마나8프레임만 보충하고 다른 판본/사용자 자산은 그대로 빌린다.
}
