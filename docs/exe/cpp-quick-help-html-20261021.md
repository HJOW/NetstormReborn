# 전용 Quick Help HTML·동적 TYPE 정보·링크와 공용 요구 에너지

단위255의 시작 커밋은 `088b3900`다. 현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-20**로 일치한다. AGENTS와 양 인수인계를 다시 읽었고 새 디컴파일·원본/CD·AGENTS 변경은 없다. 직전 단위는 전용 도움말 제어/이력·공용 팝업 표식을 구현·기록·커밋한 진행 턴이다. 전체11미션 목표는 진행 중이다.

## 실제 파생 HTML과 정보 표

`QuickHelpHtml`은 `004c4c00/004c4cc0/004c5240/004c4c60`의 세 파생 슬롯을 기존 `HtmlTextParser`에 연결한다. 생성자는 shown0으로 시작하고 `name`은 첫 글자가 소문자 n일 때만 진입한다. 첫 인용/NUL까지 검색한 뒤 원래 주제 길이만큼 대소문자를 무시한 prefix 비교를 한다. 일치하면 shown/textAllowed1·`~5~w`를 기록한다. 검색/동적 정보 중 바꾼 cursor는 **가상 슬롯 진입 위치로 복귀**한다. Ghidra의 unaff_EBX 표기와 달리 실제 저장된 원래 입력 포인터를 쓰는 어셈블리도 대조했다.

소문자 literal `info`이며 TYPE와 shown이 있을 때 제목·원소·클래스·HP·사정거리·피해·SP·에너지 HTML을 만든다. 지역 필드256/Config치환1024/전체 HTML1024의 NUL 공간을 확인한다. 번역·현재 TYPE 재조회·최대HP 첫 검사/두 번째 표시 조회와 각 ftol의 순서를 보존한다. HP flags가 없거나 최대HP0이면 Invulnerable,거리0은 n/a,피해0은 특수 flags에 따라 special/n/a,SP0은 n/a,에너지 없음은 None/n/a다. 피해 필드는 실제 TYPE+48의 **hpPerSec**이며 발사 간격을 곱해 바꾸지 않는다. NaN은 원본처럼 숫자 분기에서 ftol 하위 DWORD0이다.

완성한 정보 HTML은 선택된 파일 스트림에도 기록하고,같은 기본 HTML/링크 저장소에서 StyleText로 바꾼다. infoMode00565ec4는 변환 중0,완료 뒤1이다. 현재 정보 pane의 본문→절대 scroll0→현재 flags8이 없을 때 Push→재부착 순서다. afterTag 파일 기록은 원본처럼 다음 평문과 다음 태그를 첫 `>`까지 한 바이트씩 쓰며 본문 cursor를 바꾸지 않는다. 인접 태그/CRLF/원문 case도 보존한다. 닫힘 슬롯은 현재 cursor 바로 앞 LF/CR만 textAllowed0으로 쓴다.

## 실제 링크와 마나 그림

`HtmlLinkText`는 실제 `0046bfc0`의 첫 인용 검색·앞 # 한 번 제거·현재 쓰기 포인터의 unsigned T태그·연속 NUL 문자열을 옮겼다. 문자열/프로토콜/공백/case를 정규화하지 않는다. 인용 없음/닫힘은 T0,저장소 비활성은 태그 없음이다. 원본 포인터는 호스트 토큰+바이트 위치로 바꾸고 저장소 한도/토큰 감김/NUL 누락을 안전하게 거부한다. `ResolveHtmlLink`가 같은 소유 바이트를 읽는다.

동적 정보의 중첩 기본 파서는 부모의 현재 링크 포인터 **복사본**에서 시작한다. 두 파서가 바이트를 공유해도 부모 cursor는 전진시키지 않는다. 부모가 이어서 링크를 쓰면 중첩 정보가 기록한 바이트를 같은 위치에서 덮을 수 있는 원본 동작도 유지한다. 별도 전역 링크 인덱스/안전한 다른 결과로 바꾸지 않았다.

`ManaPictureText`는 실제 `00472e60/00473070/00473090/004735c0`의 `wrtsWRTS.?>` 프레임0..10·연속 점 건너뛰기·출력 아이콘 인덱스별 앞쪽 강조+11·250바이트 한계다. 정보 표는 원본처럼 먼저 대문자로 바꾼다. 비ASCII 전역 접근은 미복원 경계로 드러내며 현재 실제 타입은 ASCII 원소 문자열이다.

## 같은 실제 Session과 발견한 메뉴 차이

`RawCampaignSession::HelpTypeInfo`가 같은 현재 TYPE flags/원소/비용,실제 gunTypes 사정거리/hitTypes 초당 피해,보호 AST 클래스와 기존 TYPE 요구 에너지 후처리를 읽는다. 최대HP는 기존 `HitPoints().TypeHitPoints`를 쓰도록 정보를 분리했다. 새 정보 표 자체를 게임 상태로 사용하지 않는다.

기존 지식 카드/공장의 mana 공급자는 `.type`의 명시 mana만 읽고 후처리 기본값을 누락했다. 예를 들어 Sun Cannon에 명시 mana가 없어도 실제+a0은 `s`다. 같은 `BindAiTypeRequiredEnergy`의 현재 TYPE/원본 AST 후처리를 두 메뉴와 도움말에서 공유하도록 고쳤다. AI·건설이 이미 사용하는 원본 후처리 몸체/기존 fixture와 기대값은 변경하지 않았다. 이름에 AI가 있어도 공급자의 입력/계산은 현재 TYPE의 공통 요구 에너지다.

기존 요청 `004c5c00`의 `local_150[336]`은 단일336바이트 주제 버퍼가 아니다. 실제 `sub esp,150`·주제 `lea [esp+c]`·push 뒤 서식 목적지 `lea [esp+60]`를 읽으면 **80바이트 주제+256바이트 서식**이다. 현재80바이트 거부를 유지했고 기존 단위184/985행/기대값은 바꾸지 않았다. 정적 검사는 실제 PE 명령 바이트를 확인한다.

## 제품 변경 전 원문과 검증 범위

첫 `decomp_quick_help_html.py` 관측은 **83고유 입력·83정상 최상위 반환·5939980명령**이다. name case/prefix·없는 인용/다른 주제·NUL 종료,a 종료/여러 링크,info 대소문자/속성,TYPE0·무적/최대HP0·사제1/4·원소/클래스/마나·0/소수/음수/NaN,정보 pane flags0/8·스트림/링크0/1을 교차했다. 보호 `help.english` 전체120888바이트에서 themeHelp/sunCannonType/존재하지 않는 vesselPriestType의 세 요청도 포함했다. 마지막은 전체 원문을 순회하고 shown0인 실제 미발견 결과다. 실제 TYPE 대신 명시 합성 TYPE를 사용한 원문/동적 정보 관측이며 원본 게임/화면 실행은 아니다.

동적 정보의 중첩 링크 cursor 수정 전에 별도 `decomp_quick_help_html_nested.py`로 **추가3입력·3정상 반환·43371명령**을 관측했다. 명시 설명 getter에 짧은/긴·두 링크/인용 없는 링크를 공급하고 실제 정보/기본 파서 두 개와 링크 쓰기를 읽었다. 합계86입력·5983351명령이며 같은 fixture를 재생성하거나 기대값을 바꾸지 않았다.

실제 파생·기본 HTML/링크/마나 초기화/문자열·HP/ftol·CRT만 실행했다. TYPE getter/설명·ASCII case 변환/번역/Config·고정 서식·pane/파일 스트림은 명시 경계다. 스택·보존 레지스터·x87 제어어를 확인했다. strnicmp의 별도 CRT 범위와 sprintf 주소를 첫 파일 쓰기 전에 보완한 준비 실패는 정상 반환으로 세지 않는다. 최초 도구/fixture/JSON은 각각 제품 작성/중첩 cursor 수정 전에 `extracted/quick-help-html-20261021-first-capture/`에 동결했다.

첫 fixture SHA-256은 `348a5d91dce8d8cc27348f64169e8baea5e0cb4942b22b3dc717814e9f629763`,중첩 fixture는 `0e100af50604b2de483dd5bc8dda7844e469f359a11c820945a225397bc884a5`다. 기존 기본 HTML/요청985·지식25/TYPE 후처리의 독립 기대값은 유지한다.

마나 빈 문자열/점만 있는8행은 실제 반환 문자열이 비어 있으므로 TSV 마지막 빈 열을 그대로 보존한다. 이 fixture에만 Git의 blank-at-eol 검사를 제외하며 원문 바이트/열/해시는 바꾸지 않는다.

새 C++검사는 전체83행·추가3행의 실제 출력/필드·링크/파일/정보 pane 순서와 같은 문자열 소유,보호 실제 세션의 Sun Cannon HP600/거리20/초당피해16/비용400/클래스/후처리 mana `s`와 설치/범위 거부다. **C++검사·CTest·전체 회귀·게임/창 실행은 사용자 지시대로 이월하고 컴파일만 한다.** 빌드/정적 감사·현재 핀 역변환은 `cpppj/quick-help-html-session.json`과 `tools/cpp_quick_help_html_audit.py --verify`에 기록한다.

최종 전체 Release는 **경고0·오류0·종료0**,로그 `extracted/cpp-quick-help-html-20261021-build5.log`다. 첫 빌드의 검사 Scene 이름 충돌은 HelpHtmlScene으로 고쳤다. 마지막에는 기본 생성 초기화와 본문 선택 전 TYPE 조회 순서도 다시 맞췄다. 새 HTML/중첩3·기존 기본 HTML816의 첫 관측 보존과 새 HTML/기존 Help 제어·지식·기술 허용·Guide 교체·미션 수명 정적 감사가 종료0이다. 현재 핀 **123보고서·268항목**의 전체 JSON을 시작 커밋으로 역변환해 과거 files/evidence/원문/기대값/실행·바이너리/기존 이력을 보존했다.

기존 `decomp_type_required_energy.py --verify`는 이 작업 시작부터 다른 client/CMake 역사 핀 때문에 종료1이다. 해당 핀/예전 통과를 고치지 않는다. 원본/도구/2352행 fixture/제품 후처리 몸체가 보존됐는지는 새 감사에서 별도로 확인하고,현재 tests/CMake 핀만 후속 이력으로 갱신한다. 기존 RawTutorialLater의 CRLF/LF 역사 핀 차이도 새 통과로 기록하지 않는다.

다음은 실제 전용 창 `004c5380`·부착 `004c5d60`·본문/그림 `004c5970`의 같은 Gump/스크롤/장식/돌 버튼/수명과 Client Help02000017/themeHelp 소비자 연결이다. 실제 링크 라우터4a00c0/학습492a30 소비자도 필요하다. 사용자에게 전용 화면이 열리는 단계가 아니며 일반 Tell로 대체하지 않았다. 전체11미션 자연 완주와 전체 Init/State/공장·소멸·전략AI·지형/포획/희생/대피·외관/음향 동일성 검증도 남는다.
