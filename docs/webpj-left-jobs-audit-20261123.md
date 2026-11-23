# webpj 미완료 항목 전수 점검 (WB04-E1263, 2026-11-23)

사용자 요청("문서 반영할 때 LEFT_JOBS.webpj.md 에서 완료 표시 안된 항목들 다시 점검해 줘")에 따라 `LEFT_JOBS.webpj.md` 3.1·3.8절의 `- [ ]` 항목 45개와 0.6절의 favicon 항목 1개(모두 46개), 그리고 3.6·3.7절의 낡은 "남음" 문장을 코드·시험·단위 문서와 다시 대조했다. 코드는 바꾸지 않았고 문서와 대응표 한 행(GameWorld)만 고쳤다.

## 방법

- 항목을 다섯 묶음(4D-1~5 / 4D-5c~7 / 4D-8~10·3.8·3.6 / 5-A~5-C / 6~8단계)으로 나눠 읽기 전용 점검 담당 다섯이 병렬로 대조했다. 담당마다 "남음" 문장을 절 단위로 쪼개 **DONE**(해결됨, 코드·시험·문서 근거) / **UNREACHABLE**(단독 미션에서 도달하지 않거나 웹 범위 밖) / **OPEN**(정말 남음) / **NEEDS**(원본 실행이나 사용자 확인 필요)로 분류했다.
- 게임·Playwright·Node 시험은 실행하지 않았다(코드와 문서만 읽음).
- 점검 결과 중 닫는 근거가 되는 두 주장은 직접 확인했다: `cleanupClientStateWorld` 는 정의 파일 밖에서 호출되지 않고, `makeClientCampaignTransport`·`installClientPacketDefaults` 는 `www` 의 다른 모듈이 import 하지 않는다. `RawCampaignSessionLogin.inc:325` 의 `BeginGeneral` 가지는 `CampaignInstallMode::GeneralBattle`(일반 LAN)이다. 4D-6c 줄 구간의 커서·기술 바인딩은 웹에 같은 이름으로 존재한다(`raw-cursor-mana.js`·`raw-campaign-session-cursor.js` 등).
- 닫은 항목의 남은 일은 "점검" 줄에 근거를 적었고, 다른 열린 항목이 맡는 잔여는 그 항목으로 옮겼다. 확신이 낮은 판정은 문서에 그렇게 적었다.

## 결과 한눈에

| 단계 | 점검한 항목 | 닫음(`[x]`) | 문장 재작성 후 열어 둠 | 그대로 |
| --- | --- | --- | --- | --- |
| 4-D | 24 | 15 (4D-3a·3d·3f·5a·5c·6a·6b·6c·7a·7b·7c·7d·8g·9a·10a) | 7 (3b·3c·3e·4a·4b·5b·8a) | 2 (1c·10b) |
| 5-A | 2 | 0 | 1 (ClientRendererFrame) | 1 (E197·E198·E201) |
| 5-B | 1 | 0 | 1 (Cursor·RawCursor) | 0 |
| 5-C | 4 | 1 (미션 진입 명령) | 2 (메뉴·버튼 컨트롤, 허브 fixture) | 1 (M2/L4a) |
| 6 | 6 | 1 (입력 본체) | 5 | 0 |
| 7 | 3 | 0 | 2 | 1 |
| 8 | 3 | 2 (대본·대화창, 적응 fixture) | 0 | 1 (M4) |
| 3.8 | 2 | 1 (렌더러·음악) | 1 (입력·장치) | 0 |
| 0.6 | 1 (favicon 탭 시각 확인) | 0 | 0 | 1 |
| **합계** | **46** | **20** | **19** | **7** |

단계 머리글 수는 4-D 61→76/85, 5-C 2→3/6, 6 2→3/8, 8 6→8/9 로 고쳤다(5-A 44/46·5-B 12/13·7 3/6 은 그대로).

대응표는 `GameWorld` 행을 partial→port 로 올렸다(남은 갭 문장이 E898~E907·E1230~E1234 로 모두 해소). partial 은 21→**20행**, 줄 수는 16,571→**15,414**, port 986→**987**이다(`python -X utf8 tools/port_inventory.py` 재집계, `node webpj/tools/port-map.mjs --check` 통과, `tests/port-map.test.mjs`·`fixture-manifest.test.mjs` 10건 통과).

## 닫은 항목의 근거

| 항목 | 근거 요약 |
| --- | --- |
| 4D-3a | 기존 World 교체=미션 이탈 후 재시작(E1085), 화면·분리 타이머 E851~E989, 고정 수신기 E1193~E1194, 31h 개발자 명령 송신원 없음(E1242), `BeginGeneral` 계열은 LAN 설치 모드 |
| 4D-3d | Squid·Dais·Gun·Flyer·Aviary 수신 등록과 시험 모두 있음 |
| 4D-3f | 앱 프레임 e2e + `english-tutorial2.spec.js` 의 워크샾 생산→Archer 배치 |
| 4D-5a | `Session.dispose` 와 Leave/Restart·31개 시작·장시간 e2e, Native 저장소 없음 |
| 4D-5c | 전투 종료 프레임은 `missionPost` 관문(E1238), 4209..4279 함수 전수 분류, 패배 안내 E1262 |
| 4D-6a·6b·6c | 화면·타이머·입력·서비스 E851~E998, 날씨 `tint` 도달 불가, 건설 기록은 개발자 전용 |
| 4D-7a·7b·7c·7d | 장기 Session E878~E883, 메인 루프 E976·E1230~E1234, MetaMenuType 소비자가 `exclude-mp`, 렌더러 전역 앱 사용 E979~E989 |
| 4D-8g·9a·10a | `UsesNativeTransmissionDevices` 쓰는 행 전부 port/exclude, `sessionDump()`(E1129)와 오라클(E868~E869), 소유자 연결 확인 |
| 미션 진입 명령 | 환영 창→`MissionBegin`→`_beginCampaignFromDialog`, 31개·데모 e2e, 로딩 창 취소 |
| 입력 본체 | ESC 메뉴막대(E1084)·브라우저 이벤트(E919~E934)·진단 로그 장치 연결 |
| 대본·대화창 | 관련 행 전부 port, 대본 명령 전수 대조(E1190)·소비자 감사(E1231) |
| 적응 fixture·렌더러·음악 | `renderer-projection`(90행)·`scenemusic`(3,113행)은 소비 시험이 있고 GDI 글꼴 열거·`audio-reinterpret` 는 웹 적응·제외 |

## 열어 둔 항목에서 새로 드러난 실제 남은 일

1. **미션 중 열린 Tell 을 ESC 가 닫지 않는다**(코드 대조로 확인, 다음 작업 1 ⑩). 원문 `UberGump.cpp:1810~1823` 은 Dialog 단계 ESC 에서 `Tell Blank` 를 보낸다.
2. **Client 기본 수신기 표가 앱에 설치되지 않는다**(4D-3b). 앱 Session 은 내부 `LocalMissionTransport` 만 쓴다. 연결할지 웹 대체로 닫을지 결정이 필요하다.
3. **이탈 효과 4가지 미확인**(4D-5b): `MapRevision+1`·`ChatChannels.resetOccupied`·선택 해제 통지·시간 동기화 초기화. State 정리 부모를 앱이 부르지 않기 때문이다.
4. **`raw-priest-placement-*` 6개 미연결**(3.8절): 건물 파괴로 사제를 해방하는 경로의 `mayPlace` 가 도달 시 던질 수 있다(도달성 미판정).
5. **메인 메뉴 Kencloud 구름 배경**(5-A): 정적 GIF 타일을 쓰는 M1 차이 37,054픽셀.
6. **Session 큐 명령 전수와 입력 루프 허용 목록 대조**(4D-3e), **프레임 순서 차이 두 건**(4D-4a), **DestroyPatch 진단 12지점·`rememberPlacement`**(4D-8a), **채팅 허브 fixture 두 TSV**(5-C), **E197·E198·E201 재대조**(5-A·7 중복), **AI·전투 N프레임 L3·고정 시각 모드**(4D-10b·M2/M3).
7. **사용자 결정 필요**: 채팅 로그를 미션 끝에 초기화하는 적응을 인정할지(5-C 메뉴·버튼 컨트롤).

대응표 partial 20행 중 메모가 낡은 행(ClientRendererFrame·RawButtonControls·UberGump·RawMenuChat·ClientCampaignTransport·ClientState 네 행·CampaignObjectMenuClient·UberReconnect/ClientMain 문구 불일치)은 다음 작업 6 에 정리 후보로 적었다.

## 정정한 낡은 문장

- 3.1 4D-3a "E950 선행" 문단의 "기존 World 교체는 남았다".
- 4D-6a 의 "기존 World 교체, 로그인 완료 후 화면 설치…다른 UI 색상·날씨".
- 3.6 표 다섯 행(이미 port 로 끝난 Native 행들과 도달하지 않는 LAN·실소켓 항목을 남았다고 적은 칸).
- 3.7절 12차(스커미시) — E1262 관찰 반영, 7차 — 프레임 예외 결정 포함.
- 0.1 회귀 주기: Node 26 전체 6,792건 통과(E1262), v26 1/4·v22 3/8.
