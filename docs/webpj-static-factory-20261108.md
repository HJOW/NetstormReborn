# webpj 임시 문자열 고리·Factory 가상 입력 이식 — WB04-E18

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 열여덟 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawStaticStrings`(Staticstring.cpp 외곽)와 `o/RawFactoryInput`(10.78)을 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

### `www/core/raw-static-strings.js` (004b92b0·004b9320·004b9390)

같은 네 DWORD(`size`·`base`·`current`·`last`, 005c8940/44/48/4c)를 빌려 실행한다.

- `initializeStaticStrings(size)`: 이미 base가 있으면 원문 진단(줄 17) → size 쓰기 → 새 할당 → 복귀 뒤 현재 size를 다시 읽어 base에 할당 주소를 쓰고 DWORD(`size>>2`번)와 나머지 BYTE(`size&3`번)로 0을 채움 → current=base에 `0xd5` 표식 → current+1. `last`는 쓰지 않는다.
- `reserveStaticString(size)`: base가 없으면 8192로 먼저 초기화 → `end=size+base`(32비트 감김), `start=current`; `start+size+1`이 end 이상(unsigned)이면 current=base에 표식, `start=current+1` → last=start, current=start+size에 표식 후 +1 → last 반환. 예약 폭은 출력 절단 한도가 아니다.
- `formatStaticString(size, pattern)`: 예약 결과가 서식 주소와 같으면 CRT 서식을 부르지 않고, 다르면 `format(결과, 서식)`을 호출한다. CRT가 last/current를 바꿔도 캡처한 반환 주소를 유지한다.
- 주소는 장치 토큰(32비트 수)이며 각 하위 호출 뒤 원문 위치에서 전역을 다시 읽고 한 store 이전의 부분 효과를 보존한다.

### `www/core/raw-factory-input.js` (004547d0)

`RawFactoryInput(pool, state, hooks).input(parent, code, packedArgument)`: 명령을 상위 6개 수정 비트를 제외한(`0x03ffffff`) 값으로 현재 명령 전역과 비교한다. 순환(0x03000063)은 `(level+1)%3`(signed)을 저장하고 프레임을 보정해 1, 업그레이드(0x03000032)는 비용을 float로 줄여 현재 로컬 SP와 비교(`!(비용<=SP)`, NaN은 자금 부족; 편집기가 아니면 `upgradeNoBucks` 안내) → 진행 중이면 `alreadyUpgrading` → 아니면 실제 요청, 연구(0x03000080)는 연구 입력에, 나머지는 공통 부모(004ae010)에 수정키 포함 원문을 위임한다. 임의 소유자/상한 관문을 추가하지 않는다.

## 2. 검증 (`tests/static-factory.test.mjs`, 6건)

| 검사 | 근거 |
| --- | --- |
| 임시 문자열 204행(두 8192 BYTE 영역 전체, 네 전역, normal/stop/fault, 하위 호출 순서) | `static-string-runtime-x86.tsv`(판본 열 없는 10.78 전용 표) |
| Factory 입력 5건(자금 경계·자금 부족 우선·float 축소/NaN/무한대/음수·편집기 SP 선조회·순환/연구/부모 위임) | (독립 입력, cpppj 계약 4건 + 위임 보강) |

최초 실패와 교정: 6건이 처음부터 통과했다(소스는 파일 쓰기 도구로 저장). 기대값은 원문 열만 썼다.

## 3. 경계

- cpppj의 `ClientStaticStrings`(공통 10240과 lazy 8192, 실제 번역 장치와 일곱 Computer 이름 결과 주소 유지)·`Client` 통합 검사는 해당 Client 단위를 옮길 때 대조한다.
- Factory 입력의 가상 레벨 getter/setter·프레임 보정·SP 조회·예약 요청·안내·연구 입력은 호스트가 hooks로 공급한다.
- CD 10.72 경로는 범위 밖이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, 공통 `SquidPostPop` 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
