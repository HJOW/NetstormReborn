# 현재 CoreRefresh Session 후속388 제안

현재/마지막 정밀 호스트는 **HJOW-Athlon / 2026-10-23**이다. 공용 source/CMake/LEFT/git/build/lib/GUI를 변경하거나 실행하지 않았다. 387공용 빌드와 분리한 사본이며 Native 첫자료와 부모 알고리즘은 불변이다.

정상 대상의 해석을 바로잡는다. **005412ec=165(daisType)**이며 실제 Player152는005412b8이다. 원문357BYTE의 타입 비교는005412ec를 읽는다. **VT507950+80(5079d0)=004478f0**는 현재프레임/방향의 다이스 count1..3이며, 기존 **RawTempleRitual::Count**가 그 몸체다. 앞서 Player VT50ee08+80=4ad0f0/RawSquidLevel을 정상대상에 연결하려던 제안은 폐기한다. 주소 자체가 다른 가상표를 설명한다는 사실과 정상 부모의 선택을 구별한다.

[대상 정정 patch](../../extracted/cpp-player-core-refresh-target-20261024-proposed.patch), SHA **63a1eaf05cc34df5ea24b1c5ba319ce66d17c3a718671229bdf044711df6b395**는 `RawPlayerCoreRefresh.h/.cpp`와 기존 복원 문서의 주석/해석만 고친다. `playerType`/`level` 필드명과 모든 부모 연산은 그대로다. 주석 제거 후 두 제품파일이 동일한지 별도 확인했다. 첫 fixture의 타입DWORD는 관측자가 명시한 입력이며 해당 초기값 이름 해석을 증명한 자료로 확대하지 않는다.

[Session 연결 patch](../../extracted/cpp-player-core-refresh-session-20261024-proposed.patch), SHA **2eac199075e29ac6c411c4c5ffaabb5537b0e303295c7ce313d0ccff04ac8577**는 `RawCampaignSession.h/.cpp`와 신규 `SessionPlayerCoreRefreshTests.cpp` 세 파일이다. 사본 기준은 d207940c 이후 root387의 Cursor 입력 적용이 이미 들어간 실제 Session 두 파일이며 정확 SHA는 [사본 기준](../../extracted/cpp-player-core-refresh-session-20261024-proposed/baseline-sha.json)에 있다. 기존 Session 파일 전체 사본으로 덮어쓰지 않는다.

새 `RefreshCurrentPlayerCore()`는 loaded나 owner를 고치지 않고 동기적으로 원문 `RefreshPlayerCore`를 호출한다. 첫 Core getter에서 실제 `Impl::Login`을 요구하고, **같은 currentCore/cachedCore/coreActive**를 기존 **ReadPlayerCoreData**에 전달한다. challenge bool은 getter가 원문에서 한 번 읽는 입력으로만 DWORD변환하며 임시 상태에 쓰거나 실제 공유전역이라고 가장하지 않는다. 진단은 실제 위치에서 필수누락으로 중단한다. 새 Core10BYTE/cache/Clientalias나 임의 owner1 선택은 없다.

기술부모는 같은 `RawPlayerWorld::CurrentTechnology`, 매 Begin은 독립 실제 `RawContainedFinder`, Pool은 같은 현재 `core.pool.Bytes/Capacity`다. poolBase12000000은50BYTE 상대주소의 opaque32 기준이고 새 풀/할당/host포인터가 아니다. type bit는 별도 assets 사본이 아니라 **현재 core.types[type].techBitIndex**를 읽는다. typeWORDffff/범위 밖은 원문 getter의 진단이라고 꾸미지 않고 선택 load의 호스트 물리 오류로 거부한다.

가상80은 전달된 뷰가 현재 같은Pool의 같은SID/50BYTE인지 확인한 뒤 현재 VT507950만 실제 `templeRitual->Count`로 분배한다. dead 비트는 부모 원문처럼 첫 호출 뒤에만 검사되며 이 child는 Slot을 읽어 free/dead를 선행 거부하지 않는다. 다른 현재 VT를 선택하면 실제 몸체 미연결로 중단한다. 기존 `RawSquidLevel::AllocatedBytes`의 free 거부를 이 정상 다이스 경로에 끼워 넣지 않는다.

CoreWrite의 `hooks.refresh`도 새 API에 연결하되 원문 `WriteFortCoreData`의 manager/Metamap 진단과 post/editor 자연반환 뒤에만 선택된다. 실제 Editor checkpoint는 **LocalLogin이 없고**, World 앞 `004beca0(0)`도 현재 editor/post에서 자연반환한다. 따라서 Editor 정상설치에서 이 API를 미리 호출하거나 owner1 Core를 만들지 않는다. 모드가 바뀌어 refresh가 선택됐는데 Login Core가 없으면 첫 getter에서 필수누락이다. Client장기 Core/StateCleanup 접합 완료를 주장하지 않는다.

새 검사3개는 **미실행 초안**이다. 실제 LocalLogin handshake가 선택한 Player+6c alias/같은 TYPE기술/실제 다이스 Count, Login 없는 첫 store 전 필수실패, 미복원VT의 기술bit prefix보존을 검사하도록 작성했다. 외부 소리/번역/설정은 기존 Theme/LocalLogin 검사 장치 패턴이고 게임 계산은 실제 Session에 맡긴다. root388의 실제 공용 링크/실행을 거치기 전 통과 결과로 취급하지 않는다.

적용 순서는 **TypeTechBit eeeef5…→대상 주석63a1…→Session2eac…**다. 앞 TYPE는 root387에 실제 적용됐다. [별도 적용 검사](../../extracted/cpp-player-core-refresh-session-20261024-proposed/apply-verification.json)는 Git 조상 없는 C:Temp에서 세 patch의 check/apply6회0과 **실제9파일 내용/SHA·새검사 생성·공유파일 검사중불변**을 확인했다. 이 적용 검증과 Native/컴파일/실행 검증을 합산하지 않는다. Session/새검사 두 사본 번역단위의 **첫 `/Zs` 종료0·W4 경고오류0**도 확인했다. [구문 검증표](../../extracted/cpp-player-core-refresh-session-20261024-isolated/verification.json)는 첫 batch의 한국어 rem 변환으로 REM사용법만 추가 출력된 경계를 보존하며, 공용 빌드/링크/새검사 실행을 주장하지 않는다.
