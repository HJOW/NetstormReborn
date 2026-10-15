# 실제 HTML 명령 큐와 미션 완료 Config 저장

2026-10-15, 현재/마지막 정밀 디컴파일 PC는 **VM-W11-Codex**다. 같은 정밀 프로젝트에서 PATCH9함수를 읽기 전용 export했다. 보호 PE의 함수만 격리 실행했고 실제 원본 게임은 실행하지 않았다. 원본·AGENTS·dotnetpj는 변경하지 않았다. 실제 클론 GUI는 아래 검증에 사용했다.

원본 `004c9090`은 HTML의 명령을 즉시 실행하지 않는다. `00487620/00487710`으로 두 토큰을 읽고 현재 명령 표 `004d52c0`을 조회한다. 숫자로 시작하는 인자는 원본 atol, 나머지는 `004c7910`의80칸 문자열 고리에 복사한 뒤 `00452fe0`의 실제40칸 입력 큐에 넣는다. x/y는 같은 값이다. `Config`의 실제 코드는 `0200006d`, `Warning`은 `02000102`다.

`RawTextToken`은 앞 space/tab만 건너뛰고 인용부호를 출력에서 제거한다. **78개 입력 바이트** 제한에는 인용부호도 포함된다. 제한에서 멈춰도 현재 연속 구분자를 건너뛰며, 뒤 space/tab을 지울 때 첫 출력 바이트는 남긴다. 따라서 인용한 공백 한 글자는 빈 키가 아니다. 첫 명령 토큰 구분자는 `#,;>`, 둘째는 `;>`다. `Config`는 `=`와 개행을 각각 사용하므로 값의 등호/세미콜론은 보존된다.

`ScriptStringRing`은 BSS 인덱스0에서 첫 복사 때1로 이동한다. 같은 문자열도 별도 수명을 소비하며81번째부터 해당 칸의 이전 문자열만 해제한다. 호스트64비트 주소 대신 opaque32 수명 토큰을 사용하고 해제된 참조는 명시 오류다. 원본의 해제된 포인터 역참조/메모리 손상을 호스트에서 재현하는 범위는 아니다.

원본 lookup0은 진단 후에도 명령0을 큐에 넣는다. 미등록 `ffffffff`는 **인자 변환/문자열 복사 뒤** 진단하고 큐에는 넣지 않는다. 두 분기를 합치지 않았다. 최초 `inactive` 정적 항목은 값 포인터 자체가0이라 원본 getter가 잘못된 메모리를 읽는 것을 격리 실행이 차단했다. 이 손상 입력을 정상 경로로 세지 않았고, 정상 Config 항목의 현재 코드만0으로 바꾼 입력으로 실제0분기를 관찰했다.

`ApplyScriptConfig`는 원본 `00441580`의 토큰 순서로 실제 `ConfigInterface::Set`을 호출한다. 기존 정의를 덮어쓰고 변경 통지는 기존 Set에서 발생한다. 빈 키는 쓰지 않으며 반환값은 항상0이다. 원본 성공/추가 진단 로그의 문자열은 native에서 관찰하지만 C++는 기존 ConfigInterface와 같이 그 로그 출력을 재구현하지 않는다. 실제 키/값, 현재 저장소 변경, 알림과 입력/저장 순서를 완료 범위로 삼는다.

`DialogPage.commands`는 페이지의 모든 `<$...>` 명령을 원래 순서대로 보관한다. 기존 Warning 목록은 관찰용이며 실행은 큐 하나를 사용한다. `UberGump::TryTell`은 Config/Warning을 실제 ID로 예약하고 `Event`가 다음 입력 위치에서 소비한다. 명령만 있는 Tell은 기존 페이지/정지/버튼 캡처를 바꾸지 않는다. 원본 바이트에서 토큰 제한을 적용하고 Config 저장 경계에서만 Windows-1252→UTF-8을 수행한다. 현재 연결한 명령 표 항목은 Config/Warning이며 나머지는 미연결 진단이다.

`Client`의 기존 변경 감지 저장 경로가 같은 프레임의 `options.cfg`를 쓴다. 실제 Early1 `[G.]`/Early2 `[I.]`의 `<$Config,Done{mission.fileName}=1>`을 그대로 읽어 실행하며 별도 완료값을 하드코딩하지 않는다. `mission_progress` 보고도 같은 현재 Config를 읽는다.

검증은 다음과 같다.

- `decomp_script_command.py`의 실제 PATCH **262상태 입력/정상 반환**: 토큰128, Config18, 명령31, 고리85회 연속 수명이다. 고리의 같은 문자열 재등록은 다른 현재 상태 입력이며 재실행을 중복 합산하지 않는다. 원본 명령 표·토큰·문자열 복사/고리·입력 큐와 C locale CRT 몸체를 실행했다. Set/알림/로그·할당/반납·TLS 포인터만 명시 관찰 경계이며 OS 호출0이다.
- C++ 집중2개에서 전체 토큰/소비위치·Set키/값·예약코드/인자·진단시점·고리인덱스/이전수명·동일큐의 여러 완료값/실제저장문법을 대조했다. 첫 검사3체크 실패는 공백 키를 일반 Get으로 검증한 가정과 저장문자열의 따옴표 기대였다. Set경계의 실제 인자를 직접 비교하고 기존 ConfigText로 저장문법을 확인하도록 검사만 교정했다. 토큰/명령 production 몸체는 변경하지 않았다.
- 최종 Release게임/검사 빌드 **경고/오류0**, 집중 ScriptCommand2개와 Warnings6개 실패0이다. 기존 대화상자 회귀는 원본의 다음 Input 소비로 기대시점을 교정한 뒤 **97상태**에서 통과했다. 수정 전 즉시 Warning 기대가 실패한 자료는 `extracted/script-command-dialog-initial.log`에 있다.
- 실제 Early1 MORE/OK→G. 예약전0/다음입력1→Next Tutorial→Early2 MORE/MORE/OK→I. 예약전0/다음입력1→Leave Tutorials의 **9상태**가 통과했다. 종료 뒤 실제 `options.cfg`의 두 Done값1을 별도 프로세스로 다시 읽었다. 두 GUI 검사 모두 허용 설정을 복구하고 originals 전체파일명/SHA 불변을 확인했다. 이는 **완료 섹션 연결 검사이며 실제 플레이 완주가 아니다.**

근거는 `cpppj/script-command.json`, fixture, `extracted/script-command-{final-native,ui-final-build,ui-final-focused,warning-focused,dialog-final,progress}.log`, `extracted/script-commands-ui/report.json`이다. `python tools/decomp_script_command.py --verify`와 staged 공백검사를 통과했다. 기존 전체1,183개는 이 변경 이전 검사이며 최종 전체 재실행으로 세지 않는다.

현재 HTML 조건/버튼 parser는 이전 어댑터가 남아 있다. 새 원본 HTML 본문/StyleTextBuffer 연결, 실제 자동 단계/승패, 로그인/AI·건설/다리/수확/운반 및 캠페인 완주/진행은 후속이다. 목표는 active이며 DirectDraw 전체화면 오류 수정은 요청대로 제외한다.
