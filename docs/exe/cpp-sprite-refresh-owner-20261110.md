# 작업1233 — 일반 Sprite의 전용 표시와 장기 Regular

**제품1233의 구현과 허용 검증을 마감했다.** TYPE161/VT0050cc08의 실제4791b0 표시와4794b0 정기를 복원한다. 첫 빌드의 검사 파일 `<filesystem>` 누락(C2027)을 고쳤다. 빌드 종료 뒤 소유자 통합 입력을 실제 다리 역할의 현재 그래픽/개수로 명시하고 장치 관찰/중립 소유자의 공유 수명을 보완했다. 판정 조건의 전역/필드 조회 순서도 원문과 맞췄다. 첫 실패 로그/소스와 기존 기대값을 보존했으며 빌드 중 제품 소스를 바꾸지 않았다.

일반 Sprite 표시의 원문은498ff0의 현재 TYPE/frame/좌표 사각형→현재 HP 하위14bit/WORD8의 고정소수 배율→큰 배율에서 중심 확대→실제499310 dirty→BYTE35=20→기본 Form finder→현재 FindGump/Redraw(false) 순서다. 합/차/곱의 DWORD 감김과 signed 절삭 나눗셈을 유지한다. 일반 표시 억제·선택표식·그림자나 공통 FrameCheck 접두를 추가하지 않는다.

기존 메타4787c0은 전투섬/참가자 역할에 따라 확대 기준이 다르고 BYTE35를 쓰지 않는다. 두 원문의 같은 Form 순회 부분만 공유하며, 일반 Sprite의 `yesIKnowThatIShouldntLookInsideMyselfDuringADestroy=1`과 메타의0을 구별한다. Form next는 효과 전에 저장하고 다음 후보의 현재 TYPE/역할 DWORD는 다음 순회에서 읽는다. 제외 역할은 전체DWORD, processTypeEnd는 signed 비교다. free/dead 필터를 추가하지 않는다.

기존 실제 Client 장치가 현재 VT로 두 표시 본문을 선택한다. 일반 Sprite에 쓰기 가능한50BYTE를 넘기기 전 실제 같은 Storage/Pool 슬롯인지 검증하며 새 World/Session·Gump 장치를 만들지 않는다. 기존 어댑터 이름 `metaRefresh`는 두 Sprite 파생이 공유한다. SquidDisplay와 Frame/Unpop 사전 검증도 같은88/8c를 분배한다. 현재 World override가 없으면 같은 장기 Client 장치를 선택하고 누락을 빈 성공/일반 표시로 숨기지 않는다.

4794b0은 현재 프레임 DWORD를 먼저 저장한 뒤 현재 TYPE+114 개수로 `(frame+1)`의 signed 나머지를 계산하고 실제 Frame.Set(parent,next,0)을 호출한다. ADD 감김·음수 나머지·IDIV 0분모/몫 넘침을 보존하며 마지막에 원래 payload를 반환한다. Native NoWorld Host와 Session이 같은 현재 Pool/TYPE/Frame을 사용한다. 다른 Disc/방향별 프레임 진행으로 대신하지 않는다.

HJOW-Athlon의 마지막 Client 양판본2026-11-03/R전용2026-11-07 산출물과 보호 PE를 읽기 전용으로 사용했다. 새 디컴파일/원문 변경은0이다. 표시/정기·현재 개수/기본 finder 및 메타 비교13함수의 전체 C/PE를 보존했다. Sprite 표시의 CD10.72 대응은 정밀 메타데이터상00477c80이며 이번에는 실행/이식하지 않았다.

원본 격리 관찰은 **225입력(표시78·정기147)**이다. 원문의 표시/정기와 현재 개수·기본 Form finder 전체를 실행하고 bounds·dirty·FindGump/Redraw·TYPE 조회·Frame.Set만 명시 장치 경계로 두었다. 낮은/큰 배율·음수/홀수/역방향 사각형·DWORD 경계·현재 Form/역할 변경·IDIV 오류·quiet NaN/음수0 반환 비트를 포함한다. 전용 메타/MP/공중3종·게임 실행 관찰은 추가하지 않았다.

최종 Release/x64 제품2·집중1대상 빌드는29.928초/종료0·경고/오류0, 고정 소스/신규 근거16개 불변이다. 비MP **115/115**(신규6+기존109)·새 PE225입력/기존 직접 피해1420입력 재대조가 통과했다. 실제 NoWorld Host/Frame.Set의 현재 TYPE와 두 옛/새 Sprite 표시·BYTE35, 누락 장치의 부분 효과도 확인했다. 최종 EXE의 Tutorial4 생산·건설 요청→TYPE75/owner1 새 서버 객체 생성→600프레임 대기/정상 종료0·PNG 확인을 마쳤다. 보호 원본/CD/기존fixture·AGENTS **2,804파일**과 실행 전후 EXE가 불변이다. 시작한 빌드·집중·GUI 작업은 모두 종료했다. 건물 완성/전체 동일성 검증은 아니다. 실제 시전/특정 적 공격·포획/희생·승리/자연 완주·MP 단위/패킷/게임/통신/기록·공중3종 전용 실행과 사용자 직접 확인 항목은 제외한다. 전체 NoWorld/M6/목표는 미완료다.

[직전1232](cpp-bomb-treason-owner-20261110.md)·[현행 인계](../../LEFT_JOBS.md). 원시 `extracted/cpp-resume-1233/`.

표시88의 알려진83VT는 공통79·마나/볼트2·메타1·일반 Sprite1의 실제 분배로 연결했다. 이는83VT 모두 실제 실행했다는 뜻이 아니다. Regular60/87VT·잔여27VT/18함수이며 공통 Bomb16VT와 모든 큰 하위 마감도 구별한다. 다음은 공통 Carrier6VT/42a750의 같은 실제 장기 연결이다.

최초114/115의 통합 실패는 count0/frame0/표시0으로 관찰됐다. Factory 직후의 void 부모를 원문 Host가 정상적으로 건너뛴 것이므로 명시 입력을 실제 배치 상태로 교정했다. 제품 Host/시각/권한 관문은 유지했고 count1·프레임1·표시/dirty각2의 강한 단언도 유지했다. 최초 실패/진단과 각 소스/빌드를 보존했다.

- Netstorm.exe: 14100480 bytes, SHA256 `c266823fee98ba5b053da66fccdb0ae9d91f9a94ebbdc5869dc5a0f226a66558`.

- R.exe: 677376 bytes, SHA256 `e61d1013234d9a70afda5b37cd45f48da12707a47713077a6fdefa724e9dfe32`.

[확정 보고서](../../cpppj/sprite-refresh-owner-20261110.json). 전체 목표를 계속 진행한다.
