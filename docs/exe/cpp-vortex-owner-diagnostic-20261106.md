# 신전 소유자의 섬 누락 상세와 Client 진단 — 단위964

2026-11-06, HJOW-Athlon. LEFT_JOBS.md 3절4번을 이어갔다. 마지막 전체 정밀 디컴파일은 같은 호스트/2026-11-03이며 원본과 정밀 산출물을 바꾸지 않았다. 제품은964/11.01이다.

PATCH004dc58f..004dc5f8/CD0044f2a0은 이전 Player 신전 해제 뒤 섬 존재를 검사하고, 없으면 현재 count·섬 번호·좌표·이전/새 소유자를256BYTE 상세에 기록한다. 상세 writer 다음에 식0/원문 Vortex.cpp 경로/0x235(CD0x232) 진단을 호출한다. 좌표는 실제 CRT ftol의 signed64 절삭 하위DWORD이며 count/새owner도 signed DWORD로 표시한다. 범위 밖/NaN의 하위0과 큰 좌표의 감김을 보존한다.

기존 실제 Session은 이 경로를 SessionMissing 예외로 끝냈다. 새 ReportVortexOwnerMissingIsland는 같은 World Storage의 상세 기록→진단 장치를 연결한다. 진단이 정상 복귀하면 기존 신전 부모가 계속하고, 진단이 중단돼도 앞선 Player 신전 해제와 상세는 남는다. 장치를 모두 먼저 요구하지 않고 각 호출 위치에서 선택한다. 이후 별도 IslandList 진단/전체 파일·heap 수명은 이번 완료 범위가 아니다.

제품 구현 전 두 PE96행을 고정했다. 부모와 CRT ftol은 실제 명령이며 섬 조회/count·printf·상세 writer·진단은 명시 장치다. printf는 PE에서 읽은 원문 형식과 실제로 쌓인 여섯 인자만 포맷한다. 진단 진입에서 멈추므로 새 관측을 정상 ret4/전체 부모·원본 CRT printf 몸체·게임/MP 검증으로 확대하지 않는다. 상세 문자열/원문 진단 인자·전체 슬롯·아홉 Player의18DWORD를 비교하고 재관측96행이 일치했다.

첫 제품 후보에서 원문96행의 진단 경로 비교가 모두 실패했다. C++ 경로에 역슬래시를 이중 표기한 오류였다. 상세 문자열과 부분 상태 기대는 일치했다. 첫 Session 검사 기대에도 같은 경로 표기가 있어 그 첫 통과를 원문 일치 근거로 쓰지 않는다. Native 원시 바이트에 맞춰 제품 경로와 Session 기대를 교정했으며 fixture는 바꾸지 않았다. 최초96단언 실패를 원시에 보존했다.

최종 Release 게임/집중2대상은 종료0·경고/오류0, 고유9검사=새3/기존6·실패0이다. 새96행/장치 정상복귀·상세 후 장치 부재/실제 Session 상세 순서, 기존 신전864+864행/현재 Player·실제 World와962/963 Session 회귀를 포함한다. 실제 Session은 명시 좌표/상태 입력으로 섬 누락을 선택하고 Client 진단 진입에서 멈췄다. 실제 게임의 포획/희생/공격/승리를 재현하지 않았다.

새 게임/GUI·신규 MP/통신·공중3종 전용·물리 입력/PCM/CD·전체 CTest/34미션/ZIP은 실행하지 않았다. 마지막 실제 GUI는962 제품4진입이며964 제품 GUI 결과로 쓰지 않는다. 생성 slnx/vcxproj를 직접 읽은 현재82프로젝트=Application77(검사 이름72/게임1/probe·dump4)+라이브러리2+Utility3은 정적 목록이며 전체 컴파일/검사 수가 아니다.

EXE SHA-256은 `54cad8f52f8a261b7e8d0fdc5cd4bcbc297b3281d8b4ec04948d12626869c9d6`, 11,326,464바이트다. 기존 보호2795개/fixture975개 불변·새fixture1개로 다음 기준은2796개/976개다. [완료 JSON](../../cpppj/vortex-owner-diagnostic-20261106.json), [Native 보고](../../cpppj/vortex-owner-diagnostic-native-20261106.json), `extracted/cpp-resume-964/`를 따른다.

다음965도 같은3절4번의 Factory 외부 heap/할당 실패·파일/부분 I/O와 남은 실제 소비자다. checking8=부모3/부분2/MP3와 완료부모39/부분4는 불변이다. 전체 cpppj 목표는 미완료이며 계속 진행한다.
