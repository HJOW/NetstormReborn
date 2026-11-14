# webpj HUD 구분선 입력 HudDividerInput (WB04-E471, 2026-11-15)

기준선 `1f00a25a`의 `client/HudDividerInput.cpp/.h`(102줄)를 `www/ui/hud-divider-input.js`로 옮겼다. 원본 HUD 구분선(divider)의 입력(004c49f0, 가상+28)과 프레임 방송(004c4b00, 가상+2c)을 같은 공통 Gump 수집기·입력·트리에 연결한다. 별도 Idle 등록은 없다. E470의 `HudDividerNode`를 그대로 쓴다.

## 이식한 것

- `HudDividerInput(collection, input, tree, hooks)`: 공유 조회 둘(`divider`·`base`)만 만들 때 요구하고(없으면 `TypeError`) 나머지 장치(`baseHandle`·`cursor`·`menuInstances`·`poll`)는 도달한 분기에서 요구한다. 토큰 0·조회 실패·파생/기본 주소 불일치는 `Error`다. `cursorEvent` 기본값은 hover 사건 `0200003b`다.
- `handle(token, event, first, second)`: 먼저 부모 체인으로 화면 좌표(signed)를 로컬 float 두 값으로 바꾼 뒤 사건 코드(`& 0x03ffffff`)와 release 비트를 구분한다. 좌클릭 누름(축 비트 1|2가 있을 때)은 기준점(`0x10`)·방송(`0x100`) 비트를 켜고 로컬 위치를 `positioning[0/1]`에 float32 비트로 저장한 뒤 공통 입력의 첫 캡처만 유지하며 1을 반환한다. 좌클릭 release는 축과 무관하게 방송 비트만 지우고 1이다(focus 해제는 공통 Route 몫). hover는 세로(2)가 가로(1)보다 우선해 커서 5/4를 요청한다. 나머지는 기본 Help 결과를 그대로 반환한다.
- `broadcast(token, event, first, second)`: 공통 Broadcast가 가시/방송/사망 선택을 끝낸 뒤 호출한다는 계약이라 그 조건을 다시 검사하지 않는다. 메뉴 인스턴스 수가 0이 아니면 폴링 없이 반환하고, 아니면 좌클릭 폴링의 release 비트(`0x80000000`)에서 방송 비트를 지운다. 세로 축(2)은 y = 마우스 y − 2(축 8이면 정수 제한 minY/maxY)와 x = 저장된 로컬 x + 노드 x, 가로 축(1)은 x = 마우스 x − 2(축 4이면 minX/maxX)와 y = 저장된 로컬 y + 노드 y로 계산해 공통 `Move`에 넘긴다(둘 다 없으면 이동 없이 반환). 정수 제한과는 배정도로 비교하고 제한이 선택될 때만 저장된 float 제한값을 쓴다.

## 기준선 뒤 판정 (3.6절)

`1f00a25a` 뒤 `HudDividerInput.cpp/.h`·fixture·cpppj 시험에 대한 변경이 없다(base `1f00a25a`).

## 검증

- 원본 10.78 독립 관측 **13,398행**(`hud-divider-input-x86.tsv`: handle 9,666·broadcast 3,732, 독립 6,699 입력의 두 정밀도)의 최종 flags·기준점·focus 상태, 장치 호출(모달 수·폴링·커서·기본 Help)과 최종 Push/Attach 인자(float 비트·대상·모드)가 **첫 실행에서 모두 일치**했다. 앞 14열(사건·좌표·축·플래그·제한·위치·부모 체인·capture·폴링 코드·메뉴 수·기본 결과)만 입력으로 읽고 뒤 6열의 관측은 비교에만 썼다(cpppj `HudDividerInputTests`와 같은 방식, 본체 내부의 local/anchor/capture/move 표식은 제외). 실제 `GumpCollection`·`GumpEvents`·`GumpInput`·`GumpTree`를 쓴다. manifest `hud-divider-input`으로 연결했고 기존 fixture를 수정하거나 다시 실행하지 않았다.
- 독립 시험 5건: 생성/공유 노드 검사, 누름·release·첫 캡처 유지·flags 비트 보존, hover 커서(세로 우선·기본 cursorEvent·장치 없음), 방송의 메뉴 수·폴링 release 비트·축 없음·장치 없음, 방송 이동의 `−2`·제한 비트별 적용·float32로 줄인 제한값(16777217 → 16777216)·세로 우선.
- 변형 확인: 자동 변형 61개 중 원본 관측만으로 생존이 9개(release 마스크 비트·기준점 비트·기본 cursorEvent·생성자/공유 노드 검사의 논리 연산)였으나 독립 시험으로 5개를 더 잡았고 남은 4개는 동등 변형이다(정수 제한과 같은 값에서의 `<`↔`<=` 두 곳은 어느 쪽이든 같은 float, `DataView` 버퍼 크기 두 곳). 수동 의미 변형 13개 중 4개(제한값/최종 위치의 `Math.fround`)는 직후 공통 `Move`의 float32 저장과 겹쳐 동등이다.
- 첫 실행에서 어긋난 곳은 없었다(13,398행과 독립 시험 모두 일치). 모듈을 고치지 않았다.

## 남은 것

공통 Broadcast의 가시/방송/사망 선택과 divider를 직계로 둔 `UberHud.inc`·`UberRootGump`·`RawUberRootControls`·생산 패널 연결은 후속 단위다.
