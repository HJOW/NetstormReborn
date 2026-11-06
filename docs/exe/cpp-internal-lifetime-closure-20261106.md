# 비멀티 내부 수명·파일 범위 마감 — 단위980

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 이 문서는 LEFT_JOBS 3절4번을 마감할 때 사용할 유한한 구현 범위와 경계를 정리한다. **977~979의 제품·집중검사·실제 일반 종료를 확인하여4번의 활성10.78 비멀티 구현 범위를 마감한다.** 전체 원본 동일성/자연 완주·checking8/멀티플레이와8번 확장 요구는 완료한 것이 아니다. 이 단위는 문서/정적 판정이며 제품979/11.01을 유지한다.

이 조사에서는 원본·디컴파일 산출물·기존 제품 소스를 수정하지 않았다. 근거는 현재 코드와 기존 단위 기록, 읽기 전용 정적 조사 `extracted/cpp-resume-977/audit-later/priority4-closure.md`,978·979의 별도 구현안이다. 이 문서를 작성하면서 빌드·원본 CPU 실행·실제 게임·통신을 새로 실행한 것으로 합산하지 않는다.

## 1. 마감 판정의 대상

판정 대상은 현재10.78 제품의 활성 비멀티 caller에서 확인된 내부 수명·현재값 조회·원문 진단 정상 복귀·앞선 부분 효과·실제 Client/World/Session 연결이다. 모든 자연 플레이·모든 불법 주소·간접 alias의 부재·전체 픽셀 동일성을 한 번에 증명하는 작업으로 확장하지 않는다.

기존 포괄적 표현인 “전체 Factory/모든 가상 callee/다른 FILE/모든 실패 수명”은 아래의 구체 집합으로 나눈다. 현재 활성 미연결이나 실제 차이를 찾으면 해당 주소·caller·입력·차이를 수정 항목으로 올린다. 이미 연결한 부모를 과거 문서의 당시 미완료 문장만으로 다시 미구현으로 세지 않는다. 반대로 같은 오프셋·타입 번호라는 이유로 미지원 가상 함수를 base 성공으로 바꾸지 않는다.

정적 조사에서 확인한 구체 공백인 **978 raw 외부 Kernel 수명·현재 회수 대여와979 원문 new_handler 등록·종료 부모**는 구현·집중 대조를 마쳤다. 979는 실제 tutorial1 비정지 대기/활성 미션 WM_CLOSE 종료도 확인했다. FILE/현재 팔레트·Chat·State·Factory의 아래 유한한 집합에서 새 활성 비멀티 미연결은 발견하지 못했으며, 이 범위로4번을 마감한다.

## 2. 957~979의 함수별 범위와 근거

957~976의 완료 여부와 검사 수는 각각의 기존 기록에 귀속한다. 서로 다른 제품의 집중 통과 수를 현 제품 전체 회귀 합계로 더하지 않는다.977~979는 각 단위 보고서의 실제 제품·집중 결과로 확정했다.28/26/20검사는 서로 다른 제품의 고유 집합이므로 현재 전체 회귀로 합산하지 않는다.

| 단위 | 유한한 함수·소비 범위 | 근거와 판정 경계 |
| --- | --- | --- |
| 957 | Pop004b02d0의 현재 비멀티 입구·checking·TYPE/SHP·부분 효과·현재83VT | [현재 연결 감사](cpp-pop-current-scope-20261106.md).81표의 현재 코드/표시 연결과 메타맵2표를 구별한다. 전체checking8의 완료가 아니다. |
| 958 | 정렬 Content 부착004aec10의 현재 진단·링크·늦은 Pop·부모 통지 | [정렬 부착](cpp-content-attach-current-20261106.md). 실제 ContentFormHost/Session의 같은 TYPE·번호·선택 VT에 연결한다. |
| 959 | ProcessForm +74/004b1440의 접두와 현재 공통 부착 | [Process 접두](cpp-process-attach-current-20261106.md). 서버 writer/송신은 원문 코드·컴파일 범위이며 신규MP 검사를 하지 않았다. |
| 960 | 상위0041bd20의 Host/필드→Kernel→Create/Take→VT+74→OnAttached | [상위 부착](cpp-baseprocess-attach-current-20261106.md). 생성 실패의 등록/회전/필드 부분 효과를 보존하고 임의 rollback을 제거했다. |
| 961 | 공통 Owner004adf00의 현재 TYPE/모드·진단·늦은 목록/쓰기 | [Owner 계약](cpp-owner-current-20261106.md). 자산+74의 ret4와 Form+74의 ret8을 구별한다. |
| 962 | 생산 갱신 접두00454cb0·젬 고정 중립00462580·표면 소유자 | [파생 Owner](cpp-asset-owner-current-20261106.md). 실제 World/Session의 현재 VT 분배와 같은 장부/표면 전역을 사용한다. |
| 963 | 사제 Owner00491790의 진단 복귀·WORD/알림·현재 로딩/모드 | [사제 Owner](cpp-priest-owner-current-20261106.md). 포획/희생의 실제 게임 재현과 구별한다. |
| 964 | 신전004dc58f..004dc5f8의 missingIsland 상세→Client 진단 | [신전 상세](cpp-vortex-owner-diagnostic-20261106.md). 이전 Player 신전 해제와 상세의 부분 효과를 보존한다. |
| 965 | 0042cd10의 CD4BYTE 확인/7f·80 용도 분류 | [폐기 확인](cpp-retired-cd-validation-20261106.md). 서비스 종료 클라이언트 검증 비활성화 정책으로 일반 비멀티 FILE 잔여에서 제외한다. |
| 966 | 실제 options.cfg CRT 부분전달/종료·1176/1177 교체/복구 | [설정 저장 실패](cpp-options-save-failures-20261106.md). 명시 주입 상태와 실제 OS 고장 관측을 구별한다. |
| 967 | 0043c4e0/!color.dat의 현재 FILE·장기9표·표시 소비 | [현재 색표](cpp-colormap-current-file-20261106.md). 사용자 Cache와 같은 현재 표를 연결한다. |
| 968 | 색표 직접 소비·추가 Draw와 최초 palette00438dc0/004a4850 | [색표 소비/시작](cpp-colormap-consumers-startup-20261106.md). 두8행/마지막 표에 근거 없는 새 소비자를 만들지 않는다. |
| 969 | 실제 커서 취소00446e70의 현재 Form VT·공유 Factory | [커서 Form](cpp-cursor-form-factory-20261106.md). 별도 SquidForms의 사본 수명과 실제 현재 공장을 구별한다. |
| 970 | Gump/Content 삭제의 현재 클래스 분배와 혼합 종속 체인 | [Form 클래스](cpp-form-class-lifetime-20261106.md). 변경 가능한 TYPE를 현재 VT와 동일시하지 않는다. |
| 971 | Content Robust004aea60/Linked004ae080/Regular004af070·현재 재부착 | [Content 필드/Regular](cpp-content-field-current-20261106.md). Damage라는 API 이름을 실제 HP 공격 함수로 해석하지 않는다. |
| 972 | Process의 현재 VT·PID·부모/pre/post/Unpop·Kernel 반납 | [Process 클래스](cpp-process-class-current-20261106.md). 초기 NULL PID0과 실제 소진 기록 슬롯0을 구별한다. |
| 973 | Kernel.Remove004719e0의 현재 슬롯·Add00471950의 소진/슬롯0 | [Kernel 진단](cpp-kernel-diagnostic-current-20261106.md). 실제 요약/상세/단언을 연결하고 덮어쓴 소유권은 최종 호스트 반납과 구별한다. |
| 974 | CD004ed400 프로세스 요약·3999슬롯·실제 Client 상한 | [CD 요약](cpp-cd-process-summary-20261106.md). PATCH와 다른 이름/NULL/부모 캐시를 기록한 제한된10.72 비교다. |
| 975 | Create004af530/Take004af610/현재 생성004af720·직접firstPop004ad470·고정 Form | [Factory 함수표](cpp-factory-parent-scope-20261106.md). 공통 부모·실제 물리 범위와 외부 heap 주소를 반환하는 임의 생성자 계약을 구별한다. |
| 976 | PATCH SID Allocate004af1d0의 진단 복귀·현재 목록·최대50회수 | [PATCH 할당](cpp-sid-allocation-recovery-20261106.md). 긴 Pool이 현재 World의 짧은 실제 가상 삭제 대여를 사용한다. |
| 977, 완료 | CD SID004aae40의 dump/상세·고정 후보 회수·BridgesKilled | [CD 할당](cpp-cd-sid-allocation-recovery-20261106.md). CD 무효과 무한 루프는 명시 오류로 분리하며 CD 전체 객체 dump/trace는10.72 후순위다. |
| 978, 완료 | raw 외부 Kernel의 현재 회수 callable·자기 Host만 최종 반납 | [legacy 수명](cpp-legacy-world-lifetime-20261106.md). 외부/다른 Host·슬롯0/retired·다음 대여를 구별하고 shared Native Clear/Return 계약을 유지한다. |
| 979, 완료 | 시작00438dc0→new_handler435f50·종료004b3490·현재 event/음악 Stop | [메모리 실패](cpp-allocation-failure-20261106.md). 기존4b2df0을 재사용한다. Run 전 생성자/CLI·모든malloc·실제OS고갈의 전체 포괄은 주장하지 않는다. |

기존899/904/905의 원문 Create/공통 초기화/Take 부모,884~888의 내장 Form·process cdecl·자산 bare 분배,890/896/897의 현재83VT 감사는 이 표의 선행 근거다. 이번 마감 표가 해당 가상 몸체의 모든 입력을 새로 실행했다는 뜻은 아니다. 공중3종과 Outpost 전용 실행·새MP 검사는 계속 제외한다.

## 3. 직접 FILE74호출/23caller의 분류

785의 [직접 호출 장부](../../cpppj/nonmp-file-direct-callers-20261103.json)는7callee에 대한 텍스트 직접 호출74곳/23caller 그룹이다. 이를 현재 구현과 후속 기록에 대조한 유한한 분류는 **현재 연결54곳/19그룹 + MP·폐기 인터넷 정책18곳/2그룹 + 폐기CD1곳/1그룹 + 휴면Chat log1곳/1그룹 = 74곳/23그룹**이다. caller 그룹 수를 독립 제품 기능 수나 전체 PE 간접 호출 수로 해석하지 않는다.

| caller | 호출 수 | 현재 분류·근거 |
| --- | ---: | --- |
| 0041b4c0 | 1 | BaseFile 현재 열기/선택키,757·BaseFileOpenPolicy. |
| 0041bb90 | 1 | Editor r+b→w+b/쓰기/닫기,669·738·740·943. |
| 0042cd10 | 1 | 폐기 CD 확인7f/80,965. |
| 0042f590 | 1 | 최초0인 Chat logFile의 조건부 writer. 아래 휴면 경계. |
| 00438dc0 | 2 | 최초 palette968·SHP791. |
| 004395df | 1 | NativeShapeFile의 직접 OS mapping/view,791. |
| 0043c4e0 | 9 | 현재 !color.dat FILE/장기9표,967~968. |
| 00440f30 | 4 | Config 키·서명·본문 저장과 실제 CRT 실패/교체,966. |
| 00458bf0 | 2 | rootservers 저장/끝점.8번 또는 폐기 인터넷 정책. |
| 00469c80 | 1 | 현재 날씨 palette/scene,786·672~673 및 실제 LoadScenePalette. |
| 004a3090 | 11 | Font cache 순차 writer,788~789·933의 사용자 Cache 이관. |
| 004a34a0 | 5 | 같은 실제 FontStore 생성/캐시 caller,789. |
| 004a4850 | 1 | seek8/직접776 또는1024BYTE palette 부분 읽기,786. |
| 004ab3e0 | 2 | title State42 palette,782. |
| 004b54e0 | 16 | 일반 battle 재접속 !state 저장.955~956의 분류에 따라8번. |
| 004b6100 | 1 | 실제 ChangeFortPalette 전체 전환,아래4절. |
| 004b6dd0 | 1 | 단독 State7의 battlePal 현재 연결. 일반 전투/재접속 부모는8번. |
| 004c4c60 | 2 | Help 파서 현재 stream append,766. |
| 004c4cc0 | 1 | Help 정보/원문 stream,766. |
| 004c63e0 | 3 | QuickHelp HTML writer의 실제 현재 TYPE/HTML/파일,766. |
| 004d6020 | 1 | ObjectDump의 현재 공유 고리/실제 reader·writer/Client 진단,757·763·977의 본문 공유. |
| 004ddc90 | 4 | indexed BMP writer·실제 연속 수집·사용자 Captures,693~695 및 후속 이관. |
| 004dea00 | 3 | 실제 시작 Xlat 현재 FILE/버퍼/CR/닫기,787. |

785 당시 분류만 남았던 palette/Xlat/font/SHP/UberScript 후보는786/787/788~789/790~791/792의 후속 처리와 구별해 읽는다. 근거 문서는 [BaseFile 정책](cpp-basefile-open-policy-20261103.md),[palette FILE](cpp-palette-live-read-20261103.md),[Xlat FILE](cpp-xlat-live-read-20261103.md),[font FILE](cpp-font-cache-file-20261103.md),[font 실제 연결](cpp-font-cache-native-20261103.md),[SHP mapping](cpp-native-shape-file-20261103.md),[Script caller 감사](cpp-script-cache-callers-20261103.md)다.

`UberGump::Script`의10.78 직접 caller는 고정 UCampaign 조건 때문에 선택되지 않는다. CD/호스트의 필수 snapshot·성공 캐시를 일반 Config optional 부모로 임의 교체하지 않는다. 등록 snapshot·무진단 optional·OS 직접 ReadFileBytes·음악 스트림과 현재 FILE 부모의 서로 다른 계약도 보존한다.

Chat logFile0054874c는 최초0이며 현재 제품과 원문 정밀 C의 직접 비영 writer를 찾지 못했다. 보호 PE의 직접 주소 피연산자는42f590/42f5b7의 두 읽기였다. file0의 자연 반환과 명시 비영 시 fileValid/writeLog 필수 계약을 유지한다. 직접 writer 미발견은 모든 간접 alias의 부재 증명이 아니지만, 근거 없이 새 로그파일이나 자동 writer를 생성할 이유도 아니다.

## 4. 현재 팔레트·Chat·State/Session 연결

실제 ChangeFortPalette는 같은 ClientKencloudGlobals::Transition을 통해 이름 비교/현재 블록 반납/구름 Build·Step/현재 GamePalSpec/FILE 읽기/색표 재구성/기존 소비자 갱신을 호출한다. 현재 caller는 CampaignClientServices의 battlePal,RawCampaignSessionLogin의 State7 후단,UberEditor의 fortPal,UberHud의 배경 팔레트 방문이다. 최초 palette와 실제 현재9색표 소비는968/967이 후속 근거다. [Fort palette](cpp-fort-palette-transition-20261024.md) 당시의 하위 미연결 문장만으로 현재 root를 다시 만들지 않는다.

parallax 전용004a15d0/004a1650 잠금은 현재 명시 미복원이다. 선택 상위 RawRendererPrelude는 parallax16/fullscreen1/비DIB2에서만 해당 alternate 경계를 선택한다. 현대 Windows 출력은 전체화면에서도DIB2를 강제하고 창모드에서는 parallax를 끈다. 따라서 이 분기는 현재 GDI 호환 제품에서 선택되지 않는 원본 DirectDraw 전용 장치 경계로 분류한다. 원본 장치 자체를 재구현할 때 별도 범위로 올리며 현재 활성 FILE 공백으로 중복 계수하지 않는다.

일반 Chat은 실제 StyleTextBuffer의 onAppend,받기/설정/깨우기/음향/append/show/log와 현재 공통 UI에 연결된다.921은 숨긴 Chat 해상도 복원,922는 실제 Chat/Console의 모드/설정 조합을 대조했다. 시스템 뉴스의 setNewsTime/news hook은 실제 UI에 연결되지 않았지만 선택 조건이 subscriber0·header0x80인 시스템 패킷이며 초기화0042e4d0의 직접 caller는 State19 메타맵이다. **0042e470/0042e4d0/0042e4f0·현재 endpoint 이름·시스템 뉴스 UI/설정 저장은8번에 구체 이월한다.** 공식 인터넷 서버를 복구한다는 의미는 아니다. [Chat 모드/실제 UI](cpp-chat-helper-modes-ui-20261105.md)를 따른다.

[956 State 범위](cpp-mission-state-scope-20261106.md)는 cleanup34Effect를 현재 enum과 전수 대조하고 실제 Client/Session3세대 반납·재설치를 확인했다. ClientStateWorldCleanup은 현재 전송/정지/커서/시계/Player/AI/Deck/Kernel/HUD/Core/Pool/Graph/옵션·기한에 연결된다. saveFort는 같은 Session snapshot이며 saveBattle과 leaveMetamap은8번이다. MapMode의 무효과는 원문004437f0의 실제 단일RET이다.

Runtime 반납은 같은 Session/표면 세대/Player/Kernel과 여덟 AI·Player links·단일 Kernel 정상 반환·known0/Core0을 확인한 뒤 소유자를 반납한다. 원문 단일 Kernel.Clear와 C++ 최종 소유 필터를 구별하며 반납 뒤 이전 Session 주소를 읽지 않는다.978·979를 포함한 이번 판정 범위에서는 이 현재 집합 밖의 새 활성 비멀티 State/Session 수명 공백을 발견하지 못했다. 모든 장기 조합·자연 플레이의 증명은 아니다.

## 5. 구현 잔여와 구별하는 호스트 경계

- 원문32비트 외부 heap 주소를 임의 SID로 반환하는 생성자,미등록/범위 밖 코드 포인터,NULL 역참조,해제 포인터 재사용 자체는 지원하지 않는다. 실제 Create/Take/Retype와 현재 등록 생성자의 정상 범위를 이 경계로 축소하지도 않는다.
- MSVC32 FILE24BYTE/FileSpec264BYTE·스택 배치·미작성 heap/스택 비트·예외 payload·SEH unwind ABI 자체의 동등성은 현대64비트 소유권/OS 대체 경계다. 이를 guard 제거·배열 임의 확대·빈 성공으로 숨기지 않는다.
- 실제 OS 메모리 고갈,물리디스크 고장,모든 commit/close 실패,실제 UNC 서버·다른 드라이브 상대 CWD는 명시 입력/정적 연결과 별도의 환경 관측이다.966의1176/1177 명시 주입은 실제 OS에서 그 오류가 발생한 관측이 아니다.
- CD10.72의 전체 UI·dump/trace·Retype·전체 파생/OS 실행 복원은 별도 후순위다. 이번 PATCH10.78 범위 마감을 위해 그 전체를 새 필수 작업으로 늘리지 않는다. 이미 비교한 CD 차이는 각 문서에 남긴다.
- 전체 자연 완주·모든 화면/음향·프레임/행동 동일성,실제 포획/희생/특정 적 공격/승리는5·6번 사용자 확인이다. 분류 변경으로 기존 실패나 사용자 제보를 지우지 않는다.

## 6. checking8과8번의 구체 이월

집계는 **잔여8=부모3+부분2+MP분류3,완료부모39/부분4** 그대로다. 이 문서로8함수 전체를 완료 처리하거나 checking 수를 줄이지 않는다.

| 함수·분류 | 8번에 남기는 구체 범위 |
| --- | --- |
| 부모004a7510 | 참가자 재접속·팀 재편/실패 진단 등 일반 전투/원격 경로. |
| 부모004b02d0 | 메타맵 VT0050c9f0(TYPE108/109)/0050cc08(TYPE161)의 +88/관련 표시. 현재 비멀티81표와 구별한다. |
| 부모004dc220 | 신전 event4의fort territory0046f650/0046f5a0·curServer0054db4c·Descend0x02000013→004b3760/004b3620/State19 및 생성004dbac0/0x030000a7. |
| 부분004b54e0 | 일반 battle !state 저장·파일명/count 진단·0044fa00의 재접속 저장 소비자. |
| 부분004b8cb0 | StateReconnect/서버 이전·ForceServerDeath/자동 장애 caller. |
| MP분류0042bc10 | 참가자 삭제·관련 메타맵/연결 부모. |
| MP분류004b4df0/004b8b20 | meta connected/monitor의 현재State29/33 진단(0xdef/0xe39)·연결 상태 부모와 후자의 c.dat 경로. 신규MP 검사 없이 해당 원문 부모를8번에서 계속한다. |

[957 감사](cpp-pop-current-scope-20261106.md),[checking 함수별 장부](cpp-checking-consumer-gaps-20261103.md),[866 Descend 정정](cpp-temple-descend-scope-20261104.md)을 따른다. 직접 inSingle 관문이 없다는 사실을 모든 단독 도달불가의 증명으로 바꾸지 않는다. 시스템 뉴스와 위 FILE18곳도 같은8번 이월에 보존하며 신규MP 단위/게임/통신 검사는 하지 않는다.

## 7. Windows 호환성·환경 확인과 단계 전환

7번의 물리 입력 QWASZX/ESC/Home/포커스,다른 모니터/DPI·실제 앱 전환,사용자 활성 전체화면의 실제 출력,청취·실제 CD 삽입은 환경/사용자 확인이다.762의 새 부팅/RDP세션1·fresh/재선택 두 timeout 이후 환경 변화 전 재시도하지 않는 결정을 유지한다. 메시지 주입이나 MCP 앱 내부 입력은 실제 물리 입력 성공이 아니다.

**실제 PCM은936의 후속 성공으로 정정해야 한다.** [MCP 서버/936 소리 녹화](cpp-mcp-server-20261106.md)의 HJOW-Athlon HD Audio에서 영상23프레임/2.31초,프로세스 WASAPI audio.wav101,653프레임/2.305초·비무음99,726프레임과 ffmpeg 영상/소리 합성이 확인됐다.594/762의 이전 실패·미확인은 당시 이력으로 보존하되 “현재까지 실제 PCM 성공 없음”으로 쓰지 않는다. 이 성공을 청취·실제 CD·사용자 활성 전체화면 물리 출력·다른 오디오 장치의 성공으로 확대하지 않는다.

기존 전체화면 종료/재시작·화면비·가장자리 스크롤은 관련 변경 뒤 필요한 회귀로 유지하며 무기한 미완료 체크로 반복하지 않는다.979의 WM_DESTROY 연결은 실제 tutorial1 새진입/비정지100프레임대기/활성 미션 WM_CLOSE 종료0으로 확인했다.979에서Abort/Replay를 새로 실행했다고 합산하지 않는다. 실제 포획·희생·특정 공격·승리,전체34·긴 완주·전체 CTest/ZIP 반복은 이 확인에 포함하지 않는다.

유효한 사용자 결정은 “자연 완주·전체 동일성만 남으면 다음 단계”,“Windows10의 AI 작업 뒤 Windows11 실기 없이 다음 단계”다. 따라서4번의 구체 구현/검증을 마치고7번에 새 구현 결함이 없으며 환경/사용자 확인만 남은 경우4→7→8 순서로 진행할 수 있다. Windows11/ARM 실기·환경 미검증을 숨기거나 전체 호환성의 무오류 보증으로 표현하지 않는다. 전체 목표는8번의 실제 후속 구현을 마치기 전까지 계속 미완료다.

## 8. 최종 확정과 다음 작업

- 977: Release 경고/오류0·고유28검사·CD52입력/재관측. 커밋 `435cfa1d`.
- 978: 최초 검사 컴파일 C2248 교정 후 최종 Release 경고/오류0·고유26검사. 커밋 `122c8236`.
- 979: Release 경고/오류0·고유20검사와 실제 tutorial1 창 종료0·설치/보호 불변. 커밋 `9c30e204`.
- 보호2800개/fixture980개는 불변이다. 마지막 실제 GUI는979이며 현재 제품도979/11.01이다. 이 문서 정리에서 새 빌드·검사·게임·디컴파일을 실행한 것으로 기록하지 않는다.
- 다음7번에는 환경/사용자 확인과 별도로 **937의 와이드 덱80×25 검은 사각형**이라는 구체 표시 결함을 남긴다. 이는 사용자 보류인 연녹색 직사각형과 다른 문제이며 정적 원인 조사를 진행한다. 수정/판정 뒤8번으로 넘어간다.
- checking8·일반전투 FILE·시스템 뉴스는8번에 보존한다. 신규 MP 검사 없이 원문 구현을 이어간다. 원본과의 전체 동일성/자연 완주·10.72 전체·32비트SEH 자체·실제 OOM/장치 관측의 경계를 바꾸지 않는다.
