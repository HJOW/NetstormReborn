# 작업 1221 — Depend Form의 장기 Storage 연결

장기 감시 본체·현재 공장/Depend TYPE·가상 삭제/통지를 같은 실제 Pool에 연결하는 작업이다. 제품1221의 구현·제한된 검증을 마감했다. 전체 목표는 미완료다.

Native World/Core와 NoWorld가 같은 `DependForms()`를 빌린다. 독립 typed World의 지역 Host는 유지한다. 원문의 상대 삭제는 현재 VT+10으로 재진입하며 첫 상대의 효과 뒤 두 번째 상대 WORD와 VT를 다시 읽는다. 공통 선택은 전체 DWORD를 사용한다. 현재 알림만 World 수명으로 소유하고 Storage는 weak 대여 → 실제 Runtime pin → 함수 사본 → 호출 순서로 사용한다.

Form 공장/부착은 기존 현재 Runtime의 Form 전송과 장기 기본 전송을 그대로 선택한다. 직접 ACK/새 패킷/통신 성공 대체를 추가하지 않는다. 실제 자산83개 통지와 Process+34, Depend/Gump/Content의 실제 빈 +34를 공유한다. Gump/Content의 삭제 본문까지 구현한 것은 아니다.

Native 검색은 원문 FormFinder의 TYPE<70 선별·현재 전체 Depend DWORD·선저장 next·현재 부모 getter를 유지한다. 조회에 free/dead 필터를 더하지 않는다. 검색 시작과 제거/통지 진단은 실제 Client assertion 정상 복귀 후 현재 필드를 다시 읽는다. 물리 범위 밖/순환 체인의 호스트 예외와 기존 독립 typed 계약은 유지한다.

현재 호스트와 마지막 정밀 Client 양판본(2026-11-03), R 전용(2026-11-07)은 HJOW-Athlon이다. 원문/PE를 읽기만 했으며 새 디컴파일·원문 변경은 없다. 원시 근거는 `extracted/cpp-resume-1221/`에 보존한다.

신규 공통 비MP 검사16개와 기존 감시 생성5개(최초 원문1620입력 포함), 관련 기존 집중 검사를 수행했다. 새 MP·공중3종 전용·포획/희생·특정 적 공격·승리 검사는 제외했다. 실제 빌드·제한된 실행 결과와 남은 경계는 아래에 기록한다.

## 확정 결과와 남은 범위

Release/x64 제품2·집중5대상 빌드는410.328초/종료0, 경고·오류0, 소스10개 고정이다. **비MP371/371**(기존350+신규16+기존 감시 생성5)이69.581초/종료0으로 통과했다. 현재 TYPE·물리/공장 정체성·두 부착/제거·전체 DWORD·free/dead 후보·Form 선별·실제 Process 상대 삭제·첫 효과 뒤 상대 재조회·선택 해제·세 빈 통지·server0 PID 관문·실제 weak/pin 수명·진단 정상 복귀·Native World 공유를 확인했다. 새 fixture는 없고 기존 원문1620입력을 대조했다.

같은 EXE에서 Tutorial4 정상 UI의 TYPE75/owner1 생성과 요청0→1,600프레임13.405→24.077초 대기를 확인했다. 015 PNG를 검토했고 Edit TEST01↔Main2회/11관측도 종료0이다. 모든 시작 빌드·집중·GUI 핸들이 종료했다. 보호 원본/CD/fixture·AGENTS2801파일은 불변이다. MP/공중3종 전용/포획희생/특정공격/승리는 실행하지 않았다.

원문 요청37진입 중 독립 C/PE 범위를 확보한 것은29개다. 짧은 getter/Has 필터와 PATCH 공통 Post는 RET까지5개의 실제 PE 몸체를 별도로 보존했다. 원문 준비의 독립 경계 미확보와 정적 집계 실패·교정은 원시에 남겼다. 기존 Process/Gump 하위 등8진입의 C metadata 부재를 이번 전체 함수 보존으로 세지 않는다. 제품 빌드/집중 검사 실패는 없었다.

Storage 기본 Native Form 삭제는 **Process/Depend2종 연결, Gump/Content2종 잔여**다. 자산 Pre/Post·통지83/83, Regular39/87VT·잔여48VT/23함수는 유지한다. 큰 AI·다른 배치/Fort 상위 Finish·전체 호출자/owner·M6-l/m·M7/M8·K/S/B의16상위계획·등록34버그는 남는다. 전체 NoWorld/게임 동일성 완료나 전체 솔루션 통과를 주장하지 않는다.

- Netstorm.exe: 13959168 bytes, SHA256 `0fb034d0373fbb2bd256c5401270bb731c7df5ebbfffae7a006bda690ee43858`.
- R.exe: 677376 bytes, SHA256 `c1401621c911a61514ca97d7581670157c7897adcc2aeb39ff7e4eb8589d6b50`.

[현행 인계](../../LEFT_JOBS.md)·[확정 보고서](../../cpppj/depend-owner-20261109.json).
