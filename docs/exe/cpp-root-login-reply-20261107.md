# 1093 — Root0Fh 수신과 서버 목록 저장

제품1093 / 표시·파일·제품버전11.01이다. 실제 메타 TCP/UDP 공유 등록표에 **0Fh 수신/관찰기**를 연결하고 같은 Client의 메타 기록·State/갱신 예약·서버 목록을 사용한다. 1092는 `d677b74e / 20261107-172 CP`다. Netstorm.exe 12,397,056바이트/SHA-256 `5476b3d56be9acc0c0a10510a9f53996cb27963983d4088b475ec979c616447e`, 새R.exe 669,696바이트/SHA-256 `c359d752fb1b93920cc0c96b4f38fcf7af840fdeb2abe9ec2bbd5c79526ef970`다. R의 송신 형식은 이번에 바꾸지 않았다.

## 원문과 버전 경계

10.78의0040b4b0은0Fh에0040a070 관찰기/버전1·정책0/004b4250 수신기를 등록한다. 실제 PE0040a110은 존 간격47BYTE이고0040a130의 친구 위치는 여전히 `20+rootServerLen+signed(numChals)*3+index*5`다. 0040a070의 송신 길이는 `20+rootServerLen+signed(numChals)*47+signed(numFriends)*5`다. **47BYTE 분기의 친구 오프셋 이상은 원문 그대로 보존했으며 해결했다고 주장하지 않는다.**

제공된 R은00443d40/475660에서3BYTE 존을 쓰고00443c20도3BYTE로 길이를 확정한다. wire10.71은 제품11.01과 구별한다. 004b4160은10.78과 다르면 로그하고, major가10이 아닐 때만 suppressVersionMessage 옵션/MetaVerDiff 안내로 거부한다. minor71은 허용되지만 원래 수신기는47BYTE를 읽으므로 제공 R과 곧바로 호환된다고 볼 수 없었다.

현재 코드에는 **명시한10.71 compact 호환 분기**를 추가했다. major10/minor71·양수 signed 존 수/비음수 signed 친구 수·선언 길이가 정확히 `20+rootServerLen+3*numChals+5*numFriends`이며 실제 물리가 충분할 때만3BYTE로 읽는다. 단순 minor 추정으로 다른 확장 응답을 compact로 읽지 않는다. 그 외는 원문47BYTE 해석을 사용한다. compact에 없는 이름/추가DWORD는 앞선 정리 결과를 유지하며 이름·주소·가짜 플래그를 만들지 않는다. 이는10.78 원문에 없는11.01 호환 경계이며 원본R 실행이나 MP 검증 결과가 아니다.

## 실제 수신 효과

004b4250 순서대로 버전 검사→같은0042b1b0 마커/표 정리→phase+44 서버 행의 값 갱신→네트워크 순서 WORD 서버 개수/각 길이와 최대255BYTE 복사·전체 길이만큼 이동→현재 서버 목록 저장→존→친구→현재 phase 분기를 수행한다. 버전 거부는 next12/+50=0만 쓰고 이후 효과를 실행하지 않는다. 목록 길이0이라도 원문처럼+20의 개수WORD를 먼저 읽는다.

존은 기록0을 비워둔 뒤 **기록1부터 순차로** 번호/인원/평균을 signed BYTE→DWORD로 쓴다.47BYTE에서는+2b DWORD를 기록+84h에 먼저 쓰고 이름+3의 NUL 포함 바이트를 기록+5ch에 복사한다. 친구는0번을 포함한 첫 존 번호 일치와16칸 중 현재0인 첫칸에만 subscriber DWORD를 쓴다. phase23이면 next24/+34에 signed 추천 존을 쓰고 그 외는 같은 MetaRefresh를 증가시킨다. 수신 앞에 sender/현재 phase 필터나 강제 성공을 추가하지 않는다.

MetaConnectionRuntime은 같은 실제 Client MetaZones/MetaRefresh/다섯 State와 기존 ClearClientMetaZoneRecords를 빌린다. 같은 Config/로그/현재 HTML Tell로004b4160의 MetaVerDiff를 연결했다.0Fh는 동기 Run을 새로 호출하지 않고 원문처럼 next/갱신 예약을 작성한다. **1dh 수신·State28..33/CS·실제 메타 표시 부모는 다음 작업으로 남는다.**

## 서버 행과 사용자 홈 파일

ClientNsInfoGlobals의 기존 servers가 현재 목록이다.00458fe0/594a0처럼 최대78BYTE endpoint/설명·signed DWORD 형식을 쓰고, endpoint를 CRT 바이트 소문자로 바꾼 뒤 현재 행의 대소문자 무시 첫 부분 일치를 찾는다. 기존 행 교체는 값 보존 선택과 stale 체크섬을 유지한다. 새 행만 전체 중복 검사 뒤 추가/unsigned 바이트+1 체크섬 재계산을 한다. 서버 값 갱신은 phase+44의 유효 인덱스에서만 시작한다.

저장은00458e30의 같은 실제 RawFileName 한 칸(root/DataDir/!rootservers.dat)을 작성한 뒤 물리 파일만 **사용자 Saves/!rootservers.dat**에 쓴다. binary 행 첫NUL까지 기록하고 LF가 하나도 없는 행에만 LF 하나를 추가한다. Unicode FILE/RAII는 현대 Windows 경계다. 다음 시작은 사용자 파일→기존 DataDir/!rootservers.dat→rootservers.dat 순서로 읽는다. 설치/보호 원본에 쓰지 않으며 목록에 들어온 외부 주소가 현재 LAN 접속 정책을 우회하지 않는다.

기존 시작 reader의 긴 행/EOF 전체 복원, 원문 논리capacity/NULL 행·32비트 FILE ABI 자체 일치는 이 단위에서 확장하지 않았다. 실제 패킷/표 밖 읽기·쓰기와 버퍼 초과는 호스트 예외로 막는다. 새 수신/파일 갱신·옵션·메타/통신은 실행 미검증이다.

## 검증과 다음 작업

최종 NetstormCpp/NetstormRoot Release/x64 빌드 종료0·경고/오류0이다. 최초 빌드의 로컬 변수 current가 멤버를 가린 C4458 두 건은 candidate로 바꿔 최종 재빌드에서 해소했다. Native 클라이언트28함수/제공R3함수의 원문·전체 범위 PE와 현재 배선을 정적으로 대조했다. 보호2800파일 불변·새fixture0이다. **새 C++ 단위/MP/게임/통신/새R·원본R 실행0**,전체 검사/34미션/ZIP 재생성0이다. 정적 감사는 새 MP 실행 검사가 아니다.

다음은1dh 수신과State28..33의 현재 CS/메타 표시·나머지 수신을 연결한다. 세.server 콘텐츠/배포·LAN hostname/Config registryValue 경계·LAN/Outpost/checking5·한국어후속/Edit시나리오UI도 남아 전체 목표는 미완료다. 폐기7f/80/PIRATE quota와 공식 서버는 연결하지 않는다.

추적 보고서는 `cpppj/root-login-reply-20261107.json`, 원문/전체 범위 기계어·두 빌드/정적 감사 로그는 Git 제외 `extracted/cpp-resume-1093/`다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**,R 전용 **2026-11-07/HJOW-Athlon**이며 같은 호스트의 기존 산출물만 읽었다. 재디컴파일/생성 소스 변경0이다.
