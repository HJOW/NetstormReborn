# 단위661 — Edit State1의 공통 앞단과 정리 전 통지

2026-10-31, HJOW-Athlon(192.168.0.94). 마지막 전체 정밀 디컴파일은 같은 PC의2026-10-23 자료다. 원본/디컴파일 파일은 변경하지 않았다. 해시와 원시 로그는 [editor-entry-prelude-20261031.json](../../cpppj/editor-entry-prelude-20261031.json)에 있다.

## 누락된 State1 앞단

실제 `PrepareNativeFortState(true)`는 제목 첫BYTE를 지운 뒤 cleanup으로 바로 들어갔다. 원문의 State1/3 공통 앞단에는 이전 phase가 signed6 이하 또는11이면 SourceType0을 쓰는 선택과, 현재 battle/이전 phase14일 때 cleanup 전에 보내는52h 통지도 있다. 두 동작을 State3과 같은 함수로 공유하고 실제 State1에 연결했다.

`PrepareNativeEditorEntry`는 실제 current1을 요구하고 출처 선택→배경 첫BYTE0→현재 battle/phase 재조회→선택된 통지 순서로 실행한다. State1은 State3/6 전용 승패 결과 정리·전체 배경·음악 선택을 실행하지 않는다. 기존 MainMenu의 배경/음악 callback 뒤 현재값 재조회 위치도 유지했다. 통지 장치가 필요한 때 누락되거나 실패하면 cleanup/State1 확정 전에 앞선 쓰기만 보존한다.

State1과 State3은 같은 `SendBattleLeaveForState`를 사용한다. 기존5BYTE 패킷 작성과 현재 로컬9의 전송을 그대로 호출하며 수신/flush를 추가로 강제하지 않는다. 기존 Guide 해제는 Mission만 닫고 World 반납은 뒤 State1/7 부모에 남기므로 Test Battle→Edit 복귀에서도 이 위치가 필요하다.

## 원문 확인

보호 PE에서129개 명령을 게임 실행 없이 확인했다. `004b6ffd`의 signed JLE/phase11 선택 뒤 `004b7004`가 출처0, `004b7016`이 제목 첫BYTE0을 쓴다. `004b7022`의 분기는 current1을 State3/6 전용 결과/배경 처리에서 건너뛴다. 이후 battle/phase14 조건에서 `004b719d` 통지를 보내고 `004b71a6` cleanup으로 간다. 정적 명령 수를 새 PE 실행 관측으로 세지 않는다.

## 재접속 범위 분류

확인한 재접속 직접 작성/정상 호출은 일반 전투 경로다. `004b54e0`의 저장 writer는 **missionPost0·PlayerKnown 비영**일 때만 상태 파일을 쓴다. `004b5890`의 저장 내용/참여자 검증과 `004b5ba0`의 startup 질문은 이 상태 복구를 사용한다. `004b8cb0`의 서버 이전은 `004b8f4d`에서 next7, `004b8f57`에서 next+84=1을 쓰고 새 서버 재접속을 알린다.

이를 미션 중간 저장 기능으로 간주해 새 검사를 만들지 않는다. 후순위MP의 저장/서버 이전 사전 구현으로 분류하며 startup 질문/삭제 UI는 유지한다. 확인한 직접 쓰기와 정상 경로에 관한 결론이며, 간접 alias 전체가 없다는 증거로 확대하지 않는다. Zone0/메타 Poll/시계 패킷·외부 동기화도 단독 정상 미션의660 부모 경계와 구별한다.

## 확인 범위

집중60등록 중 **최종 고유11개/실패0**이다. 새2개/입력5개·기존9개이며 다른49등록은 실행하지 않았다.4회 실행/11함수 호출이고 최초 실패는 없다.

- 새 단독 Session 입력3개는 낮은 이전phase/battle0, Battle14, battle0/phase14를 구별한다. 실제 로그인/완료 뒤 Native State1 앞단을 실행해 출처/첫BYTE·이전 phase 유지·선택된52h 예약을 확인했다. 통지 직후 수신 기록은 늘지 않으며, 별도 큐 폴링 뒤 실제52h 수신과 sender의 Player flag 비트2 변경을 확인했다.
- 새 실패 입력2개는 필요한 통지 장치의 누락/예외다. cleanup에 도달하지 않고 phase/old를 확정하지 않으며 첫BYTE/출처의 중간 쓰기와 기존 World 대여를 유지한다.
- 기존 State3 여섯 검사 중 독립 원문41입력/두 제어워드82관측, Native State33행과660 부모 두 검사를 재대조했다. 재사용한 자료는115행이며82관측을82개 고유 입력으로 세지 않는다. BattleLeave 기존fixture는 보호 대조만 했고 별도 원문 재실행 수에 더하지 않았다.

cleanup은 명시 외부 경계이며 실제 Editor 전체 재설치를 성공으로 합성하지 않았다. 새 검사는 전체 파일/도구/GUI 설치나 실제 화면의 새 근거가 아니다. 단독 로컬 큐를 사용했으며 신규MP/실제 소켓 검사는 없다.

Release 빌드1회 종료0·경고/오류0이다. 관련 보호7입력(AGENTS/두PE/TAFF/기존fixture3개)은645 기준과 일치하며 전체2632개/기존fixture773개 재감사와 구별한다. 원시 근거는 이 PC의 `extracted/cpp-editor-entry-prelude-661-*`다.

게임/GUI·CD 열거·새ZIP·전체34를 실행하지 않았고 주 검사 EXE도 재빌드하지 않았다. 사제 포획/희생·특정 적 공격·승리·공중3종 전용 행동 재현은 하지 않았다. 스테이지 진입/대기는 더 많은 기능 묶음 뒤 필요한 범위를 몰아서 수행한다.

## 잔여

State1/LeaveEditMode의 나머지 파일/지형/소유자·시나리오 조합과 실제 전체 전환·UI 픽셀은 남는다. 일반 전투 저장/서버 이전, 메타 Poll/시계 패킷·외부 동기화는 후순위MP 무시험 사전 구현으로 잇는다. 기타 UI/연속 미션·호환성·BaseFile 소비자/ABI·시작 레지스트리·메타State19/LeaveBattle와4~7차 목표를 계속한다. 자연 완주/전체 동일성은 사용자 확인 대상이다.
