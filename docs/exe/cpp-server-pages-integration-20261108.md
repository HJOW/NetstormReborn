# 1164 — M5.2 세 서버 페이지와 실제 요청·응답 연결

2026-11-08 / HJOW-Athlon. 이전 CPP1163/8f8a9f22, 표시·파일·제품11.01이다. M5.1과 이번 콘텐츠/실제 장치·배포 계약을 합쳐M5를 마감했다.

원본/CD 실제 파일과 두TAFF에도세.server원본문은없다. Admin.server/Guild.server는원문R의이름/명령/치환tag를사용하는새LAN용본문이며, Update1~4는원본TAFF d/update.english의역사본문을재사용했다. 종료다운로드URL4개를제거하고11.01표시와10.70역사기록을구분했다. 원본서버본문동일성은확보하지못했다. 조사613원천해시/명시RequestMenu7개/서버15·관리자6명령/전체tag는별도출처감사JSON에보존했다.

현재canonical은cpppj/assets/server의3파일/32개비어있지않은섹션이다. UTF-8의ASCII부분집합이며BOM/NUL/외부URL·중복/END누락이없다. Admin은존1을기본으로추정하지않고0..50의명시선택을제공한다. 원문ZONELIST의herf/괄호오자와NEXTPLAYER계산자체를수정하지않았으며원문동적tag로현재페이지를구성한다. 옛GUILDADD명령은제공R에없어성공을합성하지않는다. 임의GET/GUILD/ADMIN사용자섹션전체가존재한다고확대하지않는다.

기존서버reader는Saves→현재CWD의파일을우선한다. 둘다실제로없을때세.server만모듈옆server자산을읽는기본경로를추가했다. 기존에있지만읽기실패한사용자파일을번들로숨기지않고다른관리파일/자동쓰기경로도유지한다. CMake의의존복사는원본폴더를건드리지않고실제build/bin/Release/server에세파일을준비한다. 배포는GameSource에같은파일이있으면존중하고없으면canonical을game/*.server로동봉한다. actualR CWD가game이므로같은reader가읽는다.

콘텐츠만있어서는실제진입이없어RequestMenu/RefreshPlayers의원문이름·숫자/허용표/일반입력을연결했다. RequestMenu는87h헤더→+22 GET/NUL→+4 section의strcpy순서→같은Registry11이다. 세필드Zsend로대신하지않는다. RefreshPlayers는현재CS40기록의activeBYTE→같은길드/기술이름표재작성→지정ff표시→NSInfo를매행현재값으로진행한다. 메타에빠졌던서버페이지의확장41h응답도기존Chat관찰기/중계/같은수신mask와현재ReceiveChatMessage/HTML에연결했다. 일반Session의기존41h·공유87hbootstrap은재사용한다.

두Release/x64빌드종료0·경고/오류0이다. 실제개발복사3파일의SHA가canonical/준비본과같고WindowsPowerShell의패키지DryRun종료0·1387항목/세game페이지포함계획을확인했다. ZIP은만들지않았다. 보호2800전수해시는1163불변이며이후보호경로쓰기0이다. 새C++단위/MP패킷·게임·통신실행0이므로실제페이지/길드/관리자기능동작검증은아니다. 최종ZIP바이트는M8에서확인한다.

다음은M6-a의정상로그인송신4종observer다. M6전수14묶음·M7독립5건·M8마감이남으며전체목표는미완료다. M7.1/2의읽기전용준비감사도추적보존했다. Config빈helpWord는기존cache유지이고, 메타커서는본문/장치설치부터없어단순world_분기교체로닫히지않는다. 그준비는구현/검사통과가아니다. 마지막클라이언트정밀2026-11-03/HJOW-Athlon·R2026-11-07/같은호스트이며생성원문변경0이다.

원시는extracted/cpp-resume-1164/,보고서는cpppj/server-pages-integration-20261108.json이다.

Netstorm.exe 13,204,992바이트/SHA-256 `2eaf7982bd6485018578b3549d17845332b8f86bbbc540ac68464aed91830fb8` / R.exe 677,888바이트/SHA-256 `66236e929f856558c8a5953dd5a8ff092380b9f98a51f732ceaec373ed562f81`
