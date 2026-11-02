# 763 — 명시 전체읽기41baa0와 객체덤프 수명

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23을 유지한다. 단위763 완료, 전체 목표 미완료다.

## 원문 부모와 실제 소비자

ReadWholeBaseFile은41baa0의 별도 명시 API다. check가 비영이면 현재Probe를 먼저 호출하고 미발견 시File/길이출력/할당을 건드리지 않는다. 그뒤key/keyLength만0→Open(name,0,1)→정상ctor뒤File확인→길이→signed32(length+extra)의 최소4요청→Read→선택lengthOut→Destruct 순서다. 출력버퍼를 먼저0으로 채우거나 원문에 없는buffer free를 Raw core에 넣지 않았다.

Open 실패는 아직완료되지않은File의소멸자를 호출하지 않는다. 완료ctor뒤길이/할당/읽기예외는현재File/키를반납한다. 정상Close는try밖에두어그예외로소멸자에다시진입하지않는다. File0인완료ctor도key반납은진행하고lengthOut은쓰지않는다. 모듈러추가폭·현재파일위치·부분키읽기와미작성scratch를같은기존Raw IO로처리한다.

실제Client의ObjectDump 읽기만이부모에연결했다. source VFS의현재Open/TAFF FILE/키와동일호스트Writer를사용하고논리length만반환한다. 기존TryRead/Read의snapshot·optional미발견,ReadFileBytes/TAFF등록의직접OS계약은유지했다. 다른reader를자동이관하지않았다.

## 원문과 호스트 경계

원문266관측은명시예외뒤marker0일때실제004f93a0 cleanup funclet→41b410을별도호출한다. 정상Close의marker-1에는재진입하지않는다. SEH dispatcher전체나32비트FILE24BYTE/미정stack을재현한것은아니다. 암호scratch4096BYTE=101·할당205는명시입력이다.

첫관측133행은prepared-observations에보존했다. 암호4101/available2050 두행의원시payload만교정됐고현재fixture는제품준비전에고정됐다. 최초도구95271de7…해시는native.json에유지한다. root의설명주석보완뒤별도원문재관측도현재133행전체와일치했다.

Raw core의부분읽기는미작성물리를유지한다. 실제ObjectDump는short fread를정상덤프파일내용으로반환하지않고명시거부하며extra/min4padding도노출하지않는다. 음수길이/부족할당의원문밖쓰기는호스트오류다. 바깥Writer/vector의RAII는불완전Open/예외때남은OS자원도반납하는Windows호환보완이며원문SEH동일성으로기록하지않는다.

## 검증과 다음

원문133입력/두정밀도266관측과root추가266회가일치했다. 최종Release경고/오류0,기존파일I/O집중대상 **19개/새2개/실패0**이다. 원문133행/예외접두·음수/부족할당명시경계와기존Unicode/현재TAFF payload·키/덤프공유고리·텍스트/바이너리왕복·전체Open/쓰기반납을포함한다. 출력의unable to open은명시실패검사장치의예상진단이며실패한CHECK는없다. 새실행대상은없고기존66개/전체71대상을유지했다.

기존보호 2676개불변,새fixture1개포함 2677개/fixture857. 다음기준은 extracted/cpp-resume-763/protected-after.json. 제품SHA-256 `c2c3e0a1ec7c82f628bb3e27c84b7809a4d4a18d2be972a1ed6c09c34b16c119`. 원시build-v1/v2·tests-io-v1·native/native-verify및최초관측은같은extracted경로다. [보고서](../../cpppj/basefile-whole-read-20261103.json)는실제범위와한계를구별한다.

새게임/GUI/CLI/MP·물리/PCM·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP검사는0이다. 마지막GUI746/전체34·ZIP645의644제품과별도다. 다음764~771은각caller계약대로이어간다. GIF의정확길이/0채움/디코드뒤close,팔레트청크,Xlat지속버퍼/CR처리,Font청크,ServerNames부모는41baa0으로일괄치환하지않는다.
