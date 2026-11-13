# webpj 색표 캐시 RawColorMapCache·ClientColorMapFile·ClientColorSortInput 마감 (WB04-E375, 2026-11-13, 5-A 네 번째 단위)

5-A 의 `o/RawColorMapCache`(adapt, 118줄)와 client `ClientColorMapFile`(adapt, 55줄)을 옮기고, E374 가 partial 로 남긴 `ClientColorSortInput` 의 `InitializeCache`·`OwnerHighlight`/`OwnerSolid` 를 닫아 세 PORT_MAP 행을 모두 `port` 로 올렸다. 모듈은 `core/raw-color-map-cache.js`(원문 캐시 부모 4함수)·`core/color-map-files.js`(웹 적응 저장소)와 기존 `core/client-color-sort-input.js` 의 `initializeCache`/`ownerHighlight`/`ownerSolid` 추가이다. 세 파일 모두 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`).

## 1. 규칙

- **`prepareColorMapCache(globals, hooks)` (43c4e0 접두):** 경로 장치(00459c60)의 NUL 경로를 받은 뒤 **현재** 소스(primary)를 한 번 빌려, 소유자 8행 × 256색을 행마다 현재 소유자 색 DWORD 의 하위 BYTE 로 `ownerSolid`(고정색) → `ownerHighlight`(항등) → 소스 기록 56바이트(14 DWORD, 각 DWORD 복사 직전에 물리 범위 확인) 복사 → 기록+8 double 명도 **> 0.800000011920929** 이면 `ownerHighlight` 를 소유자 색으로 바꾸는 순서로 쓰고, 끝에 소유자 표 등록(`hooks.owners`)을 한다. 명도가 임계와 같거나 NaN/−Inf/−1 이면 교체하지 않고 0.80000002·+Inf·1 만 교체한다. 실패 전에 쓴 출력은 되돌리지 않는다.
- **`initializeColorMapCache(globals, hooks, reader, writer)` (부모):** 접두 → reader 마지막 8바이트(+16..23)만 0 → reader 열기(경로, 모드 0, 1)·**파일 순서** 아홉 표 읽기(제한 `0xffffffff`; 반환 길이는 무시)·닫기. 정확한 `ColorCacheUnableToOpen` 만 캐시 재생성으로 들어가고(그 단계 이전의 경로/TYPE 표 marker 는 reader miss 로 잡지 않는다) 다른 오류는 정리(생성이 끝난 reader 만 닫음) 후 그대로 전파한다.
- **`regenerateColorMapCache(path, globals, hooks, writer)` (독립 catch 43c96b):** 9표 계산(`buildMaps`) → writer 열기(모드 1, 1)·아홉 쓰기·닫기. writer 는 reader 와 다른 지역 객체이고 같은 reader try 를 재진입하지 않는다. 정상 close 가 실패해도 같은 객체를 두 번 닫지 않는다.
- **`closeColorMapCacheFile(file, hooks)`:** 독립 정리 funclet 의 고정 this 전달 경계이다. hooks 는 도달할 때만 요구한다(없으면 Error `ColorMap cache의 실제 하위 장치 미연결`).
- **웹 적응(`ClientColorSortInput.initializeCache(files, path, currentRed, initializeOwners)`, ClientColorMapFile):** primary 필요·한 번뿐·소유자 등록 훅 필수. 파일 순서는 주소 순서 인덱스 `2·7·6·4·3·5·1·0·8`. 읽기: `files.read(path)` 가 없으면 열 수 없음(miss) → `rebuild(currentRed)` 로 9표를 계산해 `files.createWriter(path)` 로 2304바이트를 쓴다. 있으면(hit) 현재 전체 내용에서 표마다 256바이트씩 읽고 **짧은 파일은 첫 목적 표의 접두만 바꾸며** 나머지는 재생성하거나 파일을 고치지 않는다(후행 바이트도 무시). `MemoryColorMapFiles` 는 페이지 수명 메모리 저장소(읽기는 사본, 쓰기는 즉시 이어 붙임, `writable:false`/상위 폴더 목록에 없는 경로는 `ColorCacheUnableToOpen`). 영구 저장(localStorage)은 5차 요구에서 이 저장소를 교체해 연결한다.

## 2. 웹 범위

- 10.72(CD) 분기는 없다. cpppj 의 실제 `BaseFileSystem`·TAFF 등록 snapshot·암호화 아카이브 읽기는 웹에 해당 캐시 경로가 없어 옮기지 않았다(`ClientColorMapFile_CurrentEncryptedArchiveDoesNotUseRegisteredPayloadSnapshot` 제외 사유). `Screen.BindColorMaps`(화면이 현재 표를 getter 로 빌리는 연결)는 Screen 단위의 몫이다. DOM 을 쓰지 않는다.

## 3. 검사 (`tests/color-map-cache.test.mjs` 5건 · `tests/client-color-map-file.test.mjs` 6건)

- **원본 독립 관측(10.78):** `colorsort-cache-parent-x86.tsv` **130행**(진입점 prefix·parent·catch·cleanupR/W × 프로필/초기값/읽기 길이/쓰기 제한/하위 변경)의 완료 방식·독립 정리 횟수·dispatch 횟수·현재 소스 선택·소유자 DWORD 32바이트·소유자 표 4096바이트·아홉 표 2304바이트·두 파일 객체 48바이트·쓴 바이트·장치 호출 순서가 **첫 실행에서 모두 일치**했다. 장치(`CacheDevice`)는 cpppj `ColorMapCacheTests` 의 명시 입력/효과 장치를 그대로 옮긴 것이다.
- **웹 계약:** 캐시 hit 는 재생성 장치를 요구하지 않고 누락은 정확한 marker 에서만 드러남; 미지원 raw 소스 물리(빈 소스·마지막 기록 한 바이트 모자람)는 쓰기 뒤 거부하고 owners 등록은 하지 않음; 명도 임계의 엄격 초과와 NaN/무한/음수; 소유자 색을 행마다 하위 BYTE 로 읽음. `ClientColorMapFile`: miss 가 원문 9표(첫 53비트 관측의 기대)를 계산해 2304바이트 파일을 남기고 새 소유자 hit 가 다른 빨강(255)으로 다시 계산하지 않으며 소유자 8행 출력 4096바이트가 모두 같고, 알려진 표 파일 hit(후행 바이트 무시·파일 불변·이후 `rebuild` 가 같은 표 객체를 갱신), 짧은 파일의 접두 읽기, 쓸 수 없는 저장소의 열 수 없음 실패(계산한 표·소유자 등록은 남음), 최초 초기화 계약, 저장소 규칙.

## 4. 변형 확인과 남은 경계

- 세 모듈의 연산자·상수 자동 변형 32+8+13 = 53개(와 부분 단어 3개 직접 변형)를 적용했다. 1차에서 12개가 남았고 마지막 기록의 부분 단어 경계 하나(`||`→`&&`)는 시험을 보강해 잡았다. 남은 동등 변형: 소유자 행 수 8→9·행 반복의 `<=`(typed array 범위 밖 쓰기는 무시)·기록 복사의 첫 단어 시작(기록+8 의 double 만 쓰므로 첫 DWORD 는 값에 영향이 없음)·`source >= length`(바로 이어지는 남은 길이 검사가 같은 경우를 거부)·`slash <= 0`(루트 경로의 상위가 같은 빈 문자열)·이름의 NUL 종결자·24바이트 객체 크기·`position` 초기값(열 때마다 다시 0)·저장소의 `close` 가 하는 일이 없음(쓰기는 즉시 이어 붙임).
- 시험이 먼저 실패한 곳은 없었다(첫 실행에 130행·계약 모두 통과). 모듈은 관측 대조에서 바뀌지 않았다.
- 다음 5-A 항목: 팔레트(`PaletteColors`·`PaletteFlash`·`ParticlePaletteMemory`·`ScreenPaletteApply.h`) → 시각 효과 소단위(`VFXEllipse`·`VFXLine`·`VFXTransform`·`DrawWater`·`ThunderProcess`·`ConstructionVisualRenderer`·`SpellFrameVisibility.h`·`SpriteHitPointBar`) 순이다.
