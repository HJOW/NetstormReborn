# ColorSort의 실제 팔레트 입력 수명 — 정적 추가 근거

[Screen clear 제안](cpp-screen-palette-device-next-20261024.md) 동결 뒤 읽은 좁은 정적 근거다. 공유 코드·원문·기존 동결 자료를 변경하지 않았으며 새 PE 실행/빌드/GUI를 수행하지 않았다. 현재/정밀 디컴파일 PC는 HJOW-Athlon / 2026-10-23이다.

원문 `0043b430`의 직접 호출자는 시작 초기화 한 곳이다(정밀 C47028행). 초기 fortPal→GamePalSpec→LoadPalette 뒤에 실제 논리 팔레트 주소 `005c7954`를 전달한다. 이 부모는 `00555e08`이0이면 Colorsort.cpp451행 진단 뒤 계속하고, 비영인 기존 `00555d88`/`00555dd8`을 각각free/0으로 만든다. 그 다음 `new(0x3800)`→`00439de0(this,logical)`과 `new(0x5008)`→`0043a8e0`→256개0x50항목 qsort→`0053199c=10`→`0043b300` 순서다. 이 전체 부모는 새 Screen patch의 검증 범위가 아니다.

`00439de0`는 논리 팔레트 R/G/B를 각 BYTE에서 읽어 정규화한 **double RGB를 객체에 복사**한다. 이는 화면 논리 팔레트의 동적 참조가 아니다. 후속 `0043c010`은 첫 줄에서 **현재 `00555d88` 주소를 캡처**해 그 객체의 색 기록을 복사·검색한다. `004b6100`은 LoadPalette(path,1) 뒤0043c010을 부르지만0043b430을 다시 부르지 않는다. 따라서 이 경로에서 현재 화면 RGB가 달라져도 ColorSort 원본 객체의 입력이 자동 갱신된다고 가정할 수 없다.

현재 `PaletteShade`/`PreviewColorMaps`/`SpriteColorMaps`/`ProductionColorMaps`/`OwnerColorMaps`의 계산 함수는 계속 재사용할 수 있다. 다만 **어느 시점의 논리 RGB를 입력할지**가 별도 원문 계약이다. 모든 호출에 현재 `Screen::Palette()`를 넣는 것만으로 원문 동일성을 증명하지 못한다. 원문 초기 ColorSort의 RGB256개/검색자료 수명을 Client가 유지하고, 각 원문 부모가 요구하는 그 현재 객체를 명시 getter로 읽게 하는 최소 연결을 검토해야 한다. 초기·현재 팔레트가 같은 경우만으로 입력 수명 차이가 검증됐다고 표시하면 안 된다.

현재 Screen clear patch는 이 ColorSort 자료를 재생성하거나 화면 교체 때 초기화하지 않는다. `00555e08`만 Client 실제 DWORD로 소유하고 두 Screen 생성경로에 빌린다. `RefreshPaletteConsumers()`는 기존 표시 소비자 경계일 뿐0043c010/ColorSort 객체 부모 전체가 아니다. Kencloud512×512 캐시와00532710 게임 난수도 별개다. 세 수명을 섞어서 빈 성공이나 새 RNG/색표 객체로 채우지 않는다.
