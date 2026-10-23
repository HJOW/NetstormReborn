# 기존 Fort Editor의 팔레트 전환 부모

단위384 root 통합에서 제품과 검사2개를 공용 등록했다. 최초7파일은 `extracted/cpp-fort-palette-transition-20261024-meaning-before`에 보존했다. 아래 최초 격리 기록의 ColorSort 명칭은 의미 해석 오류이며, 실제00569ffc/0056b7a0/00471040/00470b20은 Kencloud 구름 버퍼·이름·생성·행 갱신이다. 현재 제품의 관련 API/주석만 Cloud/Kencloud로 정정했고 최초 tool/fixture/native report는 바꾸지 않았다. 실제 PaletteShade ColorSort는 별개다. 공용 빌드/검사 결과는 [현재 통합 기록](cpp-editor-subscriber-runtime-20261024.md)에서 판정한다. 아래 미등록·미변경 표현은 최초 격리 단계의 이력이다.

현재 PC와 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 일치한다. 단위383 이후 연결을 위한 신규 자료이며 공유 제품/CMake/LEFT/GUI/git은 변경하지 않았다. 최초 PE fixture/tool/report를 제품보다 먼저 고정하고 그 뒤 새 C++ 모듈을 작성했다. 기존 동결 팔레트 후속 문서와 다른 원문 자료는 수정하지 않았다.

## 선택 원문과 호출 순서

| 함수 | 실제 10.78 범위(끝 제외) | 새 공개 API |
|---|---|---|
| 팔레트 이름 전환 114BYTE | `004b6100..004b6172` | `TransitionFortPalette` |
| ColorSort 캐시 반환 29BYTE | `00470c30..00470c4d` | `ReleaseFortPaletteColorSortCache` |
| ColorSort 이름·횟수 부모 59BYTE | `00471190..0047119d`, `004711a0..004711ce` | `SetFortPaletteColorSortName` |
| 현재 렌더러의 dirty 20BYTE | `004973c0..004973d4` | `MarkFortPaletteDirty` |

정밀 C의 전환은144739~144769행, 캐시는92581~92594행, ColorSort 부모는92927~92952행이다. CD10.72 대응 전환은 `00433250`, ColorSort 부모는 `004b0ac0`, dirty는 `00456f30`이다. 이번 실행/제품 대조는10.78만 대상으로 한다.

`004b6100`은 다음 순서를 소유한다.

1. `004c2630(29,"init palette\n")`을 호출한 **뒤** `004b610d`에서 입력 주소를 캡처한다. API의 `const span& argument`는 로그 경계가 현재 인자 변수를 바꿀 수 있게 한다. 캡처 뒤 같은 주소의 내용 변화는 계속 관측하지만 이후 인자 변수의 다른 주소는 재사용하지 않는다.
2. 현재 `005c8910`과 캡처 입력을 CRT `004e65ed`로 비교한다. 비교 결과가0이면 로그 외 나머지를 자연 생략한다. 대소문자만 다른 입력도 이 경계의 실제 CRT 규칙으로 결정한다.
3. 같은 입력 주소에서 `005c8910`으로 NUL 포함 **순방향 BYTE** 복사를 한다. 전체 길이 선계산·문자열 사본·`memcpy/memmove`·255자 절단을 넣지 않았다. 같은 물리가 겹치면 앞선 쓰기가 이후 읽기에 영향을 준다. 물리 span이 부족하면 그 전 BYTE 쓰기를 남기고 `out_of_range`로 중단한다. 원문의 의미상 길이 제한으로 해석하지 않는다.
4. 캐시 부모는 현재 `00569ffc`가 비영일 때 그 주소를 free한다. callback이 캐시를 다른 값으로 바꾸어도 **정상 복귀 뒤 같은 DWORD에0을 쓴다**. free가 중단하면0쓰기에 도달하지 않는다. 초기0이면 free와0쓰기 모두 생략한다.
5. ColorSort 부모는 **동일 캡처 입력 주소의 현재 내용**을 `0056b7a0`으로 NUL까지 복사한다. 현재 cache0일 때만 필수 `00471040`을 호출하고, 그 뒤 현재 `00540a80`을 한 번 캡처하여 `00470b20`을 그 횟수만큼 호출한다. step callback이 현재 count를 바꾸어도 캡처된 남은 횟수는 유지된다. 임의 반복 상한/음수 조기 반환은 없다.
6. `004410f0("GamePalSpec",같은 입력 주소,0,0)`이 돌려준 현재 NUL 경로를 캡처하여 **`004a4850(path,1)`**을 호출한다. `004b6148`의 push1은 PathSpec의 다섯째 인자가 아니며 LoadPalette의 둘째 인자이다. 경로 NULL/물리 부족을 빈 경로의 성공으로 바꾸지 않는다.
7. 필수 `0043c010` 추가 표 경계를 호출한 뒤 dirty 부모로 간다. **그때 현재** `0059a8bc`가 비영이면 `0059a8b0=1`,0이면 기존 dirty를 그대로 보존한다. maps callback이 renderer/dirty를 바꾸는 효과도 반영된다.

## 실제 소유자와 필수 연결

새 [헤더](../../cpppj/src/o/RawFortPaletteTransition.h)의 `FortPaletteTransitionGlobals`는 고정 물리 이름 버퍼 두 개와 실제 cache/count/renderer/dirty DWORD를 빌린다. 실제 Client 수명이 모든 콜백보다 길어야 하고 버퍼 위치/크기를 호출 도중 바꾸지 않아야 한다. 다른 bool/임시 숫자/세션별 복제값을 DWORD 참조로 연결하면 안 된다. fixture의256BYTE는 격리 물리 장치의 범위이며 원문의 문자열 의미상 제한을 추가한 것이 아니다.

`FortPaletteTransitionHooks`의 CRT/로그/free/`00471040`/`00470b20`/PathSpec/LoadPalette/maps는 모두 **도달했을 때 필수 외부 경계**다. 전체 hook 선검사는 없어 자연 생략되는 하위의 누락은 오류가 아니며, 도달한 빈 `std::function`은 `bad_function_call`을 그대로 전파한다. 모든 callback 예외는 삼키거나 되돌리지 않고 앞선 부분 효과를 남긴다.

기존 계산 몸체를 중복 구현하지 않는다. 현재 `Screen::LoadPalette`→`GamePalette`→`PaletteColors::BuildPaletteColorTable`→`PaletteShade::BuildGumpShadeMaps`, `PreviewColorMaps`/`SpriteColorMaps`/`ProductionColorMaps`, Client의 AudioTints/소프트웨어 커서/`UberGump::PaletteChanged`→월드·커서·생산·미니맵 소비자를 재사용할 수 있다. 현재 일반 `Client::LoadScenePalette`의 `DataDir\\name` 경로는 이 부모의 **GamePalSpec** 경계와 별개다.

**현재 실제 장치의 미연결 차이:** 기존 `Screen::LoadPalette`는 원문 둘째 인자0 경로다. 원문 arg2=1의 surface-present→Lock/FillRect(0,0,width,height,0)→unlock/`004a1fb0`·`004a46d0`·`004a16d0` 선행 효과를 아직 이 신규 부모에서 실행하지 않는다. `Screen::Clear`의 단순 호출이나 빈 콜백으로 이 차이를 성공 처리하면 안 된다. `loadPalette(path,1)`을 실제 장치에 연결·대조하고 기존 표 계산/소비자 갱신을 올바른 위치에 합성하는 작업은 후속이다. ColorSort 실제 할당·계산 두 하위의 소유권도 같은 Client 전역에 연결해야 한다. 이 독립 모듈의 통과는 실제 Editor 메뉴·화면 전환 완료가 아니다.

## 최초 독립 관측과 C++ 대조

- [원문 도구](../../tools/decomp_fort_palette_transition.py), [최초 fixture](../../cpppj/tests/fixtures/fort-palette-transition-x86.tsv), [최초 native 보고서](../../cpppj/fort-palette-transition-native.json)를 제품 전에 생성했다. 이후 생성기는 덮어쓰기를 거부하며 `--verify`는 원문/정밀 C/함수목록/helper/tool/fixture 최초 SHA만 확인한다.
- 독립338입력×FPCW `027f/037f` = **676관측**, 실제179294명령, 정밀도 차이0이다. 정상642/명시·물리 중단34이며 로그·비교·free·build·step1/2/3·경로·load·maps 및 이름 출력 물리 중단을 구별한다. `ffffffff` count는 세 번째 step에서 명시 중단한 경우만 실행했고 완전 반복 성공을 주장하지 않는다.
- 이름 같음/대소문자/빈 이름/210자/같은 버퍼·좌우 겹침/NUL 없는 물리/두 인자 주소/원문 입력 내용 변경/같은 캐시의 재쓰기/build 뒤 count 변경/step 뒤 count 변경/maps 뒤 현재 renderer/NULL 경로를 포함한다.
- [C++ 검사](../../cpppj/tests/FortPaletteTransitionTests.cpp)는 반환 또는 중단 원인, 현재11필드, 두 이름 앞 BYTE와 NUL 뒤 전체 물리·입력두개·경로두개, 모든 하위 trace를 생략 없이676개 대조한다. 별도 검사는 자연 반환이 미연결 하위를 요구하지 않는 점과 도달한 필수 Screen 누락의 예외/이전 부분 효과를 확인한다.
- fixture SHA256: `73e8d0b30bc93d1626b743f40fa81ca80504fc8f5f4c037e175520eee29dea31`.
- 최초 도구 SHA256: `42c55d50733ec629b44dfd50421a18e4318cbbce7f399518abc71656ae581eb0`.

격리 경로는 `extracted/fort-palette-transition-20261024-isolated`이다. 새 모듈·새 검사·새 Runner 세 파일만 MSVC x64 `/std:c++20 /EHsc /utf-8 /W4`로 컴파일하고 공유 build/lib를 사용하지 않는다. 최초 `build1.cmd`는 UTF8 주석에 대한 cmd 코드페이지 오류가 있었으나 새 exe는 생성되었고 `test1.log`는 **2검사0실패/종료0**이다. 최초 로그/스크립트는 보존했다. 같은 소스를 직접 명령으로 재컴파일한 **build2는 종료0/컴파일 경고·오류0**, 완료 뒤 새 exe의 **test2도2검사0실패/종료0**이다. 이 격리 콘솔은 원본 게임/OS 장치/GUI/파일 쓰기를 실행하지 않는다.

후속 공용 등록 시 새 `.cpp`/검사 파일을 추가하고 검사에 `NETSTORM_FORT_PALETTE_TRANSITION_FIXTURE="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/fort-palette-transition-x86.tsv"`를 제공한다. 이번에는 등록하지 않았다. Fury의 별도 정적 읽기 검토도 네 실제 PE 범위와 h/cpp의 주요 순서에서 명백한 오류를 찾지 못했으며, 이 검토는 추가 원문 실행이나 빌드가 아니다. 최종 파일 SHA/UTF8/AST/JSON/문서 링크 검증은 격리 `verification.json`에 기록한다.
