# 기술 타입 69h 패킷과 전투 자원 프레임 이식 (WB04-E560)

`o/` 아래 두 소단위를 옮겼다. 둘 다 원본 관측 fixture 가 없고 cpppj 에도 전용 시험이 없어, cpppj 소스 계약(원문 주소·필드 폭·순서·경계)을 웹 시험이 직접 단언한다.
기준선 `225553df` 과 현재 cpppj 소스가 같음을 확인했다(기준선 뒤 변경 없음).

| 단위 | 웹 모듈 | 원문 | 시험 |
| --- | --- | --- | --- |
| `RawTechnologyTypesPacket` | `www/core/raw-technology-types-packet.js` | 0041c340(누적)·004b49e0(수신)·0040af10(송신 관찰기) | 9건 |
| `RawBattleResourceFrame` | `www/core/raw-battle-resource-frame.js` | 00486bc0 | 17건 |

## 옮긴 범위

- **기술 타입 69h(`raw-technology-types-packet.js`)**: `addTechnologyTypes`(입력 BYTE 마다 현재 목록의 첫 일치 또는 첫 빈칸에 합침, 별도 개수/정렬/종료 NUL 쓰기 없음, 0x80..0xff 도 그대로 비교, 앞선 부분 쓰기는 되돌리지 않음)·`receiveTechnologyTypesPacket`(길이 BYTE 를 읽지 않고 payload+5 를 합집합한 뒤 기존/신규 전체 목록을 첫 NUL 까지 순서대로 표시하며 표시 효과가 바꾼 다음 BYTE 를 다시 읽음)·
  `observeTechnologyTypesPacket`(송신에서만 길이 BYTE 가 0xff 이면 진단 복귀 뒤 현재 BYTE+5 를 하위 WORD 로 씀)·`registerTechnologyTypesPacket`(방향 전용 관찰기·버전 1/정책 0·가변 길이·`registerDirect` 로 NULL 연결 직접 호출과 분배가 같은 본문을 쓰는 등록, 목록은 256바이트여야 함).
- **전투 자원 프레임(`raw-battle-resource-frame.js`)**: 로그인 중 반환 → 스펠 만료(개수 signed 양수·기한 있음·시각 도달 시 보스만 보충, 개수 감산 뒤 보충 뒤 시각+20초 기한) → SP 주기 만료(간헐천 프레임 플래그가 없고 기한 도달: 비전투면 최초 표시를 0 으로, 보스·전투·전원 로그인·주기 비영이면 기한을 옮기고 최초는 1 만 쓰고 이후 숨김 아닌 구독자에게 지급) →
  일반 간헐천 보충(일반 모드면 끝, 기한 도달 시 단독 30초/옵션 주기만큼 기한 이동, 보스·비편집·주기·전원 로그인 관문, 예측 가능 설정이 아니면 벽시각 seed, 전투에서 옵션 18 이 2 이면 3×3 구역 목표 개수(예산·밀도·간격 보정과 두 번의 x87 정수화·0 방향 절삭)와 부족 구역 하나에 부족분 절반 배치, 옵션 18 이 1 이거나 단독/비전투이면 float 비교와 예산/3 목표로 전체 한 개 배치).
  보조 전역(`aux`)은 `{ spellCount, spellDeadline, incomePrimed, geyserFrameFlag, geyserDeadline, secondaryGeyserDeadline, ordinaryGeysers, resourcePool }` 속성 계약 객체로 받는다.

## 시험과 변형 확인

- `tests/raw-technology-types-packet.test.mjs` 9건: 누적의 중복/빈칸/입력 순서·마지막 칸·종료 NUL 없음 오류, 수신의 전체 재순회와 표시 효과 뒤 재조회·빈 목록의 표시 장치 불요·물리 오류 종류, 송신 관찰기(수신 방향 무동작·sentinel 260·장치 누락), 실제 `RawPacketDispatch` 등록과 분배/직접 호출.
- `tests/raw-battle-resource-frame.test.mjs` 17건: 로그인·옵션 크기 반환, 스펠 만료의 모든 조건과 시각 재조회, SP 주기의 플래그·비전투·최초/지급·조건별 차단과 현재 Player 재조회 횟수, 일반 간헐천 조기 반환과 기한 이동(현재 시각 기준), 전체 한 개 보충(예측/벽시각 seed·float 비용 비교·예산/3 절삭·signed 개수),
  구역 모드(목표 개수 표를 기준 구역별 최소 개수 탐색으로 단언·수량·후보 선택·난수 범위), 32비트 경계(큰 예산의 두 번 감김·signed 개수·부족분), 함수/단순 값 형태, Player 물리 크기와 8행 순회, 구독자 DWORD 네 BYTE, 필수 장치.

**변형 확인:** 기술 타입 41종·전투 자원 프레임 118종, 총 159가지를 넣어 동등 5가지를 뺀 모두 시험에서 잡혔다(처음 살아남은 21가지 중 동등 5가지와 효과 없는 변형 1가지를 뺀 15가지는 시험을 보강해 잡았다: 함수 형태의 거짓 값·Player 물리·8행 순회·구독자 높은 BYTE·SP 프레임 플래그·현재 시각 기준 기한·예측 장치 필수·큰 예산의 32비트 감김·부족분 signed·float 비용 비교·음수 예산 절삭·payload 물리 오류 종류).
동등 변형 5가지: 69h 목록의 일치 위치에 같은 값을 다시 쓰는 빈칸 쓰기 조건, 구독자 DWORD 의 부호(진리값만 쓴다), 정수화 입력의 비유한 검사(예산이 int32 라 도달하지 못함), 두 번의 0 방향 절삭 대 한 번의 `/8`(항등), 부족분 2 의 수량 경계(1 과 2/2 가 같다).

## 남은 경계

- 실제 Client TypeUse 목록·기술 표시 장치(69h)와 전투 자원 프레임의 실제 장치(스펠 보충 485e40·SP 송신·간헐천 생성·finder·설정)와 보조 전역 소유자는 세션 단계에서 연결한다. 웹 `game-aux-state.js` 에는 `spellDeadline`(005950d0)·`incomePrimed`(00595124) 칸이 아직 없다(스펠 개수는 기존 `savedGeyserCount` 와 같은 DWORD).
