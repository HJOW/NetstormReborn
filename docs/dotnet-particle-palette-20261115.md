# dotnetpj 0.537 — 원본 색 띠·병합과 실제 팔레트 메모리

2026-11-15, HJOW-Athlon. 직전0.536은420e4a102다. 기준은 cpppj `o/ParticleGradient.h/.cpp`의e6d976346d9f7cebb5af45aa19bb91a2b83d3294와 `client/ParticlePaletteMemory.h/.cpp`의c258c7f3723d1c32fbbfdf39fca4a6f5f0fc3db9다. 기존 소스 이식이며 디컴파일 산출물은 변경하지 않았다.

## 구현

제품 연소 결선을 조사하다 첫cold생성이 사용하는 색 띠/병합과 실제 색메모리 소유자가 미이식임을 확인했다. 이 선행 작업을0.537로 분리했으며 제품Host/사제저HP 연결은 다음0.538이다.

ParticleGradients는 실제RGBX256색을 같은binary64 상수로 정규화한 뒤 최근접 시작/끝색, 값복사 origin, R/B/G내적과 B/G/R거리, 원통 임계값, 세차례CRT정렬, 축위치 선택/동률, 마지막 선택의끝색교체를 이식했다. 정렬은 8항목 선택정렬·30구간스택·피벗이동/동률제거까지 원문 순서를 유지하며 Array.Sort로 대신하지 않는다. 병합은 실제선택수보다 요청색수가 커도 앞요청개수의 번호를 복사한다. FieryTail/Lightning의 실제PE binary64 끝점과8+3색 구성도 이식했다. 색띠 함수는 대상RGB를 임의clamp하지 않고 C++경계의 길이/유한값/후보수/영축제한을 유지한다.

ParticlePaletteMemory는 Windows전용 호스트에 둔다. 최초Assign에 HeapCreate(0,0x1000,0), HeapAlloc(heap,0,count*4)와 실제색DWORD만 쓰기를 수행한다. Snapshot은 ReadProcessMemory로색수+1DWORD를 관찰하며 논리Count에 추가칸을 포함하지 않는다. 원본 관찰상수/마지막색/0으로 인접값을 꾸미지 않는다. 재할당은 이전배열을 같은heap에 반납하고, 종료/최종화는 배열→heap 순서로 한 번만 반납한다. 포인터는nint, OS크기는nuint로 유지하고 폐기후호출/잘못된새요청은 명시거부한다. Linux대체할당은 이번범위가 아니다.

## 검증

초기새검사5개는 통과했다. particle-gradient-x86의기본027f24입력/48색띠에 대해 머리4값과256개*7의미필드/48BYTE전체를 비트동일 대조하고1/2/3색띠72병합을 확인했다. 비교037f24행은 기대자료로 보존하되 완료로 세지 않는다. FieryTail/Lightning 실제생성자의기본cold각3COL입력의11색전체도 일치했다. 선택한고유PE입력은24+3+3=30개이며 생성자fixture의다른등록/수명행까지 완료로 합산하지 않는다. 새연결3fixture220행은 원본그대로이며 기대값을 C#에서 재생성하지 않았다.

Windows창없는2검사는8/11/256/1/8색의실제재할당, 논리Count/포인터, 독립주소읽기와Snapshot추가DWORD일치, 생성전/빈/과대/폐기후경계를 검증했다. 이웃DWORD는 각호스트의실제할당관찰이며 PE의heap내용과같다고 주장하지 않는다.

동률에서 마지막후보로바꾸는변이는Core3중1실패, 인접DWORD를마지막색으로합성하는변이는Client2중1실패로검출했다. 각제품소스를바이트/SHA단위로복원했다. 초기빌드는테스트람다의Windows플랫폼경고5개/오류0이었으며 테스트클래스에실제플랫폼계약을명시해정리했다. 제품/fixture/기대값보정은없다.

복원뒤최종전체비증분Release빌드경고0·오류0, 관련Core236/236·Client43/43은실패0·건너뜀0·각종료0으로통과했다. 직전205건의선택메서드를클래스단위로확대해기존Carrier12·Vortex6·AssetPostPop10도포함했으며 신설Core3+Client2검사를더했다. 이집중결과를최근전수회귀0.529와구별한다. 자료는extracted/dotnet-palette-0537-initial/·-mutations/·-final/에있고Git제외다.

## 다음 작업

실제HostSession의공통표시목록·연소시스템/사제회복·현재TYPE/HP/전역·실제SHP/현재프레임/뷰/소리를 연결한다. 전체Renderer/HUD/카메라·실제Kernel프레임호스트루프·일반피해/삭제/배치·brain/다른파생은 계속잔여다. 원본색표만 만들었다고 제품월드표시나M1일치로 세지 않는다.

PORT_MAP850행(L1 416+port434), fixture294개(3개추가). 전체floor/reflected유지. 실제게임창/사제포획·희생/특정적공격/승리/MP 재현은 없다. 보호원본/CD·AGENTS.md·README개요·cpppj·LEFT_JOBS.md·다른프로젝트 변경없음. 목표active·M1전체0/34다.
