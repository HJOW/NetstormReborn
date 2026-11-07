# 1082 — R 인사와 LAN 제어

`RootControl`이0Bh/0Ch/66h 수신과00475e60 서버재공지를현재R일반/대기표·실제전송에연결한다. 제품1082/11.01,EXE 12,366,336바이트,SHA-256 `50308f6d730307aa2f975d148cff387c27f1dad8106b027e1e1cddcec30c8597`다. 직전CP는 `47d5d1e9 / 20261107-147 CP`다. **새R.exe/최종owner의실제Install·Announce/UDP번호발행은아직없다.** 컴파일/정적접합을LAN실행검증으로계산하지않는다.

## 인사 수신과 응답

RootControl.Install은0b/0c의가변길이observer/버전1·정책0과66h의빈원문observer/버전1·고정12,세nullable수신을현재표에등록한다. 실제옵션/주소·연결표는복제하지않고같은부모를빌린다.

00475070은exitRequested의실제_exit0→같은ReadTransceiver상위조회→실제수신연결포인터동일성진단462→비영일때0Ch응답→같은노드flush순서다. 응답은 **wire10.71/extra0**이며제품표시11.01과구분한다. 유효pending음수에서상위조회가0인원문의특이관문을여기서일반pending조회로바꾸지않았다. 진단이복귀하면원문처럼같은조회결과로진행한다.

00475040의0Ch수신은종료관문뒤즉시반환한다.443a00/443b10송신observer는extraDataLen DWORD의-1진단885/917뒤extra+12의최종WORD만쓴다. 수신방향에는실제필드읽기/쓰기없다.

## 66h와 음수 연결 guard

00475f90은Root이면명령0에서만재공지한다. 다른명령이나검색토큰을처리해새응답을만들지않는다. 비Root는signed sender>=256진단1132,음수이면현재실제pending토큰을빌려 **그노드의현재sender로ClosePending**한다.4754e0/475520을상위ReadTransceiver음수관문과합치지않았다. 양수1..255는현재일반노드에실제Terminate를요청한다.

## 현재 포트 재공지

ConnectionState에원문004d21d8/dc의현재udpBroadcastId/ipxBroadcastId(BSS0)를추가했다. 이값은일반연결번호이며노드토큰/OS핸들이아니다. 번호0도기존상위조회함수의슬롯0규칙을따른다. 최종초기화부모가실제UDP생성후반환된일반번호를여기에써야한다.

00475e60은기존MakeLocalServerDiscoveryPacket writer로명령1/현재basePort의 **동일12BYTE를한번만**작성한다. UDP의현재번호를조회한뒤같은노드로세번SendTo한다.20BYTE목적지는86800처럼초기0/family2/htons낮은WORD포트/ffffffff주소·tailDWORD0이다. 포트정보를클라이언트검색토큰으로대체하지않는다.

그뒤현재IPX번호를새로조회하고기존주소parser로0.ffffffffffff/현재포트를만들어동일packet을세번보낸다. 두갈래에서패킷을다시작성하지않아observer/통지의변경을보존한다. **실제IPX통신지원은현재LAN IPv4장치범위에없다.** 주소구성/호출순서를보존할뿐미지원장치의실패를성공으로합성하지않으며새IPX소켓/드라이버를열지않았다. 실제LAN TCP/IP대상의초기화/호출은여전히후속이다.

## 확인과 잔여

Release/x64 NetstormCpp빌드종료0·경고/오류0이다. 새단위/MP/게임/통신·소켓/새인사·재공지/IPX/원본R실행검사0,전체검사/34미션/ZIP반복0이다. 보호2800파일불변·새fixture0이다. 원문/PE·주요몸체기계어와같은현재표/장치를정적으로대조했다.

필드/실제256·32칸밖·반납노드는명시오류다. 송신실패callback뒤원문이가상호출을반복하는경우의전체실패/수명과구형힙/스택·SEH·미작성물리자체일치는실행미검증이다.

원문18수신중14구현/폐기80하나제외·**3수신(11/12/87)**이남는다. 다음은진단연결테스트11/12·87명령,보조객체·전체패킷등록/477e10초기화·478af0정리·새R.exe최종owner/실제UDP번호발행·Announce/Bind/Install/Run이다. 다른LAN hostname/0F존47·3BYTE/0F·1d·메타/OSlaunch·나머지LAN/Outpost/checking5·한국어후속/Edit시나리오UI도남아전체목표미완료다.

원시근거/기계어·정적감사·빌드로그는Git제외 `extracted/cpp-resume-1082/`,추적보고서는 `cpppj/root-control-20261107.json`이다. 마지막전체정밀디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며같은호스트기존산출물만읽었다. 재디컴파일/생성소스변경0이다.
