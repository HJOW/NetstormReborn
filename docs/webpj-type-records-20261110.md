# webpj 타입 레코드 완성 — WB04-E265 (2026-11-10, 순서4 4-A 첫 단위)

3.1절 4-A 의 첫 항목 **RiftTypeTable** 을 옮겼다. 웹 `buildTypeRecords`(`www/assets/type-flags.js`)가 `.type` 정의에서 만드는 188개 타입 레코드가 지금까지는 플래그·그룹·발자국·이름뿐이어서 `SquidFactory` 가 요구하는 `zOrder`·`maxHitPoints`·`constructorAddress` 가 없었다(LEFT_JOBS 4절 "타입 표 생성 메타"). cpppj `RiftTypeTable.cpp` 의 `ApplyDefinition`/`PostProcess`/생성자 주소 대입 전체를 기준선 판(`git show 1f00a25a:cpppj/src/o/RiftTypeTable.cpp`)으로 옮겼고, 3-E 판정(E259)이 4-A 몫으로 남긴 ③(RiftType 새 칸 6개와 속성 파서, `defaultFrame`)도 함께 해소했다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 옮긴 것 (`www/assets/type-flags.js`)

| 항목 | 원문 | 웹 처리 |
| --- | --- | --- |
| `maxHitPoints` | 숫자 필수, 두 번째 선언은 assert(`maxHitPoints == 0`), `hasHitPoints` 플래그, 0 쪽 절삭 정수(범위 밖 0) | `Error('maxHitPoints == 0')`·`flags1 \|= hasHitPoints`·`toInt` |
| `zorder` | 숫자면 절삭 정수, 문자열이면 첫 식별자를 23개 이름 표(대소문자 구분 부분 문자열, 표 순서의 첫 일치)에서 찾아 `+`/`-` 오프셋을 DWORD 로 가감(오프셋 > 2147483647 은 범위 오류, 이름 없음/모름은 거부) | `Z_ORDER_NAMES`·`zOrderFromText` |
| `cost` | 숫자 필수, float32 저장 | `Math.fround` |
| `range`·`hpPerSec`·`airdamage` | 숫자 필수, float32, 같은 이름이 여러 번이면 마지막 값 | `Math.fround`, 속성 이름은 대소문자 무시 |
| `useairdamage`·`damageEffect` | 숫자 필수, 0 쪽 절삭 정수 | `toInt` |
| `techBit` | 숫자 필수, float32 로 줄인 뒤 CRT `_ftol` signed 64 절삭의 낮은 DWORD(비유한/2^63 이상은 0) | `techBitNumber` |
| `minUsage`/`maxUsage` 3칸 | 비·천둥·바람 순서, 비영이면 `usesPower` | 레코드 배열로 보관 |
| `hotFootRatioX/Y` | float32 × 16/11 을 0 쪽 절삭(int32 범위 밖 거부), `hotFootX/Y` 는 거부 | `typeHotFootOffset` |
| `level`·`theme` | `level` = 자료값 − 1, `theme` 은 첫 BYTE 로 `wrtsWRTS.?>` 조회 표(위치 % 5)·90 미만 기본 5·122 이상 거부 | `themeNumber` |
| 종류(`kind`) | 종류 단어(buried·nugget·bomb·bridge·walker·balloon·island·factory·vortex·edgefarm·focus·guy·geyser·residence)를 8글자까지 덮어씀 | `KIND_WORDS`·`KIND_BYTES` |
| 기본 프레임·방향 수·물리 프레임 수 | `definition.defaultFrame`, `readRawArcherTypeData`(orientations·클러스터 수) | 레코드 `defaultFrame`·`directions`·`frameCount` |
| 후처리 생략 비용 | 그룹이 있는 타입이 비용 0 이면 `(level+1) × 200`(운반 walker/balloon 400)을 float32 로 저장, 마지막에 사제(158)·guy 부류 비용 0 | `postProcess` |
| 생성자 주소 | 로딩·후처리 뒤 타입별 `TypeConstructorAddress` 대입 | `typeConstructorAddress`(core) |

레코드 초기값은 원본 전체 TYPE 초기화(0 으로 지운 뒤 +98 에 패치 5)와 같게 `emptyRecord` 에 두었다. 이 모듈이 `core/squid-constructors.js`(순수 표)와 `core/raw-archer-type-data.js` 를 가져오는 `assets → core` 의존이 새로 생겼으나 둘 다 DOM 을 쓰지 않고 순환이 없다.

## 2. 검사 (`tests/type-record-complete.test.mjs` 11건)

- **cpppj `SquidFactory_TypeLoader_ConnectsHpDepthAndConstructorMetadata` 대응**(패치판): 합성 정의 116개로 TYPE 74 의 `maxHitPoints=65536`·`zorder="zoBRIDGE_CONNECTOR - 3"`(= -13), TYPE 80 의 `zorder=511`, TYPE 82 의 생성자 주소 ≠ 0·TYPE 74 = 0 을 확인하고 실제 `SquidFactory.create(74)` 슬롯의 zOrder BYTE 243·HP 하위 바이트(0,0,1)를 확인한다.
- **zorder 문자열**: 이름 5종·`+`/`-` 오프셋·부분 일치 순서·대소문자 구분·알 수 없는 이름/큰 오프셋 거부·숫자 절삭.
- **`type-default-cost-x86.tsv` 10.78 1500행**(`TypeDefaultCostTests` 대응): 그룹·레벨·명시 비용·flags2 조합마다 비용 비트가 원본 관측과 같다. 실제 자산의 생략 비용(133·72 = 400, 158·70 = 0, 71 = 300, 128 = 400, 135 = 500)도 확인한다.
- **실제 자산 대조**: 모든 자산의 `range`/`hpPerSec`/`useairdamage`/`airdamage` 가 정의의 선언값(float32/절삭 정수)과 같고 TYPE 71 의 피해율 15·공중 피해 모드 1/율 20 이 유일한 공중 피해 선언이며 내장 타입(0~69)은 전부 0 이다(`RiftTypeTests` 실자산 3건). 기본 프레임은 모든 자산에서 정의값이다.
- **중복 속성의 float 비트**: `range`/`hpPerSec`/`airdamage` 의 마지막 값 -7.5(0xc0f00000), `useairdamage` -1.75 → -1.
- **`archertype-x86.tsv` 10.78 22행**(`RiftTypeAnimation_PatchGoldenMetadata`): 첫 자산의 방향 수/물리 프레임 수가 원본 관측과 같고 내장 타입은 0.
- **theme/techBit/hotFootRatio/level/종류**: 첫 글자별 theme 표 13글자, techBit float32 축약·부호·2^63·낮은 DWORD, `hotFootRatio` 24/8, level 2, 종류 `walker`, 거부(hotFootX·theme '~'·HP 중복).

최초 실패와 교정: 처음 쓴 종류 시험이 `typename a walker {…}` 로 플래그를 줬다 — 웹 파서에서 이 단어는 constructor 수식어(`constructorWords`)이고 플래그는 `typeflags walker;` 이므로 시험 입력을 고쳤다(모듈 결함이 아님). 30개 변형(zorder 표 값·오프셋 부호·부분 일치·assert 제거·비용 단가/레벨/그룹 관문/면제 번호·속성 이름·절삭·theme 모듈러/기본값/거부·hotfoot 배율·level·techBit·종류 길이·생성자 주소·기본 프레임·방향 수·사용량·hotFootX 거부)를 sed 로 넣어 보니 4개가 살아남았다(theme 모듈러/기본값은 시험 글자가 `s`(위치 3)뿐이라 구별되지 않았고, techBit float32 는 70000.7 이 float32 에서 같아 구별되지 않았으며, 기본 프레임은 시험이 없었다). theme 13글자·techBit 경계값·실제 자산 기본 프레임 시험을 더해 30개 모두 탐지한다.

## 3. 영향과 남은 경계

- `buildTypeRecords` 의 반환 레코드는 여전히 동결(`Object.freeze`)이며 필드가 늘었다. 기존 소비자(Fort 판독기·캡처 시험 장치 `realCaptureAssets`)는 필드 추가만 받으므로 `fort.test`·`capture-assets-support` 를 쓰는 시험이 그대로 통과한다. `capture-assets-support.mjs` 는 여전히 `maxHitPoints` 를 정의에서 따로 덮어쓰는데 같은 값이므로 정리는 필요 때 한다.
- 이 단위로 LEFT_JOBS 4절 "타입 표 생성 메타"와 3.6절 `RiftTypeTable` ③ 행이 해소됐다. 실제 미션 TYPE 표를 Session/World 에 대여하는 연결은 4-D 몫이다.
- `RiftType.cpp`(프레임 코드 검색 `FindNumber`/`FindMasked`/`FindFlags`/`LetterKinds`)는 `FrameCodeTable` 에 코드 번호가 없어 아직 옮기지 않았다 — 4-A 다음 항목이다.
- CD 10.72 분기(`useairdamage`/`airdamage` 판본 조건, theme 의 CD 모듈러 4)는 웹 범위(10.78)가 아니다. `predictable` 단어의 +0x1e8 OR 99 는 cpppj 도 옮기지 않았다.
