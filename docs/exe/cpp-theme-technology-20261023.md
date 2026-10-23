# 10.78 테마 기술 지급 본체 복원 — 단위376 독립 모듈

현재 PC 및 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 같다. AGENTS.md와 공통·닷넷 인수인계를 읽고 기존 정밀 원문 및 보호 PE를 읽기 전용 사용했다. 정밀 디컴파일 산출물, AGENTS.md, originals/originalCD를 변경하지 않았다.

## 원본 계약

대상은 [10.78 정밀 원문](../../extracted/refined/originals/Netstorm.c)의 `FUN_00492ae0`(117801행 부근), **00492ae0–00492b5f의 128바이트**다. 명령 이름 CheatAllTechnology 및 명령02000028의 실제 훈련 진입 문제는 [진입 감사](cpp-solo-entry-audit-20261023.md)에 있다. 이 모듈은 그 명령의 지급 본체이며 큐 분배와 안내 부모 수명은 호출자에 속한다.

1. signed 타입 개수00541348이 양수일 때 타입 **0부터** 0049a840으로 번호순 기록을 조회한다.
2. group(+9c)가10인 타입을 제외한다. theme(+98)가5인 타입도 선택 theme가5일 때까지 항상 제외한다. 남은 항목은 선택 theme와 같아야 한다.
3. CurrentTechnology0048fc60을 호출한 결과 SID를 현재 Pool005c8464에서 부모 주소로 바꾸고, **FindContained004acac0(parent,4,type,0)**을 호출한다.
4. 결과가0이면 **CurrentTechnology를 다시 호출**하고, **Create004afa70(parent,type,0,4,0x10,0)**을 호출한다. 첫 조회의 부모를 캐시하지 않고 생성 반환값은 사용하지 않는다.
5. 매 반복 끝에 현재00541348을 다시 읽고 다음 타입과 signed 비교한다. 하위 callback이 개수를 바꾼 효과를 반영한다.

실제 명령은 00492ae0의 첫 count 읽기, 00492af2의 타입 조회, 00492b14/00492b34의 서로 다른 현재 부모 조회, 00492b2b의 Find, 00492b4e의 Create, 00492b53의 매 반복 count 재조회로 확인했다. 본체에는 Tell/창 Kill/OnExit 호출이 없다. 명시 owner를 받는 기존 RawPlayerTechnology::Ensure/Find는 별도 타입 범위 assertion 및 owner getter 경로이므로 이 본체를 대신하지 않는다.

## 제품 API와 연결 경계

[RawThemeTechnology.h](../../cpppj/src/o/RawThemeTechnology.h), [구현](../../cpppj/src/o/RawThemeTechnology.cpp)의 `RawThemeTechnology(ThemeTechnologyHooks)`와 `Apply(int32_t theme)`를 제공한다. 다섯 hook은 모두 필수이며 누락을 빈 성공으로 처리하지 않는다.

| hook | 실제 연결할 경계 |
|---|---|
| typeCount() → int32_t | 현재 실제 타입 개수 |
| typeAt(int32_t) → const RiftTypeRecord& | 실제 번호순 타입 기록; 0번 허용 |
| currentTechnology() → Sid | 현재0048fc60; callback 뒤 재조회 허용 |
| findContained(parent,mask,type,index) → Sid | 같은 Pool의 실제 RawCarrierPreDestroy::FindContained |
| createContained(parent,type,argument,flags,attachFlags,quality) → Sid | 같은 Pool의 실제 RawContainedCreate::Create |

이 어댑터는 별도 기술 목록·Player 저장소·소유자 캐시를 만들지 않는다. typeAt 참조의 group/theme는 현재 부모나 contained callback 전에만 읽는다. 타입 개수는 고정 span 길이로 캐시하지 않는다. 원본의 음수/0개수는 아무 타입도 조회하지 않고 끝난다. 타입 범위/PlayerKnown의 별도 assertion을 새로 추가하지 않으며 실제 getter에 있는 진단은 그 getter가 담당한다. 외부 작업자는 실제 RawCampaignSession 연결과 명령 큐·대화 수명을 작업하므로 이 문서의 독립 검사만으로 해당 GUI 연결 완료를 주장하지 않는다.

## 독립 x86 기준 생성

[decomp_theme_technology.py](../../tools/decomp_theme_technology.py)는 제품 소스가 생기기 전에 보호 PE의 위128바이트를 Unicorn x86에서 직접 실행했다. 기반 도구는 PE 읽기·격리 메모리·cdecl/thiscall 반환 기능만 재사용한다. 실행 허용 범위는 본체128바이트뿐이다. 49a840/48fc60/4acac0/4afa70은 명시 입력과 호출 인자 기록 경계이며 contained 하위의 실제 생성·체인 탐색을 이 독립 도구에서 실행했다는 주장은 하지 않는다. OS/원본 게임/GUI를 시작하지 않았다.

[최초 fixture](../../cpppj/tests/fixtures/theme-technology-x86.tsv)에는 **1200개 고유 입력 / 1200개 정상 cdecl 반환 / 180747개 관측 명령**이 있다. 세 TYPE 배열·선택 theme−1~6·signed 초기 count−1/0/1/7/9/16·빈/일부 존재·Find/Create/getter의 부모 변경·타입 조회/Find/Create의 count 변경을 교차했다. 92행에서 타입0 Create가 실제 발생했고, 510행에서 최종 count가 초기 count와 달라졌고, 463행에서 최종 현재 부모가50에서 바뀌었다. 선택 theme5는 어떤 항목도 지급하지 않는다.

예를 들어 `theme0,count9,variant2,pattern0,Find후부모변경`에서 첫 호출은 `Current50 → Find(50,4,0,0)=0 → Current51 → Create(51,0,0,4,16,0)`이다. 같은 입력의 TYPE getter가 count12로 바뀌면 초기9를 넘어 타입9~11까지 실제 조회한다. theme2/count1/variant1의 타입0도 `Find(50,4,0,0) → Create(50,0,0,4,16,0)`을 실행한다. 이러한 입력은 기존 Ensure의 type≥1 제한이나 최초 부모·최초 개수 캐시로 대체할 수 없다.

[실행 보고서](../../cpppj/theme-technology-native.json)는 최초 PE·원문·도구·fixture SHA를 보존한다. fixture가 이미 있으면 도구는 재생성하지 않고 거부하며 `--verify`는 모든 SHA와1200고유입력만 읽기 전용 확인한다. 이후 제품 결과에 맞춰 최초 관측을 덮어쓰지 않았다.

- 보호 PE SHA-256: `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`
- fixture SHA-256: `8a1cf581c52716c2444208fb62aeeeaa1661e9fc44139ce13f935d27a67d8843`

## 검사와 한계

[ThemeTechnologyTests.cpp](../../cpppj/tests/ThemeTechnologyTests.cpp)의 `ThemeTechnology_Native1200`은 최초 x86의 전체 타입/count/getter/Find/Create 호출 순서·최종 count·최종 현재 부모·실제 Create 요청/반환을 비교한다. 기대 출력은 Apply 종료 뒤에만 읽는다. `ThemeTechnology_RequiresEveryBoundary`는 다섯 hook 각각의 누락을 거부하는지 확인한다. fixture 경로 매크로는 `NETSTORM_THEME_TECHNOLOGY_FIXTURE`다.

`extracted/theme-technology-20261023-isolated`에서 MSVC19.51.36260·MSBuild18.11.0·C++20·/W4·/utf-8로 실제 제품 CPP와 두 검사를 별도 Release 빌드했다. **빌드 종료0·관측 경고/오류0, 검사2개·실패check0**이다. `python tools/decomp_theme_technology.py --verify`도1200고유입력/SHA를 통과했다. 이 모듈은 실제 contained의 하위 자료구조 생성/삭제, 전체 제품 연결, Fury War 안내 부모 종료, 훈련 진입 및 사용자 완주까지 검증한 결과가 아니다. 해당 근거는 단위376 루트의 실제 연결/실행 기록으로 따로 남긴다.

10.72에는 기존 전체 함수 대응표상 이 함수의 신뢰할 대응이 없으며 이번 구현·fixture는 **10.78 전용**이다. 10.72를 동일한 구현으로 검증했다고 간주하지 않는다.
