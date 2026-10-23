# Kencloud 설치와 같은 Session GameRandom 참조

단위384 후속의 좁은 **미적용 API 제안**이다. [patch](../../extracted/cpp-kencloud-game-random-20261024-proposed.patch)는 기존 파일을 읽어 두 개의 신규 사본에 getter만 추가한다. 공유 파일/CMake/LEFT/git/공용 build/lib/GUI를 바꾸거나 실행하지 않았다. 이 API 사본은 아직 컴파일/실제설치 검사를 수행하지 않았다.

현재 Client의 `ClientRandomGlobals`는 **CRT holdrand**와 Subscriber ID 최초시드를 소유한다. 이는 `004558c0`의 실제 게임 난수 `00532710`과 다르다. 현재 `RawCampaignSession::Impl::random`이 seed/SP/지형/객체 등의 같은 GameRandom이며, Client/UberGump 공개에는 그 현재 객체 getter가 없다. GameWorld::Session()과 UberGump::HasWorld()만으로 설치 도중의 현재 난수를 찾았다고 가정할 수 없다.

새 `BorrowInstallGameRandom()`은 `impl_->random`을 **참조로 직접** 돌려준다. 생성·시드·상태를 복제하지 않고 loaded 관문도 두지 않는다. 설치부모는 이미 생성된 현재 Session에서 이 참조를 빌려 `BuildKencloud(globals,session.BorrowInstallGameRandom(),hooks)`에 동기적으로 전달한다. StepKencloud는 난수를 새로 만들거나 소비하지 않는다. 기존 GameRandom 계산 본체/fixture를 그대로 재사용한다.

`menu.world_`는 `make_unique<GameWorld>`가 끝나기 전 null일 수 있다. 그래서 palette hook은 설치중 현재 Session/Impl을 직접 빌려야 하고, public loaded를 미리 켜거나 다른 Session으로 우회하면 안 된다. 참조와 이를 캡처한 callback은 해당 Session보다 오래 살아서는 안 된다. Client 장기 Kencloud 소유자에 파괴된 Session의 참조를 저장하면 안 된다.

이 getter는 **현재 설치 호출의 최소 연결**만 해결한다. 월드가 없는 메인 메뉴나 Session교체 후의 구름 프레임에서도 원본 단일00532710을 요구하는 전체수명 문제는 별도다. 그때 현재 GameRandom의 단일 Client수명 일반화를 원문 근거/현재값 보존과 함께 검토해야 한다. 새 GameRandom/별도시드/CRT치환/호출뒤 state사본동기화를 추가하지 않는다.

실제 [ColorSort 초기 논리 RGB 수명](cpp-palette-colorsort-owner-20261024.md)은 이 구름/RNG와 별개인 필수 경계다. 고정 초기 입력을 현재화면 RGB로 자동 치환하지 않으며, 이 getter 제안을 전체Kencloud/Editor설치완료로 표시하지 않는다.
