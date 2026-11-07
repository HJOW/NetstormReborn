# webpj 너겟 PostPop·Gunanim 방향 계산 이식 — WB04-E16

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 열여섯 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawNuggetPostPop`과 `o/RawGunAngle.h`(모두 10.78, 독립 원본 관측 보유)를 옮겼다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

### `www/core/raw-nugget-post-pop.js` (00484f90)

`RawNuggetPostPop(pool, hooks).postPop(sid, flags)`: flags bit0일 때만 최초 예약을 한다. ① 부모에 이벤트 1의 Regular가 아직 없으면 28 BYTE 할당이 성공했을 때 난수 `random(0, 0.3f)`를 소비해 `construct(sid, 1, payload)`, ② 그 결과와 무관하게 다시 할당이 성공하면 `random(0, 0.5f)`를 소비해 `construct(sid, 3, payload)`. 할당 실패는 난수를 소비하지 않고 뒤의 예약/기본 처리를 막지 않는다. 마지막에 항상 기본 Damageable 몸체(`damageable(sid, flags)`, 44bf80)를 호출한다. 그림 미리보기(flags 4)는 예약/난수 장치를 요구하지 않는다. 난수 반환값은 float 그대로 `construct`에 전달한다.

### `www/core/raw-gun-angle.js` (00467610)

- `patchGunFrameIndex(frame, checking, assertion)`: 프레임을 5로 나눈 signed 정수(0 쪽 절삭, `-0` 정규화)를 인덱스로 하고 18을 넘으면 19. 현재 checking이 비0이고 인덱스가 0..19 밖이면 원문 진단(`0 <= index && index < 20`, `\Ns\O\Gunanim.cpp`, 0x387). 진단이 복귀해도 지역 인덱스는 바꾸지 않는다(음수 반환은 표 역참조의 허가가 아님).
- `patchGunGraphFromFrame`: 같은 인덱스로 실제 20개 binary32 각도(DAT00540940)를 읽어 방향 0..3을 돌려준다. 음수 인덱스의 표 밖 무효 메모리 읽기 경로는 방향 0으로 가장하지 않고 `RangeError`로 거부한다.

## 2. 검증 (`tests/nugget-gun.test.mjs`, 6건)

| 검사 | 근거 |
| --- | --- |
| 너겟 288행(조회·할당·난수 소비·두 예약·기본 호출, float 비트 인자·NaN 난수 입력 포함, 두 정밀도 열 동일성) + 미리보기/판본 계약 | `nugget-postpop-x86.tsv` |
| Gunanim 방향 518행(음수 -4..-1, 0..512, INT_MAX) + 음수 인덱스 거부 | `gun-angle-x86.tsv` |
| 인덱스 진단 앞단 384행(복귀·중단·원래 인자 변경 시나리오) + 현재 checking/음수 읽기 경계 | `gun-frame-checking-x86.tsv` |

최초 실패와 교정: Gunanim 인덱스 검사 첫 실행에서 `Math.trunc(-4/5)`가 `-0`을 돌려줘 원본의 int32 나눗셈 결과 `0`과 엄격 비교가 어긋났다. `((frame | 0) / 5) | 0`으로 고쳐 통과했다(다른 모듈의 `| 0` 정규화는 영향 없음 확인). 나머지는 처음부터 통과했다.

## 3. 경계

- 너겟의 `construct`(496e80 Regular 생성/부착)·`allocate`(new28)·`random`(455930)·`damageable`(44bf80)은 호스트가 hooks로 공급한다. 실제 `Process` Regular 호스트와의 연결은 Host/World 조립 단위에서 대조한다.
- CD 10.72 경로와 `CurrentTypeHitPoints`는 범위 밖이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`, 공통 `SquidPostPop` 본체, `RawSquidPoolReset`, `RawBridgeConnect`, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
