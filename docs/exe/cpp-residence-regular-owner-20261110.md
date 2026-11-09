# 작업1237 — 거주 구역 정기 처리의 현재 상태와 장기 연결

**제품1237의 검증 기록이다. Fort Dude 비교 목록 오접속은제품1238에서 정정했다. 전체 목표는 미완료다.**

[1238 정정](cpp-residence-provider-gate-20261110.md): 아래1237 기록의00592f8c를 거주 구역 count로 해석한 부분과 실제 caller 연결은 잘못이었다. 실제 공급원 count를 사용한다. 거주 구역 count는0059a9e0이며 두 목록을 다르게 둔 회귀156/156으로 수정했다.1237의 원본 기대값/빌드/일반 미션 관측과 당시 소스 SHA는 역사적 증거로 보존한다.

실제 `0050f540 +5c=0049a4e0`을 독립 공유 본문 RawResidenceRegular로 분리했다. Native Session/NoWorld Host가 같은 현재 Pool/Hash/프레임·게임 난수·신전 목록과 지형 fringeType을 사용한다. 주민 nibble의0/1 전환→실제 통지→현재 signed frame의 짝수 정렬→난수 지연, 두 좌표 난수와 실제 finder/첫 변경 후 Next, 지면 소유자→현재 첫 일치 신전의 TYPE+98/테마 설정→두 실제 프레임 지정 순서를 유지한다. CD의 기존 별도 본문은 유지한다.

0059a9e4의 signed 최대 조사 DWORD를 GameAux의 같은 장기 주소로 옮겼다. 지형 TYPE은 실제 TerrainBitmapState의005411f0을 약하게 빌리고 현재 theammode는 실제 장기 건설 설정 owner/함수 사본을 통해 읽는다. 첫 메뉴 전에 결합하며 상태/난수/Player/World를 초기화하지 않는다. 선택된 Fort Dude의 진입점/생성/작업은 현재 Runtime 약한 대여→실제 전체 Runtime pin→함수 사본이다.

기존 미연결450a90의 작은 부모도 복원해 기존 RawDudeSpawn/RawDudeTask/RawDudeNotice.ResidencePoint와 연결한다. 원문00594fb8 Fort 관문 뒤 **0055a4f8 Dude count와00592f8c 거주 구역 count**를 signed 비교한다. 처음 작성한 population 용량 비교는 원문 재대조에서 잘못임을 발견했다. 최초 빌드 종료 뒤 수정했고 증거를 보존했다. source free만 거르고 진입점 효과 뒤 현재 owner, 생성 뒤 현재 새 객체의 free/dead를 확인한 다음5102/0을 예약한다. 일반 플레이의 사건2는 Fort가 아니므로 Runtime을 요구하지 않는다.

NoWorld의 프레임 재등록은 원문의 Residence에서 flags1이 아닌 경우에 해당하므로 같은 공통 PostPopBase를 호출한다. 최초 Residence 후처리 전체는 기존 현재 자산 모듈의 책임이며 이 단위에서 모든 standalone 첫 생성/전수 caller가 완료됐다고 세지 않는다. 다른 큰 Dude/AI/배치/Fort 상위 Finish와 전체 수명 감사는 남는다.

기존 거주 구역261입력과 별도 좌표864행(합1125)을 수정하지 않고 두 x87 정밀도/2250회 재실행해 모두 일치했다. 과거 oracle의 누락된 좁은 metadata는 현호스트 정밀 분석(refined)에서 같은 선택 함수68개(48/12/8)를 재추출해 보완했다. 기존 최초 실행에서는 검증 도구의 좌표 override가 scene4의 입력까지 바꾼 차이를 발견했고, 원래5.25/4.75 입력을 복구했다. 원본 기대값/fixture는 바꾸지 않았다.450a90은 원문 정적 대조와 별도 현재 필드/관문 검사이며 기존 parent fixture에서 그 큰 하위를 실행했다고 세지 않는다.

현재 호스트는 마지막 Client양판본 정밀 디컴파일2026-11-03와 R2026-11-07의 HJOW-Athlon이다. 새 디컴파일 실행/기존 정밀 C 수정은0이다. 원시 자료는`extracted/cpp-resume-1237/`다. 원본/CD/기존fixture·AGENTS2,807파일을 보존한다.

확정 연결 집계는71/87VT, 잔여16VT/고유14함수이며 다음은 Dude1VT/450fa0다. 전체 NoWorld/M6와16상위계획은 미완료다. 포획/희생·특정 적 공격·승리/자연완주·공중3종 전용·MP·사용자 직접 확인 실행은 제외한다.

최종 Release/x64 제품2·집중1대상 빌드는37.966초/종료0·경고/오류0이며 소스/새 근거17개를 고정했다. 비MP155/155가 통과했다. 첫 빌드의 새 검사 StoreLevel 인자는 실제 SID로 고쳤다.154/155의 새 Host 검사는 void/abstract 입력이라 원문 관문에서 실행되지 않았으므로 같은 일반 nonvoid/nonabstract 상태로 바로잡았다. 현재 Count 두 읽기의 원문 순서도 명시했다. 제품 Host/원문 기대값/fixture를 약화하지 않았다.

최종 EXE에서 Tutorial4 생산·건설 요청 뒤TYPE75/owner1 생성과600프레임 대기, Dissolved Alliance의 기존 자기 소유TYPE89 거주 구역을 정상 진입 후1000프레임씩 두 번 대기했다. 같은 SID/type/owner/좌표를 유지하면서 실제 프레임 전환을 확인했다. 두 실행 정상 종료0·PNG 직접 확인, 보호2,807파일과 실행 전후 제품/격리 설치 자료 불변이다. 모든 시작한 빌드·집중·GUI 작업이 종료됐다.

- Netstorm.exe: 14215680 bytes, SHA256 `a20f0f0cedff3090d77e3c9b517a26e2d8f2c956d69c7b10c3720f73a90de512`.

- R.exe: 677376 bytes, SHA256 `d8331a4b8d8622c1e6eff38e3a626b4af2bca7e85161c12ade8abe08db4cb434`.

[확정 보고서](../../cpppj/residence-regular-owner-20261110.json).
