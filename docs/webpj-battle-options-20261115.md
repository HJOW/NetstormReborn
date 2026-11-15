# 전투 옵션 슬롯·자원 옵션·보충 오벨리스크 배치·71h 설명 수신 이식 (WB04-E558)

`o/` 아래 다섯 소단위를 한 묶음으로 판정했다. 모두 원본 관측 fixture 가 없고 cpppj 에도 전용 시험이 없어(상위 클래스 시험이 간접으로 쓸 뿐이다), cpppj 소스 계약(원문 주소·필드 폭·순서·경계)을 웹 시험이 직접 단언한다.
기준선 `225553df` 과 현재 cpppj 소스가 같음을 확인했다(기준선 뒤 변경 없음).

| 단위 | 웹 모듈 | 원문 | 시험 |
| --- | --- | --- | --- |
| `RawBattleOptionSlots` | `www/core/raw-battle-option-slots.js` | 0041c4f0·0041c520·0041c550·0041c580·0041c5b0·0041c6a0 | 7건 |
| `RawBattleResourceOptions` | 같은 모듈 | 0041c800·0041c840·0041c870 | 3건 |
| `RawBattleNuggetPlacement` | `www/core/raw-battle-nugget-placement.js` | 0040e050 | 7건 |
| `RawStateMetaBattleDescription` | `www/core/raw-state-meta-battle-description.js` | 0040b0d0 등록·004b49c0 수신 | 9건 |
| `RawStateBattleOptions` | `www/core/raw-mission-client-packets.js`(E314) | 004b4900 | 기존 + 1건 |

## 옮긴 범위

- **슬롯(`raw-battle-option-slots.js`)**: 87바이트 전투 옵션의 슬롯 8개에 대해 색(+0x47, 증가는 BYTE 증가를 먼저 저장하고 7 초과일 때만 0, 감소는 BYTE 감소를 먼저 저장하고 0xff 일 때만 7)·랭크(+0x4f, 증가는 zero 확장 값+1 이 23 초과면 0, 감소는 음수일 때만 23)·랭크 24칸 표 조회·특수(+0x3f, 정확히 1 이면 0 아니면 1, XOR 아님). 유효 슬롯(signed 0..7) 관문이 옵션 물리 검사보다 앞서 무효 슬롯은 읽기/쓰기 없이 0 이다. 필드가 옵션 밖이면 쓰기·읽기 전에 `RangeError`.
- **자원 옵션(같은 모듈)**: +0x14 SP 지급 간격(0/30/60/120/300/600초)·+0x15 지급액(500/1000/2500/5000)·+0x13 간헐천 간격(0/30/60/120/300초). 옵션이 87바이트가 아니거나 선택 BYTE 가 표 밖이면 `RangeError`(Native 의 범위 밖 스택 읽기 대신), 호출 때마다 현재 옵션을 읽는다.
- **보충 오벨리스크(`raw-battle-nugget-placement.js`)**: `placeBattleNuggets(rectangle, quantity, state, hooks)` — single 이면 진단 → 지역 난수(`GameRandom(0)`, 공유 게임 난수와 별개) → boss 면 실제 wall seed(`RawFortFinishOwner.timeSeed`), 비boss 면 지역 0 의 6바이트 기록 byte3×103 → 실제 배치 장치를 먼저 고르고 현재 TYPE 을 읽은 뒤 지역 -1·force 1·float 값 전달 사각형으로 배치. 읽기 전용 전역은 상자·함수·단순 값을 받아 원문 위치마다 다시 읽고, 선택되지 않은 경로의 장치는 요구하지 않는다.
- **71h 전투 설명(`raw-state-meta-battle-description.js`)**: 송신 observer(`packet[9]==0xff` 면 복귀형 진단 뒤 현재 BYTE 로 길이 WORD=`packet[9]+10`, 수신 방향은 무동작)·버전 1/정책 0·가변 길이·CS 비영일 때만 자식 수신(mode/sender/known 관문 없음). 같은 분배 표에 Native 연결 수신(004b49c0)의 `canonicalReceive` 가 있으면 재등록하고, 이 웹 분배기에는 그 등록이 없어 독립 본문을 쓴다.
- **67h 옵션 부모(`RawStateBattleOptions`)**: 004b4900 본문은 E314 가 `receiveStateBattleOptions` 로 이미 옮겼다(일반/메타 두 수신이 같은 부모). 이번에 이 단위로 대응표를 판정하고 변형 확인을 다시 해 미관측 경계를 시험으로 보강했다.

## 시험과 변형 확인

- `tests/raw-battle-option-slots.test.mjs` 10건: 색·랭크·특수의 모든 순환 경계, 유효 슬롯 관문(옵션이 비어 있어도 0), 필드별 물리 경계(정확한 길이 통과/한 바이트 모자라면 거부), 자원 표 전체와 87바이트·표 범위 오류.
- `tests/raw-battle-nugget-placement.test.mjs` 7건: seed 두 경로, 호출 순서(single 진단 뒤 boss 재읽기·타입 읽기 시점), 장치 누락(미선택 경로는 불요구), 6바이트 기록, unsigned 변환과 float 사각형 사본.
- `tests/raw-state-meta-battle-description.test.mjs` 9건: 실제 `RawPacketDispatch` 에 등록한 버전/길이/관찰기/수신기, sentinel 진단 뒤 현재 BYTE 재조회, 수신 방향 무동작, 패킷 물리 범위, CS 관문(상자·함수·단순 값), 다른 버전도 수신, canonical 재등록.
- `tests/raw-mission-client-packets.test.mjs` 1건 추가: 비용의 signed float 저장·큰 정수 반올림·환불 비율 unsigned·진단/재접속 장치 필수 경계.

**변형 확인:** 슬롯/자원 48종·오벨리스크 28종·71h 설명 32종·67h 부모 21종, 총 129가지를 넣어 동등 4가지를 뺀 모두 시험에서 잡혔다(처음 살아남은 것 중 동등이 아닌 uint8 폭 마스크·사각형 float 반올림·67h 부모 4가지는 시험을 보강해 잡았고, 간헐천 표는 변형 위치 지정을 고쳐 다시 확인했다).
동등 변형 4가지: 색 증가의 `& 0xff`·랭크 증가의 `& 0xff`(둘 다 `Uint8Array` 저장 절삭과 같다), 지역 난수 초기값(바로 `setState` 로 덮어쓴다), 71h 길이 WORD 쓰기의 `packet.length < 4` 검사(설명 BYTE 읽기가 먼저 10바이트 이상을 요구해 도달하지 못하는 방어 검사).

## 남은 경계

- 실제 ChunkMap 지역 0 기록과 실제 임의 배치(0040ded0)의 Session 연결, 메타(MP) 71h 수신 자식(0042cad0)과 CS 소유자는 세션 단계에서 한다. 웹 분배기에는 `canonicalReceive` 등록이 없으므로 Native 수신 대여 분기는 장치 누락 오류만 시험했다.
