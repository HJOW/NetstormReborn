# 1155 — M2.4 메타1bh 전체 접속 수신

2026-11-08 / HJOW-Athlon. 직전 제품1154/커밋60b8e73b. 표시·파일·제품 버전11.01.

M2.4의 전체1bh/4b8b20·빈observer/고정38BYTE를 실제 공유 등록표와 같은 기록 FILE/현재State·동기Run에 연결했다. 인계 이후 재수신/재진입의 현재owner 복원은 새M2.5로 남긴다.

1bh는server 또는Zone예약이비영이면 checking·파일·패킷을읽기전에반환한다. 나머지는checking의phase29/33 진단이복귀하면계속한다. 선택적 기록은현재packet+4와자기Subscriber로 `%08X.%X.c.dat`를열며1ah의+10과다르다. 이후같은실제packet+8을현재State+54에NUL포함한BYTE씩복사하고현재게임포트/current7을쓴뒤같은processState를동기호출한다. next예약/명령큐/대기실ACK를합성하지않는다. packet+8은게임주소이며종료이유가아니다. 물리오류는선택된읽기/쓰기위치에서드러내고앞선Open/복사를되돌리지않는다.

4b8b20 C/PE의 진입server/Zone예약 조기반환→checking29/33·e39 진단→선택적 현재packet+4/Subscriber 기록Open→packet+8을현재+54에NUL포함BYTE복사→현재포트/current7→직접Run, 실제세번째packet 인자·빈observer/version1·정책0/고정38을 대조했다.

1153의정상메타→일반State7/즉시8·같은TCP/UDP/등록표/전체callback수명인계를사용한다. 다만첫State7이editor.metaTransport를비운뒤에도표에남아있는1ah/1bh가다시수신될수있다. 현재LoadNativeBattleWorldBody는그필드를요구하고,외부Poll의next7선택은World없는메타만허용하므로이반복경로는아직부족하다. 원문은checking진단이복귀하거나checking0이면추가phase거부없이같은State7로이어진다. 따라서**M2전체를완료로세지않고M2.5를즉시다음으로추가했다.** 실제source전체owner를인계받아현재World와같은장치를확인하고동기/예약State7을처리해야한다. 다른Worldcallback의기존정리보호를무조건풀거나phase/known/sender필터·다음Tick연기로가리면안된다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0,보호2800파일불변이다. 새C++단위/MP/게임/통신/전체/ZIP실행0이며정상/반복접속의동작검증이아니다. 보조C/ASM·빌드는Git제외 `extracted/cpp-resume-1155/`에있다.

Netstorm.exe 13,152,256바이트/SHA-256 `4732a47a4b6538b80abd0be8898df4e64410071e6b6c15f58fee6ac61f5569cc` / R.exe 670,208바이트/SHA-256 `01bf678c938db2ce07a61dec034c0774e57b8c77b64fb5f683a7941ad3edbe3d`

checking본체잔여는M3의부모2개4b02d0/4dc220이며,M2.5의실제재진입연결은별도로남는다.3차잔여는M2~M8의7묶음이다. 개발자이름 제한4b8be0직접Caller/요청writer는M6전수대조에남긴다. 마지막전체디컴파일2026-11-03/HJOW-Athlon,R전용2026-11-07/동일호스트이며생성원문은수정하지않았다.
