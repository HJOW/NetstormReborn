# 단위388 준비·미등록: Client primary 소유와 저장 명도 소비 사본

2026-10-24 사용자의 일시 중지 요청에 따라 **작성한 사본까지만 동결한 인계 기록**이다. 이 제안은 공용 제품에 적용하지 않았고, 신규 검사 작성·컴파일·링크·실행도 하지 않았다. 단위387의 공용 검증과 별개이며 적용 준비 완료를 뜻하지 않는다. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다.

## 현재 산출물과 확인한 범위

- 사본 patch: [`cpp-client-colorsort-primary-20261024-proposed.patch`](../../extracted/cpp-client-colorsort-primary-20261024-proposed.patch), SHA256 `40b4d5833871b7f87867bc569760b80954f7370229a43488bd11a80ee6eb03c9`.
- 최초 baseline과 수정 사본: [`before/`](../../extracted/cpp-client-colorsort-primary-20261024-proposed/before/), [`copy/`](../../extracted/cpp-client-colorsort-primary-20261024-proposed/copy/). 기존7파일과 신규 helper1파일만 포함하며 **8파일/14hunk**다.
- 최초 작성 기록: [`preparation.json`](../../extracted/cpp-client-colorsort-primary-20261024-proposed/preparation.json). 각 기존파일 before/after SHA와 줄끝을 기록한다. `ClientMain.cpp`의 CRLF를 보존했다.
- 실제 C:Temp 적용 기록: [`apply-c-temp.json`](../../extracted/cpp-client-colorsort-primary-20261024-proposed/apply-c-temp.json). `C:\Users\BUZZ\AppData\Local\Temp\NetstormReborn-colorsort-primary-20261024-wyos4lff\applied`에서 14hunk를 실제 적용했고 skipped0·8파일 전체가 제안 사본과 BYTE 단위로 같았다. git이나 공용 빌드를 사용하지 않았다.
- 중지 직전 현재 공유 baseline·사본·실제 적용·최초 자료 대조: [`pause-handoff.json`](../../extracted/cpp-client-colorsort-primary-20261024-proposed/pause-handoff.json). 생성기는 [`freeze_pause.py`](../../extracted/cpp-client-colorsort-primary-20261024-proposed/freeze_pause.py)이며 최초 기록은 덮어쓰지 않는다.

## 작성한 최소 접합

| 파일 | 사본의 변경 |
|---|---|
| `client/ClientColorSortInput.h/.cpp` | 같은 Client 소유자가 `RawColorSortPrimary` 한 개를 소유한다. `CaptureWithPrimary(logical)` 및 명시 초기물리 overload, `Primary()`를 추가했다. |
| `client/OwnerColorMaps.h/.cpp` | 기존 logical adapter를 유지하고 같은 raw primary의 `Lightness(index)`를 직접 읽는 overload를 추가했다. 전용5종의 기존 선택 순서는 공통 본문으로 옮겼다. |
| `client/RawCampaignSession.cpp` | Building 전용5종 lazy표에 서비스의 같은 primary를 공급한다. 실제 서비스가 있으나 primary가 없으면 `Primary()`가 명시 거부한다. |
| `client/CampaignClientServices.cpp` | 기존 `CurrentAllocationBytes` 본문을 새 공유 helper로 옮기고 기존 호출은 그대로 둔다. |
| `client/CurrentAllocationBytes.h` (신규) | 기존 실제 `malloc`→`memcpy`→`free` 본문을 `inline`으로 그대로 공유한다. 기본0 입력이나 별도 Client 소유자를 만들지 않는다. |
| `client/ClientMain.cpp` | `Run`의 기존 최초 `Capture(screen_->LogicalPalette())` 한 곳만 `CaptureWithPrimary`로 바꾼다. 화면 재생성·일반 팔레트 변경 경로에서 primary를 다시 만들지 않는다. |

실제 Client factory가 공급하는 `services.colorSortInput`은 단위387 Cursor 접합에서 이미 사용하는 같은 소유자다. 이 제안은 새 서비스/전역/RNG를 만들지 않는다. `Primary()`는 누락·미완성 raw를 COL 또는 현재 Screen RGB 성공으로 치환하지 않는다. 서비스를 둘 다 생략했던 기존 독립 headless 경로만 COL 명도 adapter를 유지한다. `Capture(logical)`도 독립 adapter로 남고 이전 raw를 해제하므로, 실제 Client 연결에서 이를 호출하는 실수가 숨겨지지 않는다.

raw의 초기 `0x3800`은 기존 할당 장치의 실제 물리 BYTE다. `RawColorSortPrimary`가 전체 초기값을 소유하고 각 레코드 `+52..55`의 미작성 padding을 보존한다. `CaptureWithPrimary(logical, allocationBytes)`는 정확한 물리 입력을 받는다. 인자 없는 overload는 같은 기존 helper로 초기값을 얻고 명시 overload에 전달한다. 실제 constructor 정상 반환 뒤에만 primary를 발행한다.

같은 레코드의 **`+8`에 저장된 double L**을 `Lightness`가 `memcpy`로 읽는다. 전용 타입111/141/137/94/97은 이 값을 직접 빌리며 RGB/COL에서 L을 재계산하지 않는다. 고정 논리 RGB와 현재 Screen의 명명색은 계속 별개다. 기존9표 `Rebuild`는 기존 계산 경로를 유지한다.

## 근거와 엄격한 한계

원문439de0의 독립 복원은 [기존 primary Native 기록](cpp-colorsort-primary-native-20261024.md)에 고정돼 있다. 최초80행·027f/037f 두 정밀도·미작성 padding·부분 중단 및 독립 제품3검사 결과를 이 사본에서 바꾸지 않았다. 기존 고정입력6행과 Owner HSL15행 자료도 그대로이며 새 기대값을 생성하지 않았다.

**이 사본의 신규 Client/Session 동작은 아직 검사하지 않았다.** 위의 C:Temp 적용 검증은 patch와 BYTE 확인이고 C++ 컴파일이나 실행 확인이 아니다. 기존 raw 제품3검사 통과를 이 사본의 통합 검사 통과로 세면 안 된다.

`CaptureWithPrimary`의 인자 없는 overload는 초기 할당 BYTE를 얻은 뒤 명시 overload에서 이전 primary를 비운다. 현재 제안은 **첫 Client 시작 캡처** 접합이며 원문43b430의 재진입·해제·검색객체 생성 전체 순서를 복원했다고 주장하지 않는다. 검색0x5008, qsort, `!color.dat` 전체 부모, 전체 ColorSort 재생성/캐시 수명은 별도 미연결이다. 기존9표의 개별 RGB/HLS 재계산 역시 이 제안에서 raw 기반 계산으로 일괄 변경하지 않았다.

제품 raw 생성자의 지원은 CW027f/53비트 범위다. CW037f의 H/S 레코드 차이와 기존6행의9번째 명도표 차이는 원문 증거로 보존하며 기대값을 바꾸거나 지원 성공으로 합치지 않는다. 실제 Win32 화면 재생성·팔레트2회 변경·사용자 게임 경로는 이번에 실행하지 않았다.

## 재개 시 남은 작업

1. `pause-handoff.json`의 현재 공유 SHA와 새 HEAD를 먼저 비교한다. 다른 단위가 바꾼 `RawCampaignSession.cpp`·`CampaignClientServices.cpp`·`ClientMain.cpp`는 새 문맥에서 재검토하고 별도 새 patch로 인계한다. 현재 동결 사본과 최초 기록을 덮어쓰지 않는다.
2. 신규 독립 검사를 작성한다. 명시 초기 `0x3800` 및 padding, 최초80행의 정상027f 레코드, 미캡처/legacy adapter의 `Primary()` 거부, 실제 초기 할당 helper 경로를 검사한다.
3. 같은 Client owner를 실제 두 Screen 객체의 수명에 연결하고 두 팔레트 변경 뒤 raw 주소·전체 BYTE·저장 L이 그대로인지 확인한다. 창/장치를 만들지 않는 Screen 검사와 실제 GUI 화면 재생성 검증의 범위를 구별한다.
4. 기존 Owner15행과 전용5종을 재생한다. raw `+8`만 명시 변경한 장치에서 logical RGB를 유지해, 새 overload 결과가 저장 L을 직접 읽으며 기존 logical adapter는 기존 결과를 유지하는지 확인한다. 실제 Session의5종 lazy표 경로는 별도 Client/Session 통합 검사로 확인한다.
5. 실제 C:Temp 적용본을 `/std:c++20 /EHsc /utf-8 /MD /fp:precise /W4 /WX`로 격리 컴파일·링크·실행한다. 공용 build/lib는 사용하지 않는다. 기존9표6행·Owner15행·primary80행을 바꾸지 않고 재검사한다.
6. 그 결과를 보고 root가 공용 적용/CMake 등록/기존 검사 및 실제 사용자 진입 검증을 담당한다. 현재 제안을 성공으로 가장해 먼저 등록하지 않는다.

공유 제품·원본/CD·AGENTS·LEFT·공용 CMake/build/lib·git·GUI는 이번 사본 작업에서 변경하거나 실행하지 않았다. root가 사용자 요청에 따라 현재387의 검증·인계·커밋을 마친 뒤 목표를 일시 중지한다.
