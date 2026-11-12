# webpj 썬 디스크 Regular RawDiscRegular (WB04-E364, 2026-11-13, 7-P2 총·투사체 부품 마감)

3.1절 7-P2 마지막 항목 "RawDiscRegular 70줄(+29)"을 `core/raw-disc-regular.js` 로 옮겼다. 썬 디스크(TYPE 83, 가상 표 `0x50b570`)의 가상 Handle(00469540)로, 글자별 프레임 구간을 쓰는 충돌 효과(`RawGunImpact`)와 달리 TYPE 의 **방향당 프레임 수로 modulo 하며 프레임을 되풀이**한다. 기준선 뒤 `20261109-99 CP`(`NextFrameNative`·`HandleIn` 으로 정수 산술 분리)를 반영했다(PORT_MAP base `f2f9e837`). 이로써 **7-P2 총·투사체 부품 6개가 모두 끝났다**(RawGunProcess E359·RawGunTarget E360·RawMissile E361·RawProjectileGeometry E362·RawGunImpact E363·RawDiscRegular E364).

## 1. 규칙

- **구성:** `new RawDiscRegular(pool, types, setter)` — 풀은 PATCH, `types[n] = { directions(TYPE+110 방향 수), frameCount(TYPE+114 물리 프레임 수) }` 188칸(`readRawArcherTypeData` 결과와 같은 모양), `setter` 는 같은 풀의 `SquidFrame`. 풀이 다른 지정기·길이가 188이 아닌 표는 거부한다.
- **`nextFrame(current, frameCount, directions)`:** 현재 프레임이 음수이거나 int32 최댓값이거나 방향 수가 1 미만이거나 프레임 수가 방향 수보다 작으면 `RangeError`, 그 밖에는 `nextFrameNative` 이다.
- **`nextFrameNative(current, frameCount, directions)`:** 원문의 signed IDIV 세 번과 DWORD 감김을 그대로 한다. 방향당 프레임 `stride = trunc(프레임 수 ÷ 방향 수)`, `다음 = (현재 + 1)`(int32 감김), 결과 = `(다음 % stride) − (현재 % stride) + 현재`(int32 감김). 나머지의 부호는 피제수를 따르고(−0 은 0), 0 분모와 `INT_MIN ÷ −1` 은 하드웨어 나눗셈 오류라 `RangeError('Disc 원본 IDIV 오류')` 이다(몫이 0 이 되는 프레임 수 < 방향 수 도 `% 0` 에서 같은 오류).
- **`handles(sid)` / `handle(sid, event, count, payload)`:** 현재 raw 가상 표가 `0x50b570` 일 때만 참(바로 뒤 바이트와 타입 번호는 보지 않음)이고, 아니면 `handle` 은 `TypeError` 이다. `event`/`count` 는 원본도 쓰지 않는다. 프레임을 진행한 뒤 payload 를 비트 그대로 돌려준다.
- **`handleIn(pool, setter, type, sid, payload, nativeArithmetic = false)`:** 같은 본문의 정적 함수로 현재 TYPE getter `type(번호)` 가 없으면 거부한다. 슬롯은 살아 있는 보기로 읽어 getter 가 슬롯의 프레임을 바꾸면 그 값에서 계산한다. 프레임 DWORD(+36)를 signed 로 읽어 `setter.set(sid, 다음, 0)`. Native 장기 소비자는 `nativeArithmetic = true` 로 범위 검사 없이 원래 정수 산술을 쓴다.
- **`handler(fallback)`:** 썬 디스크의 가상 Handle 만 선택하고 다른 부모는 같은 입력으로 기존 처리기에 넘기며, 처리기가 없으면 거부한다.

## 2. 웹 범위

- 10.72(CD)의 BYTE 프레임은 범위 밖이다. 앱 수명 연결(Regular 등록과 `host.setRegularHandler` 합성, Native 장기 소비자 연결)은 후속 단계 몫이다. 이 모듈은 DOM 을 쓰지 않는다.

## 3. 검사 (`tests/disc-regular.test.mjs` 7건)

- **원본 독립 관측 1:** `disc-regular-x86.tsv` 의 10.78(originals) 96행 — 현재 프레임·프레임 수·방향 수·event/count·payload 비트 입력에 대한 다음 프레임(검사 산술과 `handle` 의 Frame.Set 결과), 반환 payload 비트, 표시 갱신 두 번이 **첫 실행에서 전부** 일치했다(cpppj `DiscRegular_PatchNative`).
- **원본 독립 관측 2:** `disc-regular-boundary-1219-x86.tsv` 70행 — 정상 47행의 `nextFrameNative`(INT_MIN·INT_MAX 부근의 DWORD 감김, 음수 방향 수, 음수 프레임 포함)와 하드웨어 나눗셈 오류 23행(0 분모·`INT_MIN ÷ −1`·몫 0 의 `% 0`)의 거부가 첫 실행에서 일치했다(cpppj `ClientFrameRegular_DiscNativeIntegerBoundaryGolden`).
- **웹 계약 5건:** 생성자 검사, `nextFrame` 의 네 갈래 거부와 경계 허용(프레임 수 = 방향 수·INT_MAX−1 까지), `nextFrameNative` 의 손 계산한 부호/감김 결과, handle/handles/handler(event·count 무관·payload −0 비트·가상 표 불일치 시 슬롯 불변·바로 뒤 바이트 무관·표 밖 TYPE 188·자료 없는 TYPE 187), `handleIn`(getter 먼저·슬롯 살아 있는 읽기·getter 없음·검사/Native 산술 선택).
- 시험이 먼저 실패한 곳은 시험 쪽 한 건뿐이었다(다른 풀의 지정기를 만들 때 훅을 비워 둔 입력).

## 4. 변형 확인과 남은 경계

- 연산자·상수 변형 42 + 의미 변형 43 = 85회를 적용했다. 1차에서 4개가 남았고 셋은 **시험의 공백**(가상 표를 읽는 DWORD 루프가 네 바이트를 넘어 읽는 변형, 정확히 INT_MIN 에서의 DWORD 감김)이라 보강해 잡았다.
- 남은 동등 변형 1개: 나머지의 `| 0` 제거(−0 이 나올 수 있으나 곧바로 `(… + 현재) | 0` 에서 0 으로 정규화되고 결과는 정수 덧셈뿐).
- 7-P2 가 끝났으므로 다음 후보는 **7-P3 신전·제단·의식 부품**(RawTempleRitual 266줄 +18 외 7개)이다.
