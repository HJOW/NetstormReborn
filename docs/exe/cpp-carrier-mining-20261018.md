# 골렘의 채취 통지와 간헐천 고갈 수명 — 2026-10-18

기준 커밋은 `cd6c03a`다. 직전 목표 턴은 제품 변경·독립 원본 관측·회귀·커밋을 완료한 진행 턴이다. AGENTS와 양 LEFT를 재독했고 현재 PC와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**으로 일치한다. 보호 원본 수정·새 디컴파일·원본 게임 창 실행·DirectDraw 수정은 없다. 이번 새 몸체는10.78 전용이다.

`RawGeyserMined`4266c0의 수량·난수·고갈 몸체는 이미 복원되어 있었으나 실제 Session에는 연결되지 않았다. `RawCarrierMining`은 실제429800 안의40ac20 헤더 작성→지면SID WORD→4e2e30 송신→4266c0 로컬 수신만 담당한다. 전체 도착의 조건과 너겟 생성은 기존 `RawCarrierArrival`을 그대로 실행한다. 원본처럼 프로토콜bit7을 지우고 현재 메시지/길이를 기록한다. 선언 길이를4 이상으로 보정하지 않는다. 송신 효과가 헤더 전역을 바꾸어도 이미 작성한 버퍼는 바뀌지 않으며, 버퍼 자체를 바꾸는 명시 효과는 로컬 수신에서도 보인다. 송신 전에 지면의free/dead를 임의로 검사하지 않는다.

같은 Session이 실제 현재 연결 표의 `RawBattleBroadcast`, 현재 난수, Pool/Hash/TYPE, 실제음향, 가상 삭제/생성/Pop을 연결한다. 로컬/외부 수신 직전 현재boss/checking/nuggetType을 기존수량 처리기에 공급한다. `RegisterCarrierCommands`는0x5b/길이6/version1/mismatch0/빈observer를 같은 등록표에 설치하며, 설치 전 등록은 거부한다. 실제 외부 수신은 같은 원문 handler로 들어간다. 실제 Client 명령 소비자·전체 전송 루프 완료를 주장하지 않는다.

실제 도착에 필요한443800 시작/도착 겹침 정리는 이미 있는 `RawConstructionEffects.Displace`를 공유한다. 현재 시각/동맹/서버 상태를 기존 `PrepareBuilding`으로 갱신하고 원본 인자·반환값 무시를 보존했다. 빈 콜백을 넣거나 새 직선 경로를 만들어 통과시키지 않았다. 기존 건설 효과의 내부 몸체는 수정하지 않았다.

고갈 검사에서 TYPE122의 고유 삭제가 미연결임을 발견했다. 보호 PE의 실제 간헐천VT는50da88이며+10=4af780/+14=485580/+18=485540/+78=4acfe0/+88=4ad500이다. 4acfe0의 실제 명령은 현재TYPE+128 기본프레임 DWORD를 반환한다. `RawGeyserLifetime`은 다음을 보존한다.

- 전처리는 가상기본프레임→raw+36 직접DWORD저장→표시+88 한 번이다. 일반SetFrame의 추가 표시/Unpop/Pop으로 바꾸지 않는다.
- 표시 효과 뒤 현재extra&9가 없을 때만416720(sid,0) AI제거→현재간헐천목록40ea00 중복전체제거를 실행한다. AI제거 효과가 바꾼 현재목록/count를 읽고 남은 물리 꼬리를 지우지 않는다. 이후44b4b0 Damageable로 내려간다.
- 후처리는 현재editor와extra&9를 읽고 조건부46e6c0을 현재좌표/flags/owner로 호출한 뒤44b9e0으로 내려간다.
- Session은 실제VT50da88만 고유 수명으로 선택한다. 다른 자산의 미복원 삭제를 일반삭제로 풀지 않는다. nonnull AI 제거의 미복원 하위는 기존 `RawGeyserAiRegistration`이 계속 명시 거부한다. 편집 모드의 큰 섬정리 내부도 기존 경계를 유지한다.

독립 관측은 두 새 자료를 구별한다.

| 관측 | 새 입력 | 정상 반환 | 실제 명령 | 정밀도 차이 |
|---|---:|---:|---:|---:|
| `carrier-mining` |864|1,728|502,848|0|
| `geyser-lifetime` |2,016|4,032|122,304|0|

첫 관측은 전체429800과 실제40ac20/4df230/TYPE getter를 실행한다. boss1/AI1/owner1/빈내용물/유효간헐천/같은Graph라는 명시 계약이다. 큰 현재조회·내용물생성·송신·로컬수량수신은 경계이며, 새C++와 비교하는 범위는 그 안의6바이트통지/순서/현재헤더·지면상태다. 실제 내용물 생성 경계가 각 정상 실행에서 먼저 한 번 선택되었음도 확인했다. 전체429800의 모든 분기를 새로 증명한 것은 아니다.

두 번째 관측은485580/485540/40ea00 전체를 실행한다. 기본프레임 getter/표시/AI/섬/큰Damageable는 명시 경계다. 실제 프레임쓰기·현재extra분기·목록중복압축·물리꼬리는 원본 명령이다. 반환·GP/retN/SEH/x87/명령·읽기·쓰기 범위를 검사했고 수명 목록의 앞뒤canary도 확인했다. C++는 같은 입력·명시 효과만 공급하고 기대값 계산에는 쓰지 않았다. 기존 `geysermined`의2312행(정상2304/assert8)과 기존 `constructioneffects` 원문은 바이트를 보존하고 전체 회귀에서 다시 재생한다. 기존 입력을 새 원본 수에 더하지 않는다.

실제 캠페인 검사는 두 가지다. 첫째,1-1의 실제 간헐천15095 섬 모서리에 골렘 초기 위치와 잔량400만 준비한다. 실제 선택기/BFS/Host40/시계로 도착시각까지 프레임을 실행하며 도착·너겟·수신을 직접 호출하지 않는다. 실제 내용물 너겟 생성→연결된peer 송신 시 내용물 존재/잔량400→로컬수신 잔량200을 확인했다. 수신 패킷은 실제6바이트이고 SP는 그대로다. 실제 귀환 접근이 끊긴 섬에서 실패하여 기존40은 삭제되고 새40은 없다. 보강한 검사는 별도의 새Session에서 초기잔량200도 같은 실제 도착 프레임으로 실행한다. 마지막 너겟 생성과 통지 뒤 실제 간헐천 고갈 삭제까지 안전하게 반환함을 확인했다. 두 초기 수량의 자연 도착·채취이며 자연 귀환/SP수입/왕복 완료는 아니다.

둘째, 실제5b 등록표/단일분배가 같은 지면을 사용한다. 명시free입력은 음향·차감 없이 통과하며 이를 복구한 뒤 잔량200의 통지를 처리한다. 실제 고갈 삭제·목록 제거·공간/공통 수명이 실행되고 선택기는 그 간헐천을 다시 고르지 않는다. 연결depth와 공통 삭제/장부 깊이도 복구된다. 첫 집중의free 상태 복구에서 `AllocatedBytes`로 다시 쓰려던 검사 오류를 기존 span 복구로 고쳤다. 두 번째 집중은 실제 고유 삭제 미연결을 드러냈으며 이를 구현·원본 대조한 후 해결했다. 초기 실패 로그는 보존한다.

참조 모듈의 옛 감사3종(`decomp_geysermined_oracle`, `analyze_geysermined_shadow_lineage`, `decomp_constructioneffects_oracle`)은 기존 SHA/이름변경 이력 불일치로 실패한다. 이번 단위가 그 참조 소스·테스트·ghidra 목록을 바꾸지 않았음을 HEAD와 개행 정규화 비교로 확인했고, 두 기존fixture의 당시SHA도 일치한다. 옛 보고서를 새 기대값으로 덮어쓰지 않았다. 근거는 `extracted/goal-carrier-mining-20261018-reference-audit.json`이다. 이 상태는 새 모듈의 원본 대조/실제 실행 성공과 구별한다.

재현 명령:

```powershell
python tools/decomp_carrier_mining.py --verify
python tools/decomp_geyser_lifetime.py --verify
cpppj/build/bin/Release/netstorm_tests.exe --filter CarrierMining
cpppj/build/bin/Release/netstorm_tests.exe --filter GeyserLifetime
ctest --test-dir cpppj/build -C Release --output-on-failure
```

최종Release build4는 **경고0·오류0**이고 통지/실제Session 집중4개와 고유수명 집중2개가 모두 성공했다. 보강 전 전체회귀는205.44초에 통과했고, 실제 도착의 마지막 채취·고갈 조합을 추가한 최종 전체CTest는 **1,621개·실패0·종료0·217.55초**(CTest217.58초)다. 주감사76종은 **65성공·직전기존11실패·새실패0**이다. 앞서 구분한 참조감사3종의 기존 불일치는 별도 기록이며 성공으로 바꾸어 세지 않는다. 기존50보고서153현재핀을 갱신했고 역치환한 과거전체JSON이 일치한다. 기존CMake접두/기존fixture/보호 실행파일2개·tarc·AGENTS·DN 인계/새UTF-8·LF를 확인했다. 클론진입5PASS/originals_unchanged=true이며1-1/1-2briefing→world/tutorial1~3briefing 범위다. 전체 스테이지 완주 검증은 아니다. 모든 관측·빌드·집중·회귀·감사·클론 핸들은 종료했다.

인수인계 뒤 **`20261018-05 CP 골렘의 실제 채취 통지와 간헐천 고갈 수명 연결`**로 로컬 커밋한다. 제품 보고서는 `cpppj/campaign-carrier-mining.json`, 로그 접두는 `extracted/goal-carrier-mining-20261018-`이다. 인계 후 로컬 커밋하며 push하지 않는다. 전체11스테이지 목표는 active다.

다음 미연결은 **Halt의429ed0**(특정TYPE156 포획), **CarrierArrival의428e00**(별도 포획 요청),429010 회피다. 앞의 인계에서 둘을 같은pickup 주소로 묶었던 표현은 이 구분으로 정정한다. 현재 실제AI 간헐천 등록/제거 하위,38·41·42경로,5040c0의42ae80, 실제Client명령 소비자·생산·경제·AI와11스테이지 자연운반·승패·저장·다음미션·완주는 남아 있다. 미등록187 그래픽 거부와 원본 DirectDraw 제외 범위를 유지한다.
