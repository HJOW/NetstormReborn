# 1084 — R 명령 페이지와 CCh 응답 부품

87h가 사용하는 서버 템플릿/섹션·공통 치환과 실제 확장 CCh 응답 부품을 작성했다. 제품1084/11.01, EXE 12,366,336바이트, SHA-256 `e9700d3482ec0c8f082e9d05eb457397874b47d1100d182ed89a24ef922b65ca`다. 직전 CP는 `344cf339 / 20261107-150 CP`다. **87h 전체 수신/길드·관리자 분기와 최종 새R.exe owner는 아직 없다.** 부품 컴파일을 명령 전체나 LAN 동작 검증 완료로 계산하지 않는다.

## 실제 부모와 범위

원문00452f50은GET·길드 계열13개·ADMIN의15개 상위 명령과 관리자6개 세부명령을 처리한다. 실제15개 이름과 원문 철자는 JSON 보고서의 commandInventory를 따른다. 명령 몸체는 `00452f50-004550a7;00455e19-00455e2b` 합8555BYTE로 비연속 tail이 있다. byteCount만큼 연속된 임의 영역을 읽지 않고 함수표의 모든 범위를 별도 기계어로 확인했다. wrapper00476550은 CHAL 진단1365/signed zone·현재CCh 관문이다.

이번에는 이 전체 수신이 사용할 여덟 몸체(451f30/452170/452300/452350/452460/452570/44a5d0/44b3d0)를 복원했다. 빈 수신기나 일부 명령만 처리하는87h를 등록하지 않았다. 등록18개 중16구현·폐기80제외·87h 하나 남음은 그대로다.

## 템플릿과 문자열

CommandPages.Load는 매 요청마다 **Admin.server→Update.server→Guild.server**를 같은 실제 Working/Saves 장치로 연다. 텍스트모드·get30/최대29BYTE·NUL delimiter 미소비와 EOF/fail의 다음 반복 관문을 실제 stream에 맡긴다. 파일 사이에 LF나 구분자를 추가하지 않고 캐시하지 않는다. 사용자 홈 파일이 우선이며 원래 CWD 입력도 유지한다.

Section은 `[name]`의 첫 위치에 marker 길이를 더한 뒤 그 위치부터 첫 `[END]`를 찾는다. END가 없으면 현재 끝까지다. **시작 marker 부재를 정상 빈페이지로 고치지 않았다.** 원문은 -1+marker길이 계산을 진행하며, 이것이 실제 문자열 밖 검색/잘못된 부분 길이가 되면 호스트에서는 명시 오류다.

Replace는 원문 사본의 BYTE마다 겹치는 일치도 세고 `위치-oldLength*ordinal+newLength*ordinal`로 DWORD 보정한다. inclusive 삭제/삽입 관문과 선택 한계를 유지한다. 삽입한 tag를 재검색하지 않으며 limit-1만 무제한/limit0은 첫 교체 뒤 반환한다. ReplaceInteger는 signed %d다. 비어 있지 않은 tag가 호출 계약이며 빈tag/문자열 signed 물리초과는 명시 오류다.

## 공통 치환과 실제 응답

Expand는 **TIME→ZONELIST** 순서다. TIME은 게임 FullTime이 아니라 실제 `_time32`/현지시각/asctime 앞19BYTE와 ': '다. ZONELIST는 현재0..50칸의 비NULL CCh만 읽으며, 원문 `herf` 철자와 `X1">`/공백도 유지한다. 활성 참가자 수 관문이나 교정된 href를 새로 만들지 않았다.

ChallengeRuntime.SendCommandPage는 지역8BYTE 객체의 첫BYTE/뒤DWORD만0으로 쓰고 BYTE1..3은 현재 CRT allocation 입력을 유지한다. 기존 실제 Message(version3/확장81·text strlen+1·현재 참가자 subscriber/Send)를 공유한다. 빈페이지를 보내는지는 호출자가 판단한다.

CommandPages.Bind는 같은 장기 owner의 Load/Expand를 CCh hooks에 연결한다. **최종 owner의 실제 Bind/87h 소비자 호출은 후속**이다. 현재 originals/originalCD에는 세 `.server` 파일이 없다. 원본 콘텐츠를 만들거나 설치 경로에 기록하지 않았다. 파일 부재면 Load는 빈문자열이며 이후 Section의 원문 계산이 물리 밖이면 명시 오류다. 최종 배포/페이지 콘텐츠 확보도 남는다.

## 확인과 다음 작업

Release/x64 NetstormCpp 빌드 종료0·경고/오류0이다. 새단위/MP/게임/통신·새페이지/패킷/응답/원본R 실행검사0, 전체검사/34미션/ZIP 반복0이다. 보호2800파일 불변·새fixture0이다. 주요 원문/PE·기계어와 현재 파일/존·CCh 접합만 정적으로 대조했다.

구형 stream/CharString 힙·미작성 스택의 물리 자체 일치, 실제 locale·시간대/2038·파일손상/송신 수명은 실행 미검증이다. 다음은 길드 상태·회원/요청 변경·저장·목록 HTML과 관리자 명단·제재를 복원해87h 전체에 연결하는 작업이다. 현재 Guild.id로 이름 붙인 첫DWORD는 원문 ON/OFF/모집 상태로 사용하는 필드이므로 해당 의미/이름도 후속 길드 작업에서 고쳐야 한다.

보조객체·전체 패킷/477e10초기화·478af0정리·새R.exe 최종owner/entry·실제번호발행/Bind/Install/Run, 다른LAN hostname/0F존47·3BYTE/0F·1d·메타/OSlaunch·LAN/Outpost/checking5·한국어후속/Edit시나리오UI도 남아 전체 목표는 미완료다.

원시/기계어·정적 감사/빌드 로그는 Git제외 `extracted/cpp-resume-1084/`, 추적 보고서는 `cpppj/root-command-pages-20261107.json`이다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 같은 호스트의 기존 산출물만 읽었다. 재디컴파일/생성소스 변경0이다.
