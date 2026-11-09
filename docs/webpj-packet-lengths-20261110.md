# webpj 패킷 선언 길이 표(RawPacketLengths) 이식 — WB04-E232

2026-11-10, HJOW-Athlon. 10.78 패킷 타입별 선언 길이 WORD 표(원본 `Netstorm.exe` 가상 주소 0052ec40, 타입 0~137의 138개; 0xffff = 가변 길이)를 옮겼다. 패킷 등록 표와 수신 분배(순서4)가 파생 처리기를 등록하기 전에 이 선언 길이를 먼저 쓴다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "RawPacketDispatch(순서4) 이식" 선행 자료에 해당하며 기준선 뒤 변경이 없다.

- cpppj 기준선 `1f00a25a`. 소스(`RawPacketLengths.h`, 헤더 전용)는 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스 `e2dd58a8`(2026-10-11).
- 원본 근거: fixture 가 아니라 **원본 실행 파일 자체**다. `originals/Netstorm.exe`(10.78)를 읽기 전용으로 열어(게임 실행 없음, 파일 수정 없음) PE 구역 표로 가상 주소 0052ec40 을 `.data` 구역 파일 위치로 바꿔 138개 WORD 를 직접 읽는다. 이미 manifest 에 등록된 `original-pe-data` 항목(WB04-C2 게임 보조 전역)과 같은 방식이다.
- 공유 장치: 없음(순수 상수 표).

## 1. 옮긴 것

`www/core/raw-packet-lengths.js`(DOM 없음, 10.78 전용)

- `PATCH_PACKET_LENGTHS`: 타입 0~137의 선언 길이 138개(동결 배열). `VARIABLE_PACKET_LENGTH`(0xffff)는 가변 길이 표식이며 계산하거나 추정하지 않았다. 고정 길이 예: 타입 16 = 8 BYTE, 27 = 0x26, 137 = 0x10b.
- `PATCH_PACKET_TYPE_COUNT`(138)·`patchPacketLength(type)`: 정수 타입의 선언 길이를 돌려주고 표 밖(음수·138 이상·비정수)은 RangeError 로 거부한다(원본의 표 밖 읽기는 계약 밖).
- 값은 cpppj 헤더 텍스트에서 스크립트로 옮겼고(오타 방지) 아래 검증이 헤더와 PE 두 가지로 다시 대조한다.
- CD 10.72(originalCD 0050fd00, 타입 0~126, 127개)는 웹 범위 밖이라 옮기지 않았다.

## 2. 검증

`tests/packet-lengths.test.mjs`(3건)

- 원본 실행 파일 대조: `originals/Netstorm.exe` 의 PE 구역 표(`.data`)로 0052ec40 을 파일 위치로 변환해 138개 WORD 를 읽고 웹 표와 `deepEqual`. 가변(0xffff) 개수도 같다. `originals/Netstorm.exe` 가 없으면 건너뛴다.
- 보조 대조: cpppj `RawPacketLengths.h` 의 `kPatchPacketLengths` 선언 텍스트 파싱 결과와 같다.
- 계약: 개수 138, 가변 표식 0xffff, 알려진 값 4개, 표 밖 타입 5종 거부, 배열 동결과 쓰기 거부.

최초 실패와 교정: 모듈과 시험은 처음부터 통과했다. 표 값 변경(첫·중간·끝 항목)·개수·가변 표식·범위·정수 검사·동결 제거 8건을 적용해 모두 실패하는 것을 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 적용되지만 통과한 변형은 없다.

## 3. 경계와 남은 것

- 이 표를 쓰는 패킷 등록 표(`PacketRegistrationTable`)와 수신 분배(`RawPacketDispatch`)는 아직 옮기지 않았다(순서4). 멀티플레이어 네트워크 자체는 웹 범위 밖이다.
- manifest 는 기존 `original-pe-data` 항목(주 근거인 원본 실행 파일)의 소비자·모듈·cpppj 소스에 이 시험·모듈·헤더를 추가하고, 헤더 선언 교차 확인용 `src-packet-lengths`(source) 항목 1개를 새로 등록했다.

## 4. 다음 시작점

WB04-E233 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 단위(`RawTerrainGrowth`·`RawTotalMade`·`RawMissionOutcome`·`RawMissionStart`·`RawObjectTraceMain`)를 권한다.
