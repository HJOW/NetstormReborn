# 1044 — 삭제 선택자의 공통 송신 부모 통합

실제 Session 삭제 효과가 RawFormPacket writer와 방송을 직접 부르던 경로를 현재 World의 RawSquidTransmission으로 연결했다. 이로써 일반 Pop/재동기화와 같은004ac260의 client SID 진단·현재 가상 writer·같은2048BYTE 버퍼·양수 크기/연결 관문을 사용한다. CD10.72의 기존 호환 경로는 유지한다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이며 이번 정적 대조에서 산출물은 변경하지 않았다.

PE004ac260..004ac32e를 다시 읽었다. 원문 인자는 target/selector/writerArgument/flags 네 개다. selector=-2만 현재+58(buffer,writerArgument,flags)를 호출하고 나머지는+54(buffer,writerArgument)다. 삭제 공통 writer는 writerArgument를 읽지 않는다. **송신 대상은 writer 반환 뒤에도 첫 인자 target**이며 디컴파일의 param_5=param_4/param_5 대상 해석은 스택 추적 오류다. 기존 Pop(-1,-1,0,flags)과 SendTo(target,-3,1,1)도 같은 전체 부모를 사용한다. flags를 일반+54의 force로 바꾸지 않는다.

현재 자산 생성자83VT와 Depend/Process/Gump/Content 네 Form의 **87VT 전부 +58이004ac340**임을 PE로 확인했다. 이 범위는 기존 writer를 재사용하고 다른 현재 VT는 명시 오류로 구별한다. 공통 삭제 writer는 현재 헤더를 먼저 쓴 뒤 !isClientSid 진단(\Ns\O\Squid.cpp:2f6), SID WORD, 현재 일회 표식 sid 일치/0 소비, flags 상위WORD의fff0과 token OR 순서를 따른다. 실제 Client 진단이 정상 복귀하면 계속한다. 독립 writer의 진단 장치 부재는 기존 명시 오류를 유지한다. 앞선 헤더/표식·송신 효과를 예외에서 되돌리지 않는다.

정밀 C의 직접 호출은 6곳/4함수다.4a6d00/4a7350의 대상별 일반 writer,4ac560의 Pop,공통 삭제 부모를 현재 제품과 대조했다. 재동기화의 clientSidAllowed1→SameOwnerMode1(캐시-99)→현재 target→surface/기타 및 child 순회→SameOwnerMode0(캐시-99)→walker 공간 재등록→정상 끝에서 허용0은 **988에 이미 연결돼 있다.** 843에서 이월된 임시 허용/owner 캐시 부모를 미착수로 반복하지 않는다. 파생 프로세스 writer/나머지 decoder와 LAN 일반 프레임·메타/Outpost/checking5 감사는 계속한다.

NetstormCpp Release 빌드는 **종료0·경고/오류0**, EXE 12,146,176바이트/SHA-256 `b1e8330248bb282f957b9e5b2487d6116099d003acbaa43b7d85c298d586007c`다. 보호 2800파일 불변·새 fixture0이다. 신규 MP/단위/게임/통신 검사는 사용자 지시대로0이다. 이 기록은 정적 대조와 코드 연결/컴파일이며 삭제 패킷·LAN 플레이의 실제 검증 완료를 뜻하지 않는다. 다음은 남은 LAN→한국어 후속→Edit 시나리오 UI다.
