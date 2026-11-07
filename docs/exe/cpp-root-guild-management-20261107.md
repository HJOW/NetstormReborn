# 1085 — R 길드·가입 요청 변경과 실제 저장

기존 SharedFiles의 현재 길드/요청 표에 조회·생성/변경/제거와 사용자 홈 저장을 연결했다. 제품1085/11.01, EXE 12,366,336바이트, SHA-256 `f8bdea4263f9e2dce626cdc198ea5ad348a980c88fb560e4af81f1d5312545d7`다. 직전 CP는 `2113a51e / 20261107-151 CP`다. **87h 전체 길드/관리자 부모·목록HTML/관찰기·실제등록/최종 새R.exe owner는 아직 없다.** 부품 컴파일을 명령 전체/실제 LAN·파일 왕복 검증으로 계산하지 않는다.

## 같은 표와 올바른 필드

SharedFileTables를 복제하지 않고 기존 SharedFiles.Bind에서 같은 실제 owner를 CCh hooks.sharedFiles에 연결한다. Guild 첫DWORD의 이름을 id에서 **recruiting**으로 고쳤다. 원문은 정확히1일 때ON인 모집 상태로 읽으며 길드 번호는 vector 위치다. +4는 이름/+c는 비밀번호이므로 contact를 password로 고쳤다. GuildRequest.contact/value도 실제 의미인 playerName/approved로 고쳤다. reader의 입력 순서와 모든 DWORD 바이트 값은 같다.

5c720은 대소문자 구분 이름 첫 일치,5d7e0은 길드→회원 순서의 첫 subscriber다.0/중복 회원을 임의로 제외하지 않는다. 읽기 뷰는 같은 현재 표이며 변경/재로드 뒤 레코드 참조를 재사용하면 안 된다.

## 길드 변경과 로그인

5c9d0 생성 순서는 기존소속2→동일이름3→원문 빈리터럴 포인터동일4→모집0/이름·비밀번호/회원한명 추가·전체저장1이다. **빈 내용 비교가 아니다.** 44aa60은 빈 문자열도 별도 할당/strcpy하므로87h의 복사된 빈 이름은 리터럴 포인터와 같지 않다. GuildNameSource로 이 차이를 명시했고 CopiedString이 기본이다. 임의 빈이름 금지로 바꾸지 않았다.

5c7a0은 해당 길드 한행 제거·저장만 하며 요청은 지우지 않는다.5cd60은 중복검사 없이 회원을 추가·저장하는 원문 caller0 보조 몸체다.5d3e0은 첫 회원 하나만 제거하고 마지막이면 길드 자체 제거·저장, 그외에는 길드 저장, 미일치면 쓰기 없음이다.5d730은 정확히1→0/그외→1과 저장,5d7a0은 비밀번호 교체·저장이다.

5d960 로그인은 요청자가 **어느 길드든** 소속되어 있으면 지정 이름·비밀번호를 비교한다. 지정 길드와 실제 소속의 일치나 관리자/창설자 관문을 추가하지 않았다.

## 가입 요청과 저장

5dfe0은 이름없음0→모집!=1이면2→이미회원1→같은길드/같은subscriber 요청3→새 요청(길드/요청자명/subscriber/approved0) 추가·전체저장4다. 다른 길드 요청/승인 여부를 중복 관문에 넣지 않는다.

GUILDREQAPROVE가 사용할 변경은 첫 이름/subscriber 일치 요청에 approved1을 쓰고 즉시 전체저장한다. 실제 소속/권한 판단은 명령 부모가 맡는다. 이미 승인1이어도 저장한다.5e1b0은 요청 한행 제거·저장이다.5e210은 count에서 먼저 감소/index>0만 검사하므로 **0번을 건너뛰며 각 제거마다 저장**한다(원문 caller0). 승인만으로 회원 추가를 합성하지 않는다.

5c220은 Saves/guild.txt를 텍스트/trunc로 연다. unsigned 길드수+endl 뒤 각행은 signed 모집상태/이름/비밀번호/unsigned 회원수/회원 목록 순서이며 각 필드와 회원 뒤에 공백, 행 끝에 endl이다.5dde0의 Saves/guildreq.txt는 unsigned 요청수+endl 뒤 각 요청의 길드명/요청자명/signed subscriber+endl, 선두공백/signed approved+endl로 **요청당 두 물리줄**이다. 원문 현재순서/중복·끝공백/각endl의실제flush를 유지하며 인용/이스케이프를 넣지 않는다.

두 writer 모두 기존 WritableFile/Unicode 사용자 홈 경로를 사용하고 reader도 Saves 우선/원래 CWD 입력을 유지한다. 설치·원본 파일을 쓰지 않는다. 일반 stream 실패를 저장 성공 검증으로 보고하지 않는다.

## 확인과 후속

Release/x64 NetstormCpp 빌드 종료0·경고/오류0이다. 새단위/MP/게임/통신·길드/요청·새 파일왕복/패킷/응답/원본R 실행검사0, 전체검사/34미션/ZIP 반복0이다. 보호2800파일 불변·새fixture0이다. 원문/보호PE·기계어와 현재 표/실제 writer 접합만 정적으로 대조했다.

vector 실제 물리밖은 기존 at 경계이며 DWORD/signed 개수 초과는 명시 오류다. 기존 누락파일/32·40BYTE 필수문자열/미초기화 숫자 입력 경계도 유지한다. 구형 vector/CharString 힙/포인터·스택물리 자체, 실제 파일오류/왕복·동시수신/수명은 실행 미검증이다.

원문18수신중16구현·폐기80제외·**87h 미등록**은 그대로다. 다음은 기존 CommandPages와 이번 SharedFiles를 사용해 길드 목록HTML/전체87h와 관리자 명단/제재 변경·관찰기/실제등록을 연결하는 작업이다. 세 .server 원본콘텐츠/배포, 보조객체/전체초기화·정리·새R.exe 최종owner/실제호출·LAN hostname/0F·1d/메타/OSlaunch·LAN/Outpost/checking5·한국어후속/Edit시나리오UI도 남아 전체 목표는 미완료다.

원시/기계어·정적 감사/빌드 로그는 Git제외 `extracted/cpp-resume-1085/`, 추적 보고서는 `cpppj/root-guild-management-20261107.json`이다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 같은 호스트의 기존 산출물만 읽었다. 재디컴파일/생성소스 변경0이다.
