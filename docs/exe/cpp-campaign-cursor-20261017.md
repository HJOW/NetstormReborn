# 실제 캠페인 커서와 로컬 예측 연결 (2026-10-17)

기준 커밋 `ad64c18`, 작업 단위134, 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**이다. 새 디컴파일·PE CPU 실행·원본/클론 게임·GUI 실행은 없다. 기존 정밀 C/함수·가상 표와 보호 PE의 정렬된 기계어를 읽었다. 이번 조립은10.78 전용이며 기존10.72 관측/fixture는 보존한다. 전체 목표는 active이며 사용자 플레이 완성을 뜻하지 않는다.

## 연결한 실제 흐름

`RawCampaignSession::Load`가 같은 월드의 `RawCampaignCursor`를 소유한다. 취득/취소/Activate, Query/Prepare, Canon/SHP 미리보기 생성과 재사용, 마우스 위치, Drop, ReturnPiece, Local을 이미 복원한 각각의 몸체로 연결한다. 현재 커서/카메라/Canon 패턴/Pool/TYPE/장부/에너지/전체 MayPlace/마나/사정거리는 단일 상태다. 기존 자산 연결기에서 Canon/SHP 부분을 분리해 같은 몸체를 재사용했고 장치와 무관한 가짜 치수/허용값을 만들지 않았다.

내용물 삭제는 값형 cursor 초기값 대신 현재 `CursorDrop.preview`를 getter로 읽는다. 기존 독립 fixture는 원래 값형 필드를 유지한다. 취소는 잠긴 상태에서도 실제 꼬리의 Activate1/목록/프로세스 정리를 실행한다. ContentForm 분리/정렬 부착/현재 부모 통지는 같은 Host와 가상 분배기를 사용한다. 커서의 void/buried 임시 island94/stalag95도 실제 VT를 확인한 뒤 기존 표면/공통 삭제에 연결하며 일반 살아 있는 섬으로 이 추가 허용을 넓히지 않았다.

Local은 같은 실제 Factory/Place/Contained/Regular/Builder를 사용한다. 실제 Form 조회/삭제→조각 반환→클라이언트 SID/예측 배치→134/23 및 공급105/39 생성→서버9에 요청25바이트 송신→상위 SP 차감→마지막 커서 정리 순서를 유지한다. 반환 타이머는 실제 메모리를 예약하고 같은 Kernel/Host에 Regular46을 부착한다. 새 생성/이동/비용 계산을 따로 구현하지 않았다.

송신 callback은 원본처럼 큐에 넣는 경계여야 한다. 서버 요청을 같은 호출에서 즉시 처리하면 상위 비용 차감보다 앞서므로 실제 로컬 전송의 수명/로그인/큐잉을 별도로 연결해야 한다. 현재 검사는25바이트를 보관했다가 실제 RawPacketDispatch의 동일 서버 수신기에 전달한다.

GUI/장치 callback은 장기 저장소에서 매번 현재 것을 읽는다. 장치 교체가 커서 몸체를 재생성하지 않으며 선택한 미연결 경계는 명시 오류다. `MakePriestMenu`는 별도 취득 소비자가 없을 때 같은 커서를 기본 연결한다. 실제 Client의 메뉴/Gump/생산 GUI/입력/커서 수집·그리기는 아직 연결하지 않았다.

## 입력과 검사의 한계

보호10.78의 `43ec00` 전체를 함수 시작점에서 정렬 해제해 `43eda7..43edca`의 Form.flags8 검사와 취득41/기본1을 확인했다. 정밀 C와 일치한다. 정렬 해제 기록은 `extracted/goal-campaign-cursor-20261017-factory-take.txt`다. 이것은 새 CPU 정상 반환 관측이 아니다.

생산 준비75는 복제 Fort의 명시 저장 입력이다. 실제 로더는 타입/부모의 listFlags를 저장 BYTE보다 우선하므로 준비 조각은10이다. GUI에서 집는 반환8/품질3 Form은 같은 실제 contained 생성기로 별도 공급한다. 완성76 공급원도 실제 Create/Owner/Pop으로 명시한 시작 입력이다. 생산 완료를 이미 구현했다는 의미는 아니다. 에너지/MayPlace/SourceAllowed/권한/force는 우회하지 않았다.

마우스 검사는 실제 현재 hotspot/SHP 치수와0 및137/91 카메라 입력을 사용한다. GameWorld의 초기 Home 카메라를0으로 착각했던 검사 입력을 수정했다. 메뉴 취득 검사는 실제 SelectBuild 처리기에75를 명시 입력한 것으로,75가 원본 Open 메뉴의 행으로 제공된다는 주장은 아니다.

새5개 검사는 같은 상태의 취득/이동/취소, 현재 Form의 외부 삭제와 잠금 중 꼬리 정리, 실제 생산 Form 취득/Drop/예측/비용/송신/서버 교체/같은 Builder의30/17 완료, 미연결 장치 오류와 교체, 실제 반환 조각/품질 감소/소멸 Regular를 검사한다. 기존 커서 마나/사정거리/에너지3개와 합쳐 집중8개가 통과했다. 전체 Kernel을 켠 미션 자연 진행이나 실제 로그인된 전송 두 끝을 검증한 것은 아니다.

## 검증과 인수인계

최종 Release 빌드10은 종료0/경고0/오류0이며 빌드9 전체 **1,542개/실패0/CTest 종료0/221.54초(전체221.58초)**다. 최종10의 차이는 삭제 연결 설명 주석뿐이며 동작 변경은 없다. 집중8개/새5개/기존396행 재생도 통과했다. 수치는 `cpppj/campaign-cursor-session.json`과 LEFT_JOBS에 기록한다. 로그 접두사는 `extracted/goal-campaign-cursor-20261017-`다. 원본 보호3개 SHA, UTF-8/LF, 기존 CMake 바이트 접두, 기존 fixture/AGENTS/DN 불변과 현재 HEAD에 맞던 보고서 핀만 갱신한 내역을 감사한다.

기존 근거 감사36종은 통과했다. 추가 ContentLifetime 독립 감사는 기록CRLF SHA와 동일 HEAD/현재 LF의 차이 및 당시 `extracted/contentlifetime/*/creation.c/functions.tsv` 부재로 통과하지 못했다. LF를 CRLF로 복원한 SHA가 기존 기대값과 같음을 확인했으며 기존 보고서/기대 SHA/fixture/실행기를 바꾸지 않았다. 기존 C++ 두 판본396행 재생은 전체 회귀에 포함한다. 이 제한을 새 native 검증 완료로 확대하지 않는다.

다음은 실제 Client의 커서 장치/GUI/입력/Draw 소비자를 조립하고 실제 로컬 전송·로그인 수명으로 이어야 한다. 실제 생산 완료, 경제/AI/전체 Kernel, Early Missions1~6·캠페인1-1~1-2·데모3의 사용자 자연 완주/승패/저장/다음 미션과 전체 원본 동일성은 미완료다. 전체화면 DirectDraw 수정은 현재 작업 범위에서 제외한다.
