# cpppj 타입 표와 요새 파일(`.fort`) 읽기 복원

> 2026-10-05. 기준은 패치판 `originals/Netstorm.exe`(10.78), 대조는 CD판 `originalCD/NETSTORM.EXE`(10.72).
> 원본 게임 프로세스는 실행하지 않았다. 원본 파일은 읽기만 했다. [후속 계획](../cpp-roadmap.md)의 2단계에 해당한다.

`.fort`의 오브젝트 레코드는 타입마다 저장하는 필드가 다르고, 파일 안의 타입 번호는 저장한 실행 파일의 번호다. 그래서 요새 파일을 읽으려면 **전체 타입 번호 체계와 타입 플래그**가 먼저 필요하다. 이번에 둘을 함께 옮겼다. 파일 형식은 [요새 파일 문서](../formats/fort.md)에 있다.

## 복원한 소스

| 소스 | 원본 함수 (패치 ↔ CD) | 내용 |
|---|---|---|
| `o/RiftTypeTable.cpp` `RiftTypeTable` | `0049ebb0` ↔ `004434d0` | 타입 배열 초기화, 내장 프로세스 타입 이름 |
| | `0049c3b0` ↔ `004460f0` | 플래그 단어 48개 → 플래그 1·2 비트와 종류 단어, 속성(`group`·`level`·`foot_x/y`·`minUsage` 계열·`maxHitPoints`·`description`) |
| | `0049b0d0` ↔ `00444e10` | 후처리: 내장 타입 이름, 그룹에 따른 비트, 파생 플래그, 목록 플래그, `foot_y` 6 → 8 |
| | `0049a860` ↔ `00443410` | 이름으로 타입 번호 찾기 |
| `o/Template.cpp` `FortTemplate` | `0044d380` ↔ `0044c750` (Datamanager.cpp) | 길이 접두 섹션 35개 |
| | `004bd800` ↔ `00427e00` | `TypeNames` 해시로 타입 번호 변환표 만들기 |
| | `004bdc60` ↔ `00428320` | 오브젝트 레코드의 선택 필드 |
| | `004bd130` ↔ `00426dd0` | 내용물 목록(`Technology` 포함) |
| | `004be160` ↔ `004288a0`, `004be2c0`·`004be3c0` | 청크 레코드, `Chaff`·`TerrNN` 순회 |
| | `004bf190` ↔ `00429670`, `004bd450`, `00483300` | `Deck`, `Subscriber`, `Money` |
| `client/Mission.cpp` `MissionScript` | `00482fb0` ↔ `00481020` | 미션 스크립트 읽기, `loadFort`·`missionType`, `fortSpec` 경로 |
| `client/GameAssets.cpp` | (새 연결 계층) | 읽은 `.type` 정의로 타입 표를 만든다 |

원본은 파일을 읽으면서 곧바로 오브젝트(Squid)를 만들어 월드에 놓는다. cpppj의 `FortTemplate`은 **파일 내용을 구조로 읽는 데까지**다. 오브젝트 생성, 영역 배치에 따른 청크 위치, 소유자 결정은 옮기지 않았다.

## 타입 번호 체계

원본 타입 배열은 패치판 **188개**(`DAT_00541348`), CD판 **171개**(`DAT_0051cbf0`)다. 타입 구조체는 패치 500바이트, CD 468바이트다.

| 번호 | 내용 | 이름 |
|---|---|---|
| 0 | 없음 | (빈 이름) |
| 1, 2, 3, 4, 6 | 내장 타입 | `DependForm`, `ProcessForm`, `GumpForm`, `PlayerSquid`, `ContentForm` — 후처리가 채운다 |
| 5, 7, 8, 9 | 내장 타입 | (빈 이름) |
| 10~62 (CD 10~61) | 내장 프로세스 타입 53개 (CD 52개) | 패치 `00540fa0`, CD `0051c890`의 이름을 `strncpy`로 20바이트까지 복사 — 긴 이름은 잘린다 |
| 63~69 (CD 62~69) | 쓰지 않음 | (빈 이름) |
| 70~185 (CD 70~170) | `.type` 타입 116개 (CD 101개) | 번호 = 70 + 로딩 순서. 이름은 `typename`이 아니라 **파일 이름** |
| 186, 187 (패치판만) | 쓰지 않음 | (빈 이름) |

* CD판의 프로세스 타입 목록은 패치판 목록에서 마지막 `thunderAviaryProcessType`이 없는 것이다.
* `.type` 번호가 `70 + 순서`인 근거: `0049c3b0`은 `(읽은 파일 수 + 0x45) × 500`의 구조체에 쓰는데, 파서가 한 파일의 정의를 끝낼 때는 이미 다음 파일로 넘어가 파일 수가 1 늘어 있다(`0049c200`이 `< 0x74`까지 넘긴다).
* **이름이 20글자 이상인 타입은 이름이 설명 필드와 이어져 읽힌다.** 이름은 길이 제한 없이 `+4`에 복사되고 `description` 속성은 그 뒤 `+0x18`에 `strncpy` 된다. `fakeThreeByThreeSurface`(23글자)는 C 문자열로 읽으면 `fakeThreeByThreeSurf` + `fake3x3Surface`가 된다. 실제 파일의 해시가 이 문자열의 해시(`0534a54b`)와 같다. 그래서 원본에서 이 타입은 본래 이름으로 찾을 수 없다.

### 이름 해시 (`TypeNames`)

이름의 글자를 부호 있는 8비트로 읽어 `(위치 % 4) × 8`비트 왼쪽으로 밀어 모두 더한다(32비트로 감긴다). 이름 없는 타입의 해시는 0이다.

변환표(`004bd800`)는 **현재 타입마다** 해시가 같은 첫 파일 번호에 자기 번호를 적는다. 그래서:

* 파일에만 있고 지금은 없는 타입의 번호는 0으로 남는다.
* 해시가 0인 파일 번호 가운데 첫 번호(보통 0번)는 이름 없는 현재 타입 가운데 **가장 큰 번호**(패치 187, CD 69)가 된다. 원본 그대로이며, 보유한 파일에는 타입 바이트가 0인 오브젝트가 하나도 없어(313,712개 가운데 0개) 문제가 드러나지 않는다.
* 섹션이 비어 있으면 변환표가 없고 파일의 번호를 그대로 쓴다.

## 타입 플래그

플래그 단어 → 비트 표는 `0049c3b0`의 비교 사슬에서 옮겼다(기존 `tools/fort.py`의 표와 같고 `focus`·`predictable` 두 단어가 더 있다). 이번에 새로 확인한 것:

* **`group` 속성이 플래그 2를 켠다.** 이름 표(패치 `00542468`): `archer`(0) `cannon`(1) `blocker`(2) `aviary`(3) `flyer`(4) `battery`(5) `fence`(6) `walker`(7) `balloon`(8) `misc`(9), 기본값 10(`NO_GROUP`). 후처리가 battery → `0x800`, archer·cannon → `0x8000000`, blocker → `0x4000000`을 켠다. 이 비트들은 소유자를 저장하는 타입 마스크(`0x5d77cf00`)에 들어 있다. 기존 문서가 "내부 class 값, 출처 미확정"이라고 한 것이 이 `group`이다.
* 종류 단어(`+0xf0`)는 `strncpy(대상, 단어, min(길이, 8))`로 덮어쓴다. 종료 문자를 쓰지 않아 `residence guy`는 `guyidenc`가 된다.
* 후처리의 파생 규칙: vortex·factory·walker·balloon → `container`와 목록 플래그(`+0xfc`, walker·balloon이면 0x20, 아니면 0x10), bomb → 내용물 목록 플래그(`+0x100`) 1, buried → `saveQA`, emplacement → `dropblocking`, 3×3 emplacement(vortex·factory 제외) → 플래그 1에 6, 사용량이 0이 아니면 `0x4000` 등. 전체는 `RiftTypeTable.cpp`의 `PostProcess`에 있다.
* 내장·빈 타입도 후처리를 거쳐 플래그 1이 2(`mayDropOnIsle`)가 된다.

생성자 함수 포인터, 사거리·비용 같은 전투 수치, 요구 에너지 문자열(`+0xa0`), 최대 사거리 집계는 옮기지 않았다.

## 오브젝트 레코드에서 새로 확인한 것

기존 [형식 문서](../formats/fort.md)의 레코드 구조는 맞았다. 다음을 더 확인했다.

* **버전 0**: `004ad0a0`은 `or edx, 0x4000000` 뒤에 분기해 항상 참이다(원본 소스의 `|`/`&` 실수로 보인다). 그래서 버전 0에서는 모든 타입 뒤에 상태 1바이트가 있다. 보유한 파일의 버전은 2(2,523개 섹션)와 1(2개)뿐이다.
* **소유자**: 저장된 소유자 바이트는 0이거나 8보다 크면 1로 바꾼다. 그 값을 실제로 쓰는지는 실행 상태(`DAT_005c85a4`·`DAT_00594fa0`)에 달려 있고, 아니면 호출자가 준 영역 소유자를 쓴다. geyser·buried(`0x10002000`)와 island는 소유자 0이다. cpppj는 파일의 값을 그대로 보존하고 결정은 하지 않는다.
* **내용물의 목록 플래그**: 항목마다 `타입의 +0x100` → `그릇 타입의 +0xfc` → 둘 다 0이면 **파일에서 1바이트** 순서로 정한다. `Technology`는 플레이어(`PlayerSquid`, `+0xfc` = 0)를 그릇으로 한 내용물 목록이라 항목마다 1바이트가 있고, bomb 타입 항목에는 없다. 오브젝트의 내용물은 그릇(vortex·factory·walker·balloon)의 값이 0이 아니어서 바이트가 없다.
* 내용물의 수량은 `saveQA`면 1바이트, `saveQB`면 부호 있는 16비트다. 변환한 타입이 0인 항목에서 읽기가 끊긴다.
* `Deck` 항목 가운데 변환한 타입이 0인 것은 덱에 넣지 않는다.
* 섹션 길이는 부호 있는 16비트로 읽는다. 로더는 이름 목록(35개)만큼만 읽는다.

## 검증

```powershell
python tools/cpp_fort_smoke.py     # 결과: extracted/cpp-fort-smoke/report.json
```

| 검사 | 패치판 폴더 | CD판 폴더 |
|---|---|---|
| `.fort` 파일 (낱개 + 아카이브) | 465개 | 23개 |
| 오브젝트 레코드 | 313,712개 | 6,922개 |
| 기존 Python 판독기와 비교한 줄 | 319,027줄 | 7,142줄 |
| C++ 타입 표와 해시가 완전히 같은 `TypeNames` | 패치 표 29개, CD 표 415개 | CD 표 9개 |

* **구조 비교**: 모든 파일의 머리 값, 소유자·이름, Storm Power, 기술 목록, 덱, 섹션별 청크·오브젝트·선택 필드·내용물을 기존 `tools/fort.py`(독립 Python 판독기)의 결과와 줄 단위로 비교했다. 모두 같다.
* **타입 번호 체계**: 패치판 타입 표의 해시 188개가 타입 188개로 저장된 파일 29개의 `TypeNames`와 완전히 같다. CD판 타입 표의 해시 171개는 `originals/d`의 낱개 파일 415개(10.72 계열로 저장된 것)와 CD 아카이브의 9개와 완전히 같다. 나머지 파일은 더 오래된 빌드로 저장되어 타입 수(165~184)나 프로세스 타입 구성이 다르다.
* **타입 플래그**: 저장 형식을 정하는 비트와 플래그 단어가 직접 켜는 비트를 Python 판독기와 비교했다. 그룹·후처리가 켜는 비트는 Python 쪽에 없어 비교하지 않았다.
* **미션 머리 값**: TEST01, 캠페인 1-1(`thewarbegins`), `savetheisland`의 스크립트 경로·미션 종류·요새 경로·머리 값을 스크립트 글에서 직접 읽은 값과 비교했다.
* 원본 파일의 SHA-256이 그대로다.

단위 검사(`cpppj/tests/FortTests.cpp`)는 원본 파일 없이 합성 입력으로 번호 체계·플래그·변환·선택 필드·버전 0/1/2·내용물·오류 처리를 확인한다.

검사 명령:

```powershell
cpppj/build/bin/Release/NetstormCpp.exe --inspect-mission originals TEST01
cpppj/build/bin/Release/NetstormCpp.exe --inspect-mission originals thewarbegins
cpppj/build/bin/Release/NetstormCpp.exe --inspect-fort originals savetheisland
cpppj/build/bin/Release/NetstormCpp.exe --dump-types originals
cpppj/build/bin/Release/NetstormCpp.exe --dump-forts originalCD --cd
```

## 한계와 남은 일

* 타입 플래그와 `.fort` 읽기는 x86 실행으로 대조하지 않았다. 근거는 정적 대조, 독립 판독기와의 전수 비교, 실제 파일의 `TypeNames` 해시다. `0049c3b0`은 문법 분석기의 노드 구조를 인자로 받아 격리 실행이 어렵다.
* 그룹·후처리가 켜는 플래그 비트는 독립 비교 대상이 없다. `.fort` 전수 비교가 통과한 것은 소유자 바이트 유무가 맞다는 간접 근거다.
* `State`·`Mission`·`CoreData`·`Badges`·`CompressedData` 섹션은 원시 바이트로만 보존한다. `Territory`는 120바이트 원시 값이다.
* `TerrNN`의 청크가 월드의 어느 청크인지는 영역 배치([territory-layout.md](territory-layout.md))가 정한다. 아직 연결하지 않았다. `Chaff`는 청크 번호 i → x = i % 16, y = i / 16이다.
* 미션 종류 표 검색과 미션 객체 생성, 스크립트 명령 실행, AI·동맹·시작 자원의 적용은 옮기지 않았다.
