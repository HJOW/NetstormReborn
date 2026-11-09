# webpj Factory 업그레이드(RawFactoryUpgrade·RawFactoryUpgradeCommand) 이식 — WB04-E218

2026-11-10, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절 "자원·생산·건설 명령" 묶음의 일부다. Factory 업그레이드의 요청(00453bf0)·수신(00453d70 의 58h mode 1 분기)과, event 3 의 예측 그림 생성·완료·종속 그림 정리(004549c0·004545e0·004537f0·00453c90)를 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawFactoryUpgrade.cpp/.h`·`RawFactoryUpgradeCommand.cpp/.h`)와 시험(`FactoryUpgradeTests`·`FactoryUpgradeCommandTests`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 루트에서 확인).
- 원본 fixture: 없다. cpppj 시험 15건은 손으로 쓴 계약 시험이다(원본에는 "작성·컴파일만" 표기). 같은 PC 의 통합 시험 실행 파일(`cpppj/build/bin/Release/netstorm_tests.exe --filter FactoryUpgrade`, 2026-11-08 빌드, 게임 실행 아님)에서 관련 CampaignSession 시험 3건을 포함해 18건 통과를 확인했다.
- 공유 장치(모두 기존 모듈 재사용): E217 `raw-factory-research.js`(`writeFactoryResearchCommand`), D5c `squid-process.js`(`SquidProcessHost.findEvent/kill/addRegular/addSharedRegular`), D2 `squid-destroy.js`, E2 `raw-squid-word.js`(`setRawSquidHitPoints`).

## 1. 옮긴 것

`www/core/raw-factory-upgrade-command.js` — `RawFactoryUpgradeCommand(pool, types, state, hooks)`

- `request(factory)`(00453bf0): 원본 자체에는 비용/소유자/이미 진행 중인지 검사가 없다. SharedRegular 예약이 성공하면 event 4(payload 20초)를 붙이고(실패해도 계속), 예약/부착 효과 **뒤의 현재 시각 + 15초**를 wire 완료 시각으로 쓴 58h kind 0/mode 1 의 16 BYTE 를 채널 9 로 송신한다.
- `receive(packet)`(00453d70 의 업그레이드 분기): 부모가 free/dead(상태 비트 3)이면 무시하고, kind≠0 이거나 mode≠1 이면 무시한다(연구 분기와 달리 sender/owner 일치를 추가하지 않음). 검사 모드에서 Factory genus(0x4000)가 아니면 진단 뒤 현재 genus 를 다시 읽는다. 비용 조회 → 소유자 SP 에서 비용만큼 차감 → 원본 예약 로그 → SharedRegular 예약 성공 시 event 3(지연 = 완료 시각 − 현재 시각, **최소 1초, NaN 은 NaN 그대로**) → 현재 boss 이면 원문 16 BYTE 방송(짧은 패킷은 거부). 선택된 누락은 앞선 효과를 보존한 채 그 위치에서 거부한다.

`www/core/raw-factory-upgrade.js` — `RawFactoryUpgrade(pool, host, state, hooks)`

- `nextLevel(parent)`(00453750)·`begin(parent, payload)`(004549c0 count 0: 다음 레벨 abstract client 그림 → 레벨 → extra 8 → void 가 아니면 진단 0x282 → extra 1 → 상태 WORD 0 → 소유자 → 공간 등록(y + 0.001 오프셋) → 로그 → 건설 진행 → 감시 쌍)·`complete(parent)`(004545e0: 레벨/최대 HP DWORD 쓰기/프레임 보정/표시/위치 소리/로컬 안내 소리/플레어 → 모든 event 4 와 3 예약 제거 → 감시 Form 의 예측 그림 삭제)·`finish(parent)`(004549c0 count 양수: 로그 → 완료 → 현재 AI 통지 → −1)·`hasPicture(parent)`(004537f0)·`isUpgrading(parent)`(00453c90: 그림 → event 4 → event 3 단락 조회).
- 아웃포스트의 완료는 모든 효과와 예약/그림 삭제를 생략한다(finish 의 AI 통지는 남는다). 감시 Form 순회는 후보를 돌려주기 전에 next 를 저장하고(`#next` cursor) 같은 타입/extra 8/client SID(5..14999)인 감시자만 지우며, dead 부모 진단 0x158·자기 자신 감시 진단 0xad/0xdd 를 보존한다. 순환 체인은 오류이다.

## 2. 검증

`tests/factory-upgrade.test.mjs`(16건)

- cpppj 계약 이식 15건: Command 8건(요청 대기 20초/완료 15초, 예약 실패와 현재 시각, 비용→로그→예약→방송, 기한 전/같음/미래/NaN, 상태·검사·genus 관문, 효과 뒤 현재 owner/시각/boss, 할당 실패와 선택된 누락/짧은 패킷, 연구 수신 부모의 업그레이드 분기) + Upgrade 7건(begin 순서와 raw, 레벨 상한과 재조회, finish 순서와 현재 소유자, 아웃포스트 완료 생략, 실제 Host event 4/3 제거와 다른 사건 보존, 예측 그림 필터와 저장 next, 진단/누락 하위).
- 웹 계약 1건: 다른 풀의 Host 거부와 순환 종속 체인 거부. 상태 비트는 free(1)·dead(2)·둘(3) 각각으로 확인한다.
- 장면: 실제 `SquidProcessHost`+`Kernel`+`RawSquidDestroy`(ProcessForm 가상 삭제)에 실제 보호 TYPE 사본을 쓴다(공통 ProcessForm 생성이 읽는 내장 생성자 주소 포함).

최초 실패와 교정: 모듈은 처음부터 cpppj 계약이 모두 통과했다. 첫 작성한 한 시험이 실패했는데 시험 이식 결함이었다("짧은 패킷"에서 비용 차감 횟수를 0 으로 단언했으나 원문은 시각 필드 읽기에서 거부하기 전에 이미 차감하므로 1 이다; cpppj 의 `money.size()==1 && timers.empty()` 를 그대로 옮겨 고침). 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 모두 원본으로 되돌렸다(cmp 일치).

- 실패한 변형: 예측 그림 SID 상한 15000→15001, 완료의 event 3 제거 생략, 아웃포스트 관문 제거, 요청 대기 20→15, 수신 최소 지연 1→2, 부모 무효 상태 마스크 3→1(상태 3 만 쓰던 시험을 free/dead 각각으로 보강한 뒤 실패).
- 동치 변형: 레벨 상한 비교 `>3`→`>2`. 다음 레벨이 3 일 때는 상한 경로와 두 번째 조회 경로가 모두 3 이라 결과가 같고, 두 조회 사이에 값이 바뀌는 시험은 다음 레벨 1 에서 갈린다.

## 3. 경계와 남은 것

- 실제 월드에 연결하지 않았다. 호출 지점(Factory 메뉴 업그레이드 입력·SharedRegular 처리기의 event 3/4 분배·Handle 의 후속 AI 검사)과 실제 가상 level/표시/건설/Depend 하위는 순서4~6 몫이다(PORT_MAP session 0). cpppj `CampaignSession_FactoryUpgrade*`·`AssetRegular_FactoryUpgradeDispatchAndUpperFlare` 시험은 그 client/연결 단위를 옮길 때 대조한다.
- 58h 패킷 분배기 등록(`RegisterFactoryResearchCommand`)은 E217 과 같이 `RawPacketDispatch` 뒤이다.

## 4. 다음 시작점

WB04-E219 는 3.1절에서 고른다. "자원·생산·건설 명령" 묶음의 남은 것은 `RawPacketDispatch`(순서4)에 걸려 있다. 따로 옮길 수 있는 것으로 `RawFrameChoose` 보강(`port` 승격)과, 기준선 뒤 변경이 없는 `o/` 후보(`RawFlyerFrame`·`RawFlyerTarget`·`RawSunFlyerFrame`·`RawWindFlyer`·`SquidSpatial`·`RawTerrainGrowth` 등)를 권한다.
