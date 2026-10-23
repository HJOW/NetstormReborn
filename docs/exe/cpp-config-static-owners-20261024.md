# 장기 global·fort Config 소유자 복구 제안

2026-10-24, 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 마지막 수행 2026-10-23**이다. 최초 인계는 PE 정적 근거와 미적용 사본 패치였고 게임·GUI·원문 격리 x86·공용 빌드·추가 C++ 실행이 없었다. 이후 root 통합에서 아래 SHA의 패치를 check/apply해 모두 종료0이며 공용 빌드5와 검증을 진행한다. 최초 사본·정적 근거는 그대로 보존한다.

## 실제 결함과 최소 변경

단위385 checkpoint의 실제 진단은 `stopped=1 chat=1 title=2 config=1 subscriber=7 handle=Player configured=<missing>`이다. File→State Notify→Subscriber→Config→World의 Chat 경계에는 도달했으나, 현재 `ConfigInterface`가 이름 없는 language/configuration 두 객체만 소유해 `fort.mySubHandle`의 실제 저장 대상이 없다.

기존 `Config::Set`의 마지막 등록 객체부터 접두어를 비교하는 동작은 원문 `004404e0`과 최초 `config-x86.tsv`에 맞는다. 이름 없는 객체의 비교 길이는0이므로 `fort.mySubHandle`을 루트의 `mySubHandle`로 저장하는 현재 결과도 그 불완전한 Registry에서 자연스럽다. Set 본체나 기대 fixture를 바꾸지 않고 원본의 누락된 두 장기 소유자를 복구한다.

- [제안 패치](../../extracted/cpp-config-static-owners-20261024-proposed.patch): SHA256 `f2dd0aeed9cbf3d4b31d9ef943fe853e66a150b4c829865781ac8c9201ec6b34`.
- [원본 PE 바이트·명령 및 SHA 근거](../../extracted/cpp-config-static-owners-20261024-proposed/native-static-evidence.json).
- [원래 세 파일 SHA](../../extracted/cpp-config-static-owners-20261024-proposed/baseline.json)와 `before/`, `copy/`를 보존한다. 기존 공유 파일에는 쓰지 않았다.

패치는 `ConfigInterface.h/.cpp`에 `global_(registry_, "global")`, `fort_(registry_, "fort")` 멤버를 language/configuration 뒤에 선언·생성한다. 새 공개 API나 기본 성공 callback은 없다. 이름이 필요한 소비자는 기존 `Registry().FindByName("fort")`를 사용할 수 있다. 기존 `Config.cpp`·조회·Set·원문 fixture/report는 그대로다.

## 10.78의 실제 CRT 생성·파괴 근거

| CRT 표 | 초기화 함수 | 실제 this | 이름 인자 | atexit 파괴 함수 |
|---|---|---|---|---|
| `005280ac` | `004fc8c0` | `00558e00` | NULL, language | `004ff140` |
| `005280b0` | `004fc8e0` | `00558de8` | NULL, configuration | `004ff150` |
| `005280b4` | `004fc900` | `00558e40` | `00506d88="global"` | `004ff160` |
| `005280b8` | `004fc920` | `00558e58` | `00506d90="fort"` | `004ff170` |

네 생성 함수는 각각 `push name; mov ecx,this; call 0043fc20`을 실행한 뒤 해당 파괴 함수를 `004e5b23`(atexit)에 등록한다. 네 파괴 함수는 각각 `mov ecx,this; jmp 0043fcd0`다. `0043fc20`은 접두어와 점을 할당하고 현재 전역 Config 목록에 this를 추가한다. 생성 순서는 language→configuration→global→fort, 파괴는 fort→global→configuration→language다.

`00558e40`·`00558e58`의 주소 literal은 각각 위 생성/파괴 코드에서만 확인되었다. 정밀C에는 일부 CRT 생성 함수가 빠져 있으므로 생성자 호출 문자열 검색만으로 존재 여부를 판정하면 안 된다. PE의 문자열 주소 DWORD 참조 `004fc901`·`004fc921`과 CRT 표로 직접 확인했다. 정적 자료에서 직접 소유자 주소를 사용하는 추가 Clear/재생성은 찾지 못했으며, 장기 설정값은 기존 Registry를 통해 소비된다.

`ConfigInterface`는 registry를 먼저 선언하므로 마지막에 파괴하고, 새 두 객체는 reader/registry보다 먼저 자동 파괴된다. `onChanged`와 기존 Client 수명 규칙은 바꾸지 않는다. 이 제안은 Session 설치나 Fort 변경마다 별도 Config를 만들거나 비우지 않는다.

## 10.72 정적 비교

CD판에도 같은 네 장기 객체가 있다. CRT 표 `0050f188/18c/190/194`는 각각 초기화 wrapper `004a8850/8890/88d0/8910`을 가리킨다. 실제 생성자는 `004a8860/88a0/88e0/8920`에서 `0042a150`을 호출하고, this는 language `0056f178`, configuration `0056f190`, global `0056f1c0`, fort `0056f1a8`이다. 이름 문자열은 `00539058="global"`, `00539060="fort"`다.

각 파괴 함수 `004a8880/88c0/8900/8940`은 같은 this로 `0042a1e0`에 진입한다. CD wrapper·생성·atexit·파괴 블록도 근거 JSON에 보존한다. 이 비교는 정적 생성·파괴 계약만 확정하며 두 판본 전체 실행의 동일성을 주장하지 않는다.

## 사본 검사와 통합 검증 범위

[사본 ConfigInterfaceTests.cpp](../../extracted/cpp-config-static-owners-20261024-proposed/copy/cpppj/tests/ConfigInterfaceTests.cpp)에 새 검사3개를 추가했다.

1. `StaticOwnersMatchPatchCrtOrder`: 같은 Registry의 네 객체 주소·접두어·순서와 이름의 대소문자 무시 조회를 검사한다.
2. `QualifiedSetKeepsGlobalFortAndRootSeparate`: `fort.mySubHandle`과 `global.mySubHandle`의 실제 로컬 키, 루트·언어 값의 보존, qualified 조회와 변경 통지4회를 검사한다. 원문 Get의 this 버퍼 관문을 임의 성공으로 넘기지 않도록 실제 루트 입력을 먼저 넣는다.
3. `StaticOwnersPreserveLegacyRootsAndTemporaryScope`: 무접두어의 루트 우선순위/언어 fallback, 알 수 없는 접두어의 기존 루트 저장, 실제 임시 local 등록·해제 뒤 장기 네 객체 보존을 검사한다.

기존 검사 네 곳의 객체 개수만 `2→4`, mission 생존 중 `3→5`로 교정했다. 나머지 기존 기대는 유지하며 사본 파일은 기존9개와 신규3개, 총12검사다. first 검사의 Objects span은 추가 등록 없이 순수 FindByName만 호출하는 동안 사용하고, local 검사에는 등록 전 span을 남기지 않는다.

root는 checkpoint의 일시적인 무접두어 기대를 원래 `fort.mySubHandle` 조회로 되돌리고, 실제 외곽이 전달한 키와 장기 Fort 객체의 값을 같이 검사한다. 이 변경을 위해 원문 Config 본체나 최초 Native fixture/report를 수정하지 않았다. 실제 공용 결과는 [단위385 통합 기록](cpp-editor-checkpoint-runtime-20261024.md)에 이어 기록한다.

최종 통합에서는 신규3검사와 기존9검사를 포함한 전체2576검사가 실패0·종료0이다. 같은 제품의 자연Early1 및 고유34진입 성공도 확인했다. 최초 전체검사에서 과거 Object메뉴의 루트 `salvageSid` 기대1실패를 발견했고, 실제 장기 global 값·루트 부재를 함께 검사하도록 교정한 뒤 최종 통과했다. 제품 Config/메뉴 몸체와 최초 자료는 불변이다. GUI 시작의 별도 옵션파일 접근1실패와 복구 재시도는 통합 기록에서 미해결로 남긴다.

동료의 읽기 검토에서 선언·파괴 순서, 임시 span 수명, 실제 저장 버퍼 분리와 통지 기대의 명백한 오류를 찾지 못했다. 이 단계는 소스/정적 근거 검토이며 컴파일·실제 검사 통과로 세지 않는다. root는 패치 적용 후 원래 qualified checkpoint 기대를 복구하고 이12개, Native Config oracle, 기존 관련 검사와 전체 검사를 실행해야 한다. 기존 실패 진단과 root의 임시 기대 분리 시도 이력은 삭제하지 않는다.
