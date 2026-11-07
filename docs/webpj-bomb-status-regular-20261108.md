# webpj Bomb 상태 주문·Regular(RawBombStatus·RawBombRegular) 이식 — WB04-E53

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 쉰세 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawBombStatus`(10.78; 원본 41eca0/41fdb0 대상 자격, 41fc50/41fdd0 은신·마비 적용)와 `o/RawBombRegular`(원본 41e6a0 주문 Regular 본체)를 옮겼다. 상태 주문의 송신 observer는 [E52](webpj-bomb-effect-timer-20261108.md)의 `finalizeBombTimerPacket`을 그대로 쓴다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-bomb-status.js` — `RawBombStatus`

- `new RawBombStatus(pool, types, state, hooks)`: 10.78 풀·188 타입. `state = {altar 164, outpost 126, priest 158, fence 136, flare 106, alliances(81)}`는 외부 효과 뒤에도 현재 값을 다시 읽는다. 훅 `{refresh, unpop, repop, fence, flare}`는 선택된 지점에서만 요구한다(없으면 `TypeError`).
  - `mayTarget(parent, target)`: 부모 가상 표 502510(은신 41eca0)이면 대상 genus `0x1570004` 중 하나 또는 대상 TYPE이 제단/전초일 때 참, 502ff8(마비 41fdb0)이면 genus `0x170000`이 있을 때 참. Wind의 502e68/502f30은 자격 판정에 쓰지 않고 거부한다.
  - `apply(parent, target)`: 표 502510/502e68/502f30은 은신(41fc50), 502ff8은 마비(41fdd0). 대상이 부모의 `+12` WORD(운반체)와 같으면 아무것도 하지 않고, 마비에서 대상 TYPE이 사제이고 `동맹[부모 owner × 9 + 대상 owner]`가 참이면 면제한다. free/dead를 보지 않고 extra에 `0x10`/`0x20`을 직접 쓴다. 은신: `refresh` → 대상 genus `0x1000004`이면 `flare(대상, 1, 1, 플레어, 2)`. 마비: genus `0x200000`이면 `unpop(0)`+`repop(0x40)`, 아니면 `refresh`; 현재 TYPE이 울타리(136)면 `fence`; 끝에 `flare(대상, 1, 3, 플레어, 3)`. 효과 뒤 바뀐 TYPE/전역/동맹을 원본 위치에서 다시 읽는다.

`www/core/raw-bomb-regular.js` — `RawBombRegular`

- `new RawBombRegular(pool, types, state, hooks)`: `types`는 188개 `{firstFrame(TYPE+130), castTime(TYPE+1b8, signed)}`, `state = {boss, localOwner, effect: {message, protocol, length}}`는 현재 값. 훅은 생성 시점의 사본을 쓴다(원본 C++ 복사 의미). `pool` getter는 같은 세션 확인용.
  - `handle(parent, event, count, payload)`: event 번호는 읽지 않는다. count 0이면 운반체(`+12`)가 있을 때 `notice(운반체, 6, 시전시간×2)`, 이어 **현재 TYPE**의 첫 프레임으로 `frame(부모, 첫 프레임, 0)`; 아니면 `animate(부모, 1, 0)`. 권한이 없으면 payload를 돌려준다. 있으면 `fround(signed count × payload)`가 현재 TYPE의 시전 시간보다 클 때만 `allocate(400)` → `targets`(최대 100) → 210바이트(0xff 채움) 패킷 `[protocol&0x7f, message, 선언 길이(≥4) WORD, 부모 WORD, 대상 수 DWORD, 대상 WORD 목록]` → `broadcast(패킷)` → 현재 owner로 `receive(owner, 0, 패킷)` → `release(400)`. 반환은 수신 뒤(해제 전)의 부모 상태 비트 7이 있으면 −1, 아니면 0(해제 훅이 상태를 바꿔도 이미 캡처한 값). 확보 실패는 시전 성공으로 세지 않고 오류로 거부하며, 검색~수신 중 예외에도 `release`를 부른다.

## 2. 검증

`tests/bomb-status.test.mjs`(2건): 원본 `bomb-status-x86.tsv` 612행(I 267·P 267 적용, i 35·p 35 자격, H 8 송신 observer). 자격/면제/운반체/상태 비트 쓰기/TYPE·전역·상태를 바꾸는 mutation 1~3 뒤의 효과 문자열, 부모+대상 raw 100바이트, 32바이트 패킷까지 비교. 계약: 구성 오류, 알 수 없는 표/Wind 표의 자격 거부, 운반체 대상의 무효과, 상태 비트를 쓴 뒤 훅 누락 거부, 동맹 표 크기 오류.

`tests/bomb-regular.test.mjs`(2건): 원본 `bomb-regular-x86.tsv` 741행(13개 DWORD 입력: 이벤트·count·payload·시전 시간·운반체·권한·상태·대상 수·변형 1~8·헤더·owner·첫 프레임). 반환 float 비트, 호출 순서와 인자, 배포/수신 패킷 hex, 부모 raw 전체를 비교하고 두 x87 정밀도 관측이 같은 행만 사용한다. 계약: 비권한 초기화가 프레임 이외 훅을 쓰지 않음, 확보 실패 거부, 대상 검색 누락·100개 초과에서도 해제, 구성 오류, 생성 시점 훅 사본.

최초 실패와 교정: 두 모듈 모두 1353행 전부를 첫 실행에 통과했다.

## 3. 경계

- 실제 가상 호출(88 표시 갱신·48/4c 재배치)·썬 바리케이드 정리(457e20→4578f0)·플레어(45a480)·QA 통지(45a6c0)·프레임 설정(4acee0/4afc90)·대상 검색(41e4b0)·송신(4e2e30)·로컬 수신(420bf0)은 월드 조립에서 공급한다. 대상에 대한 피해·내용물 소비는 별도 수신 몸체(`RawBombEffect`/`RawBombBlast`)이며 이 처리기는 Regular 순서만 다룬다.
- `bomb-status-flare-x86.tsv`는 cpppj `FlareTests`가 읽는 플레어 단위 소관이다.

## 4. 다음 시작점

Bomb 나머지(`RawBombBlast`·`RawBombTargets`·`RawBombEffect`), Player 나머지, 종류별 PostPop 본체, `SquidDisplay`/`SquidFrame`이다.
