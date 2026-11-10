# webpj 자료 조회 순서 시험 — WB04-E273 (2026-11-10, 4-A 마감)

3.1절 4-A 의 마지막 항목 **자료 조회 순서 시험**이다. 미션 자료(타입 표·미션 스크립트·`.fort`·Tell)를 웹이 cpppj 와 같은 순서로 조회하는지 실제 공개 자료(`www/data`) 위에서 `GameFiles.read/find` 를 기록해 고정했다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다. 이 항목으로 4-A 가 모두 끝난다(9/9).

## 1. 고정한 조회 순서 (`tests/data-lookup-order.test.mjs` 5건)

| 단계 | 순서 | 근거 |
| --- | --- | --- |
| 타입 표 | `TYPE_LOAD_ORDER` 116개가 cpppj `o/TypeLoadOrder.cpp`(원본 `00540dd0`, CD 판은 앞 101개)와 이름·순서까지 같고, `TypeTable.fromArchive` 가 `\d\<이름>.type` 을 그 순서로 **한 번씩만** 읽는다. 번호는 70(`dude`)부터 185(`Monument`)이다 | cpppj `TypeLoadOrder.cpp` 를 시험이 직접 읽어 비교 |
| 시작 | `d/setup.cfg` → 언어 용어표 `\D\config.<언어>` 순이다. 영어는 두 파일뿐이고 Tell 은 읽지 않는다. 한국어는 용어표가 없으면 영어 용어표로 대체한다(웹 적응) | 원본 `00441790` → `00440e00` (`game-data.js` 머리 주석, `startup.test.mjs`) |
| 미션 | 타입 표(자산 초기화)가 먼저 → 미션 스크립트(`missionSpec`) → `mission.loadFort` 값(없으면 미션 이름)을 `fortSpec` 에 넣어 `.fort` 경로 → `.fort` 읽기. `{@미션.키}` 는 조회마다 스크립트를 한 번 읽는다(미션 이름은 대문자로 바뀌지만 파일 조회는 대소문자를 무시). `.fort` 해석(`parseFort`)은 앞서 읽은 타입 표로 만든 `createFortTypes` 를 쓰며 파일을 더 읽지 않는다. 실제 `Tutorial2` 의 `loadFort` 는 `SecretWorkshop` 이며 미션 이름의 `Tutorial2.fort` 는 없다 | cpppj `Mission.cpp` `MissionScript`(원본 `00482fb0`): 객체 생성 → 추가 설정 → `missionSpec` 스크립트 → `loadFort`/`missionType` → `fortSpec` |
| Tell | 등록 객체 접두어 섹션 → 현재 언어 tell → (웹 적응) 영어 tell. 현재 언어 tell 에 섹션이 있으면 **영어 tell 은 읽지 않고**, 없을 때만 한 번 읽어 캐시한다. 둘 다 없으면 null 이고 더 읽지 않는다. 객체에 있는 접두어 섹션은 파일을 읽지 않는다 | 원본 `004cf730` FindTellSection(객체 → 기본 tell) |
| 언어 파일 | 현재 언어 파일을 먼저 읽고 없을 때만 `.언어` 를 영어로 바꿔 한 번 더 읽는다(웹 적응, 원본은 대체하지 않음) | `GameData._readLanguageFile` |

cpppj `SquidFactory_TypeLoader_ConnectsHpDepthAndConstructorMetadata` 대응 검사는 E265 `type-record-complete.test.mjs` 의 "`.type` 의 HP/깊이가 레코드에 연결" 시험이며 이번에 다시 확인했다.

## 2. 이번에 고친 결함

- `GameData.tellSection` 이 `[현재 언어 tell, 영어 tell]` 을 배열로 한꺼번에 평가해, 현재 언어 tell 에 섹션이 있어도 **영어 tell 파일을 읽었다**(조회 순서 시험을 쓰다 로그에서 발견). 함수 목록으로 바꿔 지연 평가하게 고쳤다. 결과 문자열은 바뀌지 않고 읽는 파일만 줄었다(한국어에서 영어 tell 68KB 읽기 제거).

## 3. 변형 확인

- `game-data.js`: Tell 순서 뒤집기·즉시 평가·객체 접두어 무시·영어 대체 제거·설정/용어표 순서 바꿈 5개 모두 탐지. `type-table.js`: 읽기 역순·한 파일 두 번 읽기 2개 모두 탐지.

## 4. 대응표·남은 경계

- `PORT_MAP.tsv`: `client/Mission.cpp`(MissionScript)는 todo 유지, 4-D 이월 메모만 추가했다. `MissionScript` 는 웹 `Config` 에 `AppendArguments`(`@파일` 포함)·`SectionNames`·`CreateGuideMissionScript` 같은 확장이 필요하고 cpppj 시험(`MissionFileSource*`·`MissionFortSelection*`)이 편집기 작업 파일 장치와 얽혀 있어 Session 조립(4-D)과 함께 옮긴다. 이 단위가 고정한 조회 순서(스크립트 → loadFort → fortSpec)는 그때 `MissionScript` 의 호출 순서와 같아야 한다.
- 개발용 지형 화면(`loadTerrainScene`)은 `.fort` → 타입 → 셰이프 순으로 읽는다. 이는 미션 경로가 아니라 `?map=` 정적 장면이므로 위 순서의 대상이 아니다. cpppj `GameAssets` 는 셰이프 → 팔레트 → 타입 순이며 5-A 에서 대조한다.
- 다음은 **4-B** Fort·지형·섬 적재다.
