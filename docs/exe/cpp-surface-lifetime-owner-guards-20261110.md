# 공통 삭제·표면 효과의 실제 전체 소유자 — 작업1253

공통 삭제의 파생 forward와 표면 fallWalker/bridgeEffect의 실제 pin/owner를 필수화했다. **제품1253의 해당 구현과 허용 검증을 마감했으며 전체 NoWorld/M6/목표는 미완료다.**

원문 알고리즘4파일(RawSurfaceLifecycle/RawBridgeLifecycle/RawIslandLifecycle/SquidDestroyLifecycle)은 변경하지 않았다. 실제 RawSurfaceWorld의 기존 큰 효과와 현재 Runtime pin 공개/반납 연결을 유지한다. 대여 없는 공통 삭제는 같은 장기 기본 VT 분배로 간다. 선택한 현재 대여는 실제 owner 확보 뒤 함수 사본을 만들고 정상/예외 복귀까지 보유한다.

수정 전 회귀3사례/6입력/6단언 실패를 소스/로그로 보존했다. 비전송 종속 통지·실제 Bridge 첫 제거 통지·NoIsland의 같은 칸 실제 walker 탐색으로 누락을 재현했다. 추가1사례는 세 효과가 실제 사용하는 수신자 카운터와 함수의 자기 대여 반납/예외 복귀 수명을 확인한다. 성공 토큰으로 owner를 대신하지 않았다. 생성/종료의 직접 건설 부모는 이 세 후단과 별도이며 기존 장치를 변경하지 않았다.

최종 **비MP300/300**, Release/x64 제품2·집중1대상 **55.904초/종료0·경고/오류0**, 고정3파일 불변이다. 최종 EXE Tutorial4 정상 UI 생산·건설 요청→TYPE75/owner1 생성→600프레임 대기→정상 종료0/PNG 확인을 마쳤다. 건물 완공·청취·전체 동일성은 단정하지 않는다. MP 단위/패킷/통신/게임·실제 포획/희생·특정 적 공격·승리/자연 완주·공중3종 전용 실행은0이다.

보호 원본/CD/fixture·AGENTS75e4eed9e **2,808파일 불변**이다. 마지막 Client 양판본 디컴파일2026-11-03/R2026-11-07은 현재와 같은 HJOW-Athlon이다. 새 디컴파일/원문 수정/ZIP은 없다. 글꼴 외부 fonts 계약은 M8/K1/K4/K5에서 계속 확인한다.

다음은 표시/전송·건설 직접 생성/로딩의 실제 부모 수명 분류다. RawSurfaceStorage의 NoteChanged/현재 표시·Pop/전송 선택적 pin과 ClientConstructionGlobals의 공개 전 직접 생성/로딩을 실제 caller별로 분류한다. RawSurfaceWorld 생성자 안에서 Display를 사용하는 실제 경계를 무조건 pin 부재 실패로 바꾸지 않는다. 큰 후단·최초 후처리·전수 process/caller/owner·Path/AI/배치/Fort Finish·M6-l/m과 후속 상위 계획도 남는다.

[확정 보고서](../../cpppj/surface-lifetime-owner-guards-20261110.json), 원시 `extracted/cpp-resume-1253/`.
