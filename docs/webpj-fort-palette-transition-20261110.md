# webpj 팔레트 이름 전환(RawFortPaletteTransition) 이식 — WB04-E227

2026-11-10, HJOW-Athlon. 팔레트 이름 전환(004b6100)과 세 하위 부모(구름 캐시 해제 00470c30·구름 이름 설정 00471190·dirty 표시 004973c0)의 현재 전역·부분 효과 순서를 옮겼다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위(Fort 적재·지형/섬 …)"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E226 Fort 메타 로더에 이어 옮겼다. 팔레트 계산 자체는 다시 쓰지 않고 이름·캐시·하위 호출·dirty 의 순서만 소유한다.

- cpppj 기준선 `1f00a25a`. 소스(`RawFortPaletteTransition.cpp/.h`)·시험(`FortPaletteTransitionTests`)·fixture(`fort-palette-transition-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스·fixture `29bee739`(2026-10-24).
- 원본 fixture: `fort-palette-transition-x86.tsv` 676관측(두 FPCW 관측 × 입력, 10.78 전용). 입력은 진입점(`transition`·`cache`·`sort`·`dirty`)·이름 케이스 0..10(같은 이름·대소문자·빈 이름·두 이름 버퍼를 가리키는 별칭 인자·210자 이름·NUL 없는 입력)·캐시/횟수/렌더러/dirty·build 결과·경로 조회 선택·callback 변이(인자 변수 교체·source 내용 변경·캐시·횟수·렌더러·dirty 변경·경로 반환 교체·중단), 출력은 반환/물리 중단 원인·현재 11개 필드·이름 두 버퍼와 입력 두 개와 경로 두 개의 전체 물리(hex)·하위 호출 trace 다.
- 공유 장치: 없음(순수 순서 규칙).

## 1. 옮긴 것

`www/core/raw-fort-palette-transition.js`(DOM 없음, 10.78 전용)

- `transitionFortPalette(argument, globals, hooks)`(004b6100): 로그(29, `"init palette\n"`) 복귀 뒤에 인자 변수를 읽어 캡처하고(그래서 `argument` 는 `() => Uint8Array` getter), 현재 이름과 같으면(대소문자 무시 비교가 0) 자연 반환한다. 다르면 이름을 NUL 까지 순방향 복사 → 구름 캐시 해제 → 구름 이름 설정 → 경로 조회(`"GamePalSpec"`, 0, 0) → 팔레트 적재(둘째 인자 1) → 추가 표 계산 → 렌더러가 있으면 dirty = 1.
- `releaseFortPaletteCloudCache(globals, hooks)`(00470c30): 비영 현재 캐시만 해제하고 정상 복귀한 뒤 **무조건** 같은 DWORD 를 0 으로 쓴다.
- `setFortPaletteCloudName(source, globals, hooks)`(00471190): 같은 source 에서 NUL 까지 순방향 복사(구름 이름 버퍼), 캐시가 0 이면 구름 계산, 그 뒤 **현재 횟수를 한 번만** 읽어(각 행 갱신이 횟수를 바꿔도 다시 읽지 않음) 그만큼 행 갱신.
- `markFortPaletteDirty(globals)`(004973c0): 현재 렌더러 DWORD 가 비영일 때만 dirty = 1.
- 이름 복사는 길이를 미리 재지 않고 BYTE 하나씩 읽고 써서 겹치는 별칭 버퍼와 물리 중단 전 효과를 보존한다. 입력 범위를 출력 범위보다 먼저 검사해 `RangeError('팔레트 이름 입력 물리 범위')`/`('…출력 물리 범위')` 로 구별한다.
- 전역: `globals = { paletteName, cloudName: Uint8Array, cache·dirty: { value }, count·renderer: () => 정수 }` (C++ 의 가변 참조는 상자, `const&` 는 getter). 하위 장치(`log`·`compare`·`release`·`buildCloud`·`stepCloud`·`pathSpec`·`loadPalette`·`buildMaps`)가 도달 시점에 없으면 기본 성공값 대신 TypeError 로 그 위치에서 거부해 앞선 효과를 남긴다.

## 2. 검증

`tests/fort-palette-transition.test.mjs`(3건)

- 원본 관측 재생: 676관측 전부(중단 34 포함). 행마다 새 장치(고정 이름 앞 1 BYTE 포함 두 257 BYTE 버퍼·512 BYTE 입력 둘·256 BYTE 경로 둘)를 만들어 4개 진입점을 실행하고 반환/중단 원인·11개 필드·전체 물리·하위 호출 trace 를 대조한다. JS 뷰의 주소는 (buffer, byteOffset)으로 격리 PE 주소에 대응시켰다.
- cpppj 계약 포팅 1건: 같은 이름의 자연 반환은 로그·비교 외 하위 장치를 부르지 않고(trace 2개), 도달한 필수 적재 장치(`loadPalette`) 누락은 해제·구름 계산이 남긴 효과(캐시 토큰·free/build 횟수)를 보존한 채 거부하며 추가 표 계산에 도달하지 않는다.
- 웹 계약 1건(원본 fixture 에 비 ASCII 입력과 입력/출력 범위가 동시에 걸리는 행이 없어 추가): 0xe9·0x80·0xff 같은 고비트 BYTE 가 변형 없이 두 이름 버퍼에 복사되고, 입력 범위 오류가 출력 범위 오류보다 먼저이며, 출력 범위 오류 전의 쓰기가 남는다.

최초 실패와 교정: 모듈은 처음부터 676행이 모두 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치).

- 실패한 변형: 로그 제거·단계 변경, 인자 캡처를 로그 앞으로 이동, 비교 결과 반전, 이름 복사·캐시 해제·구름 설정·경로 조회·추가 표 계산·dirty 표시 제거, 경로 키/인자·적재 둘째 인자 변경, dirty 값·조건·순서 변경, 캐시 0 쓰기 제거·조건 무시·순서 교환, 구름 계산 항상/절대 안 함·횟수 캡처를 계산 앞으로·횟수 재조회, 구름 이름 복사 제거, 복사 BYTE 7비트 절삭·NUL 종료 제거, 입력/출력 범위 검사 제거와 순서 교환(각 1건 모두 실패).
- 적용되지만 통과한 변형(동등 변형): 캐시 해제 인자를 캡처한 `current` 대신 `globals.cache.value` 재조회. 검사와 해제 호출 사이에 다른 장치가 호출되지 않아 두 값이 항상 같다(원본도 한 주소를 검사와 해제 인자에 함께 쓴다).

## 3. 경계와 남은 것

- 실제 Screen/렌더러에 연결하지 않았다. 팔레트 계산(`buildCloud`·`stepCloud`·`buildMaps`)·경로 조회·팔레트 파일 적재·CRT 비교의 실제 몸체와 호출 지점(Fort 로드 흐름의 팔레트 이름 설정)은 순서4·5 몫이다(PORT_MAP session 0).
- 같은 소스를 쓰는 cpppj `ConfigurationCommandInput` 쪽 소비는 E27 `configuration-command-input.js` 에서 별도로 이식되어 있고 이 모듈과 아직 연결하지 않았다.

## 4. 다음 시작점

WB04-E228 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 Fort/섬 단위(`RawTerrainBitmap`·`RawStateWorldCleanup`·`SquidSpatial`·`RawTerrainGrowth`)를 권한다.
