# 캠페인 설치 모드 alternate Fort 섬 채우기(0046e630) 연결 (제품 1278, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음, 재디컴파일 없음).

## 1. 발견한 문제

메인 메뉴 `Multiplayer` 버튼은 `State 6` 뒤 Fort 파일이 없는 환경에서 새 Fort를 만들며(`LoadExistingFort(..., alternate=1)`) 원문 `RawFortWorldLoad::Load`의 else 분기 `islands.Exists(0) → populateIsland(state,0)`에 도달한다. 이 훅이 `RawCampaignWorldRuntime.cpp`에서 항상 "캠페인 설치의 alternate Fort populate 미연결" 오류를 던져 프로세스가 종료됐다(1276 발견, 오류 로그가 바로 원인을 지목).

## 2. 원문 근거

원문 `0046e630`(CD `004906c0`)을 C와 PE 기계어로 확인했다.

1. `this[0]`이 월드(`0054c8a0`)인지, builder `cRect` 네 필드가 −1이 아닌지 진단(`Islandbuilder.cpp` 0x57/0x58)을 보고한다.
2. `0040dfd0(DAT_005411b4, 1)` = AddRandom으로 `SecondTimerFactoryType`(005411b4, 초기값 87) 객체 1개를 지역0에 놓고 마지막 배치 SID를 eax로 받는다.
3. `0046e000(this, 0)` = `RawIslandPopulation::Populate(0)`(거주 구역·초기 교체·청크/생산 통지).
4. SID가 0이 아니면 `[pool+sid*0x32]`의 가상 `+0x10`(전체 삭제)을 flags `0`으로 꼬리 호출해 2번의 임시 객체를 지운다.

PE 디스어셈블(`0046e69d..0046e6b4`)에서 `mov [esp+4],0 ; jmp [edx+0x10]`으로 flags 0 인자를 확인했다.

## 3. 구현

- `o::AddRandomObjects`가 이제 마지막 `place` 결과(성공한 마지막 SID, 실패는 0)를 반환한다. 일반 `AddRandom` 부모는 계속 무시하고 이 부모만 읽는다.
- `RawCampaignInstallEffects::populateIsland`를 추가하고 비어 있을 때는 기존 미연결 오류를 유지한다.
- `RawCampaignSession`에 `WithIslandPopulation`(사제 준비와 같은 배치/거주 구역/교체/생산 통지 장치를 한 번 빌려 주는 공통 도우미)과 `PopulateAlternateFortIsland`를 추가했다. 기존 `PopulateInitialFortPriest`는 같은 도우미를 쓰도록 바꿨고 동작·메시지 접두는 유지된다.
- **같은 줄 주석이 코드를 삼킨 결함 교정:** `PrepareExistingFortPriests`의 `hooks.region=...; // 원문0046f530... 유지한다.hooks.setOwner=...` 줄에서 `//` 뒤의 `hooks.setOwner` 대입이 주석이 되어 사제 준비의 `setOwner`가 미연결이었다. 같은 populate 경로를 실제로 실행하자 `Fort 사제 준비의 선택 장치 미연결`로 드러나 줄을 분리했다. 전체 소스를 정규식으로 다시 훑었으나 같은 유형은 더 없었다.

## 4. 검증

- Release 빌드 0 경고(중간에 지역 변수 `placement`가 멤버를 가리는 C4458을 `addRandomPlacement`로 개명).
- `netstorm_add_random_tests` 12건 → 반환값 시험을 더해 13건 통과.
- `netstorm_guide_fort_request_tests`는 90건 중 실패 4건이며 **변경 전 기준선과 같은 4건**이다(stash 비교, 실패 목록 diff 동일).
- 실제 실행(`--mcp --window --no-audio`, 격리 `NETSTORM_USER_DATA`, 복사본 게임 폴더): 메인 → `Multiplayer` 클릭 → 새 Fort 섬 + `Player Name?` 입력창 + `Multiplayer Options` 패널(Save Game/Player Name/Server/Protocol/Connect/Cancel)이 원본 스크린샷(`screenShots/mainMenu - Multiplayer.png`)과 같은 구성으로 표시되고, ESC 뒤에도 종료 코드 0·로그 파일 미생성·게임 폴더 불변이다. 섬 월드에는 섬 1개와 TYPE 89·158 객체가 생성됐다(MCP 보고).
- **미확정:** 오프라인 로비 스크린샷에서 원본처럼 사제·거주 구역 스프라이트가 눈에 띄지 않았다(객체는 보고에 있음). 멀티플레이어 화면 충실도는 사용자 지시에 따라 직접 확인 대상이며 이 단위는 비종료·구성 일치까지만 확인했다.

## 5. 남은 일

- 로비 섬의 객체 표시 충실도(원본은 사제·거주 구역 표시)는 MP 단위 확인 때 대조한다.
- M6-n의 큰 AI 장기 소유자·일반 Descend·배치 호출자·Fort 상위 Finish 전수는 여전히 미감사다.
