# M6-n 잔여 4건의 도달성 판정과 종결 (제품 1278 유지, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음, 재디컴파일 없음). 코드 변경 없음(문서·정적 감사만). MP 실행 0.

1272 문서의 "남은 M6-n"에 남겨 둔 네 항목을 `extracted/refined/originals/Netstorm.c`의 정적 호출자 그래프(함수 경계 분할 후 `\bFUN_xxxx\b` 검색)와 현재 C++의 연결 위치로 판정했다. 결론은 네 항목 모두 **세션(World)이 있는 미션/전투/Fort 로드 안에서만 도달**하며 Session 밖 장기 Storage(NoWorld)에 새로 연결할 몸체는 없다는 것이다.

## 1. 큰 AI 장기 소유자(비영 brain)

- brain 생성 `FUN_00417e50`의 호출자는 `FUN_00417f50`과 `FUN_004c2d40`뿐이다.
- `FUN_00417f50`의 유일한 호출자 `FUN_00417fa0`은 첫 줄에서 `DAT_00594fbc`(inBattleMode)가 0이면 아무것도 하지 않는다. 그 호출자 `FUN_004d62b0`은 명령 처리(AddRandom/AI 유틸 명령)이며 월드가 있는 전투에서만 입력이 도달한다.
- `FUN_004c2d40`의 유일한 호출자 `FUN_004c2b20`은 미션 시작(`RawMissionStart`)의 한 단계다.
- 따라서 Session 없는 메뉴/전환에서는 비영 brain이 만들어지지 않는다. 2026-10-26 `cpp-state-runtime-coordinator` 문서의 "첫 메뉴의 현재 비영 brain 거부" 계약과도 일치한다. 구현 없음.

## 2. 일반 Descend(`RequestVortexDeparture`)의 Session 의존

- Descend/Ascend 명령 몸체(`CommandVortex`, `QueueVortexDescend`, `RequestVortexDeparture`)는 신전(Vortex) Regular process 안에서 실행되며 원문도 `Vortex.cpp`에서 `inBattleMode`(0x2ef)를 단언한다. 신전 객체가 존재하는 월드에서만 도달한다.
- 이 호출은 `RawCampaignSession`의 장기 Pool/Player/섬 목록을 읽으므로 현재 Session 의존은 원문의 월드 의존과 같다. Session 없는 경로로 옮길 대상이 아니다.

## 3. 배치 호출자 전수

원문 단일 배치 `0040db90`의 호출자는 `0040ded0`(지역 난수 다중 배치)과 `0040e560`(← `0046da70` Generate)뿐이고, `0040ded0`의 호출자 여섯은 모두 구현 위치가 있다.

| 호출자 | 원문 부모 | 현재 구현 위치 |
| --- | --- | --- |
| `0040dfd0` AddRandom | `004d62b0`(명령), `0046e630`(Fort 섬 채우기) | `o::AddRandomObjects`, `RawCampaignSession::AddRandomObjects`/`PopulateAlternateFortIsland`(1278) |
| `0040e050` | `00485e40` | `RawNuggetReplenish`/`RawCarrierPreDestroy` |
| `0040e130` Residence | `0046df80` | `RawIslandPopulation::Residence` |
| `0040e1a0` | `004be610`(저장 객체 로드) | `RawFortObjectLoad`/`RawFortFinishOwner` |
| `0040e2b0` 사제 교체 | `0046e000`, `004923e0`, `00492710` | `RawIslandPopulation::ReplaceInitial`, `PopulateInitialFortPriest`, `RawFortPriestPreparation` |
| `00486440` 무작위 간헐천 | `00484ab0`(설정 파서), `004b67c0`, `004c2b20`, `004d62b0` | `RawCampaignMissionStartBindings`, `AiUtilityCommandInput`, `RawCampaignSession` |

모두 월드가 있는 로드/전투/명령에서 도달한다. 이번 감사 중 이 경로의 미연결 두 곳(alternate Fort 섬 채우기와 사제 준비 `setOwner`)을 1278에서 교정했다.

## 4. Fort 상위 Finish(`004b5f70` ← `004b6dd0`)

- `004b5f70`(Fort World 로드)의 호출자는 `004b6dd0`(State 전이)뿐이고, 그 호출자 여덟(`004b8810/88c0/8940/8a10/8ac0/8b20/8cb0/9230`)은 State 이벤트 래퍼다. 앞의 일곱은 `MetaConnectionRuntime`/`State`/`NativeLoginPacketReceivers`/`ClientCampaignTransport`/`ClientLocalStateGlobals` 등에 연결돼 있다.
- `004b9230`(마지막 래퍼)은 소스에서 이름이 직접 보이지 않으나 State 37(로컬 서버 실행) 이벤트(`DAT_00542ae4`)의 몸체이며 `NativeLocalServerLaunch`가 같은 State 37 설정/검색/실행을 구현한다(`005c8600/8608` 타이머 포함).
- Fort 로드 자체는 `RawCampaignWorldRuntime::LoadExistingFort`가 실제 Session 안에서 수행하며 1278이 마지막 미연결 훅을 채웠다.

## 5. 한계

- 이 판정은 **정적 호출자 그래프와 현재 소스 위치 대조**다. MP 실행으로 확인하지 않았다. 반례(Session 없는 경로가 위 몸체에 도달)가 나오면 해당 경로를 그때 장기 소유자로 옮긴다.
- `004d62b0`은 전역 명령 처리기이므로 AddRandom(`0040dfd0`)·무작위 간헐천(`00486440`) 명령은 메뉴에서도 큐에 들어올 수 있으나 두 명령은 개발자(boss) 권한 명령이며 월드 없는 Native 경로에서 실행하는 일반 사용자 경로가 없다. 실제 실행은 Session의 설치 뒤로 제한된다(`AddRandomObjects`의 `loaded` 검사).
- 함수 포인터 표로만 도달하는 호출은 호출자 그래프에 나타나지 않는다. `00484ab0`처럼 표(`0050d954[1]`)로만 불리는 설정 파서는 위 표에 별도 근거로 넣었다.

## 6. 결과

M6-n의 두 줄(NoWorld·자산 삭제/AI, 호출자·owner 전수 감사)은 구현이 필요한 미연결이 남지 않은 정적 종결 상태가 되었다. 남은 M6 확인은 MP 실행/로비 표시 충실도로 사용자 직접 확인 대상이다.
