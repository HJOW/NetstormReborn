# 766 — CreateHelpList와 같은 Help HTML stream/지역파일 수명

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon/2026-10-23 유지. 단위766 완료·전체 목표 미완료다.

## 현재 입력/Help/파일

CreateHelpList(이름518740/등록543068/전역542964/0200009c)를현재이름표·HTML허용표·일반FIFO와UberGump에연결했다. 원문4d7cb2→4c63e0이며Guide/Gump/공통release뒤,Console/정지앞이다. World/mission/editor/battle관문이나새기본UI행을추가하지않았다.

같은전용Help를빈비NULL주제/mode0/learn0으로열거나재사용한다. 결과0은다른장치를요구하지않는다. 지역24BYTE파일Construct→전체Open(NetStormPieceList.html,1,1)→같은Help+bc stream→6BYTE<html>→현재signedTYPE순회→7BYTE</html>→+bc=0→Close→지연Kill→Tell(pieceDump)→Destruct순서다.

각TYPE첫getter의flags2+ec가어떤비트든비영이면두번째현재getter이름에고정Type을붙여직접Show한뒤3BYTE<p>를쓴다. 제외TYPE에도Cleanup하고현재signed개수를다시읽는다. 요청이력/추가lookup/번역을합성하지않는다. 첫NUL/256BYTE서식범위를보존하고초과는앞선효과뒤호스트length_error로거부한다.

RawMenuControls의현재QuickHelp/기본HTML/정보pane/링크/한BYTEstream몸체를재사용한다. 현재지역writer만불투명토큰으로등록하며포인터를DWORD로좁히지않는다. 정상stream해제는Close앞이다. 실패는원문에남은+bc를유지하되callback만먼저반납해소멸한지역FILE에다시쓰는것을명시거부한다. 이는호스트안전성보완이며원문SEH전체동일성은아니다.

실제writer는기존BaseFileSystem.Open과Construct/Write/Close/Destruct몸체를사용한다. 같은VFS/TAFF/보조root/공유10칸고리이며mode1/create1의r+b→필요한w+b재시도를유지한다. 기존파일은truncate하지않아새출력뒤옛tail이남는다. binaryLF/짧은fwrite무재시도/부분TAFF쓰기진단도기존부모에맡긴다. 호스트chdir대신게임CWD를상대경로장치에만적용하고Unicode는wideCRT경계를쓴다.

## 검증/경계

원문부모142입력/284회·일반입력128/256·이름/FIFO21·정확한파일명전체Open5/10·HTML3/6+현재HPgetter3회로총299입력/580관측을고정했다. root추가580회가네fixture와wholeOpen원시에바이트일치했다. 최초실제Help통합3행만실패한것은직접parser기대가4c5380 ctor의본문끝LF/NUL을빠뜨렸기때문이다. 실제원문ctor1회+LF추가parser6회를별도관측해세stream이각기존Native출력+LF임을확인했다. 완전바이트기대에그원문LF를추가했고제품/parser/기존fixture는변경하지않았다. 최초3실패로그는보존한다. Guide/Gump/TYPE/Show/Config/번역/OS/FILE은명시경계이며실패6종은SEH직전prefix다. HTMLprofile1은합성TYPE이며실제게임TYPE/전체UI로세지않는다.

최종Release경고/오류0,집중 **8개(새8개)/실패0**. 부모현재값/순서·해제·미도달장치·첫NUL/서식한계·실제Unicode생성/열기실패·r+b tail/binaryLF/공유고리·같은Help/HTML의정확stream바이트·지연Kill·실패callback재진입거부를확인했다. 새실행대상0/기존66개·전체71대상이다.

기존보호2684개불변·새fixture4포함2688개/fixture868. 다음기준extracted/cpp-resume-766/protected-after.json,제품SHA-256 `7de3ad603912817a2e8876b6db105faf096ee3f4246371ecf858755173f2256d`. [보고서](../../cpppj/quick-help-list-stream-20261103.json)와같은extracted의build/tests/원문최초및추가관측을따른다.

NetStormPieceList.html은새 자동생성파일이며5차에서writer뿐아니라원본PieceDump의file링크/안내도같은사용자Netstorm하위namespace로옮겨야한다. 현재1차의게임CWD계약은유지했다. 전체Help브라우저출력/픽셀·원문32비트FILE/SEH전체·CD10.72는별도다. 새게임/GUI/CLI/MP·외부프로그램/브라우저·물리/PCM·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP검사는0이다. 다음767과각reader/일반명령을이어간다.
