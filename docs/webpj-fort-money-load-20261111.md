# webpj Fort Money 읽기와 Player SP 지정(RawFortMoneyLoad) — WB04-E278 (2026-11-11, 4-B)

3.1절 4-B 의 **RawFortMoneyLoad**(163줄)를 `core/raw-fort-money-load.js` 로 옮겼다. 패치 `004bf340`(wrapper) → `00483300`(본문)의 Fort `Money` 섹션 읽기와, 같은 파일에 있는 현재 Player SP 지정·조회 정책 `00447810` SetPlayerSpAmount · `0040ef30` ReadPlayerSpAmount 다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 기준선 뒤 변경 판정 (3.6절)

`git diff 1f00a25a HEAD -- cpppj/src/o/RawFortMoneyLoad.cpp` 는 4줄 변경(커밋 `2b048419 20261109-06 CP Native HP 보상 SP와 현재 Player AI 지갑 수명 연결`)이다. `reward_.fixedSpA`/`halfHealthMode`/`fixedSpB` 직접 읽기가 `FixedSpA()`/`HalfHealthMode()`/`FixedSpB()` 접근자로 바뀐 것뿐이며, 빌린(공유) 칸이면 소유자의 현재 값을 읽는 동기화다. 웹의 `SquidRewardState` 는 E166 에서 이미 같은 접근자(`currentFixedSpA()`·`currentFixedSpB()`·`currentHalfHealthMode()`)를 옮겼으므로 **① 자동 반영**이며 PORT_MAP base 는 `2b048419` 다. 헤더(`.h`)는 기준선 뒤 변경이 없다.

## 2. 형식과 규칙

- **ReadMoney(`00483300`):** 스트림 position 에 4 를 더하고(32비트 감김), 끝(`end`)과 **signed** 비교해 끝을 넘으면 진단(`StreamPastEnd`)을 알린 뒤 계속한다. fort 고정 SP(`currentFixedSpA`)면 현재 로컬 owner(BYTE)의 분산 저장소에 `194162.0f` 비트를 쓴다. 해체 절반 HP 모드이고 경제 차단(`005c85d4`)이 아니면 난이도(BYTE, 0..2)로 `1000/2500/3000` + `4000` 을 고르고(난이도 3 이상은 거부) owner 1..8 중 Player+`0x50` flags 에 `0x20` 비트가 없는 Player 마다 SetPlayerSp 를 호출한다. **저장된 금액 float 은 SP 로 대입하지 않는다.**
- **SetPlayerSp(`00447810`):** 클라이언트 활성이고 owner 가 현재 로컬 owner 와 같으면 편집기(`005c85a4`=`currentFixedSpB`)에 따라 `194162.0f` 또는 `float(amount)` 를 **다시 조회한 로컬 owner** 에 쓰고 SP 표시 갱신(`refreshSp`)을 부른다. 그 밖에는 signed owner 1~8 일 때만 같은 값을 쓰고(표시 갱신 없음) 나머지 owner 는 저장도 장치 조회도 하지 않는다. AI 지갑 가산이나 SP 차감용 `AddSp` 를 쓰지 않는다.
- **LoadSelected(`004bf340`):** 절반 HP 모드이고 경제 차단이 아니면 SP 검사 잔여 횟수(`005c89c4`)를 99 로 쓰고, 깊이를 올려 알린 뒤 Template 없음 진단(`MissingTemplate`, 진단 뒤 계속) → 실제 섹션 선택(selector, 위치 0 초기화) → ReadMoney 를 실행한다. 선택·본문이 예외를 내도 깊이를 되돌려 알린 뒤 다시 던진다.
- **ReadPlayerSpAmount(`0040ef30`):** 활성 로컬이고 owner 가 현재 로컬 owner 와 같으면 로컬 owner 를 **다시 조회**해 그 잔액을, 그 밖에는 signed 1~8 일 때만 분산 잔액을 float 로 읽고 아니면 0 이다. 비활성/비로컬 0 은 lazy 저장소를 초기화하지 않는다.
- 현재 모드/owner/편집기/활성 여부는 모두 호출 위치에서 다시 읽는다. 필요한 조회·표시 장치가 없으면 그 호출 위치에서만 거부한다(`need`). 생성 때는 Player 기록 크기(9×0xb4)·세 효과 훅·분산 저장소·보상 접근자를 확인한다.
- **CD 10.72 분기**(`cdLocalSp`·`cdStartingSp`·stride 0xac)는 웹 목표(10.78)가 아니라 옮기지 않았다. 웹 모듈은 판본 인자를 받지 않는다(10.78 전용 선례).

## 3. 검사 (`tests/fort-money-load.test.mjs` 6건)

- `fortmoney-x86.tsv` 의 originals **244행 전부**(CD 164행은 범위 밖): wrapper(`Load`)·직접 SP 지정(`SetPlayerSp`)·본문(`ReadMoney`) 세 진입점을 같은 입력(모드·owner·난이도·flags 마스크·seed/fill·초기화 여부·스트림 pos/end)으로 실행해 깊이·pos·SP 검사 잔여 횟수·분산 저장소 초기화/offset·게임 난수 상태·owner 0..8 SP 비트·전체 288 DWORD 분산 배열·효과 순서(진단 `A:T`/`A:P`, 표시 `U`)·진단 횟수를 대조하고 Player 기록이 변하지 않음을 확인했다. **첫 실행에 모두 통과했다.**
- 손 시험: 로컬 owner/special 모드를 호출마다 다시 읽고 같은 저장소·난수를 쓴다(cpppj `SharesCurrentWorldOwnerStoreAndRandom` 의 장부·저장소 부분), 깊이 증가 → Template 진단 → 선택 → 깊이 복구와 선택 실패/누락 복구(`SelectsAfterDepthAndTemplateDiagnostic`), SP 읽기의 lazy/로컬 재조회(ForeignRitualRewardTests 의 getter 계약).
- 변형 확인 32개 중 처음에 4개가 살아남았다. 3개는 계약 검사를 보강해 잡았다: 난이도 1(2500) 값, 루프가 owner 1 부터이며 활성 로컬 owner 0 은 지정하지 않음, 활성 로컬 분기가 저장 직전 로컬 owner 를 다시 조회함. 마지막 하나 `Math.fround(amount|0)` 의 `Math.fround` 제거는 `Float32Array` 쓰기가 같은 반올림을 하는 **동치 변형**이다(결과가 같은 중복 방어, 의도적으로 유지).
- 계약 검사(fixture 가 구분하지 못하는 경계): pos 32비트 감김, 끝과의 signed 비교(`0x7ffffffe`+4 는 음수, end `-1` 은 진단), 범위 밖 owner(0·9·-1)는 저장도 장치 조회도 하지 않음, 범위 안 owner 에서 편집기 장치가 없으면 거부, int32→float 변환(2^24+1 반올림·음수·`0xffffffff`=-1), 활성 로컬 분기의 표시 장치 누락 거부, 난이도 3 이상 거부, 생성 때 기록 크기/저장소/훅 누락 거부, `playerFlags(9)` 거부.

## 4. 남은 경계

- cpppj 손 시험 `SharesCurrentWorldOwnerStoreAndRandom` 은 실제 `RawSurfaceWorld` 의 장부(`Core().bookkeeping`)와 `SetLoadingDepth` 에 연결한다. 웹은 아직 `RawSurfaceWorld` 가 없어(3.1절 4-B 의 RawSurfaceWorld 항목) 독립 `SquidPostPopState` 와 깊이 상자로 같은 단언을 옮겼다. **RawSurfaceWorld 이식 때 같은 월드의 장부·깊이에 연결한 장면으로 다시 닫는다.**
- 실제 DataManager 의 현재 Money 섹션 선택(`selector`)과 Template 존재(`templatePresent`) 연결은 DataManager 이미지/선택 항목(RawDataManagerImage·RawDataManagerSelection)과 Template 항목에서 한다.
- 이번에 옮긴 `readPlayerSpAmount`/`setPlayerSpAmount` 로 7-P3 `RawForeignRitualReward` 의 SP 모드 180행(분산 SP 저장소·정책 대조)을 열 수 있게 됐다. 해당 단위에서 소비한다. 그 밖의 소비자는 cpppj 의 `ClientStateWorldCleanup`(resetOtherPlayer)·`RawCampaignSessionLogin`(4-C/4-D)이다.
- 3.1절 4-B 는 7/30 이다. 다음은 **RawFortObjectLoad**(partial 283줄)이다.
