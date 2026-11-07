# webpj 주문 활성화 수신·공유 바람 이식 — WB04-E17

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 열일곱 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawBombActivation`(10.78)과 `o/RawAmbientWind`(10.78 경로)를 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

### `www/core/raw-bomb-activation.js` (0x5c 수신, 41e860)

`RawBombActivation(pool, types, localOwner(), hooks).receive(packet)`: 6 BYTE 패킷의 부모 WORD(+4)가 가리키는 주문 객체가 살아 있으면(상태 bit0/1 없음) ① 소유자(+34)가 현재 로컬 owner와 다르면 현재 TYPE의 복호화 비용 단위를 32비트 부호 반전한 float로 `subtract(owner, amount)`(원격 SP 증감), ② 효과 뒤 현재 TYPE의 genus를 다시 읽어 주문 비트(flags2 0x100)가 없으면 원문 진단(`assertion(0x206)`, 복귀 가능), ③ 40 BYTE 할당이 성공하면 `construct(0, parent, 0.1f)`. 패킷의 수신자/연결 인자는 읽지 않고, 짧은 패킷과 선택 하위 누락은 거부한다.

### `www/core/raw-ambient-wind.js` (004dd2b0)

`createAmbientWindState()`(X 0·Y 1·각도 0), `stepAmbientWind(state, random, diagnostics)`: 매 프레임 X/Y 바람에 double 0.9를 곱해 float 저장(감쇠) → `random.next(100)`이 10 이상이면 끝 → 아니면 `next(65536)`으로 잡음(−0.06..0.06, `value × (1/65536) × 폭 + 최소`)을 X 바람에 더해 float 저장 → `atan2(−Y, X) × 57.295827908797776`을 float 저장 → `normalizeBoardAngle`(E15, 진단 포함) 결과를 각도에 float 저장. 나머지 프레임에서는 각도와 두 번째 난수를 보존한다. 기존 `GameRandom`을 그대로 빌린다.

## 2. 검증 (`tests/bomb-wind.test.mjs`, 4건)

| 검사 | 근거 |
| --- | --- |
| 주문 활성화 434행(원격 비용·현재 genus 진단·할당·예약 호출 인자와 부모 50 BYTE, 두 정밀도 열 동일성) + 패킷 길이/dead/하위 누락 경계 | `bomb-activation-x86.tsv` |
| 공유 바람 originals 960행(장기 감쇠·난수 소비·각도 정규화 시퀀스의 세 float 비트·게임 난수 상태·checking·진단; `originalCD` 960행 제외) + 최초 상태/감쇠 | `ambient-wind-hardware-x86.tsv` |

최초 실패와 교정: 4건이 처음부터 통과했다(JS `Math.atan2`의 결과가 float 저장 경계에서 원본 x87 결과와 960행 모두 일치). 기대값은 원문 열만 썼다.

## 3. 경계

- cpppj `AmbientWind_StoragePreservesStateAcrossWorldLeases`(실제 Client Storage·난수 owner가 World 대여/반납에서 바람과 난수를 재생성하지 않음)는 World/Storage 조립 단위에서 대조한다.
- 주문 활성화의 경제(`41df80`)·할당(`new40`)·Regular 생성(`496f00`)은 호스트가 hooks로 공급한다.
- CD 10.72 경로(잡음 곱셈 순서·중간 저장이 다름)는 범위 밖이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, 공통 `SquidPostPop` 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
