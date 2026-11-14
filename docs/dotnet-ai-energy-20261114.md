# dotnetpj 0.503 — AI의 실제 TYPE에너지 조회

2026-11-14, HJOW-Athlon. 실제AI결선의 선행 미이식 조회를 해소했다. **R4라이브AI생성은 아직 미연결이다.** 코드 재확인 결과 `ClientHostGame.StartMission`은AI생성을 호출하지 않는다. 이전0.500~0.502의 “호스트 임시토큰 사용”은 부품검사/기존경로의토큰과혼동한 표현이므로 이 기록으로 정정한다.

## 구현

기준cpp소스는 `4a7b305e6`의 `o/RawTypeRequiredEnergy.h/.cpp`와 `client/AiTypeRequiredEnergy.h/.cpp`다. C#의같은이름모듈로옮겼다.

- TYPE+a0의원문입력은실제AST의 `mana`다. 순서있는모든지정값을검사하고문자열이아니면즉시거부한다. 첫NUL까지9바이트제한을보존한다.
- group10·사제플래그제외·level/theme별기본원소문자보충과ASCII소문자화를보존한다. 원소문자표마지막NUL도실제조회가능한입력이다.
- getter는현재188TYPE와116AST를공유한다. 자산70..185만정의에연결하며내부0..69·빈186/187은실제zero초기mana로후처리한다. TYPE를캐시하거나이름으로AST를재정렬하지않는다.

기존 `OriginalText`의Windows-1252경계와원본AST를재사용했다. C++의GameAssets참조는C#에서현재TYPE/정의목록의읽기전용인터페이스로받으며실제호스트로더가주는같은객체를빌린다.

## 검증

`type-required-energy-x86.tsv`를Core에Link했다(2353행, 헤더제외2352입력, SHA256 `8bcb3e215c958f5df0a395365059f5481801b1660569f2f04c0720f394f14f4c`). 원본fixture는불변이다. cpp검사3건을옮겨원본2352입력, 실제116AST/188TYPE입력별원본관측, 현재TYPE변경, 중복지정/잘못된AST/길이/비ASCII/판본/범위경계를대조했다.

신규포함집중14건통과. 레벨2의추가문자를레벨3으로옮긴대표변이에서원본2352입력중22입력불일치를검출했고원복했다. 원복뒤전체비증분Release빌드 경고0·오류0과PORT_MAP포함집중16건재통과를확인했다.

PORT_MAP4행추가(698=L1 346+port352), fixture241개다. 신규NB·외부접속변화·기존경로은퇴·게임실행없음. AGENTS·원본/CD·README개요·cpppj·LEFT_JOBS.md·병행webpj작업은보존했다.

## 다음

AI초기갱신은 `RawPlayerWorld.Create` 뒤Player+9c의실제기술부모를재조회한다. 현재호스트의화신Pop은기록만하므로이부모등록을먼저연결해야한다. 같은풀·해시·장부의실제Player Pop/PostPop와Regular확보/부착을연결한뒤AI메모리·생성자·초기갱신을함께연결한다. 사제조회는실제postPop공유목록을사용하며풀스캔으로대체하지않는다. M1은여전히0/34다.
