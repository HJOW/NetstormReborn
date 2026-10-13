# 실제 캠페인 자산 연결부 초안

현재 PC/마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 주 대상은 패치 **10.78**이며 CD **10.72**의 TYPE188/171, Player stride0xb4/0xac 및 raw 폭 차이를 유지한다. 단위14의 Sources/캡처/DudeSpace 자료와 공유 소스는 수정하지 않았다. 새 `client/RawCampaignAssetBindings.h/.cpp`는 **단위15의 미등록·미빌드 초안**이다. 초기172 설치/Gun9개 연결이나 신규 Native 대조 완료를 이 문서만으로 주장하지 않는다.

## 같은 저장소와 API

생성자는 실제 `RawSurfaceWorld&`, 같은 `RawCarrierPostPop&`, 상위가 소유한 `AssetPostPopState&`, 공유 `GameRandom&`, `RawIslandRuntime&`, 실제9 Player BYTE span, `PlayerWorldState&`를 받는다. 추가 `RawCampaignAssetGlobals`에는 상위 `AssetRegularState&/GunProcessState&`, 실제 TYPE+98 원소 span, 현재 발사기 명령 토큰 DWORD 참조가 있다. long 상태·Player·TYPE·섬/목록 사본을 만들지 않는다. 모듈 자신의 수명과 전송 중 임시 packet64BYTE만 소유한다.

```cpp
RawCampaignAssetGlobals globals{regularState,gunState,typeElements,archerNoticeCommand};
RawCampaignAssetBindings bindings(world,damageable,assetState,random,islands,
    playerRecords,playerState,globals,effects);
// 상위는 기존 사제/Player 예약과 전송을 합성하여 runtime에 설치한다.
auto regular=bindings.Handler(existingRegular);
auto transmission=bindings.Transmission(existingTransmission);
bindings.ActivatePostPop();
```

`PrepareFrame(now)`는 같은 GameClock now와 현재 Player fort/battleOwner를 조회한다. 상위는 각 Pop/Frame 진입 직전에 이를 호출하고 `SetLoadingDepth`로 실제 로더 중첩을 전달한다. `RawAssetPostPop` 생성 직후 **단위15 공유 API `BindPlayerRecords(current9BYTE)`**를 호출한다. 현재14에는 이 API가 아직 없으므로 root가14 커밋 뒤 전체 소스/근거 lineage를 보존하여 추가하기 전까지 컴파일할 수 없다. 기존 신전4DWORD의 Before/After 복사는 중첩 Pop getter 시점을 보장하지 못하므로 사용하지 않는다.

Player 신전 원문은 패치+7c/80/84/88, CD+74/78/7c/80이다. AI 등록은 `RawGeyserAiRegistration`이 동일9 BYTE의 현재 포인터를 직접 읽는다. nonnull AI는 첫 실제 요청 인자를 보관한 예외로 거부한다. 선택된 Vortex `localOwner` 읽기까지 매호출 조회하려면 상위의 진입 직전 갱신 계약 또는 공유 optional currentOwner getter가 필요하며 root와 API를 맞추고 있다.

## 직접 연결한 기존 몸체

- 초기 자산의 PostPop은 기존 실제 `RawAssetPostPop`, 예약 Handle는 `RawAssetRegular`, 프레임 진행은 같은 `Core.frame`의 `RawFrameAdvance`다. TYPE 원소 표는 같은 전체 번호순 span을 요구한다.
- Regular는 같은 Host의 `AddRegular(...flags50)`, Gun은 `CreateGunProcess(...type34/flags0)`에 연결한다. 서버 flags0 packet을 숨기는 flags 변환을 하지 않는다.
- Gun packet은 실제 `GunProcess::WritePacket`으로 작성하여 `effects.sendGunPacket`에 전체바이트를 전달한다. 다른 Process writer/transmit는 제공한 실제 fallback에 그대로 분배한다. 실제 사제/Player 사건도 Handler fallback으로 남긴다.
- 현재 surface는 실제 `RawPathSurfaceAt`의 같은 hash0, 일반 one-point 후보는 `RawSquidFinder`의 원본 순서/중복이다. 실제 island getter는 plain 절삭한 hash0→현재 region WORD이며 bridge/보드 밖/표면없음127 계약을 유지한다.
- 실제 공간은 같은 world.Unpop/Repop/Destroy, 연결 통지는 `Core.building.NotifySurface`, 간헐천 표시 갱신은 `Core.display.Update`다. 현재 섬 존재/신전 등록은 같은 `RawIslandRuntime`에 직접 기록한다.
- TYPE+130/+134는 현재 같은 `Core.frames[type]`의 A/B 첫번호다. +74 sound토큰 등 다른 TYPE DWORD는 실제 외부 metadata getter를 요구한다.
- Gun의 현재 Form 전송은 실제 Host.RefreshForm이며 fire35/rotate36 첫번호는 같은 부모의 Form 체인을 순서대로 읽는다. Animation Kill은 해당 현재 Host 객체/타입을 확인한다. 부모 blocked는 패치+40/CD+35 bit20이다. float 구간 난수는 동일 GameRandom 상태를 실제 LCG로 한 번 전진한다.

## 초기71 통지의 실제 의미

기존 `AssetPostPopHooks.removeAnimation` 이름/주석은 실제 하위 의미와 다르다. 패치 **0046a810** / CD **00437410**은 첫 GunProcess 생성 직전에 **00452fe0/CD004a6aa0(command,currentSID,0)**를 호출한다. 이 몸체는 실제40칸 입력 명령 큐다. 양 PE의 command 전역 **005429b4/CD00512b3c** 초기DWORD는 **030000b0**이다.

새 binding의 해당 훅은 현재 command 참조와 SID/0을 `effects.queueCommand`에 전달한다. 실제 `client::InputQueue::PushCommand`에 연결할 수 있다. Animation Form이 없다는 이유로 통지를 생략하거나 Host.Kill로 대체하지 않는다. 현재 shared14 주석은 동결했고 API 의미는 이 문서에서 분리하여 기록한다.

## 외부 효과와 남은 범위

Vortex 가상 owner, impactSound, 실제 region BYTE 지도, 배치 취소/Slot·생산 갱신, blocker, Residence 사건2, Ascend/이탈/피해/건설, 표적 감시·검색/공격 방향·8DWORD 문맥/실제 발사간격·사거리 시각 압축/Animation·음향 하위는 실제 콜백을 공급하거나 **선택되는 호출에서 명시 예외**로 유지한다. 콜백을 빈 성공/0 반환으로 대신하지 않는다. 일반 Residence owner만 기존 Core.owner 몸체를 사용하며 Vortex는 별도 실제 파생 setter가 필요하다.

현재 초기 생성자/첫 stage0→1까지만 추가 Gun 경계를 요구하지 않는다. 다음 검색·전투는 위 필수 효과를 닫아야 한다. 실제71 TYPE는 directions0/frame35이고 group0라 회전 단계를 건너뛰며 Fire-only로 진행한다. 과거 합성20/40 값을 실제 metadata로 주입하지 않는다.

상위는 연결 객체를 해제하기 전에 실제 Kernel을 정리하고 반환 Handler/Transmission의 사용을 끝내야 한다. class가 장기 월드를 소유하거나 독립 새 풀을 만들지는 않는다. 공유 등록/CMake·빌드/검사·commit·LEFT 상세 갱신은 root가 담당한다.
