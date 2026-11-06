# 1019 — 실제 원격 Tell70h 송수신

2026-11-07 HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 10.78 004c9150/4c9200/40b090/cf980/40a6f0/4df590과 실제70h등록·패킷표/초기값을 대조했다. CD대응은004c3ec0/40b300/40b340/418b60이며 cf980/4c9200의CD대응은 정밀본에 없다. 구현 판본은10.78이다.

원문264BYTE ff fill/1·70h·길이DWORD·첫NUL포함복사를작성했다. 원문 Ui4ef/4fb의 tellLength<256 진단이복귀하면계속하며,256BYTE는실제물리에복사하고그이상은호스트범위오류다. 호스트미확정헤더4는실제송신관찰기가현재길이로확정한다. 원본PE의미초기화544c80은0이며4df590의최소4처리와최종observerwire를구분한다. 원문스택/현재초기길이ABI자체일치는주장하지않는다.

40b090은sending비영에서만현재tellLength==-1을진단하고,복귀뒤현재DWORD를다시읽어길이+8의하위WORD를쓴다. 실제공유등록표에70h/version1/정책0/가변길이/packetObserver를등록했다. 수신cf980은sender/lengthDWORD를읽지않으며+8의첫NUL까지현재cf730으로넘긴다. 호스트는실제수신span바깥NUL읽기를명시범위오류로구분한다. 원문표시바이트는실제UI의UTF-8경계에서만DecodeDisplayText한다.

같은 실제 Session에 SendPlayerTell을추가했다. 단일대상은4c9150처럼그번호에,others는4c9200처럼현재1..8에서해당번호를제외해같은실제전송을호출한다. flags/NULL/observer·실제큐/길이확정은기존전송이담당한다. receive는현재Factory/Tell장치를재사용하며즉시월드교체나새UI를만들지않는다. Editor등록목록에도70h를넣어같은Session의해제수명을따른다.

**이하위연결은비용부족처리의실제호출완료가아니다.** 재대조결과00442b50의계산본체는기존RawConstructionPlace::ChargeShortfall에이미있고,RawConstructionPlace::Reject는00442af0이다. 앞선Bindings주석의442b50표기가둘을혼동하게했다. 다음단위는같은기존ChargeShortfall을재사용해실제70h비용안내·6ch부족자금caller·SP/취소전송장치를접합한다. 계산본체를중복작성하지않는다. 전체decoder·메타·Outpost/checking5도남는다.

첫빌드는표시문자열OriginalText헤더누락한원인/C3861·C2664오류2였다. 헤더를추가한최종Release제품빌드는종료0·경고/오류0이다. 제품11.01은11,924,480바이트,SHA-256 `b853500339f78afe875e25f7cd9da29210aab217725dadc54fdb8f699517f4e4`다. 보고서는 `cpppj/remote-tell-packet-20261107.json`이다. 신규MP/단위/게임/통신/실제Tell검사·fixture생성0이며보호2800파일은불변이다.
