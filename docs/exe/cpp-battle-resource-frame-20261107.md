# 1018 — 실제 일반 자원 프레임 전체 연결

2026-11-07 HJOW-Athlon. 마지막 전체 정밀 디컴파일은 같은 호스트의2026-11-03이다. 10.78 00486bc0/48f4e0/41c800/41c840/41c870/485e40/486130과 PE전체 분기·상수·선택표를 대조했다.486bc0의 정밀본CD대응은 없으며 하위 보충의CD004151d0을 참고한다. 구현 판본은10.78이다.

현재 LoginEnd가0일 때만 진행한다. signed SpellCount>0·비영 기한·ordered 게임시각 만료에서 실제boss의 기존 보충485e40을 실행하고, 효과 뒤 현재count를 한 번 감산한 다음 현재 게임시각+20을 기록한다. missed기한을 여러 번 따라잡지 않는다. 이어 현재 SP기한 가지의 비전투에서는00595124를0으로 만들고, boss/battle·subscriber행의flags2/20 완료·현재 지급간격비영 조건에서 새기한을 작성한다. 최초만료는같은별도장기DWORD를1로만 바꾸며 이후 만료부터숨김옵션0/현재subscriber행 각각실제금액·현재owner로6ch 종류2/11BYTE를서버9에 보낸다.1017의실제 수신/검증/Message18을 사용한다.

OrdinaryGeysers·ordered 보조기한·현재single/boss/editor/현재간격/로그인완료 관문을 원문대로 적용한다. single의보조기한은30초이고일반은현재옵션표다. 기한쓰기전에읽은boss를그분기에사용한다. predictableGeysers는현재정확히1일때만 지역seedbeef를유지하고그밖에는실제wall의기존TimeSeed를쓴다. 옵션18=2의일반전투는현재예산/밀도와첫_ftol·현재간격과두번째_ftol 뒤 /2/4의모서리수·중앙corner+1·변(center/2)을 계산한다. 최초세계생성의변/중앙수와다른점을PE표/분기에서대조했다.

같은 실제3×3분할범위를순서1..9로 읽고 flags1 RawSquidFinder와현재TYPE로각개수를센다. 현재target보다부족한구역만원문순서로모아후보1개는공유난수없이,2개이상은공유게임난수한번으로선택한다. 선택구역범위를다시읽고부족량반/최소1을실제PopulateGeysers(1/1/1)에넘긴다. 옵션18을재조회해일반전체배치1이아니면복귀한다. 그밖의전체배치는현재자원float와현재TYPE+c4를비교하고필요할때예산/3→실제Geysers목록count순서로검사한뒤전체범위에1개(0/1/1)를생성한다. 새목록/객체/성공플래그를합성하지않는다.

기존같은Pool/hash/spot/TYPE/MayPlace/Factory/Pop·자원pool/최근목록·RawWorldGeyserBounds를사용한다.00595124는RawGameAuxState에장기소유하고Native주소뷰도같은물리에연결했다. 실제Client439b34의Kernel뒤/주기writer앞에연결하고영상갱신잠금가지에서는새caller를만들지않는다. 월드없는메뉴도같은전역/현재옵션으로비boss자연관문까지실행하며임의송신/배치장치를만들지않는다. 호스트의 Fort 파일준비중에는 미설치Session 호출을 보류한다.

PE의x87 unordered 만료는C++ordered >=관문으로보존했다. 예산계산의double은기존목표FPCW027f(53bit)범위를따르며037f의전체80bit일치를새로주장하지않는다. 이번대조는정적PE/기존근거읽기뿐이며새NativeCPU입력/fixture/MP검사0이다. **일반 자원 프레임 연결을전체파생decoder·메타·의식/제재·Outpost완료로쓰지않는다.** 다음은6ch부족자금/건설Reject의00442b50제재/Tell와나머지MP다.

Release 제품 빌드 종료0·경고/오류0이다. 제품11.01은11,919,872바이트, SHA-256 `deec76174873877ae0f8aa9af9efa83c4c42a54bc952ca6c7fa3552b79637c4f`다. 보고서는 `cpppj/battle-resource-frame-20261107.json`이다. 신규 MP/단위/게임/통신 검사·NativeCPU 실행/fixture생성0이며 보호2800파일은 불변이다.
