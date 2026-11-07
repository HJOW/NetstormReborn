# 1094 — Challenge 로그인 응답과 메타 상태 본체

제품1094 / 표시·파일·제품버전11.01이다. **1dh 응답과State28~33의 원문 본체를 라이브러리에 컴파일했다. 실제1dh 등록/28~33 분배는 아직 하지 않았다.** 실제 메타 표시/CS·HTML/카메라와 후속 패킷 장치를 먼저 연결해야 하며 빈 성공 callback으로 대신하지 않는다. 직전1093은 `d97301dc / 20261107-178 CP`다. Netstorm.exe 12,397,056바이트/SHA-256 `bbe2b12a52df4837333e61881c0846af1509916e06c7b1e872b401d4dbafe8e3`, 새R.exe 669,696바이트/SHA-256 `3fd0a482fe72e277b1db472ad78b108a7397d597ed17fa1d838b82381b430059`다.

## 1dh 응답 본체

004b8ac0/실제PE처럼 코드 BYTE 로그→같은 현재 Subscriber DWORD→current+38에unsigned BYTE 응답코드/current28→004b4160 버전 검사→거부이면 current12/+38=0→같은 실제 Run 동기 호출 순서다. sender/phase 필터·next/immediate 요청으로 바꾸지 않는다. 선택된 로그·버전·Run은 필수 장치이며 실패 전에 쓴 값은 되돌리지 않는다. 수신 등록은 후속이다.

## State28~33 본체

28 성공코드1은 같은 CS Reset→즉시29 요청→phase+34를현재존에복사→카메라 Reset이다. 실패는 번역된 `Challenge server login denied:%s`와0042b090의 이유를 표시하고 CSLGenericFail/코드3 AlreadyHere·4 BadNameLen·5 ServerFull·6 Shutdown을 선택한다. 실제 Tell serial을 **immediate+3c**에 쓴 뒤32 요청/카메라 Reset이다. 이유 표는 PE005316b0의 여섯 문자열이며 Unknown error를 먼저 번역한 뒤 선택값을 다시 번역하는 원문 순서를 보존한다.

29는 실제 진행창 닫기→metadataMode1/connected1→0047dd60 표시→Root 요청 **connection11**→네캐시ffffffff/통지0→현재포트 listener→next33이다.30은 원문 `Communicating with Challenge Server.` 표시/connected0,31은connected1이다.32는 실제 현재 모달창의 존재/serial을 current+3c와 비교해 next32 또는 즉시20 요청을 선택한다.

33은11번이 닫혔으면 `Challenge Server connection lost.` 표시/next3이다. PlayerKnown0은 CS/캐시를 만지지 않고 자연 반환한다. 그 외는 진입 시 현재CS/자기+a4를 캡처한다. 전투0이면 pending→1..9 닫기→네캐시 무효화/통지0이다. 전투가 있으면004b2c60처럼 현재 자기 전투의8참가자를 읽고0을 제외한 flags BYTE bit4의 첫 master를 찾는다. 별도 active 조건을 추가하지 않는다.

선택 master/그의+a8/자기+a4/+a8과 네캐시가 다르면 index/value/battle/slot→통지0→pending/1..9닫기 순서다. 자기master면004b2c00 통지,그외는 닫힘 뒤 현재CS의주소/포트로9번접속이다. **변경 분기의master=-1도 원문은 주소를 먼저 접근하므로 임의 생략하지 않고 실제 물리 경계 예외로 거부한다.** 이후 원격master이면9번 열림 확인/필요접속 뒤 상태가 비영이고 통지0일 때통지1→004b62b0 Login→next33이다. 음수 상태도 원문의 `test/jz`처럼 비영으로 처리한다. 일반 꼬리도next33이며 공통phase 확정은 외부 분배기에 남긴다.

## 현재 제품에 적용한 좁은 변경

기존 MetaConnectionRuntime의004b40c0 복원은 송신 목적지가10으로 고정돼 있었다. 원문 함수는 connection 인자를 받고 기존State22가10,State29가11을 전달한다. **SendRootLogin(connection)으로 수정했고 현재 연결된State22는10을 유지한다.** 새29 본체의 필수 hook은11을 넘기지만 이 본체의 실제 분배는 아직 없다.

005c85c4의 별도 DWORD를 ClientLocalStateGlobals의BSS0 장기owner에 추가하고 같은 ClientStateWorldGlobals 직접read/write에 연결했다. metadataMode/통지/다른bool과 합치거나 Session 교체 때 재초기화하지 않는다. 새 State의 참조는 이 같은 owner를 후속 실제 접합에서 빌려야 한다.

## 접합 잔여와 검증

필수 실제 장치는0047dd60/479760/47cab0 표시/Poll·현재CS수신/가상표·같은오브젝트/카메라·현재HTML serial·004b2c00/master통지·004b62b0/Login·같은실제전송이다. **이 장치가 완성된 뒤 MetaConnectionRuntime에1dh 수신을 등록하고 기존 분배기의28~33 범위를 연결한다.** 이 단위를1dh 접속/메타 화면 구현 완료로 세지 않는다. 본체가 요구하는 hook을 no-op/가짜ACK로 채우지 않는다.

NetstormCpp/NetstormRoot Release/x64 최종빌드 종료0·경고/오류0이다. 클라이언트10함수의 기존 원문/전체 범위PE·이유/문구포인터·현재소스와 배선을 정적으로 대조했다. 보호2800파일 불변·새fixture0이다. 새 C++ 단위/MP/게임/통신/새R·원본R 실행0이며 전체검사/34미션/ZIP을 반복하지 않았다. 단일게임스레드/현재CS owner·참조/hooks의호출전체생존을 요구하고 실제CS/패킷/주소NUL 물리밖접근은 호스트예외로 막는다. 실패문구는 원문의 번역 printf format/문자열 한 인자와256BYTE 지역 경계를 사용한다. 새 수신·메타/전송·장치 수명은 실행 미검증이다.

전체 목표는 미완료다. 위 실제메타표시/CS·장치접합→1dh/28~33실제등록을 우선하고 세.server콘텐츠/배포·LAN hostname/Config registryValue경계·LAN/Outpost/checking5→한국어후속→Edit시나리오UI를 이어간다. 폐기7f/80/PIRATE quota/공식서버는 연결하지 않는다.

추적 보고서는 `cpppj/meta-connected-bodies-20261107.json`, 원시함수/기계어·빌드/정적감사는 Git 제외 `extracted/cpp-resume-1094/`다. 마지막전체정밀디컴파일 **2026-11-03/HJOW-Athlon**,R 전용 **2026-11-07/HJOW-Athlon**이며 같은호스트 기존산출물만읽었다. 재디컴파일/생성소스변경0이다.

## 사용자 요청에 따른 일시 중지

이번 단위를 최종검토/문서/CP커밋으로 마감한 뒤 목표를 일시 중지한다. 다음 작업 단위는 시작하지 않았다. 최종 검토에서005318e8은 callback 사본 대신 같은const DWORD 참조로 빌리게 했고33의현재포트캡처→현재CS주소조회 순서를 보존했다. 마지막 재빌드도 경고/오류0이며 보고서/현행LEFT의 해시를 최종 산출물로 갱신했다.

검토 중 스테이징했던1094 중간13파일은 병행 `d5c033e1 / 20261107-181 DN` 커밋에 함께 포함됐다. 해당외부커밋을재작성하지않고 최종포트참조교정/제품해시/완료·중지인계를 별도CP커밋으로 마감한다. 타세션dotnetpj 변경은 최종CP에 포함하지 않는다.

재개지점/검사제외/마지막원문호스트와986미적용후보는 LEFT_JOBS.md0.111이 기준이다. 실제표시/CS·장치→1dh/28~33등록이 먼저이며 전체목표는 미완료다.
