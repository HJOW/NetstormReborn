# 원본 LAN 범위 확정과 마감 검증 — 단위1380

2026-11-27, HJOW-Athlon. 사용자 지시 **“원본대로 진행 (전투 중 참가 기능 추가하지 않음)”**에 따라 D9를 해소했다. 진행 중 전투에 새 ID로 참가하는 기능은 추가하지 않는다. 원본의 기존 ID 재시작·저장 복구를 유지하며, 최신 제품1377에서 실제 TCP 경로를 다시 검증했다. 이번 단위는 문서와 검증 결과만 추가하며 제품 코드·원본·디컴파일은 변경하지 않았다. 마지막 디컴파일 호스트는 HJOW-Athlon이다.

원본은 진행 중인 게임의 로그인/재접속 요청에서 기존 Player의 subscriber를 찾는다. 새 ID 참가 경로를 새로 만드는 대신,1375에서 교정한 epoch 시계·로컬 SP 복원·Residence SavedGame 결선을 그대로 유지한다. [원본 근거와1375 교정](cpp-lan-restart-1375-20261127.md)·[봇 사용법](../testing/cpp-lan-opponent-guide.md).

실제 검증은 다음 명령으로 수행했다. 출력 경로는 이 실행에서 새로 만들었으며 두 참가자는 격리 사용자 폴더를 쓴다. 게임 상태·ACK·비용·저장 성공을 주입하지 않고 실제 메뉴와 커서 입력으로 진행했다.

```powershell
python -X utf8 tools/cpp_lan_opponent.py --exe cpppj/build/bin/Release/Netstorm.exe --game extracted/cpp-reset-all-1325/game --output extracted/cpp-scope-1380/original-restart --lobby-port 17150 --game-port 17149 --produce-golem --extra-build --restart-peer
```

| 확인 대상 | 결과 |
| --- | --- |
| 신전·골렘 | Wind 신전 완성 뒤 골렘 SID15258/좌표121,211/HP50이 양쪽에 일치. SP1500→1100, 실제 복구 저장976바이트/owner1/SP1100 |
| 추가 건설 | 썬 워크샾 SID15259/좌표124,215/HP3000과 abstract 해제, 단일 다리 SID15261/좌표140,209의 전체 행이 양쪽에 일치. 최종 SP300/저장300 |
| 기존 참가자 복구 | 상대 정상 종료→같은 사용자 폴더로 새 프로세스 실행→실제 복구 질문 Yes→로그인14/PlayerKnown. owner2/subscriber6356994/OpponentBot/flags67 복원 |
| 양쪽 상태 | 공유 객체7,305개·로컬 파생 표면51개 일치. SP는 owner1=300, owner2~8=6500. 호스트 권한1/1·복구 상대0/0 유지 |
| 종료 | host·bot·restarted 세 프로세스 모두 종료0 |

객체 대조 범위는1375와 같다. 로컬 player는 제외하고, 원본 후처리에서 로컬로 만드는 island/islandStalag/bridgeConnector는 SID를 제외한 보고 필드의 다중집합으로 대조한다. 다른 공유 객체의 SID·보고 필드는 유지하며 비동기 시각 프레임만 구별한다. 다리 프레임/수명과 로컬 표면 색/방향도 검사했다. 임의의 전체 메모리나 원본 전체 동일성을 확인한 결과로 확대하지 않는다.

현재 EXE의 SHA-256은 `6851dfabaa7f6b8eb98eb4c8b312b9374d3287ae4ee845552d6fd9d78dc0846b`이며 제품 소스 커밋aed29e3d1 이후 `cpppj/src` 변경이 없다. [최신 ZIP1378](cpp-final-package-1378-20261127.md)의 실행 파일 바이트와 일치한다. ZIP은206,351,884바이트/1,480항목이며 SHA-256 `d92b32495a068a0a0a428c0fbecafc091fec36cda688557a8c871b54c129645d`도 다시 일치했다. 제품 변경이 없어 재빌드·재포장은 하지 않았다.

기존 완료 근거도 대조했다. 튜토리얼1372는6미션/40단계/39키·3언어·집중22/22·GUI19경로 종료0이고, 로그1377은18/18·정상 GUI 종료0이다. ZIP1378은 CRC/원천 해시·글꼴/라이선스·중복0과 한국어 전체화면/일본어 창 GUI2회 종료0·실행 뒤 배포1,480파일 불변을 확인했다. 이 검사를1380에서 재실행한 것으로 세지 않는다. 이번 실제 TCP 종료 뒤에도 원본/CD·기존 fixture·AGENTS.md·README.md 보호2,810파일은 기준 해시와 모두 같았다.

현재 허용된 cpppj 구현·검증 범위는 완료했다. 명시 사용자 확인 검사와 허용 캠페인 밖의 포획/희생·특정 적 공격·승리 자연 진행, 실물 LAN/물리 입력/다른 모니터·DPI/Windows11·ARM/탐색기 아이콘 확인은 남긴다. CD10.72·QuickJS 추가 연동·비행체 보간·D8 글자 선명도 작업은 기존 보류를 유지한다. DN/WB 후속은 각 프로젝트 범위이며 CP 완료로 대신하지 않는다.

원시는 `extracted/cpp-scope-1380/original-restart`에 있다. [검증 JSON](../../cpppj/original-lan-scope-1380-20261127.json)에 실제 TCP 결과·현재 해시·기존 근거 구분·보호 대조·남은 범위를 기록했다.
