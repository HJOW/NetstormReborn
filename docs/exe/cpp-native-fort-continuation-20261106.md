# State3 Fort continuation 실제 연결과 CoreData 읽기 — 단위952

2026-11-06, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon/2026-11-03이다. 디컴파일 산출물 자체는 바꾸지 않았다. 사용자 정정대로 LEFT_JOBS 3절3번 셋째 항목을 이어갔다. 전체 목표와 해당 항목은 미완료다.

## 변경과 확인 범위

951은 State3 제목 뒤 다른 설치 callback의 반환 계약만 복원했고 실제 호스트는 바로 미연결 오류를 냈다. 952는 그 callback을 `InstallPreparedFortState(false)`에 연결한다. 정상 State1도 같은 파일/미션 분기/월드/tail 본문을 사용한다. `false`는 State3에서 이미 실행한 cleanup/listener/옵션/HUD1을 반복하지 않는다. 정상 State1은 기존 prefix와 한정 HUD scope를 그대로 실행한다.

준비 대여에 `adoptMainMenuFort`를 명시했다. 이 입력은 같은 준비된 World/Player/Core/SP/Kernel과 current1/2를 요구하고 Editor/missionPost가 실제0인지 확인한다. Editor 경로의 기존1/1 검사를 완화하거나 새 모드 쓰기로 바꾸지 않았다. 새 잘못된 대여4입력과 기존 prefix3검사가 거부 계약을 확인한다. 이것은 멀티플레이 State6 대여 경로가 아니다.

호스트 Poll은 초기 next3이라는 이유만으로 모든 후속 결과를 무시하지 않는다. 실제 phase3/next4이면 메인 공개, next7이면 Guide 조기 반환, 다른 정상 Fort 확정이면 기존 Session 최종 commit과 입력 공장을 사용한다. 일반 Fort의 호스트 표시는 기존 `mission` 페이지 이름을 사용하되 Editor 전역은 변경하지 않는다. 이 최종 정상 분기는 아래 사제 미복원 때문에 이번 일반 Fort 입력에서 도달하지 않았다.

제목은 World 생성 전에도 존재하는 현재 Fort 이름과 실제 Editor/missionPost를 읽는다. 일반 Fort는 현재 Template의 +0/+8 DWORD로 readonly 접두를 결정한다. 미션 +68/demo와 +64/Test Battle 우선순위는 유지한다. 이전 Guide 객체가 존재한다는 이유만으로 일반 Fort를 미션 제목으로 바꾸지 않는다.

## 새로 연결한 CoreData 일반 Fort 읽기

원문 PATCH `004beca0(0)`과 `004bea70`, 일반 복사 `00482ba0`, WORD 읽기 `004bcaf0`를 대조했다. `LoadExistingEditorWorld`의 기존 'Editor 자연 생략 외에는 중단' 자리에 다음 실제 본문을 연결했다.

- missionPost/Editor 자연 관문 뒤 loadingDepth 증가와 현재 manager 진단, `CoreData` writable 선택/pos0.
- 실제 active1과 현재 Core10BYTE 복사, pos 저장 뒤 signed end 진단, 실제 Chat 랭크 갱신.
- 선택된 seed.low16 읽기와 장기 Client DWORD 상위WORD 보존.
- 남아 있는 랭크/승리/패배 무결성 블록 각각8 BYTE·8 WORD·16 DWORD. 불일치 시 현재 Core rank=1과 현재00540cb8을 같은 장기005318ec에 기록한다.
- 정상/예외 양쪽에서 loadingDepth 복귀. 이미 쓴 Core/active/seed/stream/진단은 rollback하지 않는다. 물리 범위 밖은 명시 오류다.

이는 로컬 저장 내용의 원문 검사이며 외부 클라이언트 유효성 서버에 접속하지 않는다. 재접속 호출의 인자1 최소값 병합과 CD00429200의 별도 몸체는 연결하지 않았다. 실제 Session은 기존처럼 PATCH10.78 전용이다. 이번 새 검사에는 멀티플레이 실행/패킷 검사가 없다.

## 실제로 남은 본문과 검증 한계

현재 비편집 설치는 실제 File → Player/CoreData → Territory/World → Deck까지 도달한 뒤 `RawCampaignWorldLoad::FinishMissionPriests`의 '캠페인 비미션 Technology/Priest 후처리 전체 미복원'에서 중단한다. `004923e0`의 기본 기술4종 보장과 일반 Fort의 지역별 사제 준비가 남았다. 이 오류를 빈 성공/Editor 강제1로 치환하지 않았다. 따라서 일반 Fort의 UI/tail/정상 최종 공개 완료나 State3 특이 분기 전체 복원을 주장하지 않는다.

독립 Native82행의 특이 입력은 title 장치에서 current를 바꿔 본문 앞에서 멈추던 기존 범위다. 새 통합 검사는 같은 재선택을 명시 외부 입력으로 공급한 것이며 자연 OS 제목 갱신이 실제로 current를 바꾼다는 증거가 아니다. 정상 GUI에서는 해당 특이 분기의 자연 도달을 아직 관측하지 않았다.

## 검증

최종 집중 검사는 고유23개/실패0이다. 새4개는 비편집 실제 월드/Deck까지의 부분 효과와 정확한 사제 중단, 실제 Guide 요청/Champion/Zone/next7·공통 phase 확정 생략, Core10BYTE/seed/정상·불일치 무결성, 잘못된 대여4입력을 확인한다. 기존19개는 prefix3·UI commit1·Guide handoff1·NativeMainMenu11(독립PE82행 포함)·제목3이다. 원문 PE82행은 C++ 재대조이며 새 PE 관측이나 새 fixture 생성은 하지 않았다. 실제 월드 사제 중단을 확인하는 검사는 정상 Fort 설치 성공 검사가 아니다.

최초 빌드는 새 검사의 char/uint8 span 형 불일치로 실패했다. 교정 후 Release 제품/검사 빌드는 경고/오류0이다. 최초 통합 실행은 사제 미복원 경계를 실제로 드러내 2개 중1개가 실패했다. 그 원문 미복원을 잔여로 명시하고 부분 효과/정확한 오류 계약으로 분리했다. 후속 검사 빌드의 C4702 하나도 명시 반환형으로 교정하고 최종 재빌드 경고/오류0을 확인했다. 실패·경고 로그는 원시에 남긴다.

실제 제품952를 별도 자료 사본과 별도 사용자 루트에서 실행했다. Edit/TEST01 → Test Battle/Go! → Return to Editing → Main Menu/No → quit0을 확인했다. 앱 입력13, 완성된 MCP TSV8개와 OS 제목8관측, PNG4개(1024×768), 자료1395파일 불변이다. 제목은 MAIN/MAIN/EDITOR:TEST01/TEST:TEST01/TEST:TEST01/EDITOR:TEST01/EDITOR:TEST01/MAIN이다. returned-editor PNG도 직접 열어 확인했다. 공격·포획·희생·승리 조작과 물리 입력 검증은 하지 않았다. 전용 공중3종/멀티플레이/전체34미션/전체 CTest/ZIP 검사는 하지 않았다.

제품952/11.01, EXE SHA-256 `c685e4241531793383acdec2edd8d6aa47c8cec100795ea8a1e30316b6188026`, 11,254,272바이트. 보호2787개/fixture967개 불변, 새fixture0개다. 원시는 `extracted/cpp-resume-952/`, 다음 보호 기준은 그 안의 `protected-final.json`이다.

## 다음 작업

953은 같은3절3번에서 `004923e0` 실제 비미션 후처리를 연결한다. 이 함수의 `00492120` 후보 모으기, `004921a0` 자기 소유 사제 선택, `0040ea00` 남은 사제 준비와 지역/Chunk/생성 부모도 원문으로 확인해야 한다. 일반 Fort 정상 확정 이후 호스트 공개는 실제 사제 부모가 정상 복귀한 뒤 검증한다. 이후 Native3 다른 선택·State1/단독State7·cleanup Tick/연속 전환·LeaveBattle 나머지 이유/CD·State6을 대조한다. State6/일반 전투 저장 복구는 멀티플레이 이월 범위와 구분하며 신규 멀티플레이 검사는 하지 않는다.
