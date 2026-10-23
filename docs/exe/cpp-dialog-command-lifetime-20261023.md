# HTML 선택 명령과 부모 반납 수명 — 단위376 독립 분석

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 보호10.78의 `Netstorm.exe`와 현재 정밀 `extracted/refined/originals/Netstorm.c`를 읽었다. 새 전체 디컴파일·기존 원본/CD/AGENTS 수정·GUI 실행은 하지 않았다. 이 문서는 일반 HTML 부모 연결을 담당하는 상위 작업자의 실제 구현과 구별되는 독립 분석/구성요소 기록이다.

## 원문 순서

`004c79b0`은 사건을 `03ffffff`로 마스킹한다. Enter 전역 `005427d8=02000038`이면 첫 슬롯의 버튼 포인터가 존재할 때 첫 메타 명령을, click `005427a8=0200002c`이면20개 슬롯 가운데 실제 자식 serial과 첫 인자가 일치하는 첫 슬롯을 선택한다. 선택한 정수 command/parameter는 `00452f90(command,parameter,parameter)`로 **같은40칸 입력 큐에 먼저 예약**하고, 이어서 `00466480(this)`로 원래 부모를 Kill한다. code0도 별도 제외하지 않는다. 무일치는 기본 Handle `00464990`으로 위임한다.

`004c7c60`은 `$ButtonInstant`를 자식 `+ac`의 bit4로만 표시한다. `004c79b0`은 그 비트를 읽지 않는다. 일반 버튼은 자식이 release에서, Instant는 press에서 같은 click을 통지한다. 따라서 부모 발화는 두 종류 모두 예약→Kill이며 Instant를 부모 유지 사유로 사용하면 안 된다. 부모에 직접 전달한 Enter/click의 release 비트도 c79b0 자체에서는 필터하지 않는다. 이는 자식 ButtonGump의 실제 사용자 입력 필터를 제거하라는 뜻이 아니다.

일반 메뉴 행은 `MenuGumpInput::Handle`의 원문에 따라 command(cmd,menu.id,param) 예약→메뉴 Kill→부모 Bubble `MenuJustClosed=020000be`로 이어진다. c79b0의 MenuJustClosed 분기는 부모를 Kill한다. 현재 논리 `input_.Event` 메뉴 fallback도 그 부모 종료 효과가 필요하다. HTML 링크 `0054278c` 분기는 nonzero 링크에서 **부모 Kill→4a00c0 링크 처리**이며 버튼 예약 순서와 다르다.

`004cf120`은 새 HTML 부모마다 `005cab2c`를 증가시키고 실제 소멸 `004cf1c0`은 카운트를 감소시킨 뒤 OnExit가0이 아니면 같은52f90에 예약한다. **Kill과 소멸은 다르다.** 현재 프레임 선택 명령은 입력 드레인에서 실행하고, 후반 Cleanup에서 OnExit를 예약해야 한다. 선택을 다음 Tick의 State 큐에만 보관한 채 부모를 먼저 Kill하면 OnExit가 선택보다 앞서는 역전이 생길 수 있다.

`004cf270`은 먼저 임시 새 부모를 생성해1차 HTML을 처리한다. alnum 내용이 없으면 그 임시 부모를 Kill하고0을 반환한다. 기존 표시 부모를 이 경로에서 임의 종료하지 않는다. `004cf730`도 창 serial0을 그대로 반환하므로 섹션 존재와 실제 창 생성 성공을 분리해야 한다. 현재 TryTell의 무본문 기존 페이지 유지와 이 원문은 양립하지만, 무본문 true를 Mission TellStage의 ‘창 생성 성공’으로 넘기면 자기 정지 관문은 달라진다. 임시 부모도 Cleanup 전까지005cab2c에 남으므로 OnExit 없는 retired 부모를 수명 카운트에서 빼면 안 된다.

Fury War `[S]`는 `<$CheatAllTechnology,3>` 명령만 남긴다. 기존 선택 부모는 원래 버튼 발화에서 닫혀야 하며 기술 함수492ae0에는 닫기가 없다. 기술 함수나 무본문 Tell에 부모 종료를 붙이면 사제 희생 뒤 명령과 다른 비동기 명령 경로까지 잘못 닫을 수 있다.

## 독립 구성요소와 관측

신규 `DialogCommandLifetime.h/.cpp`의 `FireDialogCommand(parent,selected,queue,kill)`은 메타에서 이미 해석한 `DialogExitCommandState` 두 DWORD를 값 복사해 같은 queue(command,param,param)→kill(parent)를 수행한다. InputQueue overload는 기존40칸 큐를 그대로 사용한다. Instant 판정·메타 이름 재해석·TryTell/State·OnExit 실행은 이 helper의 책임이 아니다. 예약 callback이 현재 페이지나 메타를 바꾸어도 원래 부모/선택 값을 유지한다.

`tools/decomp_dialog_command_lifetime.py`는 **원문258입력/두정밀도516회/60,056명령**을 독립 실행했다. c79b0 전체와52f90의 실제40칸 큐를 실행하고 Kill/링크/기본 Handle을 명시 외부 경계로 관측했다. 20개 슬롯·Instant0/4·release0/1·code0/정상/ffffffff·parameter0/99/ffffffff·빈 슬롯·tag·메뉴 닫힘을 포함한다. 정밀도 차이는0이며 thiscall 정상 반환/스택/비휘발 레지스터 보존을 확인했다. fixture SHA256은 `ee27d2eca659dae70d66fd3f30636485c0eae485356d937a9d8bc24f35a13482`다. 세부 실행 범위/근거 SHA는 `extracted/cpp-dialog-command-lifetime-20261023-native.json`에 있다.

`DialogCommandLifetimeTests.cpp`는 발화된252행의 실제 정수/순서를 전행 대조하며 Instant126행/release126행을 포함한다. 기존 큐의 선행→선택→후반 OnExit 순서, callback의 메타/현재 부모 변경, code0 예약도 별도로 확인한다. 전체258행 가운데 나머지6행은 helper 앞의 선택/fallback 분기 관측이며 helper 구현 완료 범위로 확대하지 않는다.

이 독립 작성자는 공용 CMake·LEFT_JOBS·기존 제품 파일·git을 수정하지 않았다. 상위 작업자가 이 helper를 실제 HTML 발화/Enter/timeout에 연결하고 검사 등록·빌드·실제 Fury 네 원소·기존 메뉴/OnExit/Guide 회귀를 검증해야 한다. 이 문서의 원문 격리 실행은 실제 GUI/게임 완주/픽셀 동일성 증명이 아니다.
