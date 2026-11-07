# webpj 대본 CreateAt 생성 부모 이식 — WB04-E31

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 서른한 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawCreateAt`(10.78)을 옮겼다. [E30](webpj-add-random-destroy-at-20261108.md)의 `parseDestroyAtLocation`(같은 위치 해석 0044c540)을 그대로 쓰는 대본 생성 부모(0044c660, Damageable.cpp)이다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-create-at.js`

- `createObjectAt(source, hooks)`: 권한(`boss`)이 없으면 진단 803 → 권한 재조회(여전히 없으면 끝). 첫 NUL 앞 문자열(256바이트 이상이면 원문 지역 버퍼 범위라 `RangeError`)을 `parse`로 해석해 x·y·signed(type)이 **모두 양수**일 때만 진행한다. `create(type, 0)`으로 만든 현재 SID의 소유자는 해석된 owner가 0이면 원래 정수 좌표의 지면(`surface(x,y)`)이 있을 때만 그 슬롯 owner BYTE(+34)를 쓴다. 생성 SID가 0이면 원문의 NULL 역참조를 호스트에서 `CreateAtNullObject`로 명시 거부하되 선행 지면 조회는 보존한다. 이어 `owner(생성, owner)` → `hasHitPoints`이면 최대 HP를 `hitPoints`로 **직접** 쓰기(사제 가상 HP 전환 없음) → 방향형 TYPE이면 `0x15c + (반경/2, 0 쪽 절삭)×4`(DWORD 감김) 위치의 `frameField` 값을 공간 등록 전에 raw+24 직접 쓰기(`frame`) → `pop(생성, float32(x), float32(y), 0)`. 도달하지 않은 하위는 요구하지 않고 앞선 효과는 되돌리지 않는다.
- `createAtFrameField(frames, offset)`: 현재 TYPE의 복원된 `0x12c`(글자 종류 수)·`0x130..0x16f`(글자 A..P의 첫 프레임)·`0x170..0x1af`(개수) 물리 DWORD를 읽는다. `frames = { letterKinds, run(글자 코드) → {first, count} }`. 첫 프레임 -1은 DWORD 비트(`0xffffffff`)로 읽고, 이웃 미복원 필드/포인터(`0x124` 등)·4의 배수가 아닌 offset은 frame 0으로 합성하지 않고 `RangeError`로 거부한다.
- 변수명 `location`은 외부 접속 감사가 브라우저 전역으로 오인하므로 `spot`을 쓴다(E30에서 발견).

## 2. 검증 (`tests/create-at.test.mjs`, 3건)

- `create-at-parent-x86.tsv` 580행: 입력 JSON(`[문자열(NUL 포함), Boss, HP, 지면, owner 효과, NULL]`)을 `JSON.parse`로 읽어 실제 위치 해석기 + 명시 타입 조회(provider 76·sunCannon 131·special2 129·special3 140·special4 142)로 부모를 실행하고, 결과(`ok`/`null`)·호출 순서·**현재 50바이트 전체**를 원문 관측과 대조한다(float32 Pop 비트, 직접 HP 쓰기, 프레임 offset 포함).
- 계약: 권한 진단 한 번, 첫 NUL/256바이트 지역 버퍼, x≤0이면 생성 하위 비요구, 프레임 표 밖 offset(반경 44 → `0x1b4`)에서 거부되어도 앞선 `create`·`owner`·`hp` 효과 보존, owner 장치 누락 시 생성 뒤 그 위치에서 거부, `createAtFrameField`의 첫 프레임·개수·종류 읽기와 포인터 필드 거부.

최초 실패와 교정: 3건이 처음부터 통과했다.

## 3. 경계

- TYPE 프레임 표(`RiftTypeFrames`)의 실제 연결은 자산 적재 단위에서 대조한다. 웹 `TypeTable` 보기는 `letterKinds`와 글자별 `first`만 가지고 `count`가 없으므로, 연결할 때 `run(글자).count`(그 글자를 가진 프레임 수)를 추가해야 한다. 이 단위의 검사는 같은 인터페이스의 검사용 표를 쓴다.
- 실제 월드의 생성(`004af530`)·소유자 변경(가상 +74)·HP(`004adc60`/`004adcb0`/`004adca0`)·Pop(가상 +90)·지면 조회(`0040da00`)는 훅으로만 주입했다. 소비자(Damageable/월드 생성 단위)에서 연결한다. CD 10.72는 범위 밖이다.
- 일반 입력(`CreateAtInput`)·이름표(`create-at-names`)·`CampaignCreateAtTests.inc`는 해당 단위에서 대조한다.

## 4. 다음 시작점

대본 부모 `RawMakeAi`(`make-ai-parent-x86.tsv` 770행·`make-ai-objects-x86.tsv` 2592행, `MakeAiTests.cpp`)와 `CreateAtInput`·`DetonateInput`. 큰 묶음은 `RawGraph`/`RawGraphLive`, `SquidPostPop` 처리기 본체, `RawSquidPoolReset`, `RawBridgeConnect`, Player 나머지이다.
