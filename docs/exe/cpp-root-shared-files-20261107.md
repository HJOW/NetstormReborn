# 1071 — Root 공유 관리·길드·뉴스 파일

`SharedFiles`/`SharedFileTables`/`SharedFileStorage`가 실제 R의 현재 관리 표·길드·뉴스와 파일 IO를 연결한다. 제품 기준은1071 / 11.01, 게임 EXE 12,366,336바이트, SHA-256 `cd9fa2b2d237c333c9133ad6986c68e9e843be4f952e02be3146f8ce1aec0b04`다. 직전 CP는 `41e91d11 / 20261107-119 CP`다. **새R.exe 최종 owner·실제 초기화/주기 호출은 아직 없다.** 새 부품/장치의 컴파일·정적 대조를 실제 서버/통신·파일 실행 검증으로 계산하지 않는다.

## 재해석과 현재 표

00477570의 blacklist→ipbanlist→chatlist→guild→guildreq→filter 순서다. 숫자 두 표는 각각004d7220/004d723c이며 signed DWORD 추출이 실패할 때까지 현재 표에 중복 없이 추가한다. 일반 재해석에서 비우지 않는다. 숫자 삽입의 할당 실패는 미삽입이며 호스트vector의 주소/용량은 구형 CRT의 _expand 자체와 다르다. CCh의 blocked/marked/자동ban 저장과 Config reload가 이 같은 현재 표를 참조하도록 Bind를 작성했다.

IP 입력0047a1f0은 줄이 아닌 **공백 구분 std::string** 추출이다. 구간마다 왼쪽0으로3문자 보충하고 빈 구간은000이다. 현재 표에 삽입하며, 조회 입력을 다시 정규화하지 않는다. 원문00457800은 두 문자열 전체 길이가 같음을 확인한 뒤 포인터를 전진시키면서 매번 줄어드는 strlen과 증가하는 index를 비교한다. 기계어457874/457880/4578a0·a9로 **길이15일 때 앞8BYTE만 비교하는 실제 특이동작**을 확인해 보존했다.00457970은 어느 쪽의 '*'도 한 문자 일치로 처리한다. 이를 일반 IPv4/netmask 판정으로 교체하지 않는다.

filter는32BYTE char* 지역 토큰을 순서대로 '/'로 연결하며 중복을 제거하지 않는다.00465620처럼 공백/EOF/NUL 직전에 멈추고 NUL 자체는 소비하지 않는다. 파일이 없으면 빈 문자열로 교체한다. PE의 초기 빈 문자열/0보충 상수도 대조했다.

## 길드와 뉴스

guild.txt는 현재 길드를 먼저 비운 뒤 signed 행수, id/32BYTE 지역을 거친 이름·연락처/회원수·회원 DWORD를 읽는다. 각 회원 지역은 원문처럼0으로 초기화하며 추출 실패도 현재0을 삽입한다. 소속 조회는 현재 길드·회원 순서상 첫 일치다. guildreq.txt도 현재 요청을 비운 뒤40BYTE 지역의 두 문자열/두DWORD를 읽는다. 대소문자를 구분한 guild 이름이 현재 존재하고 subscriber가 어느 길드에도 속하지 않은 요청만 삽입한다. 중복 행/회원 순서는 원문을 따른다. CCh의 guildName은 같은 현재 표를 읽는다.

00478ca0은 실제 CRT stat 초 단위 mtime이 달라졌을 때만 이전 뉴스를 반납하고 binary 전체 파일을 읽는다. **뉴스 checksum은 파일 변경 시각**이며 CRC로 대체하지 않는다.00478dd0은 blacklist mtime이 달라졌을 때만 숫자 표를 비우고 binary 파일을 LF별로 sscanf한다. 각 행의 첫 signed 정수 하나/마지막 줄/embeddedNUL 경계를 보존한다. 두 watcher의 시각은 별도 BSS0이며 stat실패/동일 시각에서는 기존 값을 보존한다. 시각 갱신/표·뉴스 반납은 읽기 성공 전에 일어난다.

## 실제 파일 장치와 사용자 홈

일반 관리 로더의 원문 CWD 입력과 뉴스/blacklist watcher의 dataPrefix 입력을 구별한다. 각각 먼저 기존 사용자 Saves의 같은 이름 파일을 확인한 뒤 기존 위치를 읽는다. SharedFileStorage::DataDirectory는 최종 owner가 Configuration::DataPrefix 확정 뒤 갱신해야 한다. 파일명은 UTF-8/실제 wide 경로로 연결하고 텍스트 입력은 CRT text 모드, 전체 입력은 원문00448ad0의 mode0='rb'처럼 binary를 사용한다.

자동ban은 현재 숫자 삽입 뒤 현재 순서 전체를 **사용자 Saved Games/Netstorm/Saves/blacklist.txt**에 저장한다. 삽입 여부와 무관하게 저장하며 각 signed 정수 뒤 LF/flush다.004597e0→num_put vtable004bf398+1c→00467430/004bf3c4='ld'를 따라 기본 signed 출력도 대조했다. 새 폴더는 기존 UserDataRoot 정책으로 만들며 설치/원본에 쓰는 fallback은 없다. 새 런타임이 실제 쓰기를 수행한 것은 아니다.

미발견 guild/guildreq 파일은 빈 현재 표로 정의한다. 원문은 실패 후 미초기화 지역 count를 소비할 수 있어 그 임의 바이트를 재현하지 않는 호스트 경계다. 불완전 필수 길드 행/char·IP 지역 물리 초과는 명시 오류다. 구형 CRT/locale·실제 IO실패/SEH·힙/미작성 바이트·시각2038 바깥의 전체 동작 동일성까지 증명한 것은 아니다. 이 경계를 정상 행/상위 실제 연결 미구현과 섞지 않는다.

## 확인과 남은 연결

Release/x64 제품 빌드 종료0·경고/오류0, 보호2800파일 불변·새fixture0이다. **새 단위/MP/게임/통신·소켓/DNS·새 런타임 파일 읽기/쓰기·원본R 실행 검사는0**이며 사용자 지시대로 실행하지 않았다. 전체 검사/34미션/ZIP도 반복하지 않았다. 다음은 주소 서식/해석·시계/기록/보조 객체·나머지 Root/CS 수신/입력과 새R.exe 전체 초기화/최종 owner·파일 watcher 주기/실제 호출,0F/1d 클라이언트 수신·메타/OS launch다. 부품 Bind를 최종 실행 연결 완료로 계산하지 않는다.

원문/PE·기계어·정적 감사/빌드 로그는 Git 제외 `extracted/cpp-resume-1071/`, 추적 보고서는 `cpppj/root-shared-files-20261107.json`이다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이다. 같은 호스트 기존 산출물만 읽었으며 재디컴파일/기존 산출물 변경0이다.
