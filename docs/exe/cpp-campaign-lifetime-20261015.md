# 같은 세션의 공통 삭제·기술 내용물·Player 통지

작성일과 마지막 정밀 디컴파일 PC는 **2026-10-15, VM-W11-Codex**다. AGENTS와 양 LEFT를 확인하고 보호 PE/기존 정밀 프로젝트를 읽었다. 이번 모듈의 직접 게임/GUI 실행은0이며, 다른 작업의 화면 검사는 여기에 합산하지 않는다. 보호 원본·AGENTS·dotnetpj는 변경하지 않았다.

이 단위는 실제 emptyGeyser84의 삭제 경로와 기술 ContentForm 수명을 기존 raw 월드에 후설치한다. `RawSurfaceWorld`의 같은 공통 삭제 전역·장부·풀·Graph를 유지하며, 실제 보상과 현재 선택/UI 소비자를 공급한다. `RawCampaignLifetimeBindings`는 기존 `ContentFormLifetime`을 같은 Form 분배기에 등록하고, Player의 부모`+24`는 신규 `RawPlayerContentNotice`의 실제 몸체에 보낸다. Session 조립은 루트 담당이며 이 문서의 API 예시는 아직 적용 완료를 뜻하지 않는다.

## emptyGeyser는 실제 base 가상 표다

PATCH TYPE84의 constructorAddress는 **0**이다. `TypeConstructors.inc`와 보호 PE를 확인하면 새84 객체는 base VT`00501dd0`을 사용한다. 그 표의 실제 항목은 다음과 같다.

| 항목 | 실제 코드 | 재사용 몸체 |
| --- | --- | --- |
| `+10` | `004af780` | `RawSquidDestroy` |
| `+14` | `004b0950` | `SquidDestroyLifecycle::PreDestroy` |
| `+18` | `004b0840` | `SquidDestroyLifecycle::PostDestroy` |
| `+24` | `0041c100` | 기존 빈 부모 통지 |

따라서84에 존재하지 않는 새 파생 삭제를 발명하지 않는다. 바인딩은 **현재 raw type84와 base VT 둘 다 일치할 때만** 위 공통 삭제/Pre/Post로 보낸다. 다른 base 자산 전체나 다른 파생 VT로 지원 범위를 넓히지 않는다. 기존 hits의83/impact 경로와 다른 파생 fallback은 그대로 유지하며 미공급은 명시 오류다.

실제 원본 `00486130`은 기존 empty84를 `flags=0x400000`으로 삭제한다. 같은 공통 삭제기가 dead 재진입·종속 알림/삭제·전파·현재 선택·Unpop·pre/post 깊이·SID 반납을 수행한다. `RawCampaignLifetimeBindings`가 이 동작들을 다시 구현하지 않는다.

## 관찰 경계와 실제 후설치

기존 단독 `RawSurfaceWorld`는 표면 기하/낙하 검사에서 삭제 보상/소리를 관찰 카운터로 남겼다. 이것을 실제 캠페인 경제/UI 효과로 세면 안 된다. 기본 경계의 주석에 **독립 표면 검사 전용**임을 명시했다.

`world.BindDeletion(reward, selected, hooks)`는 첫 객체 생성 전에 한 번 호출한다. 다른 풀/장부의 reward, 비어 있는 현재 선택/실제 효과/파생 forward, 중복 설치, Load/예약/객체 생성 뒤의 설치를 거부한다. 같은 `DeletionState()` 주소를 유지하면서 실제 reward가 있는 공통 Lifecycle로 교체한다. 기존 표면/분배기가 보관한 callback은 현재 공통 객체를 읽는 proxy이므로 이전 객체의 해제된 주소를 사용하지 않는다.

실제 연결 뒤에는 다음을 적용한다.

- 매 공통 pre 진입에서 같은 GameWorld의 현재 선택을 읽고 같은 삭제 state에 반영한다. 선택 해제는 실제 UI 소비자로 전달한다.
- Refund는 같은 `SquidReward`의 실제 SP/지갑/간헐천 pool 계산을 사용한다. 관찰 카운터로 대신하지 않는다.
- 손실 소리는 실제 음향 소비자로 보낸다. 기존 파생/전파 효과는 필수 forward로 남기며 누락된 효과를 무시하지 않는다.
- 독립 표면 검사의 미후설치 기본은 기존 동작을 유지한다. 이 기본을 사용한 검사 통과는 실제 후설치 경제 검증과 구별한다.

`MakeCampaignDeletionEffects`는 실제 선택/파생 삭제 이벤트와 손실 소리 callback을 변환한다. Refund 이벤트가 이 변환기에 도달하면 실제 reward가 없는 잘못된 조립으로 보고 오류를 낸다.

## 기술 ContentForm의 실제 삭제·추가

기술 추가는 기존 `RawContainedCreate → SquidFactory::CreateContained → ContentFormHost::Attach`를 그대로 쓴다. 원본 기술 flags4/attachFlags`0x10`과 현재 부모의 정렬된 체인·부모 추가1 통지를 보존한다. 새 기술이나 별도 Player 사본을 만들지 않는다.

현재 Session은 ContentFormHost만 소유하고 Form6 삭제 수명은 등록하지 않았다. 새 바인딩은 같은 `core.forms.Bind(contents)`로 기존 `ContentFormLifetime`을 등록한다. 기술 삭제는 같은 공통 삭제 → 실제 Content pre → 실제 Host::Unpop/현재 살아 있는 부모의 제거2 통지 → post 깊이 반납 → 같은 풀의 SID 반납으로 이어진다. `RawPlayerTechnology::Clear`가 매번 현재 flags4 첫 항목을 다시 찾으므로 단순 dead표시만 하고 체인을 남기는 동작으로 대체하면 안 된다.

Content의 커서 취소/너겟/소리도 기존 몸체를 보존한다. 기술 kind71처럼 선택되지 않은 너겟 분기는 실제 조건에서 자연스럽게 생략한다. 실제 커서/너겟 경로가 선택되면 현재 ContentLifetimeState와 해당 실제 소비자가 필요하며, 빈 callback으로 성공했다고 세지 않는다. 다른 부모/파생 알림은 기존 fallback 또는 오류다.

## Player 부모+24와 현재 UI

Player152 생성자의 VT`0050ee08`에서 `+24=00490750`을 실제 PE로 확인했다. 이 wrapper는 현재 부모 SID를 구해 `004d55e0(parent,form,reason)`을 호출한다. 단순 생산 Refresh나 no-op가 아니다.

`RawPlayerContentNotice`는 이 실제 사슬의 다음 동작을 복원한다.

1. SP 정합성 접두의 두 현재 offset 값을 비교하고 진단 코드/현재 fallback을 원본 위치에서 읽고 쓴다.
2. 잔여 검사 DWORD가 비영일 때 battleShown이 비영이면 현재 localPlayer의 SP를 실제 조회한다. ROM`00505514`의 **float7000보다 엄격히 클 때** 코드를 쓴다. NaN/동일값은 해당 쓰기를 하지 않는다. SP callback 뒤 fallback을 다시 읽고, callback이 바꾼 현재 잔여 DWORD를 하나 줄인다.
3. 첫 SP 분산배열 토큰의 NULL과 첫 순열 **WORD**`-1` 조건을 검사한다. DWORD 전체`-1` 비교로 바꾸지 않는다. 이 전역은 통신 버퍼/Player 연결이 아니다.
4. 위 효과가 끝난 다음 현재 부모 head WORD부터 순회한다. 각 current type BYTE/현재 Form 타입 DWORD를 비교하고, next WORD는 등록 조회/최종 가상 처리 **전에** 보존한다. free/dead/void라는 추가 필터를 붙이지 않는다.
5. 현재 Form의`+18` 등록ID DWORD로 실제 등록표를 조회한다. 등록이0이면 건너뛰고, 있으면 현재 event`005427bc`와 원래 form/reason을 대상의 직접 가상`+28`에 보낸다. 전체 트리 재귀 사건으로 바꾸지 않는다.

`BindPlayerContentNoticeGumps`는 같은 `GumpCreateState.entries`와 실제 `GumpEventHooks.handle`을 연결한다. 현재 등록 없는0은 원본의 자연 무동작이며 nonnull 대상의 미공급 가상 handle을 무시하지 않는다. 물리1000등록 밖/순환 또는 풀 밖 체인은 host 오류다. 큰 UI/생산 패널 자체가 이번 단위에서 완성됐다는 뜻은 아니다.

SP 접두는 현재 SP 저장소의 전역 참조를 요구한다. 기존 `ScrambledSpStore::Offset()`는 값 반환이므로 그 반환을 지역 const참조로 연장해 영구 상태처럼 공급하면 안 된다. 실제 Session 조립에서 같은 현재 offset/추적 offset/배열 존재/순열/검사 잔여/코드의 소유자를 공급하는 live accessor가 필요하다. 이 남은 공급 경계를 임의0/항등 사본으로 메워 완료했다고 주장하지 않는다.

## 같은 간헐천 pool 주소

`SquidRewardState.geyserPool`과 `LocalGeyserState.resourcePool`은 둘 다 원본 **00595088**이다. `RawLocalGeyser::BindResourcePool(int32_t&)`는 실제 reward pool의 주소만 빌린다. 초기값 복사나 매 프레임 동기화, 임시 예산 사본은 없다. 연결 후 `ResourcePool()`은 항상 그 같은 주소를 읽고 쓴다. 생성/empty삭제/Pop이 pool을 바꾼 뒤에도 부모는 현재 전역을 다시 읽어 원본의 float 저장 → 현재 TYPE cost 감산 → 절삭/max0을 수행한다.

독립 fixture의 기존 값형 backend는 유지한다. 공유 backend에서는 그 값형 칸을`-777`로 두고 원본 출력과221행 전체를 대조하며, 이 칸이 끝까지 바뀌지 않았는지도 확인한다. 신규1입력은 삭제 환급이 pool12000, 후속 Pop이+1000으로 바꾸는 경계 뒤 현재 pool에서 cost를 빼는 사례다. 기존 Pop 정밀도/현재 cost 변경 입력도 동일한 공유 backend로 실행한다.

## 독립 근거와 검사 상태

Player 내용물의 읽기 전용 선택 export는 `tools/ghidra/player-content-notice-functions.json`의3함수다. `tools/decomp_player_content_notice.py`는 그 **실제 wrapper/진단 접두/체인/현재 등록표**를 실행하며 SP 결과와 최종 Gump 가상 handle만 명시 입력/관찰 경계다. 새 **88고유입력·88정상반환**, wrapper82/본문88/등록getter26회이며 정상 EIP/ESP·보존 레지스터·x87`027f`/TOP0·허용 쓰기를 확인했다. OS 실행0이다. 기대 출력은 C++에서 계산하지 않는다.

`CampaignLifetimeTests.cpp`는88행에서 전체4슬롯 **200BYTE**, 현재11전역, SP/UI의 모든 인자/순서를 대조한다. 실제 월드 후설치 검사는 같은 풀의84 생성/Pop → 실제 보상 pool/통계/비용/선택/전파/반납과, 실제 Player152에 기술 ContentForm 생성/부착 → 실제 삭제/체인 제거/깊이 반납을 확인한다. 후설치 누락/늦은 설치도 거부한다. 최종 **Release 게임/검사 빌드 경고0·오류0, Lifetime 집중3개·실패0**이다(`extracted/lifetime-condition-root-build.log`, `extracted/campaign-lifetime-root-final-focused.log`). 같은 통합의 조건4개 통과는 별도 단위의 결과이며 이번 검사 수에 더하지 않는다. 이 상태의 전체 CTest도 **1/1·내부1,218개·실패0, 118.02초**로 통과했다(`extracted/run52-53-ctest.log`). 실제 Session 후설치는 후속 단위다.

최초 집중은 PE88대조1개가 통과했고, World검사2개는 빈 BridgeConnectState의 연결타입0을 넘긴 setup 때문에 원본 guard에서 실패했다(`extracted/campaign-lifetime-root-focused.log`). 실제 PATCH 타입82/94/95/155/157을 입력하도록 고쳤으며 제품 guard는 완화하지 않았다. 같은 빌드의 `std::fill` int→BYTE 경고는 정확한 `uint8_t{}` 입력으로 교정했다. 이 실패/경고를 최종 통과 결과로 숨기거나 native 입력 수에 더하지 않는다.

간헐천 독립 근거는 기존220행을 유지하고 공유자원 변경 입력1행을 추가한 **최신221행**이다. 최초220행 prefix SHA-256은 `e4aeb920062d8db165d7245eac342f3a5f8053442a6beb3f8c8f67851b4720ef`로 포맷 교정 후 이전 근거와 같다. 이전 도구 SHA는 `753f5d3c3b528096eaeab823c381035e38c5be796a0ad66153863cb488b1b724`다. 최신 보고서는 변경된 도구/fixture 해시를 다시 저장했고 최종 `--verify`에서 Player88행과 LocalGeyser221행 모두 통과했다. 기존220을 새로운221과 더해441로 세지 않는다. 새 pool backend는 **Release 게임/검사 빌드 경고0·오류0, LocalGeyser 집중3개·실패0**이다(`extracted/run47-50-build.log`, `extracted/local-geyser-pool-root-focused.log`).

기존 공통 삭제/Content 수명의 native 관찰은 각 원본 fixture/보고서를 재사용한다. 이번 바인딩 검사를 그 몸체의 새로운 native 입력으로 다시 더하지 않는다. 보호 PE SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이며 최신 Player fixture/보고서는 `cpppj/tests/fixtures/player-content-notice-x86.tsv`, `cpppj/player-content-notice.json`이다.

## Session 후설치 순서 예시와 경계

아래는 같은 기존 멤버를 연결하는 API 순서 예시이며 별도 관찰 월드를 만들지 않는다. 실제 UI/SP 접두 전역과 소리/커서 소비자는 현재 클라이언트에서 공급해야 한다.

```cpp
// 첫 Create/Load 전에 같은 현재 삭제 전역에 실제 경제/UI를 공급한다.
world.BindDeletion(hp, services.deletion.selected,
    MakeCampaignDeletionEffects(services.deletion, actualUnitLostSound));

// 기존 contentForms/contained를 같은 수명으로 유지하고 Player 파생 통지를 연결한다.
// contentNotice의 derived callback은 lifetime->ParentNotice(parent, form, reason)를 호출한다.
lifetime = std::make_unique<RawCampaignLifetimeBindings>(world, *contentForms,
    contentLifetimeState, actualContentEffects, hp, *playerContentNotice, otherParentNotice);

// 기존 hits/다른 파생 경계를 합성한 뒤 한 번 BindRuntime에 전달한다.
worldHooks.assets = lifetime->DestroyHooks(hits->DestroyHooks(worldHooks.assets));
worldHooks.destroy = lifetime->Destroy(hits->Destroy(services.destroy));
world.BindRuntime(std::move(worldHooks));

// 이 주소는 현재 삭제 보상이 쓰는 같은 원본 전역이다.
localGeyser->BindResourcePool(rewardState.geyserPool);
```

기존 Session은 BindRuntime 뒤에 ContentHost를 만드는 위치였으므로 위 실제 조립에서는 Host/새 binding의 생성 순서를 첫 객체 이전으로 옮겨야 한다. callback 대상보다 먼저 Kernel을 비우고, 기존 Session의 단일 raw 상태를 유지한다. 공유 Session/GameWorld/CMake/LEFT/커밋은 이 하위 작업에서 변경하지 않았다.

Confirm→Notice→Clear/Place 설치는 루트의 별도 단위다. 실제 Player/AI 시작, 건설/다리/수확·운반·승패 후 저장·Early Missions/캠페인 완주와 UI/음향 전체 동일성은 계속 남는다. DirectDraw 전체화면 문제는 요청대로 제외한다.
