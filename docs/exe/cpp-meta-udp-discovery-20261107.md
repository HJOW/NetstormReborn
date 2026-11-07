# 1058 — 실제 UDP 검색과 메타 상태 연결

원문 검색 부품을 실제 UDP12와 같은 Client 전역에 연결했다. **실행 제품은1058 / 11.01**이며 메타34·35·36·38과 `66h` 수신의 실제 상태 실행이 연결됐다. **State37의 새 로컬 서버 실행 대상과 Root/CS 관리자는 아직 미복원이다.** 검색 만료 후37을 선택하면 이 미복원 부모에서 명시 오류를 보고한다. 전체 로컬 검색/서버 또는 LAN 플레이를 완료했다고 기록하지 않는다. 직전 CP 커밋은 `6272726c / 20261107-82 CP`다.

## 같은 전역과 실행 부모

`ClientLocalStateGlobals`에005c85a8 토큰,005c8600 검색 기한,005c8608 재송신 기한을 각각 최초 BSS0으로 보관한다. `ClientNsInfoGlobals.rootServerBroadcastPort`는0054db54의 BSS0이며 실제 Interpret Options가 현재값에서 읽어 누락 시 보존한다. 검색은 Config 문자열을 다시 읽는 대신 이 현재 DWORD와 같은 `ClientRandomGlobals.Crt()`를 사용한다. 전송 owner 반납이나 메타 재진입이 이 네 전역을 새로 초기화하지 않는다.

State34는 안내·실제 UDP 열기·원문 토큰/첫 송신·두 기한·chalPal·즉시35를 실행한다.35는 기존 원문 부품으로 재송신과 검색 만료를 처리한다. 서버 창 조회는 `EnumWindows`와256BYTE `GetWindowTextA`/`Netstorm Root Server` 부분 일치다. Native00435a60의 기계어처럼 모든 창 열거를 계속하며 진행 플래그를 창의 존재로 대신 쓰지 않는다.

UDP와 TCP는 같은 연결표와 같은 패킷 등록표를 사용한다. `66h`는 observer 없음/version1/정책0/길이12이며 기존 원문 수신기를 등록했다. 명령1은 현재36/현재 sender의 실제 IP:port/current+0x30 포트를 쓴 뒤 **같은 `RawLocalMissionState.Run()`을 동기 실행**한다. 임의 다음 Tick 예약으로 바꾸지 않았다.36은 발견 안내→UDP12/IPX13 정리→현재 주소와 명시 포트로 TCP11 연결→다음26/+0x34=1 순서다. IPX 장치를 새로 만들지는 않는다.

38은 자기 hostname의 첫 실제 AF_INET 주소를 구한다. 실패하면 원문 안내/즉시20이며 성공이면 포트 없는 실제 자기 IP로 TCP11 연결/즉시26/+0x34=1이다. localhost/원격 공식 서버/성공 응답을 임의 대입하지 않는다. 자기 호스트 해석 이외의 도메인은 이 장치에서 해석하지 않는다. 기존 TCP의 loopback/RFC1918/link-local 정책을 그대로 사용한다. 상태 안내는 원문 형식 문구를 한 번 번역하고 고정 `%s`를TCP/IP로 바꾼 뒤 기존 HTML 글꼴 장치로 전달한다.

## 실제 데이터그램과 수명

| 원문 | 연결한 실제 동작 |
| --- | --- |
| 004e2bd0/004dfb20/004df2c0 | 명시 Open 때 UDP socket 생성·broadcast/reuse 옵션·wildcard bind/원문 재시도·새12 공개. 생성자만으로는 열지 않음. 새 열기 실패도현재12를0으로공개 |
| 004e00e0/004e1e90 | 같은 mutable 패킷의 observer1→현재 선언WORD 재조회→일시 nonblocking sendto→blocking 복구. TCP/집합 헤더를 추가하지 않으며 짧은 송신도 종료 통지 |
| 004dff00/004e1c00 | Registry의 가상 일반 송신은 주소 없는 실제send를 사용. 미연결UDP의 OS실패를 브로드캐스트 성공으로 바꾸지 않음 |
| 004dfc00/004e1d70 | select0→한 데이터그램/실제 송신자 주소→4096BYTE 물리와 선언 길이 일치 확인→검증→단일 observer0→기존 단일/집합 분배. 원문처럼4BYTE미만은sentinel이며4096이상/길이0/불일치는종료통지 |
| 004df9f0/004df440 | 같은 마지막 수신 version·한 번의2번통지/현재 sender와 주소. 닫힘/깊이0 뒤 실제close/free하며callback중에는 물리 노드 유지 |
| 004e1800 | UDP의 준비 조회는 원문처럼0. TCP 접속 완료나 UDP 검색 성공으로 간주하지 않음 |

처음 수신 물리는 원문처럼 전체ff다. 매번 수신이 덮은 BYTE만 남기며,2번 통지는 같은 수신 물리의 타입/길이만 바꾼다. 물리 반납은 같은 Registry 장치를 호출하고, retired 노드는 최외곽 대여가 끝난 뒤 수거한다. Registry가 외부에서 호출하는 close/free callback에도 대여 guard를 적용했다. 메타 owner의 pumping guard는 전체 Native cleanup을 막고,36의 UDP 닫기는 깊이 지연 반납을 유지한다. 이 메타에는 TCP10/11과UDP12만 있으므로 slot4는 같은 TCP flush·10/11 수신 뒤12 수신을 진행한다. 임의 전송 전체의 새 순회기로 확대하지 않았다.

폐기된7f/80 검증은 직접/집합 송신과 수신에서 계속 제외한다. Native의 선택적 패킷 기록기 송신/수신 writer는 실제 메타 소비자가 아직 연결하지 않았으며 이 잔여를 유지한다. 물리 포트0..65535 검증/WSA2.2 참조 수명/현대 OS 실패 전달은 호스트 장치 경계다.

## 빌드와 다음 작업

게임 Release/x64의 첫 빌드와 Registry callback 대여 보완 후 최종 빌드 모두 종료0·경고/오류0이다. 최종 EXE는12,322,816바이트, SHA-256 `9df977c31782c295b985faac3733a3947ae7c1d1979b5df07bb8cda874ba37b0`다. 이번에는 **새 단위/MP/게임/통신/원본R 실행0**이며 전체 검사/34미션/ZIP을 반복하지 않았다. 보호2800파일 불변·새fixture0이다. 원문과 PE 기계어를 대조했으며 실제 통신 검증이나 실행 안정성 증명으로 확대하지 않는다.

현재 호스트는 **HJOW-Athlon**이다. 마지막 클라이언트 양판본 전체 정밀 디컴파일은2026-11-03/HJOW-Athlon, R전용은2026-11-07/HJOW-Athlon이며 이번에는 다시 디컴파일하지 않았다. 로컬 정적 원시는 `extracted/cpp-resume-1058/`, 기계 판독 기록은 `cpppj/meta-udp-discovery-20261107.json`이다. 로컬 원시는 Git 제외이므로 다른 PC에 있다고 가정하지 않는다.

다음은 State37/새 R 서버의 실제 모드·Root/CS 관리자와004b9230의 수동 선택/재시도 UI다. 원본 R.exe를 실행해 새 구현을 대체하지 않는다. 이후17h/1dh 응답→State28~33/메타/나머지LAN·Outpost/checking5→한국어 후속→Edit 시나리오 UI 순서다. 전체 목표는 미완료이며 MP 검사는 계속 제외한다.
