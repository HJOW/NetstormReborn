# Badges live Client 장부의 초기화·수량·공유 연결 조사

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. AGENTS·공통/닷넷 인수인계를 다시 읽었다. 이 절은 보호10.78 PE/정밀 C와 현재 C++를 읽기 전용으로 조사한 설계 근거다. 정적 대입 전개를 실제 원문 실행 관측으로 세지 않는다. 후속승인으로수행한독립CRT/reset32입력·새소유모듈·격리3검사통과는[별도검증문서](cpp-fort-badges-globals-20261024.md)에구별했다. 실제Client/Session주입안은여전히후속설계다.

## 같은6160BYTE와 초기화

장부는 **00593160 이상~00594970 미만의28×220=6160BYTE**이다. 다음주소00594970은 서버이름 정적 버퍼이며 장부에 포함하지 않는다. 각 기록은 +0 type DWORD, +4~213의210BYTE, +214~215의2BYTE, +216 count DWORD다. 실제 CRT에서210BYTE에는15BYTE 간격의ASCII(s/w/t/r/공백)/NUL 자료가 놓인다. 이는 저장하지 않는 원본 자료이며 타입과count 사이를 전부의미없는padding으로 지워서는 안 된다. 그 자료의 UI 의미를 추정해 새로운게임규칙을 만들지 않는다.

보호PE의 `.data`는RVA128000/원시크기1d000/가상크기be4b4이며 장부RVA193160은 원시파일 범위 밖이다. 따라서 로더가 제공하는 최초6160BYTE는 모두0이다. 전체0 SHA-256은 `155fb7024ecfd37be15220ff6dc1c60f798285b0e82dfedae4ea8e0255acdbce`다.

CRT표 **00528138(00528128[4])**에 등록된 **004fcbe0~004fea0f**가27typed기록과28번째0기록을 구성한다(정밀C220772~221963행). 1475개의정적명령은MOV1269/XOR142/REP STOSD28/STOSB27/레지스터보존push4/pop4/RET1이며 외부CALL이없다. 기록0의+4~11 **8BYTE는CRT가쓰지않고최초0을보존**한다. 나머지6152BYTE는CRT의지정쓰기대상이다. 첫행을다른기록처럼공백으로채우거나,임의비0입력에CRT를적용하면서그8BYTE까지0으로만들면다르다.

27개의타입 순서는 **71,72,73,78,128,111,136,133,140,139,138,75,142,141,137,148,77,79,131,135,143,144,76,147,129,146,145**이며,마지막기록의타입은0이다. 실제초기화는00541174등의현재PE상수DWORD를읽어대입한다. 첫27개는각각ASCII자료와0을쓰고,마지막기록은004fe9fd~004fea09에서55DWORD전체를0으로쓴다. 모든초기count는0이다. CRT는장부안에비0 UI/Gump/heap/vptr주소를만들지않는다. 따라서이초기화복원에UI포인터를가짜로생성할필요가없다. 다른필드도원본바이트로소유하고,현재확인되지않은UI의미나runtime주소를부여하지않는다.

상수메모리/MOV/XOR/REP의지정대입만정적으로전개한CRT후전체기대SHA-256은 `40e455f3941361b21cc236838aa108416bf859a5214aa76dbd7cbb23d159cbc8`이다. 이는이문서의정적기대이며독립Unicorn의실행결과와별도로비교해야한다.

## 수량의 실제 변경 경로

| 실제몸체 | 원본효과/호출자 |
|---|---|
| 0047d7c0 | 28개typeDWORD를선형검색해첫일치의count전체DWORD반환,미일치0. 실제CALL004be841은Badges writer004be7f0안에있다. |
| 0047d800 | signed count가0..255밖이면진단뒤계속하고,현재첫type일치기록의+216에전체DWORD를쓴다. 실제CALL004be94b은Badges loader004be8e0안에있다. 0047d810은독립함수가아닌이setter의분기명령중간이다. |
| 0047d860 | count주소00593238부터220BYTE간격으로28DWORD만0쓰기. 마지막실제쓰기는0059496c이고다음count주소00594a48과비교해끝낸다. type/210BYTE/2BYTE는보존한다. |

reset의실제PE CALL은 **0043984a 한곳**이며,map mode초기화0042e050/00422490뒤의시작초기화본류다. 정밀C는겹친코드를00438dc0/004395df에중복표시하므로두독립호출이나매Session reset으로세면안된다. CRT의전체구성과시작시count reset도서로다르다.

현재정밀C의직접호출과전체실행코드의장부범위즉시주소참조에서추가게임중count증가·감소경로는찾지못했다. CRT이전의실제주소참조는getter/setter/reset세몸체다. 격파/건설/생산에새카운터규칙을추가하지않는다. 이결론은확인한정적참조범위의결과이며,미확인간접포인터쓰기의실행관측이라고확대하지않는다.

`RawFortMetaLoad::LoadBadgesSelected`는선택된섹션에나온type/count쌍만기존`SetBadge`로쓴다. 빈/없는섹션은기존장부를보존하고,목록에빠진type도0으로지우지않는다. 따라서매Fort/Session/manager생성마다장부전체/count를초기화하면원본과다르다. getter/writer는이장부의현재count를읽어야하며원시Fort Badges섹션이나별도28개count복제본을live장부라고부를수없다.

## 실제소유자와 Client→Session 주입안

신규 `RawFortBadgesGlobals`가고정6160BYTE배열하나를Client수명으로소유하고 `Records()`의같은mutable/const span을빌려준다. 새Client생성시최초0배열→CRT를한번적용한다. 별도 `ResetCounts()`는원본시작초기화위치에서28countDWORD만0으로쓴다. 이둘을Session생성/목록MapProbe/Template manager생성에묶지않는다.

버전전역과같이UberGumpClient에`shared_ptr<o::RawFortBadgesGlobals>`를한개둔다. `RawCampaignSessionServices`로같은포인터를전달하고Session은자기보다오래사는실제배열의span을빌린다. supplied owner를받으면재구성/복사/초기화를하지않는다. 독립Session만새소유자를만드는fallback을명시하고,다른Client는다른배열을갖는다. 버전전역의manager0쓰기와달리Badges장부는목록/새manager생성으로변하지않는다.

기존 `RawFortMetaLoad(pool,Records(),loadingDepth,state,실제hooks)`에그span을넘긴다. 현재Technology부모/TypeNames/내용물/UI요구도기존실제경계로공급하며전부성공noop으로채우지않는다. genericFort의현재owner선택에서Badges를조회하는원본순서도유지한다. 새writer는 `hooks.count=[같은장부](type){return ReadFortBadgeCount(Records(),type);}`로연결한다. getter의 `std::span<const uint8_t>`는정확히6160BYTE를요구하고첫일치/전체DWORD를읽으며소유권과초기화를맡지않는다.

연결검사는같은Client의두Session/loader/writer에서`Records().data()`주소동일성,한쪽setter변경의다른쪽직접관측,빈/부분Badges로드보존,manager/MapProbe의비간섭,reset의다른6048BYTE보존,Client분리를확인해야한다. 실제badges장부를넣지않은현재Session이나원시섹션을성공한전체Editor복원으로세지않는다. 전체snapshot/Notify·genericEditor·실제메뉴·사용자전체완주·표시/음향동일성은계속후속이다.
