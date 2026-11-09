# webpj Fort 메타 로더(RawFortMetaLoad) 이식 — WB04-E226

2026-11-10, HJOW-Athlon. Fort 파일의 Badges 섹션(원본 28개 기록 장부의 setter·검색·읽기, 0047d800·004be8e0)과 Technology 섹션(004beee0 wrapper: 깊이·Template/isMine 진단·기술 부모·변환표 준비/해제·UI)을 옮겼다. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.1절의 "순서4 이후로 넘기는 `o/` 단위(Fort 적재·지형/섬 …)"에 속하며 기준선 뒤 변경이 없고 원본 관측이 있어 E225 Badges 전역 장부에 이어 옮겼다.

- cpppj 기준선 `1f00a25a`. 소스(`RawFortMetaLoad.cpp/.h`)·시험(`FortMetaLoadTests`)·fixture(`fortmeta-x86.tsv`) 모두 기준선 뒤 변경이 없다(`git diff --numstat 1f00a25a HEAD` 를 저장소 루트에서 확인, 출력 없음).
- 근거 커밋: 소스 `9f401229`(2026-10-23, 실제 섹션 선택 `*Selected`), fixture `db9b626c`(2026-10-14).
- 원본 fixture: `fortmeta-x86.tsv` 에서 10.78(`originals`) 470행(CD 10.72 `originalCD` 480행은 웹 범위 밖). 입력은 12 DWORD(종류 0 Badges·1 setter·2 Technology·3 검색, type, count, 장부 프로필, Template, slot, present, end, 깊이, 옛 변환표, 새 변환표, 변이), 출력은 검색 비트·stream pos/end·깊이·현재 변환표 유무·하위 효과 순서(`A:`·`L:`·`P:`·`N:`·`C:`·`F`·`U`)·28×220 BYTE 장부 전체(RLE)·보고 뒤 계속한 진단 수다.
- 공유 장치: E225 `raw-fort-badges-globals.js`(장부 배열), `fort-object-load.js`(`FortObjectLoad.contents`), `assets/fort.js`(`fortSection`·`SECTION_NAMES`), 실제 Contents 시험용으로 `squid-factory.js`·`content-form-host.js`·`contained-create.js`·`squid-forms.js`.

## 1. 옮긴 것

`www/core/raw-fort-meta-load.js`(DOM 없음, 10.78 전용)

- `FortMetaAssertion`(`MissingBadgesTemplate` 0·`BadgeCountRange` 1·`MissingTechnologyTemplate` 2·`TechnologyNotMine` 3)·`RawFortMetaLoad(pool, badges, loadingDepth, state, hooks)`: 10.78 SidPool·6160 BYTE 장부(Uint8Array)·깊이 상자 `{ value }`·`state { templatePresent, templateSlot, localOwner, hasTypeConversion }`·하위 효과 7개(`assertion`·`loadingDepthChanged`·`technologyParent`·`prepareTypeNames`·`contents`·`releaseTypeConversion`·`refreshTechnologyUi`)가 모두 있어야 생성된다(누락은 TypeError).
- `setBadge(type, count)`(0047d800): count 가 0..255 가 아니면 진단을 보고하고 **계속한다**. 진단이 바꾼 현재 장부를 다시 검색해 처음 일치한 type DWORD 기록의 count DWORD(+216) 네 BYTE 에만 32비트를 쓴다(type 이 중복되면 첫 기록만, 없으면 쓰지 않음).
- `findBadge(type)`: 같은 선형 검색이며 못 찾으면 −1. DWORD 로 비교하므로 type 255 이상도 BYTE 로 절삭하지 않는다.
- `loadBadges(stream)`/`loadBadgesSelected(select)`(004be8e0): 현재 Template 없음 진단 뒤 섹션을 고르고(10.78 은 깊이를 바꾸지 않는다), 섹션이 없거나 끝이 0 이면 돌아가며, 아니면 개수 BYTE 뒤 (type BYTE, count BYTE) 쌍마다 `setBadge`. 저장 순서를 그대로 따른다. stream 이 소진되면(호스트 물리 범위 밖) 기존 효과를 보존한 채 RangeError.
- `loadTechnology(stream)`/`loadTechnologySelected(select)`(004beee0): 깊이 +1 → Template 없음/isMine 아님 진단(둘 다 보고 뒤 계속) → 기술 부모 조회 → **그 뒤에** 섹션 선택 → 섹션이 있고 끝이 0 이 아니면 변환표 준비 → 내용물 생성(flags 0x11, 느슨한 변환) → 현재 변환표가 있으면 해제하고 없음으로 표시 → 항상 기술 UI 갱신 → 깊이 −1(32비트로 감김). 선택·하위 호출 실패는 깊이만 복구하고 예외를 다시 던진다(앞의 실제 효과는 되돌리지 않음).
- `makeFortMetaContentsHooks(pool, objects, fort, hooks)`: 이미 TypeNames 파싱을 끝낸 같은 Fort 의 parsed Technology 목록을 같은 풀의 `FortObjectLoad.contents` 로 전달하는 `contents` 경계를 만든다. 다른 풀·현재 원본 섹션이 아닌 stream(다른 buffer·부분 뷰·길이 차이)·섹션보다 큰 꼬리 BYTE 수·느슨한 변환(false, 디버그 진단 모드 미복원)은 내용물을 만들기 전에 거부하고, 성공하면 stream 위치를 섹션 길이에서 꼬리 BYTE 수를 뺀 값으로 놓는다.
- CD 10.72 에서 Badges 진입 때 깊이를 +1/−1 하는 분기는 웹 범위 밖이라 옮기지 않았다.

## 2. 검증

`tests/fort-meta-load.test.mjs`(6건)

- 원본 관측 재생: 10.78 470행 전부. 행마다 12 DWORD 입력으로 장부·stream·깊이·상태·하위 효과 장면을 만들고 결과 비트·위치·끝·깊이·변환표·효과 문자열·장부 전체 RLE·진단 수가 원본과 같은지 확인한다(변이 입력 1~3: UI 가 깊이를 바꿈, 내용물이 변환표를 다시 만듦, 진단 hook 이 장부를 바꿈 포함).
- cpppj 계약 포팅 4건: 같은 풀 실제 Contents(`SquidFactory`→`RawContainedCreate`→`ContentFormHost`→`FortObjectLoad`)가 Technology 목록을 생성하고 pos 6·깊이 3·변환표 해제·효과 순서(`parent`→`prepare`→`release`→`ui`)·자식 TYPE 110/111·수량 9/65534·부모 첫 자식 연결·난수 불변을 지킴, 부족한 장부(6159)·필수 효과 누락·CD 풀·소진된 물리 stream 거부, 진단·기술 부모 조회가 현재 섹션을 바꾼 뒤에만 선택 실행(`A0,S3` / `L4,A2,A3,P,S4,N,C,F,U,L3`), 선택 실패도 깊이 복구와 빠진 선택 callback 거부.
- E225 연결 1건(cpppj `FortBadgesGlobals_SharedMetaLoaderGetterAndWriter` 의 로더·조회 부분): `RawFortBadgesGlobals.records()` 같은 배열을 로더가 빌려 `[2,71,7,0,9]` 를 읽고, 범위 밖 `setBadge(72,256)` 진단 뒤에도 쓰며, 빈 섹션은 무시하고, count 외 바이트가 원본 그대로이며 `resetCounts()` 가 로더가 쓴 수량을 지운다. 저장(`SaveFortBadges`)은 편집/저장 파일이라 웹 제외 분류다.

최초 실패와 교정: 모듈은 처음부터 6건이 모두 통과했다. 모듈 사본을 바꿔 돌려 비교가 작동하는지 확인했다. 확인 뒤 원본으로 되돌렸다(diff 일치). 처음 변형 39건 중 2건이 살아남아 시험을 보강했다(`makeFortMetaContentsHooks` 가 꼬리 BYTE 수 > 섹션 길이를 내용물 생성 전에 거부하는지, 같은 buffer 의 부분 뷰를 현재 섹션으로 받지 않는지). 거부 시험은 이제 자식 수·전송 수·stream 위치가 불변인지도 본다.

- 실패한 변형: count 상·하한 경계, 범위 진단 제거, 쓰기 위치 216→212, 검색 일치 판정·type 절삭·첫 일치 대신 건너뜀, Template 진단 제거(Badges·Technology 각각)·isMine 진단 제거·slot 비교 반전, present/end 조건 각각 제거, type/count BYTE 순서 교환, 반복 상한 이상, 소진 검사 제거, 깊이 +2·실패 복구 제거·마지막 복구 제거·32비트 감김 제거, flags 0x11→0x10, 느슨한 변환 인자 반전, 변환표 표시 해제 제거·해제 호출 무조건, UI 호출 제거·섹션이 있을 때만 호출, 변환표 준비 제거, 선택을 부모 조회 앞으로, 생성 시 판본·장부 길이·필수 효과 검사 제거, 내용물 경계의 풀 검사·느슨한 변환 검사·꼬리 검사·섹션 identity/offset/length 검사·위치 설정·꼬리 반영·실제 내용물 호출 제거(각 1건).
- 적용되지 않았거나 통과한 변형: 없음.

## 3. 경계와 남은 것

- 실제 월드에 연결하지 않았다. 내용물 생성 몸체(`FortObjectLoad`)는 이식돼 있으나 로더의 `loadingDepth` 소비자(Money/Template/사제)·실제 Template 상태·기술 부모 조회·TypeNames 변환표 준비/해제·기술 UI 갱신은 순서4 몫이다(PORT_MAP session 0). TypeNames 변환 0 인 디버그 진단 모드는 cpppj 도 미복원이라 이 모듈이 거부한다.
- CD 10.72 분기(`originalCD` 480행과 Badges 깊이 증감)는 웹 범위 밖이다.

## 4. 다음 시작점

WB04-E227 은 3.1절에서 고른다. 기준선 뒤 변경이 없는 Fort/섬 단위(`RawFortPaletteTransition`·`RawTerrainBitmap`·`RawStateWorldCleanup`·`SquidSpatial`·`RawTerrainGrowth`)를 권한다.
