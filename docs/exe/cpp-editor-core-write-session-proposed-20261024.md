# Editor checkpoint의 실제 CoreWrite 접합 사본

동결 [CoreWrite364BYTE 부모](cpp-fort-core-write-20261024.md)를 현재 [checkpoint 제안](cpp-editor-checkpoint-session-proposed-20261024.md) 뒤 Session에 연결한다. 현재/마지막 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이며 공유 기존 소스·CMake·LEFT·공용 빌드·라이브러리·GUI·git은 변경하지 않았다.

- [최소 delta patch](../../extracted/cpp-editor-core-write-session-20261024-proposed.patch)는 기존 Session.h/.cpp와 기존 checkpoint 검사 파일의 사본만 변경한다. 앞선 `76c1bb69…` checkpoint와 실제 GameRandom 소유자 patch가 적용된 공유 바이트를 before로 보존했다.
- 신규 공개 `WriteExistingEditorCoreData(slot)`는 명시 ExistingEditor/최종 loaded 전/실제 슬롯0..8에서만 호출한다. 기존 Campaign·LocalLogin 경로와 mode를 바꾸지 않는다.
- checkpoint의 `fort.coreWrite`를 이 함수에 자동 연결한다. 외부 callback의 빈 성공이나 임의 Editor 생략으로 부모를 대체하지 않는다.
- 원문 manager는 같은 Storage.Headers의 현재 첫DWORD, Metamap은 현재 `core.ownerMode.challenge`, post/editor는 같은 `globals`, Player는 실제 `s.players`, 현재 번호는 `playerState.battleOwner`다. Template owner로 바꾸지 않는다.
- 도달 진단은 실제 Template.cpp `0x45f/0x460/0x467` 번호를 포함한 선택 장치 누락으로 중단한다. 현재 장치가 없는 진단을 성공으로 무시하지 않는다.
- 자연 post/editor 관문은 실제 RawFortCoreWrite 본문이 판정한다. 신규 seed DWORD나 성공 CoreWrite stub이 없다.
- 직렬화가 실제 선택되면 같은 Storage.CreateSection을 한 번 실행하고 그 실제 노드/생성수명을 보관한다. 반환 body token1은 함수 내부 registry이며 기존 섹션 handle로 가장하지 않는다. 일반 쓰기도 같은 실제 Storage.WriteSection이다.
- RNG 장치는 기존 같은 `s.seed.Reseed()` / `s.random.Next()`를 빌린다. 새로운 GameRandom/CRT/동기화 사본을 만들지 않는다.
- 현재 없는 `0044fc00 refresh`, `004be960 SP`, 실제 `005c89b4` 소유자의 seed getter는 비워 둔다. 선택 위치에서 부모가 필수 누락으로 거부한다. 미선택 자연 Editor는 이 장치들을 선행 요구하지 않는다.

추가 Session 검사 초안3개는 실제 Client/Storage를 사용한다. 자연 Editor 반환 뒤 manager/cursor/기존 CoreData/전체 Player/같은 GameRandom 상태와 checkpoint/loaded 미완료를 확인한다. manager0에서는 자연 관문 전에 `1119(0x45f)`, 현재 challenge1에서는 `1120(0x460)` 진단을 확인한다. 실제 CoreData 전체 직렬화 완료를 기대하는 검사는 없다. 기존 checkpoint 검사5개는 그대로 보존한다.

검증은 [verification.json](../../extracted/cpp-editor-core-write-session-20261024-proposed/verification.json)에 기록한다. MSVC `/Zs /std:c++20 /utf-8 /W4`로 두 사본 소스의 구문만 검사했으며 공용 라이브러리 링크·Session 검사 실행은 하지 않았다. 별도 CoreWrite 모듈의396원문대조/격리5검사 성공과 이 Session 사본의 구문 검증 범위를 구별한다.

seed의 PE초기0/장기 소유자 필요와 refresh/SP의 실제 의미는 CoreWrite 문서에 있다. 이후 저장 분기를 연결하려면 그 실제 상태·본문을 복원해야 하며 스크래치 seed·SP float4BYTE·일반 SP 저장소로 대체할 수 없다. 최종 HUD/State/loaded/UI는 계속 범위 밖이다.
