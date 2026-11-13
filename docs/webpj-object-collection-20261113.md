# webpj ObjectCollection — 일반 자산/그림자 수집기 (WB04-E389, 2026-11-13, 5-A 열여덟 번째 단위)

5-A 렌더러 단위 중 client **ObjectCollection**(cpppj `client/ObjectCollection.cpp/.h`, 기준선 157줄)을 `render/object-collection.js` 로 옮겼다. 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). 원본 일반 자산/그림자 수집기(10.78 `00433660`)로, 공통 수집기 묶음의 `objects` 슬롯에 연결되어 공통 표시 목록의 kind 0 항목을 만든다. 이전 단위 SpriteDraw(E388)가 그 항목을 그린다.

## 1. 규칙

- **`ObjectCollection.collect(view, output, limit)`:** 해시 1·2·3단계를 `select(단계)` 하면서 월드 가시 사각형을 단계 크기로 나눈 칸을 훑는다. 칸 계산은 좌표 `+ 0.5` 의 CRT 절삭 하위 DWORD 를 signed 로 읽어 단계 크기로 나누고(0 방향 절삭) 단계의 상단 칸으로 제한한 뒤, 각 끝을 한 칸씩 넓히고 0~상단으로 독립 clamp 한다. 순서는 **y 바깥 · x 안쪽**. 같은 체인의 순환은 거부하지만 다른 칸/단계에서 같은 SID 가 반복되는 것은 전역 중복 제거 없이 그대로 낸다. 정상 반환 때 해시 선택은 2 로 돌아간다(실패하면 현재 선택을 바꾸지 않은 채 예외).
- **슬롯 읽기:** next(+4 WORD)·타입(+10)·위치 float(+14/+18)·프레임(+36 DWORD)·깊이(+35 signed BYTE). 타입 0 이거나 표 밖이면 거부(`nonzero type`). 다음 SID 는 판정이 끝난 뒤 현재 슬롯에서 다시 읽는다.
- **프레임 검사:** 로드된 타입이고 프레임이 물리 프레임 수 안이며(아니면 `physical frame`), 논리 프레임 수 ×(abstract/그림자 타입이면 2)를 넘지 않고 HP 쓰레기 표식(`0xdddddddd`·`0xcdcdcdcd`)이 아니어야 한다(아니면 `frameCheck`).
- **본체/그림자:** 본체 프레임 상자가 가시 사각형과 닫힌 구간으로 교차하면 기록한다. 교차는 `view.left − cellWidth − 1`·`view.top − cellHeight − 1` 부터 `view.right/bottom` 까지에 `위치 − 기준점` 이 들어갈 때(x 는 x87 정밀도, y 는 float 저장). 타입 flags1 의 그림자 비트(0x40000)가 있으면 `프레임 + 논리 프레임 수` 의 그림자 상자도 같은 판정으로 기록한다. 항목: x/y 는 현재 슬롯 float 위치(그림자는 y − float(0.1), walker(flags2 0x10000) 본체는 y + 0.5, float 저장), 토큰 `tokenBase + SID × 50`, 깊이 signed BYTE, kind 0, tail 은 그림자 4·abstract(0x400000)이고 프레임이 논리 프레임 수 이상이면 4·그 밖에는 0.
- **한도:** 원본 strict limit 은 **기록 후** `count >= limit` 이면 거부한다. 실제 출력 버퍼 칸이 모자라면 쓰기 전에 용량 오류를 낸다(둘은 다른 오류 문구).
- **`object(token)`:** 수집된 토큰을 같은 풀 SID 로 되돌린다(기준 미만·0·stride 배수 아님·용량 밖은 거부). **`bindObjectCollection(hooks, collector)`** 가 공통 묶음의 `objects` 슬롯에 연결한다.

## 2. 웹 범위

- 앱 수명에 연결하지 않았다(`PORT_MAP` session 0). 10.78 전용(CD/10.37 프레임 폭 분기는 옮기지 않음). 해시·풀·타입·셰이프 표는 호출자가 공급한다.
- `CommonSubmissionIntegration`(GumpCollection·TileGroupCollection 과 한 묶음)은 해당 수집기 단위에서 건다. 이 단위는 E388 이 미뤄 둔 `SpriteDraw_ObjectCollectionPixels`(실제 raw 해시 수집→CRT 정렬→공통 분배→SHP 픽셀)까지 연결했다.

## 3. 검사 (`tests/object-collection.test.mjs` 6건)

- **원본 독립 관측(10.78):** `objectcollection-x86.tsv` originals **477행**(가시 사각형·한도·초기 해시 선택, 네 단계 해시 칸, 완성 raw 슬롯 50바이트, 타입 플래그/최대 HP/논리 프레임 수와 물리 프레임 상자 → 반환 수·작업 칸 64개 전체 바이트·작업 배열 앞뒤 0xa5 보호)이 **첫 실행에서 전부 일치**했다. original1037·originalCD 행은 읽지 않는다. manifest id `object-collection`.
- cpppj 시험 이식: `ObjectCollection_ContractsAndEditionFrames` 의 패치 부분, `SpriteDraw_ObjectCollectionPixels`.
- 웹 계약 두 묶음: 그림자 y/walker y/abstract tail/깊이 부호/판정 구간 경계(닫힌 구간 ±0.25)/체인 순서와 next 재읽기, 타입 비트 마스크(낮은 비트만 있는 flags1·flags2 는 abstract/그림자/walker 아님)·프레임 배수·물리 프레임 경계·출력 용량 문구·토큰 경계(2^32)·월드 끝 칸·확장 칸(위·아래·왼쪽·오른쪽 정확히 한 칸, 월드 밖 가시 사각형의 상단 칸 제한)·2바이트 next 폭·프레임 DWORD 상위 바이트.

## 4. 변형 확인과 남은 경계

- 자동 연산자·상수 변형 108개를 적용했다. 1차 생존 13건 중 4건(`ABSTRACT`/`WALKER` 낮은 비트, 상단 칸 제한, 아래 확장 폭)은 독립 입력을 보강해 모두 잡았다(재확인 4/4).
- 남은 9건은 동등 변형: ① `readField` 의 기본 폭 4→5(뒤의 `| 0`/`>>>`로 잘림), ② `#cell` 의 `Number.isFinite`/`±2^63` 범위 판정과 `BigInt.asIntN` 폭(2^56 이상 float 의 하위 32비트는 0).
- 다음 5-A 항목: `RawCursorPreviews`(185, 커서 전역 K 영역 선행), 입자 본체(`RawCampaignParticles`·`ParticlePaletteMemory`), 렌더러 프레임 본체의 `Renderer`·`ClientRendererFrame`·`ScreenPresentation`·`Screen`.
