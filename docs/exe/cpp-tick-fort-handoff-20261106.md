# Native Poll과 호스트 미션 설치의 Tick 경계 — 단위954

2026-11-06, HJOW-Athlon. 마지막 정밀 디컴파일은 HJOW-Athlon/2026-11-03이다. 정밀 산출물은 수정하지 않았다. 사용자 순서 정정대로 LEFT_JOBS 3절3번의 State1/State7·Tick/연속 전환을 이어갔다.

## 찾은 차이와 수정

원문 PATCH004b88c0은 next가 비영이면144BYTE를 current에 복사하고 next0 → 004b6dd0 한 번 → return이다. State1/3의 미션 분기는004b50b0이 다음7을 쓰고004b6dd0을 조기 반환한다. 따라서 새7은 다음 Poll에서 소비된다. 현재 `UberGump::Tick`은 Native Poll 뒤 `state_.Take()`를 호출해 **그 Poll이 새로 Post한 LoadMissionWorld도 같은 Tick에서** 실행했다. 952/953의 조기 반환/본문 연결 자체와 호스트 루프의 소비 순서는 다른 경계였다.

`State::TakeAfterPoll`은 Poll 진입 전에 있던 같은 호스트 큐 항목만 복귀 뒤 소비한다. 항목 수명 번호는 새 Post에서만 증가하며 Native state/phase/loaded를 대체하지 않는다. 새 요청, 기존 요청 취소 뒤 같은 명령 이름으로 교체한 요청은 다음 Tick까지 남긴다. 기존 단일 항목/꽉 찬 큐의 Post 무시·Cancel/일반 Take는 유지한다. Poll 예외는 이미 Post된 요청을 되돌리거나 꺼내지 않고 전파한다. 대기 요청이 있으면 기존 호스트 브리핑 진행도 앞당기지 않는다.

이 지연 구간에서 실제 GameWorld의 부분 Fort가 커서/프레임에 노출될 수 있어 설치 대여도 연결했다. 기존 ExistingEditor 공통 Fort의 지연 생성은 `preparingFort/connecting`을 유지한다. 이 동안 기존 프레임/표시 관문을 사용하고 호스트 packet slot은 미공개 Editor 수신기를 펌프하지 않는다. `CompleteFortInstall`은 실제 Session의 전체 State 확정·checkpoint/UI/tail 검증/commit을 실행한 뒤 실제 표시 사본을 만들고 대여를 닫는다. 정상 입력 공장은 그 뒤에 구성한다. Session의 원래 미설치 패킷/커서 접근 거부를 빈 성공으로 바꾸지 않았다. 진단에는 raw_fort_pending을 별도로 보고하며 정상 접속/loaded라고 쓰지 않는다.

## 검사와 원래 동작 대조

새4개 큐 검사에는 실제 RawLocalMissionState의 State1 조기 ret → 다음7/phase 보존 → 다음 호스트 Poll의 설치 인계/공통 확정을 연결했다. State7의 큰 설치는 명시 장치이며 이 검사만으로 실제 월드 정리를 수행했다고 주장하지 않는다. 기존 요청 보존, 같은 명령 취소/교체, Poll 예외/누락 장치의 부분 효과도 검사한다. 새1개는 실제 Client/GameWorld/Session의 닫힌 프레임/보고/커서 → 실제 checkpoint/UI/RawState 확정 → 최종 commit/프레임 공개와 중복·조기 공개 거부를 확인한다. 외부 카메라 표시 적용은 관측 장치다.

수정본 새4개가 통과한 뒤 이전 Tick과 동등한 `poll(); return Take();` 대조 변이를 실제 라이브러리에 넣었다. 4개 중2사례/5단언이 실패해 새 요청과 교체 요청의 조기 소비를 검출했다. 변이는 원래 수정 바이트로 복원하고 제품/검사를 재빌드했다. 대조 빌드에서는 게임을 실행하지 않았다.

최종 Release 제품·두 집중 대상 빌드는 경고/오류0, 고유26검사(새5/기존21) 실패0이다. 큐4/실제Fort대여1/기존NativeMainMenu11/미션수명2/continuation4/prefix3/UI확정1이다. 기존 독립 Native82행과 MissionLifecycle831행은 C++ 재대조이며 새 CPU 관측/fixture 생성은 하지 않았다.

최초 새 Native 장치는 menuWindow를 immediate+84와 다르게 공급해1검사가 실패했고 같은 실제 별칭으로 교정했다. Fort 대여 검사 첫 빌드는 존재하지 않는 GameWorld.Objects 조회로 실패해 실제 Session.IslandCount를 사용했다. 첫 실행의 즉시 카메라 표시 장치 누락도 실제 외부 적용 관측 장치를 공급해 교정했다. 이 실패들은 로그에 남긴다.

## 실제 실행

먼저 큐 순서만 고친 중간 후보로 기존 Early1 새 진입2/Replay2·Continue/Abort·메인 왕복/quit0을 확인했다(24관측). 이 후보는 후속 Fort 대여가 들어간 최종 EXE와 해시가 다르며 최종 제품 결과로 합산하지 않는다.

최종 제품은 격리 자료/사용자 루트에서 두 실행을 확인했다.

- 실제 tutorial1 진입 뒤 AskEditFort → **보고서의 현재 Fort 이름 BridgeTheGap**으로 ReadFort → 같은 미션 입력 가능 상태 복귀 → quit0. 앱 입력5/완성 TSV13개이며 stderr는 State1 한 번, State7/State14 각각 두 번을 보인다. 설치 자료1395개 불변이다. 짧은 raw_fort_pending 자체는 MCP 관측에서 포착되지 않았다. 한 Tick 지연/닫힌 프레임의 직접 근거는 새 C++ 검사이고 실제 로그는 자연 State1/7 재설치·복귀 근거다. State3 특이 분기의 자연 도달 증거로 쓰지 않는다.
- Edit/TEST01 → Test Battle/Go! → Return to Editing → Main Menu/No → quit0. 앱 입력13/완성 TSV8·OS제목8·1024×768 PNG4개, 자료1395개 불변이다. 제목은 MAIN/MAIN/EDITOR:TEST01/TEST:TEST01/TEST:TEST01/EDITOR:TEST01/EDITOR:TEST01/MAIN이다. returned-editor PNG를 직접 열어 표시도 확인했다.

새 handoff 도구의 첫 실행은 **Early1을 Fort 파일 이름으로 잘못 입력**해 MapProbe State 진단/종료1이었다. 이 원시는 gui-handoff에 보존했고 자료는 불변이다. 올바른 BridgeTheGap을 실제 보고서에서 읽은 두 번째 실행만 통과로 기록했다. 올바른 실행에도 원문 장치의 비치명적인 '편집 복귀의 Template 파일 이름 불일치' 로그는 남아 있으며 숨기지 않았다. 별도의 새 게임 결함으로 단정하지 않는다.

handoff 도구는 실제 재설치 로그도 필수로 검사하도록 실행 뒤 보강했다. 실행 당시 도구는 각 출력의 tool-at-launch.py에 고정했다. 두 번째 실행은 보강 전 판본이므로 stderr/13원문 TSV로 만든 별도 audit.json의 State1/7/14와 현재 Fort 이름 대조를 함께 근거로 쓴다. 도구 보강 때문에 게임을 다시 실행하지 않았다. 보강 후 Python 문법도 확인했다.

공격·사제 포획/희생·승리·공중3종 전용·멀티플레이·물리입력 검사는 하지 않았다. 단독 미션의 기존 로컬 로그인 흐름이며 State6/원격 전투를 실행하지 않았다. 전체 CTest/34미션/ZIP도 반복하지 않았다.

제품954/11.01, EXE `6942d8f7f4f57235b9598200715ed459c5c85b33772ae42cea439f723c4eb601`, 11,280,896바이트. 보호2788개/fixture968개 불변·새fixture0개다. 원시/다음 보호 기준은 `extracted/cpp-resume-954/`의 로그와 `protected-final.json`이다.

## 다음 작업

955도 같은3절3번의 Native3 다른 선택/State1·단독State7 나머지·cleanup Tick 연속 수명/LeaveBattle 다른 이유/CD·State6을 대조한다. 이 단위는 앞선 조기 반환과 실제 루프 접합의 두 차이를 고친 것이며 셋째 체크 항목 전체 완료가 아니다. 일반 전투/State6의 MP 범위는8번으로 구분하고 신규 MP 검사는 하지 않는다. 마지막 정밀 디컴파일 HJOW-Athlon/2026-11-03 기준을 유지한다.
