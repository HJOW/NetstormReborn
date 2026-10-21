# AI 이동 명령과 실제 이름·경로 연결

2026-10-22 단위287, 시작 HEAD `c58a544c / 20261022-09 CP`. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-21**로 같다. 기존 정밀 원문과 보호10.78 PE를 정적으로 읽었다. 새 디컴파일·원본/CD/AGENTS/dotnetpj 변경과 게임·집중 C++ 검사 재개는 없다.

## 이동 하위

[`RawAiMovement`](../../cpppj/src/o/RawAiMovement.cpp)는 사제/수집자 판단에 공통인 원본 명령 하위를 같은 AI 본체·풀·기하·경로에 연결한다. 전체 사제 행동 `00411cd0`이나 자동 AI 프레임을 대신하지 않는다.

- `00411630`은 현재 actor TYPE로 실제 이동 프로세스 TYPE를 고르고 같은 `RawContainedLookup`의 순번0/remove0으로 첫 Form을 조회한다. `00411680`은 actor0 무동작, 그 외 같은 조회의 remove1이다. 기계어의 `push 1 / push 0 / getMovementType / push type / push actor / FindContained`를 확인했다. 디컴파일의 `GetMovementType(type,0,1)` 표현으로 잘못 옮기지 않는다.
- `004116d0` 대상 이동은 처음 비서버이면 현재 목표 발자국→그 효과 뒤 actor 위치→실제 `RawBoardTarget::Nearest(flags1)`→18바이트40h 송신 순서다. 처음 서버였고 sacrifice가 비영이면 기존 실제 `RawPriestSacrificeRequest::Begin`으로 즉시 돌아간다. 송신 뒤 현재 boss가 참이면 같은 `RawCarrierTarget::StartTarget(actor,target,0,0)`을 실행하고 실패하면 끝낸다. 송신이 boss를 바꿨다고 희생 분기로 되돌아가지 않는다.
- 대상 접근을 통과하면 **목표 이름→actor 이름**을 항상 조회한 뒤 현재 `brain+b00` 디버그 값을 확인해 원본 Move 로그를 남긴다. 디버그가 꺼져 있다고 이름 조회를 생략하지 않는다.
- `004117e0` 좌표 이동도 비서버 송신 뒤 boss를 다시 읽는다. 현재 서버이면 같은 실제 `489080`의 나머지 인자0 경로를 시작하고 실패하면 이름/로그를 생략한다. 성공 또는 비서버 송신 뒤 원래 y→x의 signed64 `ftol` 하위 DWORD로 로그 좌표를 절삭하고 actor 이름을 조회한다. 실제 요청/경로 목적지의 float 비트는 바꾸지 않는다.
- `004118a0`은 현재 신전294가 있을 때 저장한 actor 좌표와 해당 신전 발자국으로 `0041d840` 거리를 구한다. x 차이만 float 저장을 거치고 y는 넓은 계산값을 유지하며, 최종 unordered는 x를 선택한다. PE `00501444`는 **float2.0**이다. `test ah,5 / jnp`에 맞춰 **거리<2만 복귀를 생략하고 NaN도 복귀**한다. 거리 효과 뒤 현재294를 다시 읽어 대상 이동을 실행한다.

현재 프로세스 종류38/41/42 등 기존에 미복원인 실제 부모 경로는 계속 명시 오류로 남는다. 새 명령 연결을 위해 가짜 경로나 즉시 도착을 추가하지 않는다. 명시 Session API의 bool은 해당 AI 본체가 있어 원본 명령을 호출했다는 뜻이며, 경로 생성/도착 성공을 의미하지 않는다.

## 이름 조회와 패킷

이동 하위의 가상+68을 복원하기 위해 [`RawSquidName`](../../cpppj/src/o/RawSquidName.cpp)을 추가했다. 보호 PE의 기존83개 생성 가상 표에서 +68을 직접 읽어 [`SquidNameVtables.inc`](../../cpppj/src/o/SquidNameVtables.inc)에 기록했다. 모두 `004ace20` 공통 이름 또는 `00454d30` Factory 이름이며 미확인 표는 임의 base 이름으로 보내지 않는다.

공통 이름은 현재 TYPE 포인터를 먼저 선택하고 `004ace20`의 **JMP0049af80**으로 이어진다. `0049af80/004de380`처럼 언어0이면 한 번, 그 외 두 번까지 언어를 읽고 두 번째가1이 아니면 현재 설명을 번역한다. 80바이트 이름 복사와 실제 현재 설정 치환 순서를 유지한다. Factory는 base 이름 뒤 ` Level %s`를 항상 번역하고 그 효과 뒤 현재 가상80 레벨→원본 I..X/ff sentinel→CRT 접미사 서식을 적용한다. 기존 메뉴의 `LevelName`도 같은 문자표를 사용한다. 원본80/256바이트 밖 결과는 명시 거부한다. 유효한 원본 CRT 서식을 대상으로 하며 잘못된 인자/메모리 손상 입력은 미검증이다.

실제 Client의 언어 번호/번역/설정 치환과 Session의 현재 레벨을 연결했다. 이름은 DEBUG가 꺼져 있어도 선택된 위치에서 실제로 계산한다. 디버그 로그 전역 `0052f3d8`의 초기값은99다. 현재 AI owner 접두와 원본 Move 문자열을 사용한다.

사용자/AI 이동은 새로운 별도 헤더를 만들지 않고 같은 `CarrierMoveState`와 `WriteCarrierMoveCommand`를 공유한다. 원본 `0040a850`처럼 protocol의 bit7만 지우고 선언 length를4로 보정하지 않으며, WORD actor/두 float 비트/전체 sacrifice DWORD를18바이트에 쓴다. 버퍼 뒤는 보존한다. 기존 사용자 이동의 선택 해제/동일 목적지 관문은 기존 위치에만 유지한다.

Session의 실제 경로 분배는 공통 `StartMotion`으로 모아 기존 사용자 대상/좌표 이동과 AI가 같은 BFS·Host를 사용한다. `ConnectAiMovement`는 **Load의 실제 Motion/Board/CarrierMove 설치 뒤** 호출한다. 생성자 초기 AI 연결 시 아직 없는 Board를 역참조하지 않는다. 의식/대상/경로 callback은 Loaded 이후에 호출되며 같은 장기 모듈을 빌린다.

## 확인 기록

새14검사는 이동8개+이름4개+공유 패킷1개+실제 Session1개다. 경로 TYPE/삭제 인자, 서버 실패/희생·이름 순서, 실제 Board 접근점/원문, 송신 뒤 boss, 실제 시작 인자/좌표 절삭, 거리2/unordered/현재 신전, signed 발자국 거리, 선택 하위 누락의 앞선 효과 보존, 현재 언어/TYPE·설정/Factory 레벨, 원본 버퍼·로마 숫자/서식, 패킷 꼬리/짧은 버퍼와 실제 Session BFS/Host 경로·이름·정지를 다룬다. **모두 작성·컴파일만 하며 실행하지 않는다.** 실제 Session 검사도 Kernel 프레임을 실행하지 않고 선택된 원본 명령으로 생성·정지만 확인하도록 작성했다.

Release 빌드 세션26899는 실제 종료까지 조회했고 **종료0·경고0·오류0**이다. `extracted/cpp-ai-movement-20261022-build1.log`와 `build1-result.json`을 보존했다. 로그는 내용/개행을 보존해 UTF-8 바이트로 변환하고 동등성을 확인했다.

[`cpp_ai_movement_audit.py`](../../tools/cpp_ai_movement_audit.py)의 [신규 정적 보고서](../../cpppj/ai-movement-static-20261022.json)는 원본17함수/거리2·로그99·이름 가상 표83개를 보호 PE에서 확인하고, 직전과 같은 보호 원본 SHA·현재 소스/바이너리 SHA·새14검사 링크·실제 Motion 설치 뒤 연결을 기록한다. **C++ 검사·CTest·게임·native 실행0**이다. 과거 기록은 덮어쓰지 않았다.

CD10.72는 기존 매핑의 대상 이동 `004dc620`, 공통 이름 `004acaf0` 등을 참고 주소로만 남겼으며 별도 구현/실행 대조는 하지 않았다. 전체 x87 확장 정밀도/예외·CRT의 잘못된 서식·미확인 가상 표는 미검증/명시 경계다. 자연 경로 진행·의식·로그·화면/음향과 전체 AI 자동 호출은 실행 검증하지 않았다.

사제 행동/다른 건물·다리·수집/포획 판단, 가짜 에너지 부족 좌표 선정, 원래 순서 자동 AI 프레임/종료와 목표15개(Early Missions6+캠페인1-1..1-6+데모3), Edit/custom Fort·시나리오·전환/저장 수명과 자연 플레이 동일성은 계속 남아 있다. 빌드/정적 기록/인수인계 뒤 `20261022-10 CP`로 로컬 커밋하며 원격 push하지 않는다.
