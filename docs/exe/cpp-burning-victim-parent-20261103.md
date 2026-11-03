# 연소 세 부모의 현재 SID·진단 공통 앞단

단위797은423d50 캡처·424110 연소점·424530 프레임이 공유하는 SID 재조회/가상 표/HP 진단의 순수 앞단을 작성하고 원문과 대조했다. **실제 세 소비자 연결은 다음 작업이다.** 이번 결과만으로 함수별 checking 잔여46을 줄이지 않으며 실행 제품은796/11.01 그대로다.

원문은 현재 checking이 비영이면 현재 SID의 live1 robust를 호출하고 실패 시 부모별 줄로 진단한다. 진단 복귀 뒤 현재 SID를 다시 robust로 조회하며0이면 본문에 들어가지 않는다. 통과하면 그때의 SID를 고정한다. 캡처만 가상 표0을 무조건 진단하고, 세 부모 모두 같은 고정 슬롯의 hasHitPoints 실패를 무조건 진단한다. 이 진단이 돌아와도 고정한 슬롯은 바꾸지 않는다. 공통 RunBurningVictimParent는 빈 반환·선택 SID 반환·선택 장치/진단 예외를 구별한다.

`tools/decomp_burning_victim_parent.py`는 실제 세 부모의 해당 명령만 실행한다. robust/HP/진단과 캡처의 선행 clear는 명시 장치이며 각 본문의 첫 명령 전에 중단한다. Capture423e0b·Sources42418f·Frame4245e0을 disassembly로 확인했다. 각168개, 총504입력을 두 x87 제어값에서1008회 관측했고 재관측1008회가 최초 바이트와 일치했다. 빈 반환210·진단 중단137·본문 도달157입력이다. 현재 DWORD0/1/상위비트/FFFFFFFF, robust 반환 조합, 가상 표/HP0과 진단 복귀, 첫 robust/HP 중 현재 SID 변경, 번호0/65535의 실패를 포함한다. 뒤 본문·실제 상세 SID 진단·캡처/입자 동작·전체 SEH/진단 UI는 이번 관측 범위가 아니다.

기존 World 집중 대상과 공용 라이브러리만 Release 빌드해 경고·오류0이다. 새 순수 검사2개가 통과했다. 처음 집중 검사를 빌드 종료 전에 요청해 이전 바이너리가 필터를 찾지 못했다(실행 검사0·실패 단언1). 이 도구 실행 순서 오류는 `tests-parents-v1.log`에 보존했으며, 빌드 정상 종료 뒤의 `tests-parents-v2.log`가 최종2개/실패0이다. Native 최초 관측/재관측과 C++ 새 구현의 첫 유효 실행은 모두 성공했다.

다음은 실제 RawBurningFrame::Capture/Step과 RawBurningSources::Select에 이 앞단을 연결하고 같은 장기 Client checking·RawIsSidRobust 진단 owner를 공급하는 작업이다. Capture의 clear 뒤, Step의 capacity/첫 캡처 단계 뒤에 연결해야 한다. Sources/Step의 이후 슬롯 읽기는 반환한 고정 SID를 사용하고, Capture의 이후 이미지 요청은 원문처럼 그 시점의 현재 필드 SID를 사용한다. 기존 CD와 독립 입력·이전 전체 캡처/입자 fixture는 유지한다. 기존 무조건 hasHitPoints throw도 실제 진단 복귀로 연결해야 하며, 전부 연결/대조하기 전에는 세 부모를 완료로 세지 않는다.

마지막 전체 정밀 디컴파일은 HJOW-Athlon/2026-10-23이며 현재 호스트가 같고 추출물 변경은 없다. 원시 근거/다음 보호 기준은 `extracted/cpp-resume-797/`이다. 새 게임·원본 GUI·MP·공중 전용·지정 플레이·전체 C++/71대상·34경로·ZIP·물리/PCM 검사는0이다. 게임 EXE 재링크도 하지 않았다. 마지막 GUI는794의793 제품이다. 실행 제품796 SHA와 보호 개수는 최종 보고서에 기록한다.

최종 실행 제품796 EXE SHA256은 `67e2bd3503a216038e385f1e69577cb553f9ce0d0a4fc2b228580d4edc7336f1`로 불변이다. 기존 보호2727개 불변·새fixture 포함2728개/fixture908개다. 최초 fixture SHA256은 `77cea61c5e9a9f571dd5e1ed094adb352e53437ed12959816b006d3d237b1350`이며 다음 기준은 `extracted/cpp-resume-797/protected-after.json`이다. [JSON 보고서](../../cpppj/burning-victim-parent-20261103.json)를 따른다.
