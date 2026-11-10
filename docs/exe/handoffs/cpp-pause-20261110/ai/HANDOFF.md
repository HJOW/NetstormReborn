# M6-n 비영 AI NoWorld 경로 — 부분 읽기 전용 감사 인계

2026-11-10 / HJOW-Athlon. 부모의 제품1264 확정·1265 진행 중 수행한 **부분 읽기 전용 조사**다. 사용자 일시중지 요청에 따라 범위를 넓히지 않고 여기서 마친다. 제품·LEFT_JOBS·docs·AGENTS·원본/CD는 수정하지 않았다. 빌드·구문 검사·단위/게임/MP/패킷·원본 실행·커밋은 모두0이다. 새 디컴파일도0이다. 마지막 디컴파일은 LEFT_JOBS의 Client 양판본2026-11-03/R2026-11-07, 현재와 같은 HJOW-Athlon이다.

## 확인한 현행 구조

1. `ClientMain.cpp:423~426`의 장기 PostPop 기본 `aiNotice`는 현재 challenge/owner/Player+b0의 자연 관문을 거친 뒤, brain 비영이면 `RequireSession → PinCurrentRuntime → NotifyAiObject`를 요구한다. `:451`의 장기 Geyser 등록도 비영 brain의 하위에서 같은 Session을 요구한다. `:460`의 장기 Reward 지갑은 `RawAiWallet`의 비활성/owner/NULL brain 관문 뒤 현재 Session의 `BorrowAiWalletMemory()`를 빌린다. 없는 Runtime을 가짜 성공으로 처리하지는 않지만 장기 본문 자체는 아직 아니다.

2. `RawCampaignSession.cpp:513`의 `RawAiBrainMemory aiMemory`가 본체0xb08과 목록 할당의 실제 소유자다. 생성자는`:998`에서 Session의 allocationBytes 서비스에 연결된다. `ConnectAi(:2763~)`에서 같은 메모리를 constructor·wallet·object notice·world registration과 전체 전략이 공유한다. `NotifyAiObject(:4622)`·`NotifyGeyserAi(:4620)`·`BorrowAiWalletMemory(:4631)`는 이 Session 보유 메모리/모듈로 위임한다.

3. `RawAiWallet`은 이미 현재 Player getter 및 `AiWalletMemoryAccess{memory, owner}`를 받는 overload가 있다. **지갑만 장기화할 때 새 지갑 계산을 만들 필요가 없다.** 같은 실제 Client 메모리를 확보하는 acquire 장치로 바꾸면 기존00413bb0/00413c20 본문을 재사용할 수 있다. disabled는 challenge, demo는 현재 Mission+68이며 missionPost로 대체하면 안 된다.

4. `RawAiObjectNotice`는 생성/삭제 통지의 실제 본체/목록/타이머를 처리하고, 원문 주소는 헤더의004166e0/00414de0이다. 외부 하위는 carrierTargets(00426150), distanceSquared(00411120), log 세 종류다. 현재 `players_`는 고정 span이므로 Client 장기 객체로 단순 이동하면 PlayerRecords 재할당 뒤 stale span이 될 수 있다. `RawAiWorldRegistration`도 고정 players span·allies span·surface span을 보관한다. 장기화에는 실제 현재 Player getter 또는 호출마다 같은 장기 자료를 빌리는 어댑터가 필요하다. Player/brain/list의 사본을 만들면 안 된다.

5. `RawAiWorldRegistration`의004153c0 Geyser는 실제 다섯 목록을 갱신한다. 같은 메모리·Pool·현재 TYPE·Player9·방향 동맹81·hash0 65536WORD·같은 ObjectNotice를 요구하며, 중심점/farther 로그/표면0 진단을 외부 장치로 받는다. `FullScan(00415720)`은 실제 임시300DWORD 할당도 같은 메모리에서 수행한다. 이들 하위는 원문이 선택한 위치에서만 요구해야 한다.

6. `RawCampaignSession::AiCarrierTargets(:2931)`는 현재 이동 TYPE→내용물 조회→실제 Host의 RawPathProcess→TargetA/B다. 장기화할 때 빈 `{0,0}`을 고정 반환해서는 안 된다. 현재 부품은 `SelectMovementProcessType`(RawPriestWorldBindings.cpp:14~29), 장기 Storage의 `PriestState().movement`, `RawContainedLookup`, `Storage.Processes().Host().FromForm`, `RawPathProcess::Runtime()`로 분리 가능한 후보지만 **이 접합은 미구현·전수 대조 미완료**다.

7. 거리/중심은 현재 `RawGunTarget::Center(:119)`의 raw float 좌표에서 TYPE footX/Y 절반을 뺀 float 결과다. 새 임시 전투 타깃 시스템을 구성할 필요는 없어 보이나, 장기 Neighbors.Center와의 입력/진단/정밀도 동등성은 추가 확인해야 한다. 기존 Session은 목표 중심을 먼저, source 중심을 나중에 읽어 distance 제곱을 계산한다.

## 수명에서 특히 주의할 점

- **메모리만 Client로 옮겨 끝낼 수 없다.** `RawAiBrainConstructor`의 `live_`/`destructed_`도 현재 Session 객체 안에 있다. `Destruct(:101)`와 `FreeBody(:107)`는 이 장부를 검사하며, 실제 선택 효과는 같은 메모리의 ae4 조건부 free→16개 목록 역순 free→본체 free다. constructor 전체를 그대로 Client에 보관하면 짧은 RawPlayerWorld/Participation/IslandRuntime 참조가 남는다. 실제 메모리와 생성/소멸 장부를 같은 장기 owner로 분리하고, constructor/전략은 그 owner를 빌리는 방식을 먼저 검토해야 한다. 독립 typed 경로의 기존 소유는 유지해야 한다.
- `ClientStateRuntimeRetirement::ResetAi(:52)`는 현재 Session이 없고 실제 Player brain이 비영이면 명시 오류다. 정상 State 반납은 owner1..8 ResetAi 증빙→brain0을 확인한 뒤 Runtime을 반납한다(`Retire(:75~)`). 따라서 **현재 정상 반납 뒤 항상 비영 brain이 남는다고 단정할 근거는 없다.** 정상 State 수명과 원문 전체 NoWorld 지원을 구분한다.
- Session 소멸자는 원래 참여 모듈로 brain1..8을 먼저 정리하고, 실패도 stderr에 남긴 후 다른 자식을 정리한다(`~Impl:3954~`). 메모리를 Client로 옮기더라도 원문 reset 위치와 이 기존 안전 반납을 임의 생략하면 안 된다. NoWorld reset도 장기 실제 소멸 장치를 선택할 수 있게 연결해야 한다.
- 메모리 `Fill`은 원문 미작성 바이트 입력이다. 장기 getter를 생성할 때 본체/목록을 조기 할당하거나 새 Player·난수·World·brain0을 합성하지 않는다. 현재 실제 Client의 allocationBytes 서비스 위치/소유를 먼저 대조해야 한다.

## 다음 구현을 시작할 최소 순서

1. 현재 제품/마지막 호스트·이 후보 해시를 다시 확인하고 원문004166e0/00414de0·00416720/004153c0·00413bb0/00413c20·00413ae0/00410100을 직접 대조한다. 이 조사에서는 **신규 C/PE 전수 대조를 마치지 않았다.** `original-fragments.c`는 후속 재개용 발췌이며 실행/검증 증거가 아니다.
2. 같은 Client의 `RawAiBrainMemory` 및 생성/소멸 장부 owner를 설계하고 Native Session의 모든 aiMemory 소비자가 그것을 빌리게 한다. 단순히 별도 두 번째 메모리를 추가하지 않는다.
3. wallet acquire를 같은 장기 owner로 연결하고 현재 Player getter를 유지한다. 그 다음 ObjectNotice/WorldRegistration의 현재 Player/시각/Outpost/동맹/표면/경로 getter와 실제 로그/진단을 장기 장치에 연결한다.
4. Native Session의 해당 모듈도 같은 장기 본문/장부를 빌리도록 통합한다. 독립 typed/CD의 기존 경로와 전수 전략/constructor의 미완료 경계를 따로 남긴다.
5. Client의 aiNotice·geyserRegistration·Reward와 NoWorld State reset을 같은 owner로 접합한 뒤 원문 자연 관문·정상 반납·현재값 재조회·동기 호출 수명을 검증한다. 사용자 제외 MP/포획/희생/특정 공격·승리 실행 검사는 하지 않는다.

**미완료:** 실제 원문 전수 C/PE 대조, NoWorld 유효 비영 AI 도달성 전체 감사, 장기 AI owner 구현, 생성/소멸 장부 분리, 현재 Player getter 변경, AI path/center/log/진단 접합, 관련 빌드·허용 검증·전체 M6-n 마감. 후보 구현 파일은 만들지 않았다. 이 문서를 완료 판정으로 사용하지 않는다.

파일 바이트 기준선과 발췌 함수 목록은 `audit.json`에 있다. 부모가 제품 단위/LEFT_JOBS에 이 경로를 반영하며, 이 하위 작업은 추적 문서나 커밋을 별도로 만들지 않는다.
