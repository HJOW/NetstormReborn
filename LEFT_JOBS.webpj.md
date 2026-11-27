# LEFT_JOBS.webpj — 현행 상태와 남은 작업

현재 진행 상황, 남은 단계와 유효한 결정을 기록한다. 단위별 구현과 검증은 `docs/webpj-*.md` 및 [PORT_MAP.tsv](webpj/PORT_MAP.tsv), 지난 인계 원문은 최신 [E1380 정리 전 전문](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380)과 [E1339 정리 전 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261126-through-wb04-e1339)과 그 아래 날짜별 보관본을 참조한다. 공통 사항과 cpppj는 [LEFT_JOBS.md](LEFT_JOBS.md), dotnetpj는 [LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)를 따른다.

**읽는 순서:** 0.1(현재 값·다음 작업) → 0.2(단계 진행판) → 3.1(단계별 항목) → 0.5(단위 착수·종료 절차) → 필요할 때 나머지 절. 진행 요약을 요청받으면 0.1의 **실행·플레이**와 **원본과의 동일성** 두 항목으로 답한다(AGENTS.md).

## 0. 한눈에 보기

### 0.1 현재 값 (2026-11-27, 마지막 완료 단위 WB04-E1423·1-6 승리 수집 거부/첫 안내 분리)

- **실행·플레이**
  - **화면:** 브라우저 ‘클릭하여 시작’(첫 적재 뒤 최소 6초, E1308), 시작 title 그림, 메인 메뉴·캠페인/데모 목록·도움말·옵션(Reset All 게임 안 확인 창 포함)·Credits와 개발용 지형 미리보기가 동작한다.
  - **미션:** 캠페인 목록 31개(Early Missions 6·세 캠페인 묶음 16·Priest Training 9)와 데모 3개가 시작되고, 장시간 진행과 임의 조작(몽키) 시험에서 오류·먹통이 없다. Campaign ▸ Skirmish는 무작위 섬·간헐천 월드에서 AI와 진행되며 AI 생산·공격으로 사제가 죽으면 패배 안내까지 이어진다(E1262, 약 18분 관찰). Tutorial2는 시작부터 완료 창·Tutorial3 시작까지 자연 진행으로 확인했고, 승패 안내 뒤 흐름(Continue·Replay Mission·Leave/Next Mission·진행 저장)도 확인했다. 미션 프레임은 다리 붕괴 검사·전투 자원·제단 결과 알림·주변음/공유 바람·장면 음악·미니맵 갱신을 원본 주 루프 순서로 돈다(E1230~E1234·E1266).
  - **조작:** 사제 선택·이동, 가장자리·Alt·가운데 버튼 스크롤, 미니맵 클릭·끌기, 생산 격자 클릭·단축키(D 포함)·조각 회전(C·회전 방향 옵션)·다리 배치, 사제·워크샾·제단·신전·배터리·제네레이터·운반체 우클릭 메뉴(회수·기도·Cast/Drop·Carrying/Drop·폭탄 Detonate!), 객체 명령(Detonate·Meltdown·신전 명령), 주문 시전과 주문 보유 골렘의 적 사제 포획·Drop(E1318~E1319), 화면 위치 키(F4/H·F5·TAB·U·Shift+숫자 저장/숫자 복귀)와 방향키 카메라 이동(32픽셀, 조각을 든 ←/→는 회전, E1291), 미니맵·스톰 파워 표시(원본처럼 About, E1324)·생산 창·채팅 도우미 단추의 우클릭 빠른 도움말이 동작한다.
  - **게임 메뉴:** ESC 또는 화면 우측 상단 버튼(웹 전용)으로 Game·View·Options·Players·About 메뉴막대를 열고(열 때 원본처럼 SP 검산 회전, E1327), Leave/Restart Mission(ABORT 확인 창), F1·F2·F6·F7·F8·F9·Shift-F3·T, Pause(Shift-F9·Pause 키·메뉴), Shift-F7 화면 GIF 저장, Players 메뉴의 송금(Send X 숫자 입력 창 포함, 수신 안내 E1326)·동맹이 실행된다. ESC는 열린 Tell 창(브리핑·로딩 제외)을 원본처럼 닫는다(E1264). 버전 창의 11.01 패치·`Copyright 2026 HJOW` 줄(E1289)과 한국어 메뉴 전수 점검(E1290)을 마쳤다. 브리핑·완료 창의 Review Knowledge·GetTechnology 단추와 Priest Training 7의 Training Menu(원본 오타 별칭, E1245)도 동작한다.
  - **HUD 틀·채팅 창:** 실제 Uber 루트가 좌측 사이드바(SP·생산 덱)·하단 미니맵 칸·구분선을 원본 위치로 도킹하고 지도 영역이 `{85,0,W,H}`다(4:3·16:9·16:10·1920×1080·창 크기 변경 확인). F3 채팅 창의 로그·입력줄·제출(로컬 서버 경유 `You> 본문`)·점 명령·Tab 이름 완성(동맹 AI)·붙여넣기/복사/잘라내기·도우미 메뉴·Edit Back/Front Format 창이 동작하고(E1169~E1179) 로그는 원본처럼 앱 수명 동안 유지된다(E1320).
  - **미완성·범위 밖:** 다른 미션의 목표 수행과 자연 완주가 남았다. 지식 NSInfo 패널·Players 하위의 Ignore chat·Whisper To·정보 보기·의식 보상 선택 UI·인트로 영상은 웹 범위 밖이다(E1167·E1171·E1180·E1202).
- **원본과의 동일성**
  - **M1 메뉴:** 영어 4:3 메뉴 8장면은 cpppj 오라클과 알려진 차이 영역(Campaign 창 크기·Options 팝업 위치) 밖에서 픽셀 차이 0이고 구름 배경도 같다(Kencloud 캐시, E1298).
  - **L2 시작 상태(E1283 닫음):** 검사 전용 고정 시각 모드(`?fixed=25`)에서 Tutorial1 Load/Start의 Game 난수열(90프레임 6,851회)·논리 화면·Session 덤프가 cpppj 기준선 오라클과 같다. 4:3·16:9(1366×768)·16:10(1280×800, E1322)의 90프레임 화면 0픽셀, 90·130프레임 덤프는 환경 의존인 `sp 1`·`player:*` 행만 다르다. 한국어(E1323)는 상태가 같고 첫 안내 창 밖 0픽셀이며 창 안 차이는 웹 독립 번역·글꼴 차이로 분리 확인했다([근거](docs/webpj-korean-m2-20261124.md)). 31개 미션의 시작 5·30·120프레임 덤프도 모두 같다(E1313).
  - **L3 입력 진행:** cpp `--ui-script` 입력 단계표를 그대로 적용해 Tutorial1(4,200프레임, E1284~E1286·E1299·E1301~E1304)·Tutorial2(5,400프레임, E1305~E1307)·Tutorial3(Capture The Priest, 6,200프레임, E1309·E1329~E1333)의 시작~'Mission Accomplished!' 완료 창·다음 미션 해제와 Tutorial4 단계 A→C→E1 `'Sacrificing Priests, Part 1'` 안내 창 진입 7,240프레임(E1334~E1335)·Tutorial5 단계 A2 실제 피격·B 안내까지 1,000프레임(E1339~E1340), ESC 메뉴 여닫기 360프레임(E1327)까지 체크포인트 화면 0픽셀·덤프 일치다(단계별 근거는 [진행 요약](#e951-e1081) L3 행과 3.1절 4D-4b). 이 대조로 다리 붕괴 스캔 초기화 누락(`Reset422490`, E1301)·생산 패널 HUD 생성 순서(E1305)·로그인 중 간헐천 보충 기한(E1313)·메뉴 SP 추적(E1327)·주변음/장면음악 천둥의 `ThunderProcess`(TYPE 55) 미연결(E1329)과 `e2e/l3-common.js`의 `INJECT_SHIFT.right`(-1, E1333)를 고쳤다. E1336 마나 그림 보충 뒤 Tutorial4 `a220`의 생산 하위 메뉴 영역은 기준선 오라클과 8,656픽셀 달라 E1337부터 사유·영역·픽셀 수를 명시한 알려진 차이로 검사하며(영역 밖 0픽셀), E1336 이전의 0픽셀 결과를 그 뒤로 넓혀 읽지 않는다.
  - **L3 캠페인(16/16):** E1350~E1365에서 모든 미션의 브리핑·시작·대기1200프레임을 미션마다 화면3곳0픽셀·덤프3곳 허용 밖0으로 대조했다. 허용 미션의 실제 목표 수행·승리는 별도 단위다.
  - **부품:** 월드·UI 부품은 원본 10.78 x86 fixture 또는 독립 계산으로 대조했다(원본 공통 메뉴 788행·Factory 메뉴 39행·Uber 루트 160행·장면 음악 3,113행, 채팅 입력줄 대상 720행(E1265), 파편·건물 캡처 4,474행(E1275~E1276)·골렘/받침 264행(E1312), 실제 Session의 EvilRay·Burning·섬 투하(E1314~E1316) 등). 대응표 1,007단위 중 732행은 Tutorial1 실행에서 웹 모듈의 함수가 모두 실행된 `session`이다(E1283). 좌측 HUD 하단 85×82픽셀은 원본 스크린샷과 일치한다.
  - **한국어 글자:** 갈무리11 12px 격자로 그리되 제목 배율·굵기(사용자 결정으로 보통체)·한글 기준선을 cpppj와 다르게 둔다([0.7절](#korean-font-change)).
  - **미검증:** 세 캠페인 묶음의 목표 수행 L3·자연 완주와 L4b 소리(16개 진입/대기는 완료). Tutorial4 E1~I1·Tutorial5 D 이후·Tutorial6 E 이후의 특정 적 공격·포획·희생·승리는 사용자 지시로 건너뛰고 사용자가 직접 확인한다. Tutorial6 A~D 생산·건설·납품은 E1343~E1346 완료다.
- **최근 완료 요약:** E1396에서 최신웹/현재C++의1-3 AI관찰을비교했다. C++60,000프레임정상종료0·AI진단0/마지막보고54,000미결정, 웹60,000프레임게임오류0·미승리/검사종료1이다. 양쪽실제AI가생존/연결된상태를기록했고 입력/시드가달라정확한오라클일치로판정하지않는다. **1-3 승리는미완료**다. [E1396](docs/webpj-campaign1-3-ai-comparison-20261127.md). E1394 저장/E1395캐시도완료다.
- **현재 재개 지점:** ESLint/webpack(E1400/E1406), 일본어(E1410/E1412), 제단 예약 피해(E1413)와 실제 그래프 복구(E1416/E1419)를 완료했다. E1420 Node26전체6,910/6,910·3539원시해시동일을확인했다. E1423에서1-6승리거부/1-5뒤첫안내허용도반영했고 실제AI1-3/1-4/스커미시목표·승리와채취량절반/M4/M5잔여를이어간다. **최신사용자지시로1-6승리는사용자직접확인으로이월하며더실행하지않는다.**
- **E1402 완료:** Linux GUI 비고정 시작/직접 프레임 거부 검사를 원래 코드·단언·60초 상한 그대로 단독 재시도해1/1 통과했다(검사9.3초·전체22.8초). E1397의5통과/1실패와 구분하며 이전 시간 초과 원인이나 부하 안정성 해결로 세지 않는다. 물리 음향은 여전히 사용자 확인이다. [원문·한계](docs/webpj-linux-startup-retry-20261127.md).
- **E1403 완료:** 실제 메뉴에서60/120을 두 번씩 선택해 네4초 구간의 원래 게임 시계와 호스트 경과를 비교했다(차이0.2~0.4ms). 관측 rAF/처리는약60Hz라 물리120FPS로 세지 않는다. GC뒤 JS사용량9,585,820→9,721,992바이트·backing storage39,579,834바이트 고정이며 짧은 메뉴 표본이다. 미션/장시간 메모리는 남는다. 도구도ESLint에 포함했고 전체 선행 검사/빌드가 통과했으며 게임 소스/운영 번들은 E1400과 같다. [수치·조건·한계](docs/webpj-menu-frame-profile-20261127.md).
- **E1406 완료:** 동일 출력이면 webpack이 파일mtime을 남겨 자동 갱신이 반복되는 실제 상태를 재현했다. output.compareBeforeEmit=false로 정상 빌드 때 출력 시각을 갱신했다. 실제세오류/경고경로 종료2·기존SHA/mtime보존과 두번연속최신판정/무재생성이 통과했다. 게임소스와3,500,786바이트운영번들은E1400과같다. [재현·검증](docs/webpj-webpack-fresh-mtime-20261127.md).
- **E1405 완료:** ESLint/webpack 관문 적용 뒤 Node26.13.0 전체6,897/6,897 통과(1174.102초·실패/취소/건너뜀0·종료0). 작성 소스/설정2222개 원시·LF해시가 모두 시작 때와 같다. 이후 E1408/E1404/E1410을 완료했고 E1411 v22전체6,901/6,901을통과해 E1415 Node26전체6,902/6,902통과뒤현재v26주기는0/4·v22는1/8이다. [원문·고정 증빙](docs/webpj-node26-regression-eslint-20261127.md).
- **E1408 완료·승리 미완료:**1-3 진입 전 메뉴28프레임(0.7초) 실제 대기 뒤 아군AI로 진행했다.194.675초 아군사제포획·356초희생·356.025초사망과13,900프레임의Failure/Continue 안내를재현했고 게임오류0이다. 자연승리단언은실패했고 완료값/다음미션검사는진행하지않았다. 기본관찰/C++엄격오라클과구분한다. [원문·화면](docs/webpj-campaign1-3-ai-defeat-20261127.md).
- **E1413 구현 완료·E1409 실게임 후속 중:**1-6실제AI ForceEnemy=3에서55,500관측프레임/첫1395.5초·SID21289/TYPE164/VT5076b8/6699의제단가상98누락을재현했다. 기존총기의448b70현재방향11/12면역/그외일반HP몸체를공유해예약경로에연결했다. 소유자/공격자·unknownVT거부를보존했고집중29/29·선행ESLint/빌드·대응표오류0·ChromiumL2 1/1통과다. 수집기도단언전전체오류/덤프/화면을남긴다. 후보2모듈과본소스LF해시는같으며실제AI120,000상한후속은아래E1416의새그래프오류로조기종료했다. 실제60,000프레임/1509.175초까지오류0으로기존55,500수집구간을통과했다. 승리/저장은아직미완료다. CP예약분기도같이누락됐으나CP실행/코드는수정하지않았다. [재현·수정·한계](docs/webpj-altar-scheduled-damage-20261127.md).
- **E1416 구현 완료·E1419 실게임 후속 중:** 실제World의 recovery문자열 fullPool/공용값FullPool 불일치를정책상수로교정했다. 같은Create/Pop오류의수정전실패·수정뒤집중86/86·선행ESLint/운영빌드·재번들/webpack관문3/3·영어L2 1/1통과다. 불필요한ESLint예외를177→176건으로줄였다. 원래실게임75,000관측프레임/1884.175초/종료1과두상세오류·덤프·화면을보존했다. 본소스후속은현재42,000관측프레임/1059.175초/오류0이며ES모듈1070파일SHA가같다. 기존오류구간통과/승리/저장은아직미완료로E1419에서계속한다. [원인·회귀·원문·한계](docs/webpj-graph-exhaustion-recovery-20261127.md).
- **E1415 완료:** Node26.13.0 전체6,902/6,902통과(796.964초·실패/취소/건너뜀0·종료0). 소스/설정/대응표/공개자산/원본2983파일원시해시가모두같다. E1413구현이포함됐고일본어10모듈후보는아직미적용이다. v26은0/4로다시시작하며v22는1/8이다. [원문·고정](docs/webpj-node26-regression-altar-20261127.md).
- **E1404 완료:** 실제AI1-1 여섯번 진입/600프레임/원래 Game→Leave→ABORT에서 마지막World/Session이 남던 강한 heap 루트를 찾아 공용 진단 팩터리를WeakRef/약한키캡처로 고쳤다. 현재/복사함수 계약35/35·선행ESLint/빌드·재번들일치1/1·Chromium시작/저장읽기2/2 통과다. 본소스 여섯반복은 매번World/Session전부회수/게임오류0이었다. 짧은 수명검증이며 장시간/물리120Hz M5는 남는다. [원인·실측·원문](docs/webpj-mission-memory-release-20261127.md).
- **E1410 완료:** 공용일본어43개언어파일/번역표를44개웹자산으로 동일바이트복사하고 지원객체/실제GameData 소비를 연결했다. 원본실행구조감사43/43·전체자산377개검사·일본어/기존한국어집중41/41·선행ESLint/빌드통과, 운영번들은E1404와바이트동일이다. 언어선택/실제CDN글꼴/UI는다음단위다. [범위·검증](docs/webpj-japanese-assets-20261127.md). E1411 v22전체재검사6,901/6,901을통과해 E1415전체통과뒤현재v26은0/4·v22는1/8이다.
- **E1417 관찰 완료·1-4 승리 미완료:** 실제아군AI120,000프레임/3009.175초까지오류0·포획/사망/승패창없음으로상한종료했다. 자연승리단언실패/종료1이며저장/Next검사는실행하지않았다. 양쪽brain은초기화/스캔1·중지0이고현재살아있는상대사제를가리킨다. 원래SP2000/동맹/이름/기술과실제모든내부프레임을보존했다. [원문·화면·AI본문·한계](docs/webpj-campaign1-4-ai-120000-20261127.md).
- **E1418 예약 주기 완료·채취량 절반 미완료:** 같은지도seed1/양측시작SP12000의자금충분생성미션에서세난이도각8000프레임/209.175초/오류0이다. 실제생산/다리/채취제어는쉬움4/4/8초·보통1/1/2초·어려움0.5/0.5/1초를확인했다. 기본5000두표본은생산부족으로단언실패/종료1이며성공으로세지않는다. 목표필드3/6/6이나실제최대계수1/1/7로실제생산량절반은남는다. 공용난이도보존선택과유지도구를추가하고실제helper3/3·선행ESLint/빌드통과했다. 제품소스/번들은E1416과같다. [입력·수치·실패·경계](docs/webpj-skirmish-ai-periods-20261127.md).
- **E1419 완료·1-6 승리 사용자 확인:** 실제1882초/SID17499의Graph251→18·같은배열복구를확인했고마지막116,000표본/2909.175초/오류0이다. 사용자1-6승리생략지시로해당노드/자식브라우저만중단했다. 종료1은사용자중단이며상한완료/최종프레임/승리/저장을추정하지않는다. 준비한재선정후속도미실행이다. [원문·중단·증빙](docs/webpj-graph-live-recovery-stop-20261127.md). E1420전체6,910/6,910통과뒤E1419완료분을v26=1/4이월·v22=6/8로관리한다. E1423에서유지승리수집기도1-6을명시거부하도록변경했다.1-6승리실행금지다.
- **E1420 완료:** Node26.13.0전체6,910/6,910통과(1103.009초·실패/취소/건너뜀0·종료0)이며3539파일원시/LF해시가모두같다. 일본어UI/CDN/줄바꿈/외부감사·실제Graph복구·난이도보존/수집도구를포함한다. Node22전체와동시에실행하지않았다. 실행중완료E1419관찰을새주기에이월해v26=1/4·v22=6/8이다. [원문·고정·한계](docs/webpj-node26-regression-japanese-graph-20261127.md).
- **E1423 완료:** 최신사용자지시를승리CLI에반영해1-1~1-5만허용하고1-6은브라우저시작전에사용자이월오류로거부한다.1-5Next의1-6첫안내는별도집합으로허용하고Play는누르지않는다. 직접1-6/후반거부예상종료1·정책집합확인·전역ESLint종료0이다. 제품/번들은같다. [범위·원문](docs/webpj-autoplay-scope-1-6-skip-20261127.md). 현재v26=2/4·v22=7/8이다.
- **E1411 완료:** Node22.23.5 전체재검사6,901/6,901통과(977.044초·실패/취소/건너뜀0·종료0). 소스/대응표/공개자산/원본2983파일원시해시가모두같다. 최초6,899통과/2실패와E1410분류등록보완/집중7통과도보존했다. v22는0/8로재시작하고E1415전체6,902/6,902통과뒤v26은0/4·v22는1/8이다. 일본어UI/제단피해후보는이결과에포함하지않는다. [전문·고정·실패보완](docs/webpj-node22-regression-japanese-assets-20261127.md).
- **E1412 완료:** 일본어 선택/저장3·원본번호5·자동감지·실제Noto Sans JP CDN·제품문구·HTML게임메뉴·일본어전용줄바꿈을 연결했다. 드문한자 첫캐시 문제를 광고된 모든부분글꼴 선적재로 해결했다. 본소스집중69/69·네트워크12/12·마지막폰트계약6/6·전역ESLint/자산377/운영빌드·재번들동일성1/1통과다. Chromium최초5/5와독립증빙재실행3/3·Chrome/Edge/Firefox6/6·영어L2 1/1·한국어M2 2/2는각각별개실행이다. 31미션은진입/첫안내만확인했고플레이/목표/승리는실행하지않았다. 최대창540px·줄초과0·메뉴31개Noto다. CDN실패시영어시작안내/재시도와정확한소스감사예외를검증했다. 당시번들3,508,533바이트이며 E1420전체6,910/6,910통과뒤E1423완료뒤현재주기v26=2/4·v22=7/8이다. [구현·증빙·한계](docs/webpj-japanese-browser-support-20261127.md).
- **E1401 관찰 기록 완료·승리 미완료:** 실제1-6 아군AI120,000프레임/3009.175초에서 적2 포획1296.6·희생1443.45를 재현했으나 적3 생존·승리 없음이다. 오류0이며 승리 미확인 단언으로 종료1, 저장/후속 플레이는 수행하지 않았다. E1399와0~60,000의 관측JSON31행이 같지만 전체상태 동등으로 세지 않는다. 최종 아군AI의 적은2이고 그 적 사제SID는0이어서 실제 적 재선정 경로를 다음에 확인한다. [원문·화면·한계](docs/webpj-campaign1-6-ai-120000-20261127.md).
- **E1400 완료:** flat ESLint 권장 설정·개발 의존성/lockfile·`npm run lint`를 추가했다. 운영/개발/직접 webpack/API/자동 갱신에서 컴파일 전 검사를 기다린다. 실제 세 오류/경고 경로가 종료2·기존 번들 SHA/수정 시각 보존을 확인했고 정상 운영/개발 빌드도 통과했다. 기존 미사용 선언은126파일177건의 건수 예외로만 유지하며 규칙은 활성화했다. Node26 집중181/181·Node22 관문/번들 동일성3/3·Chromium초기L2/제품진행복원2/2 통과, 운영 번들3,500,786바이트다. [설정·유지보수·근거](docs/webpj-eslint-20261127.md).
- **E1398 완료:** 사용자 지시에 따라1-6 helper/CLI 허용과 완료 후 후속 플레이 차단을 반영했다. 집중3/3 통과(1-6 SP4000/이름/동맹/기술 보존·15초,1-1 6000프레임,허용 밖 거부). 기존 brain 재사용 분기도 보존하되 이번1-6은 실제 신규 생성이었다. [범위·검증](docs/webpj-ally-ai-scope-20261127.md)·[검사 안내](docs/testing/webpj-autoplay-test-guide.md). 미션 승리 완료로 세지 않는다.
- **E1397 완료·Linux 잔여 분리:** Node26/22 전체 각각6,895/6,895 통과(실패/취소/건너뜀0). Linux GUI의 세 화면비 L2·실제 진행 저장 읽기·하위 경로 배포5건도 통과했다. 실제 native AudioContext sink none/게임 audio=true 조건이며 물리 청취와 구분한다. 비고정 시작60초 대기1건은 실패로 보존해 M5에 남겼다. 원시 해시2파일은 CRLF만 달라 LF 내용 동일임을 확인했다. [원문·출처·한계](docs/webpj-linux-node-regression-20261127.md).
- **E1399 관찰 기록 완료·승리 미완료:** 1-6 실제 AI60,000프레임에서 사제 포획1296.6초·희생1443.45초를 관찰했으나 승리는 없었다. 게임 오류0/승리 단언 실패 종료1이며 후속 캠페인 플레이를 시작하지 않았다. [결과·원문](docs/webpj-campaign1-6-ai-observation-20261127.md).
- **다음 안내 자료:** CP 오류 문맥/Residence/epoch 초안은 E1390~E1392 제품에 반영했으므로 재적용하지 않는다. 준비 메모 `webpj/extracted/cpp-post1381-plan-20261127.json`의 clock 판정은 E1392 완료로 대체한다. 실제 비고정 부팅은 원래 UTC초 경계를 비동기로 기다리며 고정 시각0기준/4틱은 유지한다. 일본어39키는13차에 반영한다.
- **검증 교정(E1385):** 첫 전체 회귀의 새모듈 대응표 등록 누락2건과 옛가짜World 반납시험1건을 수정했다. 실제 Runtime 반납에서 캡처 기한/건물 숨김 초기화를 확인하며 제품 수명 검사는 유지했다. 최종 전체6,870개가 모두 통과했다.
- **최신 cpppj:** 제품1377/ZIP1378·검증1380이다. 웹 고정 오라클1313/225553df를 자동 교체하지 않는다. CP 튜토리얼 안내1372는 [WB-TUT-1~4](#tutorial-objective-port)의 필수 새 기능이며 E1386~E1389에서 완료했다. 정리 시 대응표는 기존 변경9행·대응표 밖 신규10파일을 보고했다([3.6](#cpp-delta)). 이전 동시 빌드 해시는 당시 자료로 보관하고 최신 해시는 공통 인계를 따른다.
- **현행 작업 지시(2026-11-27 최신 사용자 추가 지시):** 단위별 문서/로컬 커밋을 유지한다. **단순 미션 진입/안내 메시지 확인은 모든 캠페인에서 가능**하다. 실제 목표/승리 검사는 아군AI 방식이며 캠페인1-1~1-5·스커미시에서 계속한다. **1-6 승리도 사용자가 직접 플레이해 검증하므로 자동 검사를 건너뛴다.** 그 이후 플레이/목표/승리도 기존 사용자 확인 범위다.
- **전체 회귀**
  - **Node26.13.0 E1389:** 6,878/6,878 통과, 실패/취소/건너뜀0(795.306초).
  - **Node22.23.3 E1389:** Node26 종료 뒤 순차 실행해6,878/6,878 통과, 실패/취소/건너뜀0(674.318초).
  - **Node22.23.5 E1372:** 6,860/6,860통과, 실패·취소·미실행0(1092.815초). v26종료 뒤 순차 실행했다.
  - **Node26.13.0 E1372:** 6,860/6,860통과, 실패·취소·미실행0(1102.274초). 최초 전체6,857통과·3실패 뒤 웹 전용 분류와 검사 노드 위치를 고쳐 재검증했다.
  - **Node26.13.0 E1376:** 6,864/6,864통과, 실패·취소·미실행0(742.474초).
  - **Node26·22 E1380:** 각각6,867/6,867통과, 실패·취소·미실행0. 순차 실행 601.122초·821.347초. 검증 중www/tests는 고정했다.
  - **주기(E1397):** Node26 전체6,895/6,895(1342.706초), Node22 전체6,895/6,895(1053.522초), 실패/취소/건너뜀0. 실행은 순차였고 제품/Node검사 코드를 유지했다. E1398/E1399의 겹친 완료 단위까지 기록한 뒤 E1400/E1402/E1403/E1401/E1406을 완료했다. E1406은 E1401 종료 전 착수한 병행 단위였고 임시오류파일제거/검사경로를 마쳤다. E1405 Node26 전체6,897/6,897 통과(1174.102초), 작성소스2222개원시해시동일로 v26을0/4로 재시작했다. 이후 E1408/E1404/E1410을 마쳤고E1411 Node22.23.5전체6,901/6,901통과(977.044초·2983파일원시해시동일)로v22는0/8이다. E1415전체6,902/6,902통과와E1412완료뒤E1420 Node26전체6,910/6,910통과뒤E1419완료분을이월해E1423완료뒤현재v26=2/4·v22=7/8로관리이다. E1397의 전체 값은 ESLint 적용 전 소스의 이력이며 이후 집중 검사는 위 E1400을 따른다. 현재 번들은 E1416의3,508,534바이트다.
  - **Chromium:** E1313 전 미션 시작 상태 옵트인 전체 32/32 통과(24.3분, `webpj/extracted/webpj-e1313-final-e2e.log`). 기존 전체 e2e는 E1312 199건 중 186 통과·12 옵트인 미실행·한국어 브리핑 1건 부하 중 시간 초과(단독·3회 반복 통과)다. 단위별 실패·교정은 [E1313](docs/webpj-l3-mission-starts-20261124.md)과 각 단위 문서, 이전 회귀 이력은 [E1327 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1327)을 따른다.
- **브라우저 검사:** 미션 화면은 단위마다 Chromium e2e로 확인한다(최근 전체 실행 E1312, 고정 시각 L2·L3 `e2e/fixed-time.spec.js`·`e2e/l3-tutorial1.spec.js`·`e2e/l3-tutorial2.spec.js`·`e2e/l3-tutorial3.spec.js`·`e2e/l3-tutorial4.spec.js`(E1337부터 345프레임까지만 재실행)·`e2e/l3-tutorial5.spec.js`(E1339), 프레임 예외 뒤 진행 `e2e/error-reporting.spec.js` E1338, 앱 서비스 결선 `e2e/session-services-wiring.spec.js` E1328, 한국어 글자 배치 `e2e/korean-glyph-layout.spec.js` E1288). 메뉴는 2026-11-17 Chromium 2/2(갈무리11 일반·굵은 글꼴, 한국어 메뉴·도움말, 적재 실패 시 영어 대체), 최초 실행·진행 잠금/체크·저장 재실행은 E616 Chromium 2/2. [갈무리11](docs/webpj-korean-galmuri11-20261117.md) · [최초 실행](docs/webpj-first-run-20261116.md)

<a id="next"></a>

#### 다음 작업

1. **사용자 추가 요구 완료: ESLint(E1400).** 설정/선행 관문·운영/개발/자동 실패 차단·정상 생성·Node22 동일성·초기 앱 회귀를 마쳤다. 이후 JavaScript 수정은 `npm --prefix webpj run lint`를 통과시킨 뒤 번들을 만들고 기존 예외를 고친 경우 `--prune-suppressions`로 줄인다.
2. **허용 미션의 실제 AI 목표·승리.** 캠페인16개 진입·대기 L3는 E1350~E1365 완료다.1-1 승리(E1368),1-2·1-5 승리/진행 저장/새로고침 해제(E1369·E1371)도 완료이며 반복하지 않는다. 나머지는 아래와 같다.
   - [x] 1-1 신규 완료값 저장/재실행·다음 미션 해제 — E1394. 자연생성저장과네브라우저실제다음World/브리핑복원을확인했다. 재실행의30초시간초과도구분해보존했다.
   - [ ] 1-3 목표/승리 — E1408 메뉴28프레임대기조건은13,900프레임실제패배/게임오류0이다. E1396 현재C++60,000(마지막보고54,000)/최신웹60,000비교도둘다미승리다. 기존웹120,000자료도보존했다. 허용된실제AI입력조건을정해목표수행/승리를계속확인한다.
   - [ ] 1-4 목표/승리 — E1417 실제아군AI120,000프레임/3009.175초에서도오류0·포획/사망/승리없음이다. 양쪽brain초기화1·중지0·현재상대사제참조를보존했으며자연승리단언실패/종료1이다. 목표/승리·저장/Next는미완료다.
   - **[사용자 직접 확인으로 이월] 1-6 목표/승리·저장.** 최신사용자지시로승리자동검사를생략한다. E1419까지실제그래프복구/116,000표본오류0을확인한뒤실행을중단했다. 이후적재선정/승리/저장후속은실행하지않는다. 단순진입/첫안내확인은가능하다.
   - [ ] 스커미시 승리·후속 흐름 — 120000프레임 관찰 오류0·미승리 뒤 실제 적 전멸/승리 안내를 확인한다.
3. **스커미시 나머지.** E1418에서같은씨앗/자금충분조건의실제생산·다리·채취제어예약배율을확인했다. 실제채취유닛생산량절반(최소1)과소유자별360~600초다리정체의cpp오라클대조는남는다. [정량/한계](docs/webpj-skirmish-ai-periods-20261127.md)·[관찰](docs/webpj-skirmish-watch-20261123.md).
4. **남은 관문/후속 요구.** M4→M5와 [3.6 변경 판정](#cpp-delta)·3.7 후순위 요구를 진행한다. 모딩 API는 DN MOD-1 계약 설계 뒤 착수한다. 사람/물리 확인·결정 대기는3.5절이다.

**완료 범위:** Tutorial1~6의 자동 검사 허용 범위는 E1346, 캠페인16개 브리핑/시작/대기는 E1365, CP 변경16행 판정은 E1348에서 마쳤다. 입력 좌표·단위별 오라클 폴더·로컬 산출물 경과와 오래된 다음 작업 번호는 [정리 전0.1](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380)에 보존했다. 조사 자료·오라클은 Git 제외 `extracted/`·`webpj/extracted/`에 그대로 두며 지우거나 커밋하지 않는다.
- **미션 식별.** 공통 fixture의 `thewarbegins.fort`는 캠페인 1-1이고, 실제 `tutorial1`이 선택하는 Fort는 `BridgeTheGap`이다(E753 재대조). [미션명 정정](docs/webpj-session-login-shipped-forts-20261118.md)

<a id="e951-e1081"></a>

#### 완료 이력 요약

단위별 E951~E1380 완료 문장·근거 표는 [E1380 보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380)의0.1 진행 요약과 각 `docs/webpj-*.md`에서 확인한다. 현재 범위는 위 실행/동일성·3.1 체크가 기준이다. 대표 묶음은 메뉴·입력/채팅·경제·건설·공통 세션 수명·한국어·L2/L3·실제 아군 AI·WebMCP·HUD 생성 수명이며, 완료된 구현을 다시 만들지 않는다.

### 0.2 단계 진행판

`n/m`은 상위 체크 수이며 보관한 완료 항목을 포함한다. 상위 항목 안의 완료 부분(세부 단위 `- [x]`)은 중복 가산하지 않는다. 남은 단위·줄 수는 `PORT_MAP.tsv`의 partial·todo 기준(partial은 파일 전체 줄 수)이며, 2026-11-27 정리 재집계(`node webpj/tools/port-map.mjs`·`python -X utf8 tools/port_inventory.py`)도 **partial 0·todo 0, 0단위 / 0줄**이다(마지막 partial은 E1328, 3.6절 판정 6행은 E1336에서 닫음). cpppj 1345·1363~1370의 변경 판정도 E1347~E1348에서 닫았다. 이 수치는 고정한 이식 대상의 집계다. 이후 CP 변경9행·신규10파일은 E1393에서 별도 판정을 마쳤으며 새 튜토리얼4단위는0.8에서 관리한다. 남은 일은 표의 '남은 상위 체크'(3.1절 미체크 상위 항목, 정리 스크립트가 모음)와 3.6절 연결 이월, 3.7절 후순위 요구다. 4-D는 E1346에서 자동 검사 허용 범위85/85를 마쳤다.

| 순서 | 단계 | 체크·상태 | 남은 단위 / 줄 | 남은 상위 체크(3.1) |
| --- | --- | --- | --- | --- |
| 1~3 | M1·월드 선행 부품 | 완료 | — | — |
| 4 | 4-A 자료·4-B 적재·4-C 전송·4-R 재기준선 | 완료(4-C 통합 잔여는 4D-3에서 닫음) | — | — |
| 4 | **4-D Session 조립·Tutorial1 Load/Start** | **85/85, L2·Tutorial 자동 검사 허용 범위 닫음** | 0 / 0 | 건너뛴 공격·포획·희생·승리는1절 사용자 확인 |
| 5 | 5-A 렌더러·Draw | 46/46 | 0 / 0 | — |
| 5 | 5-B 카메라·HUD·미니맵·커서 | 13/13 | 0 / 0 | — |
| 5 | 5-C 화면 허브·미션 진입 | 6/6(E1325) | 0 / 0 | — |
| 6 | 입력·게임 메뉴·경제·건설 | 10/10 | 0 / 0 | — |
| 7 | 전투·의식·AI | 7/7 | 0 / 0 | E1385 전투 바인딩·AI7화면 완료 |
| 8 | 미션 수명·대본·오디오·도움말 | 8/9 → M4 | 0 / 0 | M4 |
| 9 | 표시 품질·브라우저/OS·배포 | E945 하위 경로·설정 복원 검증, M5 미완료 | 0 / 0 | M5(3.2절: 60/120 표시·성능·메모리, 브라우저·GUI Linux, 저장 호환; HTTP캐시 E1395 완료) |
| 10 | AGENTS 후순위 웹 요구 | 일부 선행, 3.7절 참조 | 0 / 0 | 3.7절 표의 남은 일(4·5·6·10·12·14차; 7차는 E1338, 13차 일본어는 E1412로 끝남) |
| **합계** | | | **0 / 0** | **3**(기존3.1 범위, 새 안내 WB-TUT-1~4는 별도) |

### 0.3 이식량

기준선 `225553df`의 이식 대상 1,007단위는 **port 이상 1,007(100%; `port` 275·`session` 732)·partial 0·todo 0**이다(2026-11-27 정리 재집계, E1328 이후 같음). 제외 153단위(멀티 73·Edit 42·호스트 37·구판 1)를 합하면 대응표 1,160행이다. 앱 수명 연결을 뜻하는 `session` 732행은 E1283 에서 `e2e/session-coverage.spec.js`(고정 시각 Tutorial1 4:3 130프레임 V8 함수 커버리지)와 `tools/port-map-session.mjs` 로 올렸다: **행의 웹 모듈이 모두 그 실행에서 함수를 하나 이상 실행했을 때만** 올리는 보수적 판정이며, 남은 `port` 275행의 `session` 승격은 시나리오를 늘려 같은 커버리지 도구로 재검사한다(전투·AI·의식·편집 아닌 UI 등). 오라클 대조를 뜻하는 `oracle`은 0이다(승격 규칙은 이후 시나리오가 늘 때 같은 도구로 다시 돌린다; [근거](docs/webpj-port-map-session-20261123.md)). E1098~E1237의 재판정 내역은 대응표 메모와 [E1166 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166)·[E1237 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1237)에, E1238~E1288의 행 닫기는 대응표 메모와 [E1288 보관본 0.2절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)에 있다.

| 범위 | port / 대상 | partial | todo |
| --- | --- | --- | --- |
| `o/` 월드·규칙 | 573 / 573 | 0 | 0 |
| `client/` 세션·UI·표시 | 433 / 433 | 0 | 0 |
| `zacket/` 내부 미션 전송 | 1 / 1 | 0 | 0 |

남은 partial·todo 단위는 0이다. 재기준선 추가분 RawClockSynchronization은 E1317에서 플랫폼 적응으로 닫았다. 메모 머리 태그와 이전 집계는 [E801 보관본 0.3절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), 현행 계열은 `python -X utf8 tools/port_inventory.py --families wb`(남은 계열이 없어 지금은 출력이 없다)를 따른다.

### 0.4 기준선과 자료

- **이식 기준선**은 cpppj 제품 1313(11.01) `225553df`다. 이후 변경 판정은 3.6절을 따른다. 별도 오라클 빌드는 HJOW-Athlon의 `extracted/webpj-oracle/225553df/Netstorm.exe`에 있다(SHA-256 `5b52d96d…b77eb46`, Git 제외).
- **자료 추적**은 [PORT_MAP.tsv](webpj/PORT_MAP.tsv)와 [fixture manifest](webpj/tests/fixtures/manifest.json) 1,330항목(E1370 현행 AI 신규17항목 포함)을 기준으로 한다. 이식 자료는 cpppj 소스·fixture·문서를 사용한다.

### 0.5 단위 착수와 종료

1. AGENTS.md와 공통·dotnetpj·웹 인계를 읽고 Git 상태·원격 변경을 확인한다. 같은 작업 트리의 다른 세션 변경을 고려해 파일명을 지정해 stage한다.
2. 대응표 검사와 기준선 뒤 변경을 확인한다. 3.1절의 단계 순서와 선행 조건에 맞춰 기준선 cpppj 코드, 기존 시험, fixture, 단위 문서를 읽는다. 큰 단위는 공개 API 경계로 나눈다.
3. 원본/클론·오라클 수집·별도 창 실행에는 AGENTS.md의 확인·예외 규칙을 적용한다. 웹 이식에는 디컴파일 소스 분석이 필요 없다.
4. 단위 문서에 근거 커밋·fixture/오라클·검사·실패와 교정·미연결 경계를 기록한다. 대응표의 web·base(8자리)·status·메모와 해당 manifest를 갱신한다. 현행 인계에는 현재 값·다음 작업·검증과 해당 체크만 갱신하고 완료 항목 상세는 정리 때 이관한다.
5. 각 작업 단위마다 해당 변경의 집중 검사와 연관 검사를 수행한다. HJOW-Athlon에서는 완료된 작업 단위 4회마다 Node v26 전체 회귀를, 8회마다 Node v22 전체 회귀를 수행한다. 두 버전의 전체 회귀가 함께 필요한 경우 순차 실행하며 동시에 실행하지 않는다. 다른 PC에서는 사용 가능한 최신 Node 버전으로 완료된 작업 단위 4회마다 전체 회귀를 수행한다. 필요하면 변경의 위험이나 통합 범위에 맞춰 추가 검사를 수행한다. 신호 NaN 비트 대조 시험은 맨 위에 `import './nan-exact.mjs'`를 둔다. (2026-11-19 사용자 지시)
6. `www` 소스를 바꾼 단위는 `npm --prefix webpj run build`로 webpack 번들(`www/dist/netstorm.bundle.js`)을 다시 만들어 함께 stage 한다(번들 Git 추적, E1297; `tests/bundle-fresh.test.mjs`가 낡은 번들을 실패로 잡는다). 완료 후 당일 최대 번호를 확인하고 `YYYYMMDD-NN WB 요약`으로 로컬 커밋한다. 기존 커밋의 Codex 공동 작성자 표기와 AGENTS.md의 push 금지 규칙을 따른다.

### 0.6 제품 공통 요구

11.01 표기·Credits 세 번째 항목과 HJOW/AI 문구·`Copyright 2026 HJOW`, 시작 클릭 화면, 공개 루트 내 실행 자산, 외부 접속 문서화·유효성 검사 비활성화는 반영했다. Credits AI 문구는 사용자가 다시 지시할 때 수정한다. **15차 Reset All은 E863에서 구현·검증했다.** [제품 11.01](docs/webpj-product-1101-20261105.md) · [외부 접속](docs/webpj-external-access-20261105.md) · [초기화](docs/webpj-reset-all-20261119.md)

<a id="favicon"></a>

- [ ] **favicon 탭 시각 확인.** E894에서 공통 ICO를 복사하고 상대 경로 링크를 추가했다. 원본 두 이미지의 바이트 대조와 Chromium의 HTTP 200·해독을 확인했다. 2026-11-20 Chrome의 localhost NetStorm 탭은 접근성 트리에서 확인했으나 Windows 창 캡처가 반복 시간 초과하여 탭 그림의 시각 확인이 남았다(E1263 점검: 탭 아이콘은 페이지 화면 캡처에 나오지 않는 브라우저 창 부분이라 자동 확인이 안 되고, 사용자 눈 확인 범위이다). [작업·검증](docs/webpj-favicon-20261119.md) · [공통 기준](LEFT_JOBS.md#exe-icon)

<a id="about-version"></a>

- [x] **정보(About) ▸ 버전 창의 Copyright(사용자 요구 2026-11-23) — E1289 완료.** 게임 메뉴·메인 메뉴의 버전 창에 `NetStorm 11.01 Patch by HJOW.`와 `Copyright 2026 HJOW`를 한 번만 넣었다(cpppj 1330과 같음). [근거](docs/webpj-about-version-copyright-20261123.md) · [공통](LEFT_JOBS.md#user-requests-20261123)

<a id="korean-font-change"></a>

### 0.7 한국어 갈무리11

E896에서 실제 미션 설치의 You→당신 번역과 Player 표시 바이트를 연결했고, 제공된 일반·굵은 WOFF2와 라이선스를 공개 데이터에 넣어 로딩·Canvas·자료 준비가 같은 글꼴 설정을 공유한다. Chromium 메뉴·도움말(영어 대체 포함)과 Tutorial1 한국어 브리핑 두 페이지에서 확인했다. [설치](docs/webpj-app-install-display-20261119.md) · [구현·검증](docs/webpj-korean-galmuri11-20261117.md) · [한국어 미션](docs/webpj-korean-tutorial1-20261120.md)

- **글자 품질(E1244·E1254·E1259~E1261·E1288):** 갈무리11은 12px 설계 격자로 그린다(제목 24·슬롯 4 48px, 4배 그리기·칸 평균, E1288). 숫자·영문·공백은 cpppj처럼 원본 비트맵 글리프, 슬롯 1·6의 한글 기준선은 `max(원본, 배율×11)`, 큰 슬롯은 원문 정수 배율(E1259), Tell 본문 폭은 가장 긴 낱말 이상(E1260), 굵은 글꼴에 없는 8,271자는 일반 갈무리11로 대체(E1244)한다. 게임 안 글자는 브라우저 최소·기본 글꼴 설정과 무관한 고정 크기다(예외: 웹 전용 ☰ 메뉴 DOM 버튼). [12px 격자](docs/webpj-korean-glyph-grid-20261123.md) · [제목 판독](docs/webpj-korean-title-legibility-20261123.md) · [굵은 글꼴 대체](docs/webpj-galmuri-bold-fallback-20261123.md)
- **사용자 결정:** 굵기는 원문 Bold 대신 지금의 보통체를 유지한다(2026-11-23 "지금은 그대로"). 글자만 출력 해상도로 덧그리는 고해상도 글자 층은 도입하지 않고 현재 픽셀 표시를 유지하기로 사용자가 결정했다(2026-11-24, 3.5절). cpppj와 다르게 둔 점(제목 배율·굵기·한글 기준선)은 근거 문서에 있다. 같은 점검의 cpppj·dotnetpj 후속은 사용자 지시로 작업을 보류하고 [LEFT_JOBS.md](LEFT_JOBS.md)·[LEFT_JOBS.dotnetpj.md](LEFT_JOBS.dotnetpj.md)에만 기록했다(`92ea7bfd1`).

<a id="tutorial-objective-port"></a>

### 0.8 사용자 추가 요구 — 튜토리얼 다음 단계 조건 안내

**2026-11-27 필수 반영, TUT-1~4 완료(E1386~E1389).** 공식 Tutorial1~6/40단계에서 다음 알림창·미션 종료창의 실제 조건을 우측 하단에 배경 없이 계속 표시한다. 실제 gate·예약/타이머는 읽기만 하고 진행/승패를 바꾸지 않는다. 기존 원본 이식의85/85·1,007단위 완료에 포함하지 않는 추가 기능이다. [공통 계약](docs/tutorial-objectives-porting-20261127.md)·[CP 화면/근거](docs/exe/cpp-tutorial-objective-display-1372-20261127.md).

- [x] **WB-TUT-1 조건·문구 — E1386.** 실제40단계/39키·읽기 전용 gate 조회와 영어/한국어 공개 자산을 반영했다. CP 카탈로그·예약/타이머·범위 제외·중복/누락0·번들 등11/11 통과. 일본어39키도 E1410/E1412에서 공용 확정본으로 반영했다. [근거](docs/webpj-tutorial-objective-catalog-20261127.md).
- [x] **WB-TUT-2 우측 하단 표시 — E1387.** 실제 Gump 글리프·여백8/최대폭440/간격2·보통체1픽셀 그림자, 배경 없음·입력 투과·크기 변경/부모 삭제 수명을 구현했다. 기존x86관측과3화면비/입력/픽셀 등28/28 통과. [근거](docs/webpj-tutorial-objective-gump-20261127.md).
- [x] **WB-TUT-3 진행·수명 — E1388.** 실제 Guide/World 토큰·gate·창/메뉴 가림·복원과 State/World/Client 정리에 연결했다. 덤프/조회 불변·실제 반납 등25/25, Chromium F4 A→B·F8·ESC1경로 통과. 화면비·전환 확대 검증은 TUT-4다. [근거](docs/webpj-tutorial-objective-lifetime-20261127.md).
- [x] **WB-TUT-4 검증·배포 — E1389.** 실제 F4/F8·영어/한국어3화면비·F1/F6·입력/리사이즈·재시작/전환/이탈/destroy와 실제39키 글꼴 부품·원본L2/M2 대조16경로를 확인했다. Node26/22 전체 각각6,878/6,878·번들/자산 검증 완료. 후반 목표 수행은 사용자 확인으로 남긴다. [근거/증빙](docs/webpj-tutorial-objective-verification-20261127.md).

## 1. 범위와 유지되는 결정

- **범위.** 일반 31경로와 데모 3경로를 대상으로 한다. 단독 미션의 같은 프로세스 안 내부 전송·패킷·Login·Zone·서버/클라이언트는 포함한다. LAN TCP·멀티플레이 UI·재접속/메타, 공식 검증 서버, Edit·커스텀 맵·User Made Campaigns, 설치/CD·Quit, 옵션 해상도/전체화면은 웹 범위에서 제외한다. 스커미시의 MP 시작 규칙과 Outpost는 12차에 포함한다.
- **이식.** cpppj의 파일과 API 경계, 부작용과 호출 순서, 정수 폭과 저장 시 정밀도, 난수 소비를 보존한다. 단독에서 도달하는지 확인해 웹 제외 분기에는 명시 오류를 유지한다. 현재 단계가 직접 요구하는 부품만 앞당긴다.
- **웹 구조.** `netstorm.js`가 앱 수명을 맡는다. 모든 스크립트는 webpack 으로 번들 하나(`www/dist/netstorm.bundle.js`, 진입 `www/bootstrap.js`)로 묶어 `index.html`이 그것만 불러온다(E1293, 번들은 사용자 결정으로 Git 에 추적한다 — `www` 소스를 고친 단위는 `npm --prefix webpj run build`로 번들을 다시 만들어 함께 커밋하며 `tests/bundle-fresh.test.mjs`가 낡은 번들을 잡는다). 번들 없이 이전 방식(ES 모듈 직접 적재)으로 구동하는 디버그 페이지는 `www/debug.html`이며, 앱과 같은 모듈 인스턴스가 필요한 e2e(`import()` 사용 11개 파일)도 이 페이지를 연다. `index.html`은 최소 변경하고 스타일은 JS에서 관리한다. 기존 CommonJS 서버와 `www/` ES 모듈 경계를 유지한다. 코어를 DOM·Canvas·Web Audio에서 분리하고 8비트 논리 화면과 최종 표시를 구분한다.
- **시간·입력·수명.** 시작 입력에서 오디오를 활성화한다. 탭이 숨으면 진행을 멈추고 복귀 시 시계를 다시 잡는다. `requestAnimationFrame`은 표시 기회다. 4:3부터 확인하고 16:9·16:10, 입력 역변환·DPI를 비교한다. 가장자리 스크롤은 활성 미션과 게임 영역 안의 포인터에서 동작한다.
- **자산·설정·언어.** `assets/game-data`·`fonts`·`webpj/translations`에서 `webpj/www/data`를 준비하며 느슨한 파일→TAFF 조회 순서를 유지한다. 버전 있는 `netstorm.settings`·`netstorm.progress` JSON은 localStorage에 두며 오류 시에도 계속할 수 있게 한다. 한국어 누락 파일·섹션·주제는 영어로 대체하고 미션 문구는 cpppj 확정본을 기본으로 삼는다. 사용자 검수와 언어별 글꼴은 3.5·3.7절을 따른다.
- **외부 링크.** Tell/도움말에서 치환된 `http:`·`https:`·`mailto:` 링크는 새 탭/창으로 열고 다른 주소의 제외 사유는 console에 기록한다. 개발 서버는 모든 인터페이스에서 수신한다. [외부 접속 7절](docs/webpj-external-access-20261105.md)
- **사용자 확인 범위.** 최신2026-11-27지시에따라1-6승리는사용자직접플레이로이월해자동검사를생략한다. 목표/승리자동검사는1-1~1-5·스커미시의실제아군AI방식으로계속한다. 그이후플레이도사용자직접확인이다. 단순진입/안내확인은모든캠페인허용이다. 전체외관/음향동일성은별도사용자확인범위이며공중3종(훨리기그·맨오워·더스트데빌)은정적이식범위다.
- **보호·분담.** AGENTS.md·원본/CD·기존 fixture·공통 인계·dotnetpj 파일을 보존한다. cpppj 동시 작업은 사용자의 목표 지시를 기준으로 하며 별도 언급이 없으면 필요한 cpppj 수정이 가능하다. README 공통 상태는 일괄 갱신 때 반영한다. 새 문서와 코드는 UTF-8, 새 코드의 한국어 주석은 설명·사용법을 먼저 쓰고 추가·수정 시점과 이유를 뒤에 쓴다.

## 2. 완료 범위 요약

| 범위 | 확인한 결과 |
| --- | --- |
| WB00~WB04-D5c | 시작·메뉴·설정/진행·도움말·언어/제품 표기, 자산/TAFF/Fort 조회, SID·난수·Kernel·Form/Process 공통층. [기반과 단위 링크](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801) |
| E1~E328 | 월드 규칙 부품, 자료·타입 표, Fort·지형·섬과 단독 미션 전송 앞단. [순서 3 및 4-A~4-C](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801) |
| E329~E632 | 이동·비행·전투·의식 선행 부품, 렌더러·Gump·카메라/HUD, 제품 1313으로 재기준선, typed Session 생성자·공유 snapshot writer 선행. 최초 실행(E616)과 내부 미션 전환 flush/종료 수신(E620) 반영. [선행 부품과 재기준선](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801) |
| E633~E702 | 외곽 생성자·snapshot/콜백과 typed `Load()` 완료분. 실제 1-1 Fort 적재, Battle snapshot 재적재, 기술·로컬/세계 간헐천 및 단일 미션 너겟 관문 확인. [typed Load](docs/webpj-session-load-20261118.md) · [Battle snapshot](docs/webpj-session-battle-snapshot-20261118.md) |
| E703~E774 | 실제 typed Login/Ready·Start, 출하 스크립트 32종의 Fort 로그인, 같은 Session의 사제·골렘·벌룬 이동과 워크샾 배치·격리 완공. [Login](docs/webpj-session-login-ready-20261118.md) · [Start](docs/webpj-session-mission-start-20261118.md) · [워크샾](docs/webpj-workshop-isolated-completion-20261118.md) |
| E775~E801 | Session 카메라·입자·튜토리얼 게이트, AI/Player/Process 반납 경계, 지연 GameWorld·팔레트·자산 보기·선택. World 입력 부품과 UserInput 폴링·단축키·우클릭까지 연결. [공통 게이트](docs/webpj-session-tutorial-gate-20261118.md) · [지연 GameWorld](docs/webpj-campaign-game-world-front-20261118.md) · [입력](docs/webpj-campaign-selection-keys-20261118.md) |
| E802~E950 | 입력 클릭·NB20 취소, GameWorld 프레임·세 운반 유닛 도착, 워크샾 Kernel 완공·연구 동기화, 전투 완료 관문, Client 진단·프레임 시계, Session 덤프 형식과 오라클 시나리오(4D-9b), 앱 Guide·State7·Gump 루트·팔레트·Screen·입력 큐·TextEntry/IME·갈무리11 편집, Reset All(E863), 16:9 최대 1920×1080(E943), 하위 경로 배포(E945). [E1081 보관본 0.1·3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261120-through-wb04-e1081) |
| E951~E1339 | 미션 앱 프레임(로딩·Start·음향·Renderer/Gump 출력)과 브라우저 입력·카메라·조작키, 31개 미션·데모·스커미시 진입과 장시간 진행, 생산·건설·객체 메뉴·게임 메뉴막대·승패 뒤 흐름·도움말, 한국어 대본·게임 안 문구·글자 품질, HUD 도킹·채팅 창, 웹 범위 판정과 Native 대응 대조, 감사와 partial 34→0행(E1328), 고정 시각 L2(E1283)·Tutorial1~3 L3 완주와 Tutorial4 단계 E1 진입(E1335), 원본 캡처·Session 대조, 사용자 추가 요구 세 건(버전 창 Copyright·한국어 ESC 메뉴·방향키 카메라), webpack 번들, 처음 방문 시작 화면 안정화, cpppj 1335 마나 그림 보충(E1336), Tutorial4 자동 검사 범위 제한(E1337)·일반 프레임 예외 뒤 계속 진행(E1338)·Tutorial5 단계 A1 L3(E1339). 주제별 표는 [0.1절 요약](#e951-e1081), 범위별 행 원문은 보관본에 있다. |

## 3. 남은 작업과 실행 순서

### 3.0 개발 방식

Tutorial1 하나를 Load→Start→표시→입력→진행까지 먼저 확인하고, Early Missions·캠페인 1-1/1-2와 전체 34경로로 넓힌다. cpppj 클래스 단위로 기존 시험을 옮기고 원본 x86 fixture를 직접 읽는다. 플랫폼 경계는 대응표의 `adapt`로 판정한다. 생성 표는 생성 도구의 `--check`로 대조한다.

### 3.1 단계별 항목 목록

단계 안에서 선행 조건을 따라 위에서부터 진행한다. 막힌 사유를 기록하고 해당 단계의 다음 항목으로 간다. 완료 체크에 단위 링크를 붙이고 0.2절과 단계 머리글을 함께 갱신한다. 여러 독립 작업을 묶은 항목은 `- [ ]` 세부 단위로 나누고 세부마다 완료 기준을 적는다(세부 단위는 머리 수에 넣지 않는다, 0.2절). 닫힌 체크는 단계마다 '완료한 체크' 줄이나 표로 줄이고 점검·완료 근거 원문은 보관본에 둔다.

#### 4-D Session 조립·Tutorial1 Load/Start (85/85)

**완료 기준 L2는 E1283에서 닫았다.** 같은 시점·난수에서 Tutorial1 Load/Start의 덤프·첫 프레임 논리 화면이 cpppj 오라클과 같다(영어 4:3·16:9, 90·130프레임: 화면 0픽셀, 덤프는 환경 의존인 `sp 1`·`player:*` 행만 다름 — `player`의 +1·+2 바이트는 주소 잔재, Player 1 +0x68·+0x69는 Deck 할당 토큰의 표현·시점 차이). 기준선 구간 지도와 E633~E801의 완료 체크는 [E801 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), E802~E1166의 항목별 완료 나열은 [E1166 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261121-through-wb04-e1166), 아래 닫힌 항목의 점검·완료 근거 원문(E1263~E1283)은 [E1288 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261123-through-wb04-e1288)에, 그 뒤 항목별 닫은 근거 표 원문은 [E1329 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1329)에, Tutorial3 단계별 완료 줄과 Tutorial4 단계 A~E1 상세는 [E1336 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261125-through-wb04-e1336)에 있다. cpppj 줄 번호는 `225553df`의 [RawCampaignSession.cpp](cpppj/src/client/RawCampaignSession.cpp) 기준이다.

- [x] **4D-4b Tutorial 게이트·표시 장치의 자동 검사 허용 범위** — Tutorial1~3 기존 완주(E1304·E1307·E1333), Tutorial4 E1 안내(E1335·E1337), Tutorial5 D 안내(E1342), Tutorial6 E 안내(E1343~E1346)까지 완료했다. 이후 특정 적 공격·포획·희생·승리 자연 전환은 사용자 확인이다. E1336 뒤 Tutorial4 a220의 알려진 생산 메뉴 차이8,656픽셀을 유지하며 이전0픽셀 결과를 새 판에 확대하지 않는다. 단계별 화면/덤프 수치·입력 표는 [정리 전3.1](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380)과 [E1346](docs/webpj-l3-tutorial6-crossbow-build-20261126.md)에 있다. 새 진행 조건 안내는 [WB-TUT-1~4](#tutorial-objective-port)로 별도 관리한다.

완료한 체크(머리글 수에 포함; 항목별 닫은 근거 표 원문은 [E1329 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1329)):
- **4D-1~4D-3:** 4D-1 외곽·snapshot(E946·E1270, 비미션 너겟은 미션 모드라 미도달), 4D-2 typed Load(E697), 4D-3a Login(E1263), 4D-3b ClientCampaignTransport(E1274), 4D-3c 전송 앞단(E1268), 4D-3d Squid 수신(E1263), 4D-3e 명령 등록(E1267), 4D-3f 통합 시험(E1263). [Fort 경계](docs/webpj-session-fort-snapshot-boundary-20261119.md) · [기본 수신기](docs/webpj-client-default-receivers-scope-20261123.md)
- **4D-4a~4D-7:** 4D-4a 카메라·프레임(E1266, 주 루프 순서), 4D-5a 종료 수명·4D-5b Client State 정리(`_releaseCampaignWorld`)·4D-5c 이탈·전투 판정(`missionPost` 관문)(E1263·E1268), 4D-6a GameWorld·4D-6b Client 서비스(CampaignClientServices port E1296)·4D-6c Session 공개 조회(E1263), 4D-7a~7d ClientMain·Client 전역(E1263; 일반 프레임 예외 뒤 다음 프레임 진행은 E1338, 3.5절 2026-11-26 결정). [프레임 순서](docs/webpj-frame-order-20261123.md) · [이탈 정리](docs/webpj-mission-leave-state-reset-20261123.md)
- **4D-8~4D-10:** 4D-8a RawSurface 삭제·수명(E1273·E1321), 4D-8b~8f Native 분기(E1193~E1221)·4D-8g `UsesNativeTransmissionDevices()` 재대조(E1263), 4D-9a·9b 덤프·오라클과 앱 `sessionDump()`(E869·E1263), 4D-10a 실제 소유자 재대조(E1263), 4D-10b L2 판정(E1283). [정상 복귀](docs/webpj-destroy-patch-continue-20261124.md) · [Native 수신기](docs/webpj-native-receivers-audit-20261122.md) · [고정 시각](docs/webpj-fixed-time-mode-20261123.md)

#### 5-A 렌더러·Draw·시각 시스템 (46/46)

완료한 체크(머리글 수에 포함, 보관한 44개 외; 아래 두 개의 본문은 [E1327 보관본 3.1절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1327)):
- **ClientRendererFrame** — E1298. 미션 화면 출력의 원본 순서(E978~E1082)와 메인 메뉴 Kencloud 구름 캐시(M1 구름 차이 37,054픽셀 → 0). [Kencloud](docs/webpj-menu-kencloud-20261124.md)
- **E197·E198·E201 재대조** — E1316. 파편·건물(E1275~E1276)·골렘/받침(E1312) 캡처와 실제 Session의 EvilRay(E1314)·피해 연소(E1315)·섬 투하(E1316)를 C++ 시험과 대조했다. [마지막 근거](docs/webpj-drop-island-session-20261124.md)

#### 5-B 카메라·HUD·미니맵 (13/13)

완료한 체크(머리글 수에 포함, 보관한 9개 외): **UberHud**(E1255, [근거](docs/webpj-uber-hud-close-20261123.md)), **RawMinimapControls**(E1248, [근거](docs/webpj-minimap-controls-close-20261123.md)), **FloatingSpGump·RawProductionControls**(E1257, [근거](docs/webpj-production-controls-close-20261123.md)).

- [x] **Cursor·RawCursor 표시** — E1366. 기존 카메라·커서·마나 볼트에 더해 실제 가장자리 이동/정지 화면까지 대조했다. [근거](docs/webpj-l3-campaign-edge-scroll-20261126.md)
  - [x] 카메라 투영·커서 표시 — L2/L3 고정 시각 대조에서 F4 카메라 이동 뒤(300프레임)와 다리 조각을 든 커서(345프레임) 화면이 cpppj와 0픽셀이다(E1280~E1286).
  - [x] 가장자리 스크롤 뒤 화면 — E1366에서 입력 방법과 같은 시점의 화면을 확인했다.
    - [x] 입력 방법 확인 — C++ `inspectPoll`의 좌버튼 뗌 `pointer 2147549184,x,y`와 웹 N−1프레임 DOM 이동을 대응했다. 전체화면 테두리0과 실제 카메라 이동/정지로 검증했다.
    - [x] 화면 대조 — E1366, 실제 오른쪽 스크롤·정지4화면0픽셀.
  - [x] 마나 볼트 정기 표시 — 마나 볼트가 날아가는 장면의 고정 시각 화면이 cpppj와 0픽셀이다(E1332 L3 `e2510`·`e3240`, E1333 `f3620`). [근거](docs/webpj-l3-tutorial3-stage-def-20261124.md)

#### 5-C 게임 화면 허브·미션 진입 (6/6)

완료한 체크(머리글 수에 포함): **Uber 허브**(E912~E917·E1082·E1169~E1179, 배경 GIF·NSInfo 자식은 웹 범위 밖 E1243, [판정](docs/webpj-uber-root-background-scope-20261123.md)), **미션 진입 명령**(E1263, [Tutorial 시작](docs/webpj-browser-tutorial-menu-start-20261120.md)), **허브 fixture**(E1265, [근거](docs/webpj-chat-hub-fixtures-20261123.md)), **메인 메뉴·타이틀 상태**(E1184, [문서](docs/webpj-title-screens-20261122.md)), **메뉴·버튼 컨트롤**(E1325, RawButtonControls·RawMenuChat·RawMenuHtml·RawMenuControls·RawMenuLoading·RawTextEntryControls port, [메뉴 컨트롤](docs/webpj-menu-controls-close-20261123.md)), **M2 / L4a**(E1323, 영어 4:3·16:9·16:10과 한국어, [한국어](docs/webpj-korean-m2-20261124.md)).

#### 6 입력·게임 메뉴·사제/운반·경제·건설 (10/10)

완료한 체크(머리글 수에 포함, 보관한 1개 외): **입력 본체**(E1263, [E802](docs/webpj-campaign-user-input-movement-click-20261118.md)), **게임 메뉴**(E1264, [게임 메뉴막대](docs/webpj-game-menu-bar-20261120.md)), **공장·지식 메뉴와 기술**(E1076~E1079·E1189, [E1189](docs/webpj-fury-war-technology-20261122.md)), **객체 우클릭 메뉴**(E1310, [e2e](docs/webpj-object-menu-rows-20261124.md)), **방향키 카메라 이동(사용자 요구 2026-11-23)**(E1291, [근거](docs/webpj-arrow-camera-20261124.md)), **ESC 게임 메뉴 한국어 점검(사용자 제보 2026-11-23)**(E1290, [근거](docs/webpj-esc-menu-korean-20261123.md)), **건설·생산·경제**(다리 조각 연속 배치 E1299~E1307로 마감, 건설 기록 재생은 웹 범위 밖 E1242, [다리 조각](docs/webpj-l3-tutorial1-20261123.md#확장-다리-조각-연속-배치wb04-e1299-2026-11-24)), **M3 / L3**(Tutorial1 입력 단계표 세부 넷 E1284~E1307, Tutorial3~6 확장은 4D-4b, [L3](docs/webpj-l3-tutorial1-20261123.md)).

- [x] **사제·운반·경로 바인딩** — E1367의 저장 사제 L3로 마지막 경계를 마쳤다. [근거](docs/webpj-l3-campaign-priest-path-20261126.md)
  - 완료: 캠페인 Session 운반 입력과 일반 사제 이동(E794), 실제 마우스의 사제 선택→이동(E1139 e2e), 단독 미션 제단 희생 메뉴 훅(E1223), 기도·Cast/Drop·Factory 순환 명령 소비(E1225), Review Knowledge·GetTechnology 단추와 기술 획득 Fort snapshot 장치(E1227), Bulf 너겟 줍기(0x030000b1)·메뉴 Drop(0x030000b2) Chromium e2e(E1311), 주문 Cast와 주문 보유 골렘의 사제 포획·Drop(E1318~E1319). MetaMap은 도달하지 않고(E1188), 저장 미션의 전체 초기화와 경로 루프는 31개 시작 e2e·`raw-campaign-session-load.test.mjs`·`mission-priest-move.spec.js`·`raw-campaign-motion-bindings.test.mjs`로 확인했다.
  - [x] 저장 미션 경로 L3 — E1367,1-2 실제 UI 이동의4화면0픽셀·전체 덤프 일치.
  - 근거: [E794](docs/webpj-session-carrier-input-20261118.md) · [실사용 점검](docs/webpj-real-use-checks-20261121.md) · [줍기·Drop e2e](docs/webpj-object-menu-rows-20261124.md) · [포획](docs/webpj-spell-priest-pickup-20261124.md)

#### 7 전투·의식·AI (7/7)

완료한 체크(머리글 수에 포함): **파괴·주문 효과**(E1199), **의식**(E1180, 보상 선택 UI는 웹 범위 밖, [의식 판정](docs/webpj-ritual-reward-scope-20261122.md)), **AI 잔여·전투 이탈**(E1198), **전용 시험이 없었던 시스템**(E1316, [근거](docs/webpj-drop-island-session-20261124.md)), **주문(스펠) 사용·골렘 사제 포획(사용자 제보 2026-11-23)**(E1318~E1319, cpppj 1339도 같게 교정, [주문](docs/webpj-spell-cast-20261124.md) · [포획](docs/webpj-spell-priest-pickup-20261124.md)).

- [x] **전투 바인딩** — Hit·GunQueries·Archer·Motion·BombBlast·Damageable 여섯 표 행은 모두 port이고 Session에 결선돼 있으며(`raw-campaign-session-{hits,guns,gun-views,combat,flyers,damageable}.js`와 각 Node 시험) 실제 Chromium에서 AI 혼전이 약 18분 오류 없이 돈다(E1262). Archer의 초기 배치·비행 및 factory/clock loop는 1절의 검증 범위를 적용한다. E1385에서 실제AI7덤프/7화면0픽셀로 마감했다.
  - [x] 전투 장면 오라클 — E1370 독립AI17자료의7덤프·7화면을 E1385의 실제 State/Runtime과 명시건설 메모리 입력에서 모두 일치(0픽셀)로 고정했다. E1384가7000·12000의31·99픽셀을, E1385가8000의1픽셀을 해소했다. 마스킹/알려진 차이 허용은 없으며 관측 입력의 환경 한계는 [근거](docs/webpj-construction-memory-input-20261127.md)에 기록했다.
  - [x] 교전 N프레임 대조 — E1334 Tutorial4 단계 B 교전 구간 체크포인트(`b1550`·`b2100`·`b2750`, 발사기와 적 `Sun Cannon`)와 E1335 썬 캐논 사격·적 발사기 연쇄 격파(`e7200`)의 화면이 cpppj와 0픽셀, 덤프가 허용 행 밖 차이 0이다. [단계 A~B](docs/webpj-l3-tutorial4-stage-ab-20261125.md) · [단계 C~E1](docs/webpj-l3-tutorial4-stage-ce-20261125.md)
- [x] **AI 세션 결선** — E1370의 실제 두 AI7시점 상태·행동 기한 대조로 마쳤다. [근거](docs/webpj-campaign-ai-telemetry-20261126.md) — Start의 일반/데모 AI·동맹과 typed 프레임, 현재 Player·SP·명령·난수와 장기 RawSurface 소비자를 연결했다(`raw-campaign-session-ai*.js`·`netstorm-campaign-ai.test.mjs`, E1122·E1124·E1198·E1206~E1216). DestroyPatch 진단은 사용자 결정에 따라 E1321에서 표시 후 정상 복귀로 고쳐 실제 앱에 연결했다(3.5절, [근거](docs/webpj-destroy-patch-continue-20261124.md)).
  - [x] AI 행동 N프레임 대조 — E1370, 실제1-1 AI1000~12000프레임7곳의 난수·Regular·객체·AI 본문/기한·SP 일치. 당시화면3곳 차이는E1384~E1385에서 모두 해소했다.

#### 8 미션 수명·안내·대본·소리·채팅·도움말 (8/9)

완료한 체크(머리글 수에 포함): **대본·대화창**(E1263, [E1190 점검](docs/webpj-script-commands-audit-20261122.md)), **소리 본체**(E1261), **음악**(E1258, [근거](docs/webpj-scene-music-close-20261123.md)), **글자·설정·그림**(E1261, [근거](docs/webpj-korean-title-legibility-20261123.md)), **적응 fixture**(E1263), **도움말·Tell**(E1197~E1240 해당 대응표 행 모두 port, [근거](docs/webpj-main-menu-help-type-20261123.md)), **동영상**(E1202 사용자 결정으로 웹 제외, [결정 기록](docs/webpj-intro-movie-decision-20261122.md)), **미션 수명**(E1085~E1161, [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md)). 실제 청취 판정은 사용자 확인 범위이다.

- [ ] **M4** — 일반 31 + 데모 3경로의 진입·진행·저장과 한국어 미션 문구를 확인한다. 자연 완주는 1절의 사용자 확인 범위다.
  - 완료: 캠페인 목록 31개 미션의 연속 시작(E1121, Priest Training 9개는 E1157에서 실제 연결하고 Auto-Demo가 대신 돈 e2e 거짓 통과를 정정), 캠페인 16개 장시간 진행과 AI 시점 미연결 서비스·좌표 오류 네 가지 수정·소리/안내 서비스 여섯 개 연결(E1122), Early Missions·Priest Training 15개 100초 진행(E1123)과 Priest Training 9개 150초씩 26분(E1158), 스커미시 AI 7명 300초(E1124), 데모 420초 진행과 360초 재시작(E1125), 후반 4개·중반 13개 장시간 진행으로 시계 정지 누수 수정(E1132~E1133), 몽키 시험(E1128·E1134·E1165, 죽은 Gump 정리 수정 포함), 승패 안내 뒤 흐름과 진행 저장·다음 미션 해제(E1152·E1161).
  - [ ] 자연 승패 진행 저장·자연 완주 — 허용 미션은 실제 아군 AI로 확인한다.1-1·1-2·1-5의 실제 승리 → 신규 완료값 저장 → 새로고침 → 다음 미션 해제는 E1394·E1369·E1371 완료. 1-3/1-4/스커미시는실제AI로계속하며1-6승리와그이후단계는사용자직접확인으로남긴다.
  - [ ] 한국어 전체 확인 — 사람 검수(3.7절 6차)와 미션 본문 줄 나눔 실제 화면. 완료 기준: 사용자 검수 결과가 기록되고 지적 사항이 반영된다.
  - 현재 구분: 한국어 사람 검수와 허용 밖 자연 완주는 사용자 확인, 허용 미션의 아군 AI 목표/승리와 저장은0.1 다음 작업1에서 계속한다.
  - 근거: [모든 미션 시작](docs/webpj-all-missions-start-20261120.md) · [장시간 진행](docs/webpj-mission-services-long-run-20261120.md) · [데모 재시작](docs/webpj-demo-restart-20261120.md) · [시계 정지 누수](docs/webpj-mission-leave-pause-20261121.md) · [Gump 정리](docs/webpj-gump-cleanup-frame-20261121.md) · [몽키·미니맵](docs/webpj-monkey-minimap-20261121.md) · [승패 흐름](docs/webpj-mission-outcome-flow-20261121.md) · [Priest Training](docs/webpj-priest-training-missions-20261121.md)

### 3.2 실행 순서와 남은 마일스톤

M1은 메뉴 관문을 통과했다. 4-D의 Tutorial1 L2 → 5-A/5-B·5-C의 Tutorial1 화면(M2) → 6단계 입력과 플레이(M3) → 7/8단계 전체 범위(M4) → 9단계 배포(M5) 순으로 진행한다. 9단계는 원본 시계를 유지하며 60/120 표시, 성능·메모리, Chrome/Edge/Firefox·GUI Linux와 하위 경로 정적 배포·새로고침·캐시·저장 호환을 검증한다. AGENTS 후순위는 3.7절을 따른다. E945에서 Windows의 Chromium·Chrome·Edge·Firefox로 하위 경로의 정적 모듈·자산 로드와 같은 URL 새로고침 뒤 음량 설정 복원을 확인했다. E1395에서 조건부 HTTP캐시/같은URL 배포교체와 네브라우저 설정복원을 확인했다. E1397에서 Linux GUI 세 화면비 L2·진행 저장 읽기·하위 경로 배포를 확인했다. E1402에서 비고정 초기 시작을 단독 재시도해 통과했고 E1403에서 메뉴60/120 선택·시계 경과·짧은 메모리 표본을 확인했다. E1404의 짧은실제미션반복 반납/GC는 완료했고 물리120FPS·장시간 메모리는 아직 남는다. 앞선 시간 초과 원인/부하 안정성과 물리 음향은 남겨 두었다. [캐시](docs/webpj-http-cache-20261127.md) [E945](docs/webpj-static-deployment-20261119.md)

### 3.3 cpppj 오라클 사용법

- **같은 조건.** 자료 해시·언어·화면 크기·옵션·시드·프레임과 입력을 맞춘다. 기준선 실행 파일은 0.4절의 별도 빌드를 쓴다. 독립 `NETSTORM_USER_DATA`와 `--mute-audio`로 소리 장치 상태를 보존한다.
- **수집·비교.** `tools/cpp-oracle.mjs`의 시나리오에서 `--ui-script`의 `snapshot`·`report`·`session-dump`를 수집하고 `tools/screen-compare.mjs`로 논리 화면을 비교한다. 알려진 차이는 영역·픽셀 수·원인을 기록한다. 웹은 검사 전용 고정 시각 모드(`?fixed=25`, `stepFixedFrames`, E1271)로 같은 프레임을 만든다. L2는 `e2e/fixed-time.spec.js`(E1283 닫음), L3는 같은 단계표를 웹에 적용하는 `e2e/l3-tutorial1.spec.js`(E1284)와 공통 틀 `e2e/l3-common.js`·`tools/l3-collect.mjs`(E1305)이며 웹 입력은 클릭 N+1·키 N−1 프레임에 넣는다. 난수 소비 순서는 cpp 임시 난수 추적(호출 스택 RVA와 링크 맵)으로 대조할 수 있다(E1280). L4b는 cpppj MCP 녹화를 쓴다. [오라클 비교](docs/webpj-cpp-oracle-20261107.md) · [덤프·고정 시각](docs/exe/cpp-session-state-dump-20261109.md) · [난수 추적](docs/webpj-state-install-rng-20261123.md) · [L3](docs/webpj-l3-tutorial1-20261123.md)
- **실행·추적.** 창을 띄우는 수집은 AGENTS.md의 확인과 호스트 예외를 적용한다. 결과·cpppj 커밋·EXE SHA-256·명령줄을 manifest에 남긴다. Node 검사는 저장된 결과를 읽는다.

### 3.4 cpppj 현황과 기준선 관계

공통 인계상 최신 cpppj 제품은1377(최신 ZIP1378·검증1380·M8 관문1327)이며 웹 비교 기준선은 제품 1313이다. 1314 최초 실행·1320 미션 전환(E616·E620), 1328~1330 사용자 추가 요구(방향키 E1291·ESC 한국어 E1290·버전 창 E1289), 1339 주문/포획 교정(웹 E1319와 같은 교정, 대응표 base `13e74579`)을 웹에 반영했다. 1331~1338은 시험 교정·배포 확인과 일본어(13차) 작업이며 소스 변경 판정은 3.6절에 있다(1335의 언어 무관 마나 그림 별칭·CD 25프레임 보충은 E1336에서 반영). 1340~1362는 일본어 본문 43개·도움말 128주제 번역과 최종 관문(13차)이며 그중 `cpppj/src` 변경은 1345의 AI 기본 이름 길이 제한뿐이다. 1363은 언어 무관 공통 도움말 플래그·숫자 칸 우클릭, 1364~1370은 TCP 상대 봇·LAN 멀티플레이(웹 제외 범위)이지만 port 분류 파일을 고쳐 둘 다 3.6절 판정 대상이다. 1315의 한국어 `Type` 번역 교정본은 8단계 공식 한국어 자료에 적용하고, cpppj 시험만 바뀐 항목(1316~1319·1321·1324)은 해당 시험을 옮길 때 현행 기대값을 확인한다. 1322(갈무리11)·1325(Reset All)·1326(테스트 AI 진단)의 판정은 3.6절, 1323 아이콘은 favicon(0.6절)으로 따로 반영했다. [공통 인계](LEFT_JOBS.md)

### 3.5 사용자 확인과 결정 대기

- 자연 완주·실제 사제 포획/희생·공격·승리·전체 화면/음향은1절범위를따른다. 최신2026-11-27추가지시로1-6승리도사용자직접확인에이월해자동검사를생략한다.1-1~1-5/스커미시는실제아군AI로진행하고단순진입/첫안내는모든캠페인허용이다.
- 미션 한국어는 cpppj K1~K5 확정본을 기본으로 한다. 자료 분리 여부와 실제 미션 줄바꿈/혼합 제목·대체 표시를 8단계에서 확인한다.
- 와이드 전투 시야와 카메라 재배치를 연결할 때 16:9의 논리 크기 전환도 검증한다. WebMCP는 E1372에서 Document API 지원 Chromium156의 실제 입력·캡처·소리 포함 녹화·다운로드로 완료했다. 미지원 브라우저의 일반 실행도 검증했다. [사용법](docs/webpj-webmcp-20261126.md)
- **사용자 결정 완료(2026-11-24).** 채팅 로그 앱 수명 유지(E1320, E1170의 미션 종료 초기화 적응 해제)·DestroyPatch 진단 열두 지점 표시 후 계속(E1321)·16:10 논리 크기 1280×800(E1322)·한국어 고해상도 글자 층 미도입과 보통체 유지(0.7절). 결정 문장 원문은 [E1339 보관본 3.5절](LEFT_JOBS.webpj.old.md#archive-webpj-20261126-through-wb04-e1339)에 있다.
- **사용자 결정 완료(2026-11-25, 인계 정리 뒤 질문에 답변).**
  - 사제 포획/희생이 들어간 L3 대조(Tutorial4 단계 E1~I1·Tutorial5 단계 E~G)는 작업 지시의 건너뛸 시험으로 보고 건너뛴다. 사용자가 직접 확인한다(3.1절 4D-4b).
  - 7단계 '교전 N프레임 대조'는 플레이어가 공격하는 교전 체크포인트 일치(E1334~E1335)로 완료 표시를 유지한다. AI가 공격해 오는 장면은 '전투 장면 오라클'·'AI 행동 N프레임 대조'에 남는다.
  - 저장소 루트 `test-results/`(`560920813`에 함께 커밋된 실패 기록)는 지우고 `.gitignore`에 `/test-results/`를 더했다.
- **사용자 결정(2026-11-26).** 일반 프레임 예외는 상세 오류를 기록한 뒤 다음 프레임을 진행한다. E1338에서 구현하고 실제 미션의 한 프레임 실패 뒤 진행을 검증했다([근거](docs/webpj-frame-error-continue-20261126.md)). DestroyPatch 정상 복귀와 구별한다.
- **검사 환경 결정 처리(2026-11-26).**
  - E1349 해결: 사용자 작업 지시의 허용 미션에서 검사 helper가 원래 Start 직후 실제 Session AI를 활성화한다. 현재 SP·이름·동맹·기술을 보존하고 제품 CampaignTestPlay 제외 분류는 유지한다. 별도 제품 옵션 추가 없이 Chromium2/2로 검증했다. [절차](docs/webpj-ally-ai-test-control-20261126.md)
  - `webpj/extracted/` 무시 규칙 결정은 사용자 커밋 `6b9b11ccf`의 `/webpj/extracted/**` 추가로 해결됐다(목표 재개 시 확인).
- **원본 실행·사람 확인.** favicon 탭 그림(0.6절), 도움말·안내 마나 아이콘(E1336) 육안 대조, 주변음·효과음·음악 실제 청취, 한국어 사람 검수와 미션 본문 줄 나눔 실제 화면, 시작 화면 안정화(E1300·E1308)의 배포본 처음 방문. 예산 칸 원본 우클릭은 E1324에서 확인했다.

<a id="cpp-delta"></a>

### 3.6 cpppj 변경 재반영

대응표 검사에서 행 base 이후 변경을 확인한다. ① 웹 영향 없음, ② 웹 재반영, ③ 해당 소비 단계에서 처리로 판정한다. ②는 별도 단위로 반영하고 ③은 대상 단계에 남긴다. 판정 근거를 대응표와 단위 문서에 기록한다.
- **현재 재집계(2026-11-27 E1393):** 대응표1,160행/오류0·port275/session732·partial/todo0·기존 행 변경0·삭제0이다. 정리1381 당시 변경9행은 E1387~E1393에서 모두 반영/판정했다: GumpEvents/RawMenuControls(E1387), UberGump(E1388), ClientDiagnostics(E1390), RawAssetPostPop/RawCampaignSession(E1391), ClientMain/GameClock(E1392), RawCampaignSessionLogin(E1393). 신규10파일과 별도 cpp.in 생성템플릿도 개별로 판정했다. TutorialObjective3파일은 필수 기능으로 E1386~E1389 완료, 외부 글꼴 Native경계/Reset All은 플랫폼 적응, ManaPicture자료2파일은 E1336 반영이다. 일본어39키는13차에 남는다. 고정 기준선225553df/대상1,007단위/오라클은 올리지 않았다. [행·파일별 근거](docs/webpj-login-reconnect-sp-20261127.md)·[감사 JSON](docs/webpj-e1393-evidence/cpp-delta-judgments.json).
- **판정 끝:** cpppj 1322(갈무리11)·1325(Reset All)·1326(테스트 AI 진단)의 변경 아홉 행(E1241, base `63590635`·`710b9647`·`baf141eb`, [판정표](docs/webpj-cpppj-1322-1325-1326-judgment-20261123.md)), 사용자 추가 요구 1328~1330(E1289~E1291), 1339 주문/포획 교정(E1319, base `13e74579`), SoundMusicSelection base 정정(2026-11-22, `225553df`), 일본어(13차) 커밋 `5238ee833`(1334)·`87324232f`(1335)의 여섯 행(E1336: 마나 그림 별칭 `imana.8`~`11`·`17`→`Imana.4`~`7`·`9`와 CD 마나 25프레임 보충의 `HtmlTextParser`·`Markup`·`StyleTextMarkup`·`UberGump` 행은 ② 반영, `KoreanResources.cpp`(`.japanese` 번들 읽기)·`StyleTextBuffer.cpp`(가나·한자 줄바꿈)는 ③ 13차, [근거](docs/webpj-mana-picture-supplement-20261125.md)). 커밋·행별 경과는 [E1329 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261124-through-wb04-e1329)과 [E1336 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261125-through-wb04-e1336)에 있다.
- **2026-11-26 변경 판정 완료(E1347~E1348):** 공통 도움말·확장 AI 이름은 E1347, 나머지16행은①11·②5로 E1348에서 닫았다. 미션2bh 채널 해제와 Fort 읽기 오류 문맥도 실제 웹 경로에 반영했다. 당시 기존 행 변경 보고0이었으며 E1393에서 마감한 후속9행/신규10파일과 구별한다. [16행 판정표](docs/webpj-cpppj-lan-change-audit-20261126.md)
  - [x] 언어 무관 공통 변경 — E1347: 도움말 LOOK/HELP·빈 값 규칙의 기존 일치를 확인하고 첫 NUL 절단·확장 AI 기본 이름31BYTE 제한을② 반영했다. 한국어에도 적용된다. [근거](docs/webpj-cpppj-common-help-ai-name-20261126.md)
  - [x] LAN 변경16행 — E1348,①11·②5·③0. 미션에도 선택되는2bh removeChat의 선언·장부·수신기·현재 앱과 공통 Fort 물리 읽기 진단을 반영했다. 나머지는 일반 TCP/LAN/외부 시각 보정·진단 전용 경로로 판정하고16행base를 갱신했다. [근거](docs/webpj-cpppj-lan-change-audit-20261126.md)
- **대응표 밖10파일의 판정:** E1393의 파일별 표/JSON을 따른다. 기존7파일에 더해 TutorialObjective.cpp/.h/Catalog.inc3파일도 반영 완료했다. 다음 재기준선 때 등록하며 현재 이식 완료율에 혼합하지 않는다.

| 처리 단계 | 남은 판정·연결 |
| --- | --- |
| 4D-3·4D-6 | 아비어리·공용 Form writer 경계에서 확인하지 못한 칸(확신 낮음, E594·E1210). 이탈 State 정리(E1268)·Client 기본 수신기(E1274)·분리 Poll과 `pollClientConnectionHealth`(4D-7b)·RawClockSynchronization 플랫폼 적응(E1317)은 닫혔고 PacketDelays·RawPacketCopies·RawMonitorTime은 도달하지 않는다. |
| 4D-8 | SidPool 소진 회수·진단과 GameAuxState 새 전역 칸의 소비 여부(코드와 행은 있음). DestroyPatch 정상 복귀(E1321)·`rememberPlacement`(E1273)·기본 수신기(E1274)·나머지 Native 경계(E1193~E1221)는 끝났다. |
| 5-A~5-C | 없음. ‘Creating Islands.’ 안내(`creatingIslands`, `RawMissionServerPackets.cpp:60-63`의 `createArchipelago`)는 로컬 미션 Zone `0x82000004`(`local-mission-transport.js:189`)에 0x100 비트가 없어 도달하지 않는다(E1328). |
| 6~7 | 없음. 건설 ActualConfirm/ActualPlace·동맹 협상 입력·RawGunHit Rain override는 끝났고 건설 기록은 개발자 전용으로 제외했다(E1242). RawPathRuntime/RawPathData는 '사제·운반·경로 바인딩' 항목이 이어받는다. |
| 8~10 | 한국어/일본어 자원 우선순위·입력(13차와 한국어 사람 검수). TellSection·Frame Rate·Difficulty·NB10·스커미시 일반 시작은 끝났다(E1263 점검). |

커밋·함수별 이력은 [E801 보관본 3.6절](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801), [대응표](webpj/PORT_MAP.tsv), [Login 변경 판정](docs/webpj-session-login-commit-audit-20261118.md)에 있다. typed 공유 Regular·압축 수신은 E734·E735, 일반 Storage Squid 송신은 E594, 기술·Chat·연결 종료를 포함한 typed 기본 Client 수신기는 E767~E770에서 반영했고 Native 대여는 E1193~E1194에서 끝났다.

### 3.7 AGENTS 후순위 요구(4~15차)의 웹 처리

| 차수 | 요구와 남은 일 |
| --- | --- |
| 4 | E943에서 16:9 논리 화면을 최대 1920×1080까지 지원하고 창 크기 변경과 입력 좌표를 확인했다. E1366의 카메라·HUD 단계13/13과 E1372의 WebMCP 실제 입력·PNG·효과음/음악 WebM·다운로드를 완료했다. 와이드 전투의 종합 확인은 M5에서 이어간다. [WebMCP](docs/webpj-webmcp-20261126.md) |
| 5 | localStorage 설정·진행 저장과 오류 대응, Reset All을 구현했다. E1394·E1369·E1371에서 실제1-1·1-2·1-5 승리의 신규 완료값 저장·새로고침 유지·다음 미션 해제를 확인했다. |
| 6 | 메뉴·Tell·도움말·갈무리11, 모든 미션 대본(E1108~E1116)과 게임 안 메뉴·객체 메뉴·미션 화면·채팅 문구(E1118~E1250), 글자 품질([0.7절](#korean-font-change)), 5-C M2 한국어 오라클(E1323)까지 구현했다. 단위별 경과는 0.1 진행 요약의 '한국어 문구'·'한국어 글자' 행에 있다(미니맵 도움말은 원본이 번역하지 않음, E1246). 전체 미션 대본의 사람 검수·줄 나눔 확인이 남았다. |
| 7 | E862의 초기화·적재·프레임·이벤트와 E944의 화면 관찰·한국어 적재·오디오는 단계·상태·시각과 원본 예외를 console에 보고한다. E1338에서 사용자 결정에 따라 실제 미션 프레임 예외를 기록한 뒤 다음 프레임 진행을 구현·검증했다. 오류·시계 관련 Node13/13·실제 Chromium3/3 통과. 웹 7차의 남은 일은 없다(2026-11-26 정리). [계속 진행 정책](docs/webpj-frame-error-continue-20261126.md). [E862·E944](docs/webpj-browser-error-reporting-20261119.md) |
| 8·9 | 웹 범위 제외(AA/FSR, Edit 시나리오 편집). |
| 10 | 10.78 알려진 버그 중 웹 적용분을 cpppj 판정과 대조한다. NB04/29 간헐천 계산·callback, NB18 요청/수신 관문과 NB20 실제 Session 선택/취소는 선행했다. 실제 스커미시와 브라우저 조작 검증은 남았다. |
| 11 | 쉬움/보통/어려움(기본 보통; 쉬움은 적 AI 다리/생산 주기 4배와 충분한 SP에서 채취 유닛 절반(최소 1), 어려움은 주기 절반)의 계산·AiCreate 연결(E554·E555), Options·ESC 메뉴 선택·저장(E1096), 실제 스커미시 난이도별 건설 속도 확인과 옵트인 e2e(E1149, [난이도 진행](docs/webpj-skirmish-difficulty-20261121.md))를 마쳤다. 쉬움의 채취 유닛 절반은 실진행 편차로 구분되지 않아 설정 단위 Node 시험과 AI 생성 연결을 근거로 둔다(E1156, [빨리 감기](docs/webpj-fast-forward-clock-20261121.md)). E1418에서동일지도/자금충분조건의실제예약배율을정량확인했다. 실제채취생산량절반은계수1/1/7로아직미완료이며설정목표3/6/6과구분한다. |
| 12 | Campaign의 Skirmish와 AI 수·개별 난이도/동맹·금지 유닛 최대 5(Outpost 포함)·시작/취소 UI, 무작위 섬·간헐천. Outpost 목록 수명·분류, 규칙·설정 화면·저장·시작 섬 fort 묶음·섬 생성 부모·세션 월드 생성과 시작·어려움 AI 진행·금지 유닛 반영(E1101~E1107), AI 생산·공격과 사제 사망 패배 안내의 실제 시간 관찰(E1262), 미션 모드라 비미션 너겟 배치·보충이 도달하지 않고 간헐천은 무작위로 놓임(E1270)을 마쳤다. 승리 경로와 AI 다리 멈춤 시점의 원본 대조가 남았다([다음 작업 3](#next)). [규칙](docs/webpj-skirmish-setup-20261120.md) · [UI](docs/webpj-skirmish-ui-20261120.md) · [시작](docs/webpj-skirmish-start-20261120.md) · [관찰](docs/webpj-skirmish-watch-20261123.md) |
| 13 | **E1410/E1412 구현·자동 검증 완료.** 공용43일본어파일/번역표·튜토리얼39키·선택/저장/자동감지·실제Google Fonts CDN·영어대체/재시도·Credits카피라이트·일본어줄바꿈과31미션첫안내를 확인했다. 영어/한국어 회귀와Chrome/Edge/Firefox도 통과했다. 모국어 교정·후반 목표/승리·원본일본어픽셀동일성을 입증한 것은 아니다. [범위·근거](docs/webpj-japanese-browser-support-20261127.md). |
| 14 | 웹 자체 JS 사용으로 QuickJS 탑재 불필요. **2026-11-23 사용자 결정:** dotnetpj가 QuickJS로 제공할 모드 API를 webpj 이름 체계의 공통 계약으로 만들고 webpj가 기준 구현이 된다([모딩 API](#modding-api)). |
| 15 | Reset All 완료: E863 구현, E1251에서 게임 안 Tell 확인 창(옵션 최하단 → `Patch1101.ResetAll` → `Patch1101.ResetAllConfirmed`)으로 교체. 확정 뒤 현재 게임 소유 설정·진행 키만 지우고 다시 로딩하며 영어·한국어 Chromium에서 확인했다. 유지 규칙: 영구 저장 키가 추가되면 초기화 목록도 갱신하고, 일본어 본문은 13차에서 더한다. [E863](docs/webpj-reset-all-20261119.md) · [E1251](docs/webpj-reset-all-tell-20261123.md) |

- **12차 Outpost 잔여.** 배치/거리 후보 목록의 실제 소유 관계 확인, 지역 소유 통지·Factory/연구/운반·AI·전투·Client 연결, 건설·금지 목록·삭제를 확인한다. 일반 캠페인/데모에서의 제외도 확인한다. [Outpost 부품](docs/webpj-raw-outpost-lifecycle-20261115.md) · [이관 전 범위](LEFT_JOBS.webpj.old.md#archive-webpj-20261118-through-wb04-e801)

<a id="modding-api"></a>

- **14차 모딩 API 기준 구현(2026-11-23 사용자 결정, 계약 설계 뒤 착수).** dotnetpj가 QuickJS로 제공할 모드 API는 webpj의 함수·클래스 이름 체계를 따르는 공통 계약이며 webpj가 기준 구현이다([DN 3.1](LEFT_JOBS.dotnetpj.md#dotnet-modding)). 계약 설계(DN MOD-1) 뒤 webpj도 그에 맞게 수정할 수 있다. cpppj 모딩은 아직 진행하지 않고, 게임 로직을 JS로 옮겨 dotnetpj가 webpj 코드를 재사용하는 안은 채택하지 않았다.
  - [ ] 공개 계약 층 — `Raw*`·원문 주소식 내부 모듈을 그대로 노출하지 않고 계약 v0의 이벤트·읽기 전용 조회·명령(원본 입력 큐 경유)을 감싸는 얇은 층을 둔다. 완료 기준: 계약 문서의 모든 항목이 webpj에서 동작하고 내부 모듈 직접 노출이 없다.
  - [ ] 이름 정렬 — 계약에서 정한 이름과 다른 webpj 클래스·함수는 계약 이름에 맞춘다(내부 이름 변경이 크면 단위로 나눈다). 완료 기준: 계약 이름 목록 대조 시험 통과.
  - [ ] 모드 로드 — 브라우저에서 모드 스크립트를 불러오는 방식(파일 선택·localStorage 보관, 5차 대응)과 기본 꺼짐. 외부 URL에서 불러오는 기능을 두면 외부 접속 문서화 규칙을 따른다. 완료 기준: 켜기·끄기·오류 격리 e2e 통과.
  - [ ] 공통 적합성 시험 — 같은 모드 스크립트를 webpj와 dotnetpj에서 돌려 결과를 비교하는 시험 묶음. 완료 기준: webpj 쪽 시험 통과와 DN MOD-4 대조 결과 일치.
  - 근거(2026-11-23 검토): QuickJS가 webpj ES 모듈을 고치지 않고 실행한다. QuickJS는 V8 JIT보다 약 41배 느려 모드 API는 이벤트·명령 단위로 한다. Math 함수 double 결과의 엔진 차이는 DN이 fdlibm 구현으로 맞춘다.

### 3.8 원본 관측 연결 이월

완료한 체크: **입력·장치**(E1269, 화면 위치 키·D 키·객체 명령·회전 키·Pause 키 분배기와 `raw-priest-placement-*` Session 설치), **렌더러·음악**(E1263, `renderer-projection-x86.tsv`·`scenemusic-x86.tsv` 소비), **채팅 허브**(E1169~E1175, `chat-callbacks`·`chat-input-recipients` 실제 연결), **점 명령/HTML**(E1192, HTML `$Input=`은 웹 범위 밖). 본문과 근거 링크 원문은 [E1336 보관본 3.8절](LEFT_JOBS.webpj.old.md#archive-webpj-20261125-through-wb04-e1336)에 있다.

[원본 fixture 감사](docs/webpj-fixture-audit-20261115.md)

## 4. 남은 차이와 검증 한계

| 영역 | 후속 확인 |
| --- | --- |
| 지도·지형 | 중립색·가운데 카메라·고정 구름은 개발 미리보기 정책이다. 실제 지도에서 owner·원소 투표/themeMode·도장/flood·카메라 제한/스크롤·공유 난수·색 번호·팔레트/ColorSort와 `useBanners`를 확인한다. 절벽 외 0x10/0x18 지형 선택 및 회전2/3의 실제 저장 사례도 남아 있다. |
| edgeFarm·보상/SP | edgeFarm의 spots/finder·MayPlace·Retype를 실 월드와 맞추고, 보상/SP의 재시드·실제 지갑·알림/소리·보상/후처리 장부 공유는 4D-8·6단계에 연결한다. [지형 추가 관측](docs/webpj-terrain-extras-20261102.md) · [보상과 Content 수명](docs/webpj-content-lifetime-20261107.md) |
| 메뉴·입력 | 메인 메뉴 구름은 Kencloud 캐시로 cpppj 와 같다(E1298, M1 구름 차이 0). Help/Options 위치·와이드 배치, Overlay 드래그/캡처·미션 커서는 미확인이다. Edge Scroll(가장자리 스크롤)과 Auto-Demo(메인 메뉴 45초 무활동)의 실제 동작은 E990~E1001·E1092에서 확인했다([가장자리](docs/webpj-app-edge-scroll-frame-20261120.md) · [데모](docs/webpj-demo-start-20261120.md)). |
| 도움말·Tell | `<info>` 머리글/능력치, `#`/`&` 맞춤·색 치환, mana.8~11의 실제 원본 F1 모습(cpppj 1335처럼 현행 마나 그림으로 바꿔 그린다, E1336 반영·육안 확인은 사용자), `<background>`/nsisles 사용, `{@미션.키}`·스크립트 도움말 Back 및 필요 시 두 열 메뉴를 확인한다. 도움말 그림은 레이어0/그림자 없음 경계를 유지한다. |
| 소리·환경 | 실제 청취·위치 음향, 다양한 브라우저·GUI Linux·고DPI/모니터 이동은 미확인이다. |
| Credits | 영어 AI 문구 `OpenAI Codex, ChatGPT and Anthropic Claude Code`와 한국어 문구는 사용자 수정 지시까지 유지한다. 수정 시 `www/assets/product.js`와 `tests/product.test.mjs`를 함께 갱신한다. |

## 5. 검증과 완료 판정

| 단계 | 판정 |
| --- | --- |
| L1 모듈 | 원본 10.78 x86 fixture와 cpppj 검사로 반환값·슬롯·정수/float/double 비트·부작용·난수 순서를 대조한다. 체크섬 비교와 바이트 비교를 구분한다. |
| L2 시작 상태 | 같은 Fort·언어·옵션·시드 및 Load/Start 시점의 Session 덤프와 논리 화면을 비교한다. |
| L3 입력 진행 | 같은 프레임·입력 단계표의 상태·보고서·화면을 비교한다. 표시 주사율·비동기 자산 로딩과 게임 시계/난수를 분리한다. |
| L4 화면·소리 | L4a는 논리 화면과 cpppj snapshot, L4b는 소리 시점·곡 선택과 MCP 녹화를 비교한다. 전체 외관과 청취는 사용자 확인 범위다. |
| L5 자연 플레이 | 사용자가 자연 승패 → 새 완료값 저장 → 새로고침/재실행 → 다음 미션 해제를 확인한다. |

실제 브라우저·OS·논리/물리 해상도·DPI·음향/저장 환경과 검사 범위를 기록한다. 원본 fixture를 덮어쓰지 않고 미연결 장치, 웹 적응과 누락 fixture를 단위 문서에 남긴다.

## 6. 작업 명령과 참고

```powershell
npm --prefix webpj start
npm --prefix webpj test
node webpj/tools/port-map.mjs
node webpj/tools/fixture-manifest.mjs
node webpj/tools/headless.mjs --scenarios webpj/tests/fixtures/cpppj/m1-scenarios.json --out <폴더>
node webpj/tools/screen-compare.mjs --diff <폴더>
npm --prefix webpj run check-assets
npm --prefix webpj run build
npm --prefix webpj run build:dev
npm --prefix webpj run check-bundle
npm --prefix webpj run test:e2e
```

- 개발 서버는 `http://localhost:9803/`이다. `?map=savetheisland&mute&lang=ko&aspect=4:3`는 규칙 없는 개발용 미리보기다.
- 필요한 항목만 `node webpj/tools/fixture-manifest.mjs --write --only <아이디,아이디>`로 갱신한다. 자산 변경 시 `npm --prefix webpj run prepare-assets`를 쓴다.
- Node v22는 HJOW-Athlon의 webpj 폴더에서 `& "C:\Program Files\nodejs22\node.exe" --test "tests/*.test.mjs"`로 순차 실행한다.
- [이식 대응표 사용법](docs/webpj-port-map-20261106.md) · [실행 자산](docs/runtime-resources.md) · [형식 목록](docs/formats/README.md) · [공유 명칭](assets/unit-names.korean.json) · [cpppj 소스 지도](cpppj/SOURCE_MAP.md) · [외부 접속 정책](docs/webpj-external-access-20261105.md)

## 7. 정리 기록

**2026-11-27 정리1381.** [정리/검증 근거](docs/handoff-cleanup-1381-20261127.md). [E1380 전문](LEFT_JOBS.webpj.old.md#archive-webpj-20261127-through-wb04-e1380)을 바이트/SHA-256과 함께 보관했다. E1340~E1380 단위별 완료 행·E951 이후 긴 요약 표·완료 Tutorial 세부는 보관본으로 옮겼다. 캠페인16개 진입/대기와1-1·1-2·1-5 승리 완료를 현행 다음 작업에서 정리하고, 건설 픽셀의 원인 미확정 문구·M4 검사 범위·최신 CP 제품/변경9행·신규10파일을 바로잡았다. WB-TUT-1~4를 새 필수 작업으로 나눴다. WB 제품/목표 일시 중지·기존 기준선/사용자 확인은 유지한다.

**2026-11-26.** E1339(사용자 일시 중지 인계) 직후 문서 전체를 [보관본](LEFT_JOBS.webpj.old.md#archive-webpj-20261126-through-wb04-e1339)에 이관했다(AGENTS.md `1fc63cb1` 정리 규칙).
- **줄인 것(보관본에 원문):** 0.1 최근·이전 완료 세 줄(E1336·E1338·E1339 → E1337~E1339 한 줄)과 다음 작업 1의 현재 위치 문장, 4D-4b Tutorial4 범위 정리·Tutorial5 A1 완료 줄의 검사 수치, 3.5절 2026-11-24 결정 네 개, 3.7절 6·11·15차 행의 완료 경과, 로컬 산출물의 루트 `test-results/` 경과, 지난 7절 기록.
- **바로잡은 것:** 0.2·3.6절 'cpppj 변경 행 0'·'변경 감지, 후속 단위' → 16행(②/③ 판정 필요 13·① 3, `node webpj/tools/port-map.mjs`), 3.4절 최신 cpppj 1339 → 1370, 번들 E1336 3,445,839 → E1338 3,446,031바이트, 0.1 미검증 'Tutorial5 단계 A~D' → A2~C(D→E는 사용자 확인), 다음 작업 2의 'Tutorial4~6 … 완료 창까지'에 Tutorial4·5 완료 창은 사용자 확인 범위임을 명시, 4D-7의 '프레임 예외 때 렌더 루프 정지 설계' → E1338 다음 프레임 진행, 0.2 표 10단계의 남은 차수에서 7차 제외(3.7절 7차는 E1338 뒤 남은 일 없음), 0.1 L3에 E1336 뒤 알려진 차이(E1337 `a220` 8,656픽셀) 추가.
- **새로 넣은 것·나눈 것:** 3.6절 판정 남음의 세부 둘, 다음 작업 6(3.6절 판정), 다음 작업 1의 세 캠페인 묶음 세부 둘, 3.5절 결정 대기 둘(아군 AI 시험 방식·`webpj/extracted/`), 진행 요약 표의 E1338 행. `[x]`로 바꾼 체크는 없다. 재집계(partial 0·todo 0, `port` 275·`session` 732, manifest 1,086, 단계 머리 수)는 문서 값과 같았다. 형식 밖 커밋 `e6ff58d0b`(사용자, `TODO.md`의 cpppj 멀티플레이 봇 요구)는 webpj 영향이 없다.

이전 정리(2026-11-25 E1336 이관과 같은 날 사용자 결정 반영, 2026-11-24 E1329·E1327 이관, 2026-11-23 E1288·E1237 이관과 그 앞)의 기록은 보관본의 날짜별 원문 7절에서 확인한다. AGENTS.md 기준은 `faecb9cb`(15차 설정 초기화), `2c8ca3bf`(갈무리11·다른 언어 글꼴), `d047bf9a`(cpppj·dotnetpj 실행 파일 아이콘, 웹 해당 없음), `11b16b77`(webpj favicon, [0.6절](#favicon)), `1fc63cb1`(LEFT_JOBS 정리 규칙 — 이번에도 적용)까지 반영했고 그 뒤 AGENTS.md 변경은 없다. `96d81381`은 Noto Sans KR 글꼴 파일과 AGENTS.md가 가리키는 `fonts/webfonts.css`(Google Fonts CDN 예시)를 더한 것이며 한국어 게임 글꼴 규칙은 갈무리11 그대로이다(3.7절 13차).

**E1342 이후 표시 검증 범위(2026-11-26).** 부분 invalidate가 전체 갱신으로 바뀌던 앱 연결을 교정하고 Tutorial5 C 화면5곳·덤프2곳, 4:3 반복 결정론·16:9/16:10 시작 상태를 Chromium5/5로 재검증했다. 기존 Tutorial1~3 완주 L3는 공격·포획·희생·승리가 포함돼 현재 사용자 제한에 따라 재실행하지 않았으며 당시 코드의 결과로 유지한다. 운영 번들은 E1342의3,446,009바이트다.
