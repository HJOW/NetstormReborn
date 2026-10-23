# Player Deck 개별 반납 제안

현재 호스트와 마지막 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 같다. 보호 PE/원문과 공유 제품은 변경하지 않았다. 이 문서는 후속387용 사본 제안이며 실제 Session/StateCleanup 연결은 포함하지 않는다.

원문 **0044f700/94BYTE**는 Player+68이0일 때 `operator_new(352)`로 한 블록을 얻고 다섯 DWORD만 초기화한다. 기존 `RawProductionDeck::blocks_`가 이 할당의 실제 단일 소유자다. 같은 원문에서 온 `Initialize`/`ClearEntries`는 수명 반납이 아니다.

**0048fcd0/79BYTE**의 비영+68은 **004e4354→004e422a `_free`**에 전달된다. 004e422a는 NULL이면 무효과이고, 비영이면 현재 CRT heap 종류에 따라 small-block lock/find/free 또는 실제 `HeapFree`를 선택한다. 이를 에뮬레이터의 토큰 삭제로 성공했다고 주장할 수 없다. 선택한 인자와 free복귀 뒤 현재 Playerbase 재조회/0쓰기는 이미 제품 전 [Native162](../../extracted/cpp-player-links-cleanup-20261024-native.json)로 고정됐으므로 이번에는 중복 관측하지 않았다. 기존 [Get/352 할당 관측](../../cpppj/production-deck.json)도 보존한다.

제안은 기존 클래스에 **`void Release(std::uint32_t token) const`** 하나를 추가한다. 같은 `blocks_`의 정확한 한 항목만 `erase`하며 Player/다른 블록/`next_`를 쓰지 않는다. NULL은 CRT 계약대로 무효과다. 미등록 비영·중복 토큰은 `out_of_range`이며 다른 소유자의 블록을 성공 반납한 것처럼 처리하지 않는다. 잘못된 포인터의 원문 CRT 동작은 호스트 예외와 별개다. 반환 직후 빌린 해당 `Bytes` span은 무효다.

부모의 실제 연결은 `hooks.release=[&deck](auto token) { deck.Release(token); }`이며 같은 Deck owner가 부모 호출보다 오래 살아야 한다. Player+68의0쓰기는 **부모에서만** 한다. Release 뒤 그 슬롯을 남기면 `Get(owner)`가 실제 할당 누락을 거부한다. 기존 소멸자는 아직 소유한 블록을 가리키는 현재 Player 포인터만 지우므로 Release 뒤 부모의0쓰기를 소멸자로 대신할 수 없다. 재진입 중 원문 중복free는 안전한 동작으로 바꾸지 않는다.

[제안 patch](../../extracted/cpp-production-deck-release-20261024-proposed.patch)의 SHA는 **490839956d9d7f7a786f1abc168d1e2339c8e5dd64caf8323a3240b1c6220fbf**다. 변경 대상은 `RawProductionDeck.h/.cpp`, 신규 `ProductionDeckReleaseTests.cpp`뿐이다. 공용 CMake/LEFT/git 변경은 하지 않았다.

실제 SidPool/SquidPostPop/RawPlayerWorld와 같은 Deck를 사용한 격리 **3검사/실패0**, 최종 **/W4 경고0·빌드종료0**이다. 한 블록 반납과 다른 블록/Player 보존, 실제 CleanupPlayerLinks→같은 owner 반납→후속0쓰기, NULL/미등록/중복과 소멸자 수명을 확인한다. 별도 allocator registry나 성공 stub은 없다. 최초 전체 소스 컴파일의 미선택69링크오류와 다음 추출파일명 오류는 보존했다. 최종 격리 링크는 필요한 기존 함수 몸체를 내용 변경 없이 추출하며 [선택 몸체 SHA/최종 로그](../../extracted/cpp-production-deck-release-20261024-isolated/verification.json)에 근거를 기록한다. 공유 lib나 게임은 사용하지 않았다.

`git apply`는 Git 조상 없는 **C:\Users\BUZZ\AppData\Local\Temp\cpp-deck-release-4lubzaux**에서 check/apply 모두0이고 세 실제 결과·신규 검사 생성·공유 두 파일 SHA 불변을 확인했다. 실제 결과의 CRLF와 사본 LF는 전체 정규화문자열로 비교하고 양쪽 raw SHA를 [적용 보고서](../../extracted/cpp-production-deck-release-20261024-proposed/apply-verification.json)에 각각 남긴다.

동료 fury의 사본 두 제품파일/신규3검사 읽기 검토에서도 명백한 수명/계약 오류는 발견되지 않았다. 추가 Native/빌드/공유 변경을 한 검토는 아니다. 위 patch와 세 사본 파일은 동결했다.

후속 CoreRefresh 정적 경계: Player VT **0050ee08**의+80(**0050ee88**)은 **004ad0f0**이다. 기존 `RawSquidLevel::Get/Has`와 Session의 같은 `level/core.pool/core.types`를 재사용할 수 있다. 이 getter의 현재 TYPE 접근/AllocatedBytes의 free 거부는 가상 분배의 명시 물리 계약이며0레벨로 대체할 수 없다. 별도 Session 사본은 현재 Login Core와 기술 TYPE+1ec의 실제 초기화·`techBit` 속성 저장을 먼저 확인한 뒤 작성한다.

후속 정정: 위 Player VT 주소 자체는 맞지만 **CoreRefresh의 정상 대상 제안으로는 폐기**한다. 원문 비교 전역005412ec는 실제165(다이스)이며 Player152의005412b8과 다르다. 정상 VT507950+80은004478f0이고 기존 `RawTempleRitual::Count`를 연결해야 한다. 최초 Native/Deck Release patch는 이 해석과 독립이며 변경하지 않았다.
