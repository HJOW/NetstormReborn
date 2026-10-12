# cpppj StyleText 문자열 조각·짧은 명령·측정/출력 연결

작업일 **2026-10-12**, PC **HJOW-Athlon / 192.168.0.94**. AGENTS.md·두 LEFT_JOBS를 읽고 같은 PC의 정밀 디컴파일 기록을 확인했다. 두 정밀 Ghidra 프로젝트에서 함수 **16/16개**를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-12.**

## 구현 범위와 원본 주소

[StyleText.h](../../cpppj/src/client/StyleText.h)·[StyleText.cpp](../../cpppj/src/client/StyleText.cpp)를 추가했다. 문자열 조각의 NUL 복사·소멸·복제, 단방향 소유 체인, 짧은 `~` 명령, 두 번 순회하는 전체 측정/출력을 구현했다. 다중 줄 버퍼·자동 줄바꿈·스크롤·TextGump/Form 전체 생성은 이번 범위에 포함되지 않는다.

| 원본 몸체 | 10.78 | CD 10.72 | C++ |
| --- | --- | --- | --- |
| 기본 조각 초기화 | `004755c0` | `004504a0` | 새 조각의 next/문맥 초기화 |
| 체인 출력 | `004755e0` | `004504c0` | `Draw` |
| 체인 삭제 / 단일 추가 / 소유 이전 | `004756b0` / `004756e0` / `00475720` | `00450590` / `004505c0` / `00450600` | `Clear` / `Append` / `Transfer` |
| 체인 측정 | `00475760` | `00450640` | `Measure` |
| 문자열 조각 생성 / 소멸 / scalar 삭제 | `004b93c0` / `004b9490` / `004bbe20` | `004b4de0` / `004b4ed0` / `004b7e50` | `NewText` / `Destroy` |
| 조각 기준선 / 측정 / 출력 | `004b94f0` / `004b9510` / `004b9530` | `004b4f50` / `004b4f80` / `004b4fa0` | `Baseline` / `MeasureFragment` / `DrawFragment` |
| 문자열 조각 복제 | `004bbda0` | `004b7dc0` | `CloneText` |
| 스타일 문자열 파서 | `004bb430` | `004b7770` | `Parse`의 짧은 명령·구간 분리 |
| 캐시 메트릭 / 다음 명령 검색 | `004a3ed0` / `004e63e0` | `00426060` / `004f1bb0` | `GumpText::Metrics` / 문자열 검색 |

문자열 조각의 실제 가상 표는 패치 `005143cc`, CD `00505d48`이다. 다섯 슬롯은 scalar 삭제·기준선·측정·출력·복제 순서다. 일부 디컴파일 함수의 인자 수·스택 임시값이 손상되어 있어 두 PE의 실제 명령과 호출 인자로 보완했다. `extracted/styletext/{originals,originalCD}/assembly.txt`에 주소/바이트/명령을 보관했다.

## 소유와 저장소

`StyleTextState`의 map은 안정된 조각 주소를 제공하며 0을 null로 사용한다. 토큰은 호스트 식별자이고 원본 메모리 주소/Gump 등록 ID와 별개다. 원본 +4의 next, +8의 용도 미확정 DWORD, +c의 소유 문자열, +10의 문맥 8 DWORD를 의미별 필드로 옮겼다. 메뉴의 `items[index].styled`가 실제 head를 소유하며 별도 head 복사본을 유지하지 않는다.

`NewText`는 명시 길이를 복사하고 NUL을 하나 붙인다. 길이 `ffffffff`는 범위 안의 첫 NUL까지 복사한다. 명시 길이에는 중간 NUL도 복사할 수 있지만 실제 측정/출력은 첫 NUL에서 멈춘다. `Append`는 단일 조각을 현재 꼬리에 붙인 다음 그 조각의 next를 0으로 만든다. `Transfer`는 source 체인 전체를 이전하고 source를 0으로 만든다.

`Clear`는 삭제 전에 다음 토큰을 저장한다. 텍스트 소멸은 소유 문자열과 next를 비운다. scalar 플래그 `1`일 때 반납 경계 후 map에서 제거한다. 삭제 콜백이 head를 바꿔도 마지막에 현재 소유 필드를 0으로 쓴다. 파생 그림 조각의 자원 해제는 별도 경계다. 소유자는 처리기/폰트가 살아 있는 동안 각 체인을 `Clear`해야 한다.

복제의 실제 호출 인자는 **새 생성자의 길이에 원본 +8 DWORD, 새 +8에 0**을 전달한다. 문자열 전체 복사로 단순화하지 않았다. 기본 파서 조각의 +8=0이면 복제 문자열이 빈 문자열이다. 이 용도 미확정 필드의 원래 이름/의미는 추정하지 않는다. 이후 줄 버퍼 복제를 복원할 때 해당 인자를 독립 동작 자료와 다시 대조해야 한다.

배열 밖 문자열 길이·null/미등록 토큰·순환 체인·같은 소유 필드로의 이전은 예외로 거부한다. 토큰 발급은 기존 외부 조각을 덮어쓰지 않는다. 보호 밖 원본 메모리 손상·assert 이후 진행과 allocator/SEH의 원본 바이트 배치는 재현하지 않는다.

## 짧은 스타일 명령

`Parse`는 기존 head를 지우지 않고 새 조각을 추가한다. 메뉴 연결에서는 먼저 원본 Clear를 수행한다. `~` 앞 구간을 당시의 문맥으로 저장한 뒤 명령을 적용하며, 마지막 구간은 빈 문자열이어도 조각으로 남긴다.

| 명령 | 처리 |
| --- | --- |
| `~0`, `~1`, `~3`~`~6` | font 슬롯 변경; `~2`는 명령이 아님 |
| `~B`, `~I` | 굵게/기울임 스타일 |
| `~S`, `~U` | 취소선/밑줄; 10.78 전용 |
| `~N` | 스타일 0, 그림자 비트 `2`만 제거 |
| `~E` | 그림자 비트 추가, 검정색, x/y 각 1 |
| `~*` | flags만 0으로 지정 |
| `~.` | 처음 문맥 8 DWORD 전체 복귀 |
| `~n` | 처음 본문 색만 복귀 |
| `~~` | 당시 문맥의 별도 `~` 문자열 조각 |
| `~b/g/i/j/k/l/m/o/q/r/t/w/x/y` | 원본 공통 색 전역 |
| `~a/d/p/s/u` | 10.78 추가 색 전역 |

모르는 명령은 `~`만 소비하고 다음 문자는 리터럴 구간에 남긴다. CD의 `~S/~U`와 추가 색도 이 경로를 따른다. 닫는 `]`가 없는 `~[`는 두 접두 문자를 소비하고 이어지는 문자열을 보존한다.

**이 단계에서 닫는 `]`가 있는 `~[...]` 내부는 그림/옵션 명시 경계였다.** 같은 날 [대괄호 명령 후속](cpp-style-text-markup.md)에서 그림/태그/색/그림자/소리 해석을 구현하고 현재 문맥/head/공통 태그를 훅에 함께 전달하도록 확장했다. 훅이 없으면 해당 위치에서 거부하며 앞서 추가한 조각은 남는다. 아래 빌드/근거는 문자열 조각 구현 당시 기록이다.

`BindStyleTextPalette`는 실제 `PaletteColorTable`의 기본 별칭과 원본 RGB 검색 결과를 공급한다. `~x`는 RGB(47,24,17) 전역이다. 메뉴 비활성 라벨의 `~*~x`는 기존 문맥의 flags를 지우고 해당 색을 쓰게 된다. 별도 임시 팔레트를 만들지 않는다.

## 기준선·전체 측정·출력

새 `GumpText::Metrics`는 선택 캐시의 **높이/상승/하강**을 반환한다. 측정의 별도 논리 슬롯 높이와 구분한다. 그림자 여백은 더하지 않는다. 스타일 범위 밖은 normal 슬롯 0, 원본이 비운 캐시 조합은 0이다. CD의 스타일 제한/빈 6번 Bold도 유지한다.

전체 `Measure`는 먼저 기준선 메트릭을 순회해 최대 상승 `A`를 구하고 **현재 소유 head**부터 두 번째 순회를 시작한다. 각 조각에서 기준선 메트릭을 다시 읽고 실제 폭/높이를 측정한다. 최종 높이는 `max(현재 높이, 조각 높이 - 조각 상승 + A)`, 폭은 순서대로 합산한다. signed 덧셈/뺄셈은 x86 하위 DWORD 감김을 유지하며 출력 참조가 같아도 높이→폭 쓰기 순서를 보존한다.

CD는 출력 0 초기화 전에 첫 head를 읽고, 패치는 초기화 뒤 읽는다. 두 번째 순회에서 CD는 출력 쓰기 전에 next를 저장하고 패치는 쓴 뒤 조회한다. 출력이 소유 필드/next와 겹칠 수 있는 경우의 시점을 실제 명령으로 구분했다. 정상 UI에서는 출력이 스택의 별도 정수다.

전체 `Draw`는 입력 head를 값으로 저장한다. 첫 순회에서 최대 캐시 높이와 상승을 구한 뒤 같은 시작 토큰에서 두 번째 순회를 시작한다. 조각의 y는 `기준 y + A - 조각 상승`, 다음 x는 **실제 측정 폭**을 더한 값이다. 가상 호출 후 현재 next를 읽는다. 반환값은 첫 순회의 최대 메트릭 높이이며 그림자 측정 높이와 다를 수 있다.

출력 override가 있으면 문맥 8 DWORD를 복사한 뒤 **font 슬롯만 조각의 저장 슬롯**으로 바꾼다. 배치 측정에는 원래 저장 문맥을 사용한다. `BindStyleTextFonts`는 실제 FontStore 메트릭·첫 폭 표 측정·`GumpTextDraw`의 글리프 출력/현재 표면에 연결한다. 그림 등 파생 조각의 가상 메트릭/측정/출력은 별도 훅을 공급한다.

## 메뉴 연결과 사용 순서

`BindMenuLayoutStyleText`는 같은 메뉴 항목의 styled head를 Clear/Parse/전체 측정에 넘긴다. 기존 `MenuGumpLayout::SetText`의 문맥 복사/font=0·Clear·현재 disabled 접두 처리 순서를 그대로 사용한다. 일반 라벨/자식 표시 문자는 기존 `BindMenuLayoutText`를 사용한다.

아래는 처리기를 구성하는 순서다. 관련 객체와 상태는 메뉴 처리기보다 오래 살아야 하며 각 훅은 처리기를 생성하기 전에 설정한다.

```cpp
// 실제 폰트/표면/팔레트와 같은 조각 저장소를 연결한다.
StyleTextState styleState;
StyleTextHooks styleHooks;
BindStyleTextFonts(styleHooks, gumpText, &gumpTextDraw);
BindStyleTextPalette(styleHooks, palette);
StyleText styleText(styleState, std::move(styleHooks), edition);
// 기본 문자열과 스타일 문자열을 같은 메뉴 저장소에 공급한다.
BindMenuLayoutText(menuHooks, gumpText);
BindMenuLayoutStyleText(menuHooks, styleText);
MenuGumpLayout layout(createState, menu, std::move(menuHooks));
```

## 빌드와 남은 대조

게임 라이브러리/`NetstormCpp` **Release 빌드 경고 0·오류 0**. SOURCE_MAP은 생성 도구로 갱신해 StyleText 파일 존재를 표시했다. 파일 존재 표시는 전체 원본 StyleText 모듈 복원 완료를 뜻하지 않는다.

**이번 단계는 정적 복원이며 새 테스트 추가/실행·전체 CTest·독립 PE 동작 대조·GUI 실행은 미수행**이다. 앞 단계의 원본 관찰 누적 **523,634개**, C++ 대조 누적 **523,374개**, **854개 통과** 수치는 이번 변경을 검증한 수치가 아니다.

자료: `cpppj/recovery-styletext-evidence.json`, `tools/analyze_styletext.py`, `tools/ghidra/styletext-functions.json`, `extracted/styletext/`, `extracted/styletext-*-export.log`, `extracted/styletext-build.log`. 분석 기록에는 두 바이너리·내보내기·실제 가상 표·파서 case·현재 구현의 SHA가 있다. 원본 게임/에뮬레이터/창을 실행하지 않고 다음 명령으로 정적 자료와 빌드를 재현한다.

```powershell
python -X utf8 tools/analyze_styletext.py
cmake --build cpppj/build --config Release --target netstorm NetstormCpp --parallel 2
```

다음은 이번 조각/파서/메뉴 연결의 독립 동작 대조, 대괄호 옵션/그림 조각·다중 줄 버퍼·줄바꿈·스크롤·TextGump/Form·MenuGump 가변 인자 생성/장식/제목/소멸·Attach 보정·입력/통지다. 이후 OS 큐/폴링·overlay·Client 자동 UI 프레임을 이어서 연결한다. **Client/UberGump는 기존 메뉴 어댑터를 사용하며 새 raw 메뉴의 자동 GUI 연결과 게임 내 건설/미션 완주는 미완료다.**

원본 바이너리 SHA가 기존 기록과 같다. AGENTS.md·보호 원본·dotnetpj 변경, 커밋/푸시 없음.
