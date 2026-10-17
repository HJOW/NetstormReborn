# 로컬 건설 요청25바이트와 실제 서버 수신·MayPlace

2026-10-17 단위133, 작업 PC `HJOW-Athlon`, 마지막 정밀 디컴파일 `HJOW-Athlon / 2026-10-16`. 기준 커밋 `aa054a6`. 같은 정밀 소스와 보호 PE를 읽었으며 새 디컴파일·원본/클론 게임·GUI 실행은 없다.

## 실제 수신 경로

4433b0이40a7c0/4df230으로 쓰는 요청은 프로토콜1/메시지3e/길이25다. TYPE BYTE(+4), x/y float(+5/+9), argument WORD(+d), 방향/품질 BYTE(+f/+10), 생성자의0시각8바이트(+11)를 보존한다. 실제 등록은 빈 stdcall observer40b380·버전1·정책0·수신444af0이다.

444af0은 현재558e74가 비영이면 바로 반환한다. 현재594fa4가0이고 sender DWORD가 로컬 owner와 다를 때 Money를 선검사한다. 이 비교는 ordered less이며 NaN은 부족 분기로 가지 않는다. Request444760이 실패하면 Reject442af0을 호출한다. Money 뒤 비용, Shortfall/Request/Reject 직전 패킷 필드는 현재 값으로 다시 읽으며 wire 시각은 사용하지 않고 Request에0/0을 전달한다. 연결 객체 인자는 사용하지 않는다. 비용 부족/환불은 단위132의 실제 Place에 연결한다.

Session의 실제0x3e 등록 API는 기존 RawPacketDispatch/LocalMissionTransport.Dispatch에 같은 수신기를 등록한다. Request는 전체 MayPlace→abstract 충돌→실제 유닛 밀어내기→받침/본 확정→실패 섬 정리다. 원본 Confirm으로 바로 건너뛰지 않는다. 생성용 지형 파이프라인의 임의 동맹 표와 별개로 게임 배치의 현재 Player/방향별 동맹/Graph/Editor와 원본 IslandList의 제한(신전)/존재 DWORD를 읽는다. IslandList 주소는 count DWORD를 포함한 +id×32+4/+8이다. CurrentPermission의 useAlliances540cb0 초기값1도 실제 PE에서 확인했다.

## 독립 관측

`tools/decomp_construction_command.py`는 실제 전체4433b0/40a7c0/4df230의25바이트64개와444af0의144개 입력을 두 정밀도로 실행했다. 총416정상 반환·정밀도 차이0이다. 타입/Money/Request/Shortfall/Reject와 원래 로컬 배치의 Form/생성/배치/이동/전송은 명시 입력 경계다. Money는 실제 ABI의 float10 입력을 공급하며 이 새 관측이 Money 몸체를 다시 검증한 것은 아니다. 기존 Money/Place의 원본 관측도 별도로 재사용한다. OS/GUI/실제 게임 호출은 없다. 새 두 fixture는 기존 자료와 구별한다.

## 검증과 남은 사용자 경로

최종 **Release 빌드2 종료0·경고0·오류0**, 새 집중6개/실패0, 전체 **1,537개/실패0/CTest 종료0/236.45초(전체236.48초)**다. 로그 접두사는`extracted/goal-local-construction-20261017-`다.

이 단계는 실제 요청 바이트·분배·서버 수신·공간 판정을 연결한다. 실제 Client의 전체 DWORD 메뉴→ContentForm6 취득/위치/수집/그리기/내려놓기→상위 비용 차감→RawConstructionLocal의 예측/조각 반환/기존 Builder→로컬 전송 큐와 로그인 연결은 계속 조립해야 한다. 커서/ContentForm 삭제의 현재 커서 취소·Prepare/Query/Activate도 실제 수명 몸체로 묶어야 한다. 검사에서 임의 성공으로 대신하지 않는다. 준비된 부품 생산, 경제/AI·GUI 자연 진행/승패·저장·다음 미션/사용자 완주도 남았다. 전체 목표는 active이며 전체화면 DirectDraw 수정은 제외한다.

기존 CD 대응은 Local4d14a0/요청 생성40a470/전체 Request4d17d0다. 현재444af0 수신의 직접 CD 대응은 기존 정밀 자료에서 확정되지 않았다. 새 조립과 새 CPU 관측은10.78 대상이다.

근거 감사36종도 통과했다. 기존 Request2,192/Local1,450행을 변경 없이 재감사했으며 Request 함수 목록의 첫 SHA 실패는 작업 사본의 CRLF였다. LF 내용=HEAD=기존 기대 SHA임을 확인해 해당 한 파일만 같은 HEAD LF로 복구하고 eol=lf를 고정했다. 내용/기존 기대 SHA/fixture는 그대로다. 새 관측의 실제 로컬 배치 범위 표와 실행기 의존 SHA·두 x87 제어값도 수록했다. 최종 재생성의 두 fixture 전체 바이트는 직전 검사 자료와 같았다. 현재 HEAD에 맞던21개 보고서의79핀만 이력과 갱신했으며 과거138핀은 보존했다. 보호3개 SHA도 일치한다.

서버 수신 등록이 참조하는 Session은 분배기의 해당 등록보다 오래 살아야 한다. 실제 Client/전송 소유자가 월드 교체 전에 등록을 해제하거나 분배기를 먼저 해제하는 수명으로 연결해야 한다. 이번 검사는 순수 송신 논리 입력에서 실제 RawPacketDispatch와 같은 Session의 수신을 실행했으며 로그인된 실제 로컬 전송 두 끝이나 GUI 내려놓기를 대신 검증하지 않았다.

인수인계를 완료한 뒤 `20261017-33 CP 실제 건설 요청 바이트와 서버 수신 및 전체 MayPlace 연결`로 로컬 커밋하며 push하지 않는다.
