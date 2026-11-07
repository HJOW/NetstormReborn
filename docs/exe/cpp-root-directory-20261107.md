# 1065 — R Root 로그인 목록 응답

`root::RootDirectory`가 제공된 R의16h 요청에 대한 **0Fh 응답**과 추천 존/참가자 통계/친구 검색을 구현하고, 명시 Install에서 같은 실제 TransceiverTransport에 연결한다. 제품 기준은1065 / 11.01, 게임 EXE 12,365,824바이트, SHA-256 `16d80aadd1a345e53a8ec81e65fa08c8a34c0eb3c4324f91d2e7520f87c74907`다. 새 모듈은 정적 라이브러리에 컴파일됐으며 현재 게임 진입점의 실제 R 호출은 없다. 직전 CP는 `d1e53e49 / 20261107-99 CP`다. **새 R.exe·실제 초기화/호출과 CCh 관리자는 아직 없다.** 등록 API 작성을 서버 실행·접속 성공으로 계산하지 않는다.

## 응답 번호 정정

1056~1064 인계/보고서에서 Root 로그인 응답을17h로 적은 것은 오류였다. R00475660의 alloca 호출은 디컴파일에 allocator 인자가 누락돼 있다. 실제004756cb~004756ed와 PE004cd27e의 `0f 01`을 확인하면 Type0F/Version1, PE004cf188의 초기길이0→최소4이며 물리 크기253BYTE다. R00442950도0F observer00443c20과16h receiver00475660을 등록한다. **17h는 별도 RootNotifyPlayerChange/00476070**이다. 현행 LEFT의 번호를0Fh로 정정했으며 과거 개별 보고서는 당시 기록으로 남긴다. 1dh Challenge 응답 잔여는 그대로다.

## 현재 저장소와 응답

실제 관리자가 소유한 CCh2b80BYTE를 ChallengeDirectoryView가 빌린다. 원문 참가자는+0xd78에서188BYTE 간격40개이며 클라이언트의 다른 크기 CCh로 바꾸지 않는다. Count/Statistics는 참가자1..39의+64h DWORD bit1만 읽고, 평균은 활성 참가자의+5eh BYTE 합을 수로 나눈다. 친구 조회0044d3b0은0번도 포함한0..39의+4 subscriber를 찾으며 활성 조건을 추가하지 않는다. 실제 ConnectionState의51개 현재 토큰과 옵션/목록 checksum의 참조를 사용한다. 빈 관리자를 합성하지 않는다.

00475660처럼51칸의 비NULL 개수를 세고 설정 개수-1/0 여부를 실제969·970 진단에 전달한다. 그룹화는 `(레벨-1)*개수`의 IMUL 하위DWORD와 signed IDIV43으로 계산한다. 빈 존 하강→최소1→가득 찬 존 상승→상한→가득 찬 존 하강→최초 추천 복귀/마지막 한계 순서다. IsFull은 실제39명이며 옵션0이 아니면 현재 Count를 다시 읽는다. 임의 가장 한산한 존 알고리즘으로 대체하지 않는다.

allocator처럼253BYTE를ff로 채운 뒤 version1/type0F/초기길이4와 wire10.71·0·추천 번호·두 개수를 쓴다. 제공 R의 지역 서버목록 begin/end는00475697/69e에서0으로 초기화된 뒤 바뀌지 않으므로 요청 checksum의 양쪽 분기 모두 rootServerLen0이다. 현재 checksum DWORD는+16에 쓴다. 임의 외부 서버 목록을 생성하지 않는다. 이어 각 현재 비NULL 존의 번호/인원/평균을3BYTE씩, 친구를 존 순서→요청 순서로5BYTE씩 쓰고16건에서 멈춘다. 친구 중복은 원문처럼 보존한다. 송신 observer00443c20이 `20+rootServerLen+signed(numChals)*3+signed(numFriends)*5`를 최종 길이 WORD에 쓴다.16h observer00444180과 버전1/정책0도 등록한다.

음수 sender는 ReadTransceiver의 별도 관문을 거치지 않고 lower PendingTransceiver를 읽는다. 같은 실제 노드로 보낸 뒤 guard처럼 그 노드의 **현재 sender**를 읽어 ClosePending한다. 송신 예외에서도 guard 정리를 시도한다. 일반1..255는 현재 일반 토큰을, 복합 번호는 현재 CCh의+2ad8/lowBYTE 토큰을 사용한다. 전송 자체는 기존 실제 observer/소켓/부분 쓰기/기록 장치에 맡긴다. 실제 CCh 관리자와 이 부모는 등록 callback보다 오래 살아야 한다.

## 확인 범위와 후속

원문을 대조하다 **10.78 클라이언트0040a110은 존 항목 간격47BYTE**, 제공 R00443d40은3BYTE이며 클라이언트 친구 오프셋0040a130은 여전히 존당3BYTE인 차이도 확인했다. 현재 미연결0F 클라이언트 수신을 추측으로 채우지 않았다. 004b4250의 실제 필드/버전·변환과 이 차이를 해결하는 후속 작업이 필요하다. 제공 R의 wire10.71을 제품 표시11.01과 혼동하지 않는다. 이 부품 작성은 LAN 호환 검증이 아니다.

Release/x64 NetstormCpp 빌드 종료0·경고/오류0, 보호2800파일 불변·새fixture0이다. 사용자 지시대로 새 단위/MP/게임/통신/소켓 열기/원본R 실행 검사는 모두0이며 전체 검사/34미션/ZIP도 반복하지 않았다. null CCh/잘못된 참가자 번호/실제 패킷·저장소 물리 부족은 호스트 예외로 막고 원문의 잘못된 주소 역참조를 재현하지 않는다. 예외 중 추가 진단/닫힘이 실패하는 경우 원문 SEH 자체 일치는 주장하지 않는다.

다음은 실제 R 초기화/옵션·CCh 생성과 관리자·Root/CS 나머지 수신·시계/기록/보조 객체·새R.exe/호출이다. 이후 안전한0F/1d 수신과 메타·OS launch를 연결한다. 원시 함수/PE/기계어·빌드 로그는 Git 제외 `extracted/cpp-resume-1065/`, 추적 보고서는 `cpppj/root-directory-20261107.json`이다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**, R 전용은 **2026-11-07 / HJOW-Athlon**이며 이번 재디컴파일/산출물 수정은0이다.
