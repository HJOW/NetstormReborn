# 1165 — M6-a1 로그인 송신 관찰기 본체와 실제 caller 잔여

2026-11-08 / HJOW-Athlon. 이전 CPP1164/5a40ee37, 표시·파일·제품11.01이다.

RawMissionClientPackets의2a/2b/67/81 송신 방향을무조건throw하던장치를mutable packet observer로교체했다. 원문40a540/40a590/40ae80/40b460 전체C·PE에따라수신방향0은필드/장치를읽지않고복귀한다. 송신은원문DWORD/BYTE/WORD의미정값진단후현재필드를다시읽어선언WORD를쓴다. 2a는+c DWORD진단/WORD+11h,2b는+33 BYTE+3bh,67은+5 WORD+7,81은+4 DWORD진단/현재WORD+2의16bit감김→*4다. 기존같은Storage.AssertDisplay의식/파일/줄로복귀형진단을연결했다.

**M6-a 전체는 아직 미완료다.** 실제Login writer→hook→Composite/각backend를끝까지추적하니기존일반writer들은const Send를사용하며각SendPacket은mutable span이있을때만observer를호출한다. 따라서초기감사의“일반로그인송신에서상시throw”판정을이const경로에대해정정한다. 기존throw는mutable caller에서만선택되고일반const송신은관찰기를우회한다. 이번부품/등록복원은그우회까지해결한것이아니다.

다음M6-a2에서로컬버퍼를소유한SendFort2a/SendPlayer2b/Settings67/SendSp81의실제hook/장치를mutable송신에연결해야한다. 수신relay의const span과새로작성한로컬packet을구별하며단순const_cast·빈observer/별도성공으로닫지않는다. 같은버퍼의진단부분효과와원문송신gate뒤한번관찰을보존한다. 현재CS관문은기존challengeGlobals.Current()의같은참조라변경하지않았다.

두Release/x64빌드종료0·경고/오류0·보호2800파일불변이다. 새C++단위/MP패킷·게임·통신/ZIP실행0이며관찰기동작/로그인성공검증이아니다. M6-a1부품완료와실제caller미완료를구분한다. 마지막클라이언트정밀2026-11-03/HJOW-Athlon·R2026-11-07/같은호스트, 생성원문변경0이다. 원시는extracted/cpp-resume-1165/,보고서는cpppj/login-send-observers-20261108.json이다.

Netstorm.exe 13,206,528바이트/SHA-256 `acaf079329ca6d496d629ebe2a7408d0f99d1e84088d23b1056bddc5f5edaa8c` / R.exe 677,888바이트/SHA-256 `035914e9ca7e289c1888a2e6045c9736faa97145a34e1f33dd60e62d7f556826`
