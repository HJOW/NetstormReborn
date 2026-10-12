# cpppj StyleText 대괄호 그림·태그·색·그림자·소리 명령

작업일 **2026-10-12**, PC **HJOW-Athlon / 192.168.0.94**. AGENTS.md와 두 LEFT_JOBS를 읽고 같은 PC의 정밀 디컴파일 기록을 확인했다. 이번 단계에서 이 PC의 창 작업/게임 실행을 허용한 사용자 지시를 반영했다. 작업은 정적 복원과 빌드로 진행했으며 게임/GUI를 실행하지 않았다. 두 정밀 프로젝트에서 함수 **16/10개**를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-12.**

## 구현과 실제 원본

[StyleTextMarkup.h](../../cpppj/src/client/StyleTextMarkup.h)·[StyleTextMarkup.cpp](../../cpppj/src/client/StyleTextMarkup.cpp)에 `~[...]` 내부 처리를 추가했다. 기존 [StyleText](../../cpppj/src/client/StyleText.h)와 [Markup](../../cpppj/src/client/Markup.h)의 공통 소유 체인·실제 타입/SHP·배율 출력·팔레트·설정·소리 계층에 연결한다.

| 근거 | 10.78 | CD 10.72 |
| --- | --- | --- |
| StyleText 전체 파서 | `004bb430` | `004b7770` |
| 정규식 초기화 / 검색 / 캡처 | `00496c90` / `004964e0` / `00495a10` | `00463650` / `00463680` / `004636b0` |
| 이름으로 타입 조회 | `0049a860` | `00443410` |
| 타입의 .type 로딩 | `0049c3b0` | `004460f0` |
| 숫자 변환 | `_atol` `004e66de` | `_atoi` `004f1ea0` |

패치의 빠진 정적 초기화 여섯 함수 `004fed30/50/70/90/b0/d0`도 읽기 전용으로 내보내 실제 ROM 정규식 주소를 확인했다. 그림 선택/클램프와 공통 태그는 디컴파일이 누락한 인자·대입을 실제 명령에서 확인해 옮겼다.

## 형식과 검색 순서

패치는 **I → S → T → C → B → E**, CD StyleText는 **I → T** 순서로 검사한다. 한 대괄호에서는 첫 성립 형식 하나만 처리한다. 대응 옵션이 0이어도 이미 성립한 형식을 소비하며 뒤 형식으로 넘어가지 않는다. 모르는 형식은 문맥/태그/체인을 바꾸지 않는다.

| 명령 | 동작 |
| --- | --- |
| `I타입.프레임` | 이름/번호로 타입을 고르고 그림 조각 추가 |
| `S.파일이름` | allowzonesounds가 비0이면 이름 재생 요청 |
| `T숫자` | 이후 조각의 공통 +8 태그 갱신 |
| `C.숫자` | zonefgcolor가 비0이면 문맥 +8의 전경 색 번호 갱신 |
| `B.숫자` | zonebgcolor가 비0이면 문맥 +14의 그림자 색 번호 갱신 |
| `E.x.y` | zoneoffset가 비0이고 조건을 만족하면 그림자 flags/검정색/x/y 갱신 |

원본 정규식은 시작/끝 앵커가 없고 **대소문자를 구분하는 부분 검색**이다. 공백 반복은 ASCII space만 소비한다. 캡처의 제외 클래스는 dot/CR/LF/space이며 **탭은 캡처에 포함**된다. 정규식의 점은 이스케이프되지 않아 임의의 NUL 아닌 BYTE를 소비한다. CR/LF도 점이 소비할 수 있다. 각 반복은 가장 긴 길이부터 되돌아가며 성립하는 첫 조합을 선택한다.

C++는 이 여섯 단순 형식에 필요한 검색/캡처 순서를 직접 구현했다. 전체 Regexp 엔진을 복원한 것은 아니다. 캡처 조회는 원본의 `Get(...,80,0)`처럼 첫 80바이트를 사용한다. 원본 그림 프레임 식은 이를 80바이트 지역 배열에 NUL까지 복사하므로 80바이트 캡처는 호스트에서 범위 오류로 거부한다.

## 그림 타입·프레임과 현재 태그

타입 캡처의 숫자 변환 결과가 0이면 같은 문자열을 원본 타입 이름 표에서 대소문자 무시 검색한다. -1일 때만 그림 생성을 생략한다. 호스트 타입 표 밖 번호는 실제 자산 경계에서 거부한다. 숫자는 기존 `ConfigParseLong`의 공백/부호/첫 숫자 구간/DWORD 감김을 사용한다.

- `*`: gumpFrame을 사용하며 -1이면 defaultFrame으로 복귀한다.
- `#`: 같은 프레임에 **65×60 상자 맞춤**을 적용한다.
- `&`: 같은 상자 맞춤과 **색 치환 BYTE 1**을 함께 적용한다.
- 글자 식: 첫 글자가 signed 비교로 `A <= 글자 < A + letterKinds`이면 해당 글자 구간의 첫 프레임에 숫자 suffix를 더한다. 글자 구간 수는 실제 존재 글자의 수이며 없는 중간 글자를 자동 건너뛰지 않는다.
- 그 외: 숫자 식을 프레임으로 사용한다. 최종 클램프의 상한은 **count-1이 아닌 count**다. count 프레임은 이후 Markup 조회에서 무자산으로 처리된다.

패치의 타입 helper 재조회 순서를 유지한다. 그림은 현재 태그로 생성한 뒤 mapped BYTE → 필요하면 Fit → 공통 꼬리 추가 순서다. Fit은 기존 판본별 float32/NaN 규칙을 사용하고 출력은 직전 복원한 회전 없는 VFX 배율 경로를 사용한다.

`StyleText::Parse`의 지역 태그는 매 호출 0에서 시작한다. `T` 뒤 문자열·그림·리터럴 `~~`·마지막 빈 문자열 조각에도 전달된다. **`~.`는 문맥만 복구하고 태그를 초기화하지 않는다.** 기존 +8의 용도를 공통 T 태그로 확인했으며, 기존 원본 문자열 Clone의 +8을 복사 길이로 넘기는 호출은 그대로 유지한다.

`AppendStyleTextFragment`로 문자열 처리기와 그림 파서가 같은 저장소의 꼬리 조회/next=0을 공유한다. 태그/head를 대괄호 훅에 참조로 넘겨 별도 소유 목록을 만들지 않는다.

## 기존 타입 로더 수정

두 원본 .type 로딩은 **+11c gumpFrame을 -1로 먼저 초기화**한다. 이전 `RiftTypeDefinition::Parse`는 이를 0으로 두어 `gumpframe` 미지정 그림이 첫 프레임으로 선택되었다. [TypeParser.cpp](../../cpppj/src/o/TypeParser.cpp)에 -1 초기화를 추가했다. 원본 빈 내장 타입의 0 초기값은 `TypeSpecialFrames`의 기본 생성에 남긴다.

기존 `AssetTests`의 해당 기대값 한 곳을 **0 → -1**로 수정했다. 새 테스트는 추가하지 않았으며 기존 테스트도 실행하지 않았다.

## 색·그림자·설정·소리 연결

E는 **x 또는 y 중 하나만 -4~4에 있으면** 두 값을 함께 저장한다. 양쪽 모두 범위 밖이면 변경하지 않는다. 조건을 통과하면 flags에 2를 더하고 현재 기본 검정색/입력 x/y를 저장한다. B는 색 번호만 바꾸며 그림자 비트를 추가하지 않는다.

`StyleTextMarkupState`의 zone 허용 기본값은 실제 패치 PE에서 읽은 1이다. `LoadStyleTextMarkupOptions`는 원본 Client의 allowzonesounds/zonebgcolor/zonefgcolor/zoneoffset 설정으로 존재하는 키만 갱신한다. `BindStyleTextMarkupPalette`는 기본 검정 별칭을 공급한다. `BindStyleTextMarkupSound`는 기존 `SoundPlayer::PlayByName(name,0,0,0,0,0)`에 연결한다. 소리 장치/실제 재생의 초기화는 해당 기존 계층이 담당한다.

구성 예시는 다음과 같다. 자산/설정/표면/팔레트/소리·공통 조각·그림·대괄호 처리기는 StyleText 훅보다 오래 살아야 한다.

```cpp
// 공통 조각과 기존 Markup을 사용해 그림 선택 경계를 구성한다.
StyleTextMarkupState markupState;
StyleTextMarkupHooks markupHooks;
BindStyleTextMarkupAssets(markupHooks, assets);
BindStyleTextMarkupPalette(markupHooks, palette);
BindStyleTextMarkupSound(markupHooks, sound);
LoadStyleTextMarkupOptions(markupState, configuration);
StyleTextMarkup markup(styleState, pictures, markupState, std::move(markupHooks), edition);
// 기존 글꼴/팔레트/그림 가상 분배 뒤에 대괄호 훅을 연결한다.
styleHooks = markup.Hooks(std::move(styleHooks));
StyleText styled(styleState, std::move(styleHooks), edition);
```

## 빌드·분석 자료·후속

게임 라이브러리/`NetstormCpp` **Release 빌드 경고 0·오류 0**. SOURCE_MAP 생성/내용 차이 없음. **새 테스트 추가/실행·전체 CTest·독립 원본 동작 대조·게임/GUI 실행은 미수행**이다. 원본 관찰 **523,634개**, C++ 대조 **523,374개**, CTest **854개 통과**는 앞 단계 기록이며 이번 변경의 검증 수치가 아니다.

자료: `cpppj/recovery-stylemarkup-evidence.json`, `tools/analyze_stylemarkup.py`, `tools/ghidra/stylemarkup-functions.json`, `extracted/stylemarkup/`, `extracted/stylemarkup-*-export.log`, `extracted/stylemarkup-build.log`, `extracted/stylemarkup-final-build.log`. 보고서는 실제 ROM 정규식/옵션 기본값/맞춤 크기/함수 범위/명령과 현재 코드 해시를 기록한다. 앞 단계 StyleText/Markup 보고서는 당시 코드의 기록이며 이번 변경은 새 보고서를 따른다.

후속은 이번 명령·공통 태그/타입 초기값·직전 VFX 배율·StyleText/메뉴의 독립 원본/C++ 대조, 원본 그림 변환 표 공급, TextGump의 W 명령/다중 줄/줄바꿈/스크롤·Form/메뉴 생성·입력/자동 UI 연결이다. **Client/UberGump는 기존 메뉴 어댑터이며 raw 자동 GUI와 건설·미션 완주는 미완료다.**

원본 두 PE SHA는 기존 기록과 같다. AGENTS.md·보호 원본·dotnetpj 변경, 커밋/푸시 없음.
