# 편집 우클릭 선택과 소유자 변경 대화

2026-10-28 단위519. 현재 PC와 마지막 완료된 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 디컴파일 산출물은 바꾸지 않았다.

## 재현한 결함과 원문 경로

기존518 제품에서 원래 `BreakingThrough` 제단의 `CHEAT Set Owner`를 누르면 `스크립트 문자열의 해제되었거나 잘못된 토큰`으로 종료1이었다. 객체 메뉴의 정적 문자열 토큰을 Script 문자열 고리의 번호로 해석한 오류였다. 원본의 정적 `"SetOwner"`와 현재 호스트 고리 토큰은 수명이 다르므로, 객체 메뉴가 소유하는 정적 저장소에서 먼저 조회하고 기존 Script 해석은 그대로 유지했다. 메뉴를 닫아도 정적 문자열은 Client 수명 동안 유지된다.

소유자 변경 대화만 연결해서는 대상이 맞지 않았다. 원본 일반 입력의 편집 우클릭은 메뉴를 열기 전에 `4d5c90(clickedSid,0,0)`으로 선택하고, 현재 Editor가 계속 참이면 `4d5290(clickedSid)`을 실행한다. 이 함수는 현재 객체의 owner BYTE가 비영이면 **현재 Player Core를 SelectPlayerCoreData(owner,1)로 선택하고 map revision을 증가**시킨다. Subscriber 연결 변경이 아니다. 다이스를 클릭한 경우 선택은 원래 구현처럼 제단으로 따라갈 수 있으며, 후속4d5290은 처음 클릭한 SID를 사용한다.

SetOwner의 실제 이름 표/명령은 `542a24 / 020000cc`다. 원문 기본블록 `4d8176..4d81cb`는 다음 순서를 사용한다.

1. Editor가 거짓일 때만 missionPost를 조회한다. 둘 다 거짓이면 변경하지 않는다.
2. 현재 selected SID를 ESI에 보관한다. 비영이면 가상74에 새 owner 전체 DWORD를 전달한다.
3. 가상 효과 뒤 현재 Editor를 다시 읽어 참이면 **보관한 SID**로4d5290을 호출한다.
4. 허용 모드에서는 선택0도 map revision을1 증가시킨다. DWORD 감김을 유지한다.

release는 그 앞의 일반 입력에서 거부된다. 새 분배기는 release·미허용 모드·선택0에서 불필요한 월드 Runtime을 요구하지 않는다. 실제 owner 효과에 도달했을 때만 현재 Session을 요구한다.

`RawCampaignSession::SetObjectOwner`는 기존 실제 신전/사제/공통 가상 소유자 분배를 사용한다. 별도 owner BYTE 쓰기나 메뉴 전용 Player 기록을 만들지 않았다. HTML의 `Human Player`, `Computer #1..7`, `No Owner`, `Cancel`은 원래 Tell.SetOwner 자료와 기존 정수 명령 큐를 사용한다.

## 원문·빌드·검사

신규 도구는 **실제 x86 기본블록과4d5290**을 실행한다. 가상74와48f3b0 Core 선택은 효과 인자를 기록하는 명시 외부 경계다. 현재 Editor·missionPost·선택·새 owner·가상 효과 뒤 상태 변경·revision 경계의 **256고유 입력**을 생성했고 fixture 재현도 일치했다. 일반 입력 전체나 원본 게임 실행의 증거로 세지 않는다. [원문 보고서](../../cpppj/object-owner-command-native-20261028.json).

최종 Release 경고/오류0·실제 등록 **3347개**다. 최종 제품의 집중 **48고유/실패0**을 통과했다: ObjectOwnerCommand2, ObjectMenu20, ObjectCommand3, UserSelection13, PlayerCoreData3, VortexOwner4, ScriptCommand3. 앞 제품에서 같은48개를 실행한 관측과 중복되므로96고유로 세지 않는다. 전체3347개 검사 실행은 아니다.

이번 빌드에만 `CL=/MP3`을 전달해4코어 PC에서 번역 단위 컴파일을 병렬 처리했다. 프로젝트의 배포/런타임 설정은 바꾸지 않았다. 전체 재빌드 뒤 테스트 입력만 정정한 증분 빌드, 선택 경계의 지연 요구를 정리한 최종 빌드가 각각 종료0했다. 동시에 같은 출력 라이브러리를 갱신하는 빌드는 하지 않았다.

별도 소형 probe의 초기 실패도 보존했다. 응답 파일의 출력 경로 인용 오류, 검사에서 없는 Rows helper를 사용한 컴파일 오류, SID50 준비에 서버 할당을 사용한 검사 입력 오류다. 제품의 풀 쓰기 검증을 완화하지 않고 **클라이언트 할당(2)으로 실제 SID50을 준비**하도록 고쳐 정식 검사에서 통과했다. 최종 통과 수에 실패한 probe를 포함하지 않는다.

## 실제 입력 검증

최종 같은 제품에서 **제단(TYPE164)·윈드 타워(TYPE142, 자산 이름 windBlocker) × 1280×720·1024×768의4사례**를 수락했다. 각 사례는 같은 원래 객체에 대해 다음을 실제 UI로 실행했다.

- 편집 우클릭 → 실제 선택 SID와 현재 Player2/Core 선택 → 원래 Set Player 대화 열기.
- Cancel → owner2 유지, 선택과 현재 Player2 유지.
- Human Player → 같은 SID의 owner1·현재 Player1.
- Computer #1 → 같은 SID의 owner2·현재 Player2.
- No Owner → 같은 SID의 owner0·현재 Player2 유지. 원본4d5290은 owner0에서 Core를 초기화하지 않는다.
- 중립 객체를 다시 우클릭 → Destroy·About·CHEAT Set Owner만 표시, Player 하위 제외.

객체 TYPE·좌표·SID와 실제 활성 창, 대화 뒤 Mission phase 복귀를 확인했다. 각 사례는 원래 BreakingThrough 3355BYTE 맵을 담은 새 실행 사본을 사용했다. 입력 파일과 설정·원본/CD/AGENTS는 보존됐으며 편집 결과는 별도 data-editor 경로에 남는다. 실제 스크립트 상태나 선택/owner를 검사 setter로 주입하지 않았다. 도구는 [cpp_object_owner_smoke.py](../../tools/cpp_object_owner_smoke.py), 명령/해시/실패 포함 원시는 `extracted/cpp-object-owner-519/`, 통합 근거는 [보고서](../../cpppj/object-owner-runtime-20261028.json)다.

## 남은 범위

이4사례는 전체 타입/레벨/소유자/포획 상태의 동일성 검증이 아니다. 사제의 No Owner0은 기존 RawPriestOwner의 원문 진단 경계가 있어 일반 건물형의 중립 변경과 합치지 않는다. 신전/워크샾/나머지 건물형의 실제 전체 메뉴·전용 효과, Meltdown/Detonate·Dig Up special 클릭, 비석 후속 frame2 표시, 원본 전체 선택기004d5840과 입력 모드별 처리 등은 계속 남는다.

세 공격형 공중 유닛을 위한 생산·생성·우클릭 재현은 하지 않았고 정적 분석 제한을 유지했다. 연녹색 직사각형 분석·자연 완주·english 누락 개선 보류, 후순위 LAN의 새 검사/실제 통신 금지도 유지한다. 전체화면·Windows10/11·전체 캠페인 동일성의 완료 증거는 아니다.
