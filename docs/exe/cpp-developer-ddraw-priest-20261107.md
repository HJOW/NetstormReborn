# 1039 — DirectDraw 진단과 getPriest 명령

31h의 queryDirectDraw/qdd와 getPriest/gp 두 명령을 연결했다. 누적32개 접합이며 addLabel과 그 기록 부모, 실제 입력/명령 소비자 감사는 남았다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이다. 원문 C와 보호 PE를 읽었으며 원본/디컴파일 산출물 변경은 없다.

qdd의4429e0은 보호 PE에서 xor eax,eax/ret(33c0c3)이므로 실제 원문은 항상 드라이버 부재 안내 두 줄을 출력한다. 이 반환을 새 DirectDraw 열거로 바꾸지 않았다. 이어 같은 시스템 ddraw.dll의 버전 리소스를 Windows W API로 읽고 Product/File 네 개 signed WORD 버전과 VS_FIXEDFILEINFO의 날짜 두DWORD를 원문 순서/서식으로 출력한다. Creation Date는 OS 파일 생성 시각이 아니다. 원문의 ANSI 시스템 경로는 Windows10/11 호환을 위해 W 경로로 대체했다. 자료 부재는 원문 세 getter처럼0이며, 손상된 리소스의 미정 포인터 읽기는 호스트에서 거부한다. version 라이브러리를 정적으로 연결했으며 DLL/DirectDraw 장치/네트워크를 초기화하지 않는다.

getPriest는 server가 아닌 현재 missionPost 관문 뒤0,0 중심의1000칸 일반 Finder(flags0)를 순회하고 현재 로컬 owner의 첫 사제를 선택한다. 같은 현재 사제 TYPE로 생성하고 비로그인 첫 Player1..8에 WORD→가상 owner를 지정한다. 모두 로그인 상태라면 생성자 값을 유지한다. 원래 사제의 현재 발자국에서 기존 RawBoardTarget.Side(2,4)로 동쪽 변 중간점을 구하고 실제 Pop(flags0) 뒤 기존 가상98을 호출한다. 기계어44e466..475는10000/현재 원래 사제 owner/원래 사제SID를 인자로 전달하므로 디컴파일의 모호한 지역변수를0으로 합성하지 않았다. 이 명령의 실행 검증은 피해/포획 관련 사용자 제외 범위이므로 하지 않았다.

첫 제품 빌드는 종료0·경고1(C4244)·오류0이었다. Player1..8을 전달하는 WORD 인자를 명시uint16_t로 바꾼 뒤 보완 NetstormCpp Release 빌드 **종료0·경고/오류0**을 확인했다. 원시는 build-first.log와 build.log로 나눠 보존했다. EXE 12,109,824바이트/SHA-256 `7df7af5b10534d0bbefbbc7e07a8e011a63f91ddd2aff4f0572c041247c2388a`다. 보호 2800파일 불변·신규fixture0이며 신규 MP/단위/게임/통신/진단GUI·공중3종전용 및 지정된 사용자 검사는 실행하지 않았다. 실제 명령/LAN 동작 검증 완료가 아니다.

다음은 addLabel/기록 부모와 입력·cr65h 감사다. cr의 PE 직접 참조 조사(명령54288c,설정5326d0/d4)에는 기존 setter·32h 송수신 외 직접 소비자가 아직 없으며 임의 화면 크기 변경을 추가하지 않았다. 이 조사를 전체 간접 경로 미도달 증명으로 확대하지 않는다. 이후 LAN 나머지→한국어 후속→시나리오 UI 순서를 유지한다.
