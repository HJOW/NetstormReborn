# Session 커서의 고정 색상 입력 접합 제안과 raw primary 경계

단위387 이후용 **미적용 사본**이다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon /2026-10-23이다. root386의 ColorSort15·장기 Core/Chat/Dot·seed/SP 연결 뒤 세 파일을 기준으로 만들었다. 기존 제품/CMake/LEFT/git·공용 build/lib·게임/GUI·원본 옵션에는 쓰지 않았다.

## 원본에서 확인한 서로 다른 입력

전용5종 `111/141/137/94/97`의 표는 현재 COL에서 직접 계산하는 표가 아니다. `0043b5a0`의 `0043b5d6`과 `0043b750`의 `0043b7d9`가 현재 `00555d88`을 읽고, 소스와 후보 양쪽의 `primary + index*56 +8`에 저장된 **double 명도**를 사용한다. 정밀 C48768~48926행도 같은 입력이다. 후보 번호는 정적 `005319e8` DWORD들이다. 일반 `0043ba00`/BuildStandardOwnerColorMaps는 이 RGB/HLS 검색을 사용하지 않으므로 바꾸지 않는다.

`0043be30`은 세 전용 건물의 고정 소스 목록을 `0043bbe0`에 전달하고, ThemeMode 부모 `0043bc40`은 받침 두 종류의 설치/해제를 고른다. 기존 [owner HSL 원문15관측](../../cpppj/tests/fixtures/owner-hsl-colors-x86.tsv)은 실제439de0와43b5a0/43b750을 실행한 근거이며 등록·ThemeMode 부모 전체의 새 실행 증거는 아니다.

커서의 명명색/흰색/노랑·good/bad 입력과 고정 검색 입력은 별개다. `0043c010`은 `0043c01d`/`0043c212`에서 primary를 읽고 그 저장 RGB로 변환표를 만든다. Destroy표는 현재 명명 빨강 DWORD `005b5e58`의 lowBYTE, LabelShadow는0이며, Illegal표는 고정 항목의 R×2.1(상한1)·기존 G/B 검색이다. 기존 RawCampaignCursorDisplay의 API는 명명색을 전달된 현재 팔레트에서, Preview/전용5표를 별도 fixed 입력에서 계산한다. 현재 Session `ConnectCursor`만 fixed 인수를 생략하고 COL을 전달해 독립 fallback을 선택하고 있었다. 뒤의 GameWorld SetPalette 갱신이 초기 설치 입력 누락 자체를 없애지는 않는다.

[정적 PE/정밀 C 기록](../../extracted/cpp-session-color-input-20261024-proposed/static-evidence.json)은 위8함수의 실제 범위·BYTE·분해와 보호 원문 SHA를 보존한다. 이번에 새 Native 실행/fixture 생성은 하지 않았다. 기존 [고정입력6관측](../../cpppj/tests/fixtures/colorsort-fixed-input-x86.tsv)과 캐시부모130관측·도구·보고서도 변경하지 않았다.

## 이번 최소 사본

[패치](../../extracted/cpp-session-color-input-20261024-proposed.patch) SHA는 `74ffc5366ff2e991ec47d2870decb90d9121e864fc638046de268f594ae529ab`다. 기존 제품3파일만 포함하며, 새 검사 초안은 별도 사본1파일이다.

| 제품 파일 | root386 기준 SHA256 |
|---|---|
| RawCampaignSession.h | `091c609502a7cc97bdb68dbf28b229499cc1a417329f4c209b035e7b9b37ea14` |
| RawCampaignSession.cpp | `5bb7c03fb059fe60bdb222085e4a39b38540cf544921322935baa2cc720fd09f` |
| CampaignClientServices.cpp | `e847d6baf298c5e4a77fa41e155b37bf6134b64beabcf9f1cd347b5b4d9e679d` |

서비스에 `shared_ptr<ClientColorSortInput> colorSortInput`과 `screenPalette()`를 추가한다. 실제 MakeCampaignClientServices는 같은 Client owner를 반드시 공급하고, getter는 호출 당시 `client.GetScreen().Palette()`를 조회한다. Screen 교체 전 span을 저장하지 않는다. Session의 서비스를 통한 shared owner는 연결/커서보다 오래 유지되고, 현재 palette span은 SetPalette 호출 안에서만 소비한다. Client와 Screen getter의 참조 수명은 기존 전체 Session보다 길어야 한다.

둘 중 하나라도 제공되면 둘 다 필요하다. 일부 누락과 미캡처 owner는 명시적으로 실패하며 COL 성공 fallback으로 바꾸지 않는다. 둘을 **모두 생략한 기존 독립 headless 검사**만 원래 COL/예약BYTE/검정·흰색 끝점 adapter를 유지한다. 실제 Client factory는 그 adapter를 선택하지 않는다.

새 [SessionCursorColorInputTests.cpp 초안](../../extracted/cpp-session-color-input-20261024-proposed/copy/cpppj/tests/SessionCursorColorInputTests.cpp)은 실제 Fury Session.Load·현재 getter의 명명색/문자 필드·같은 owner 주소/값/수명·부분 누락/미캡처 거부·기존 headless adapter를3검사로 나눈다. 장치 fixture는 기존 ThemeSessionServices를 그대로 재사용했다. 기존 검증된 명명색 검색으로 입력 배선을 비교하는 검사이며 새 독립 Native 기대값으로 표현하지 않는다. 공용 등록/실행은 하지 않았다. 사본 구문검증과 최종 공유 문맥 재조회는 [검증 기록](../../extracted/cpp-session-color-input-20261024-proposed/verification.json)에 기록한다.

사본 제품2cpp/신규 검사1cpp가 같은 사본 header로 MSVC14.51.36231 `/MD /W4 /WX /Zs` 구문검증 **종료0·경고오류0**이다. [최초 로그](../../extracted/cpp-session-color-input-20261024-proposed/syntax1.log)를 보존한다. obj/실행파일 생성·링크·검사 실행은 하지 않았다.

이 사본은 기존15소비자와 같은 팔레트/고정 Logical API를 초기 커서에 연결하는 최소 수정이다. Screen의 LoadPalette가 저장한 Colors와 별도의 SetPalette/flash 적용만 이뤄진 순간의 명명색 수명 전체를 새로 복원한 것은 아니다. Screen::Colors의 원문 전역 소비와 재계산 API를 완전히 통합하는 작업도 이3파일 패치에 가장하지 않는다.

## Building에 남긴 실제 필수 의존성

**ConnectBuilding의 lazy5종 표를 이번 패치에서 바꾸지 않았다.** 현재 ClientColorSortInput은 `logical_[256]`과9표만 소유하고 raw primary0x3800/저장 double을 갖지 않는다. RawColorSortStartup의 primary는 명시 외부 토큰 훅이며 실제 Client raw 소유자는 아직 없다. 따라서 존재하지 않는 필수 getter를 노출하거나, Logical/COL에서 명도를 다시 계산해 실제 primary를 빌렸다고 할 수 없다. 표가 우연히 같아도 이 경계는 남는다.

다음 선행 단위는 기존 실제 `439de0`/아래 헬퍼 재사용을 먼저 조사한 뒤, 원문 생성자의 전체56BYTE 항목 배치·미작성padding·연산/쓰기 순서를 독립 관측하는 것이다. 초기화된 raw 레코드에서 +8 double을 그대로 빌리는 API를 만들어야 하며, 그 뒤에만 Building lazy5종 계산이 같은 primary를 사용하게 바꾼다. 원문 제어어027f/037f의 필드 비트 차이와 제품 지원 정밀도를 각각 기록해야 한다. 최초6관측의037f 아홉째 명도표 차이를 기대값 교체로 숨기지 않는다.

raw 소유자/439de0·검색객체43a8e0/qsort·캐시부모와 등록/해제 수명 전체가 이 커서 사본으로 완료된 것은 아니다. 기존5종의 현재 COL 재계산은 **제품 잔여 결함**으로 명시해 유지한다. 이 상태에서 전체 Session 색상 동일성이나 Editor 완료를 주장하지 않는다.
