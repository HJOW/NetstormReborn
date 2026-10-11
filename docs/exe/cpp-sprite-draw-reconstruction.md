# 일반 sprite Draw 복원

작업 PC: **HJOW-Athlon**, 기록일: **2026-10-12**. AGENTS.md와 두 LEFT_JOBS를 확인했고 착수 시 Git 트리는 깨끗했다. 마지막 디컴파일 수행 PC도 같았다. `spritedraw` **10/10/10개**를 Ghidra 읽기 전용으로 내보냈다. 원본 게임·복사본·analyzeManager·클론 창은 실행하지 않았다.

## 완료 범위와 연결

패치 10.78의 **00498220**, CD/추가판의 **00455f60**에서 일반 본체·그림자·소유자/명시 색·가상 프레임/Draw·추가 표식·선택선·체력/상태 막대의 분기와 호출 순서를 `SpriteDraw`로 복원했다. `BindSpriteDraw`는 공통 kind 0과 kind 1 묶음의 각 항목을 연결한다. 실제 `ObjectCollection`→CRT 정렬→공통 분배→SHP 픽셀 출력도 검사한다.

`SpritePixelTarget`은 실제 SHP 압축 해제/투명 픽셀/색 치환과 직교 선택선·half-open 막대 fill을 클립된 8비트 표면으로 출력한다. `MakeSpriteAssetViews`는 GameAssets의 논리 프레임 수·모든 물리 선택 헤더·gumpFrame을 연결한다. `MakeSpriteAssetShapes`는 그림자 alias의 현재 SHP 포인터를 반영한다. `MakeSpriteSurfaceLookup`는 실제 0단계 해시 표에 표식 조회를 연결한다.

## raw 본체·그림자·표식

프레임은 패치 raw +36 DWORD, CD +34 BYTE다. extra는 패치 +40, CD +35이고 일반 소유자는 패치 +34, CD +32 BYTE다. 지정된 특수 타입은 +12 WORD를 소유자로 읽는다. signed 화면 WORD +22/+24에서 카메라를 빼고 타입 hotspot을 뺀 그림 기준점을 사용한다. 저장 화면 좌표가 raw float ×16/11+0.5의 CRT 절삭 결과와 다르면 원본 assert에 대응하는 예외로 거부한다.

tail bit 4 그림자는 extra `0x10`에서 즉시 반환한다. 패치의 별도 고정 상태가 꺼져 있으면 지정 타입의 SHP 포인터만 현재 그림 타입으로 바꾼다. 프레임이 정상 개수 이상이면 개수를 **한 번만** 빼고 다시 더하여 그림자 레이어를 선택한다. 특수 표시/extra bit 1이면 그림자는 출력하지 않는다. alias 쓰기는 이 조기 반환보다 앞에 있다. 타입의 논리 프레임 수·gumpFrame·hotspot은 alias로 바뀌지 않는다.

일반 본체는 타입별 소유자 표가 있고 owner가 양수일 때 현재 소유자 표 선택→mapped Draw를 호출한다. extra bit 1·`0x10`·특수 표시/타입 필터는 각각 명시 변환 표를 사용하며 현재 소유자 문맥을 바꾸지 않는다. `SpriteShapeMode`의 Dead/Abstract 이름은 코드의 표/분기를 구분하는 이름이다. 모든 객체에서 해당 bit의 게임 의미를 새로 확정했다는 뜻은 아니다.

extra bit 2는 vtable +0x78의 가상 프레임과 +0xa4의 가상 Draw를 사용한다. -1의 기본 gumpFrame, abstract 소유자, 특수 필터의 가상 Draw 생략을 원본대로 구분한다. 가상 프레임 조회 이후의 현재 필터 전역과 가상 Draw 이후의 현재 raw 선택/표식 비트도 읽는다.

추가 표식은 표시 옵션·flags1 `0x400`·소유자·표면/제외 타입을 확인한 뒤 팔레트 번호-1을 frame으로 출력한다. 표면은 entry의 float XY에 **원본 float `0.9999`를 더하고 CRT 절삭**해 256×256 표에서 읽는다. raw 좌표만으로 대신하지 않는다. extra `0x20` 표식은 원래 타입 footprint를 0방향으로 반씩 나누어 기준점을 보정한다.

그림자 함수도 원본의 명시 translated Draw다. 따라서 픽셀은 **소스 SHP 색을 치환**한다. 기존 Renderer의 별도 대상 화면 색 변환 기능을 여기에 사용하지 않는다.

## 선택선·판본별 막대

현재 extra bit 4와 선택 억제 전역을 확인한다. 현재 물리 SHP 앞의 signed WORD 폭/높이/hotspot으로 아래 모서리 두 겹의 선 **8개**를 원본 순서대로 출력하고 선택 SID 전역을 갱신한다. 가로 길이는 폭/4, 세로 길이는 (높이-1)/4의 0방향 절삭이다. 패치는 원본 frameCheck와 해제 흔적도 확인하고, CD의 물리 표 밖 접근은 안전하게 거부한다.

패치 HP는 raw +26 signed DWORD, CD는 signed WORD다. hasHP의 flags2 `0x400000` 특수 분기와 flags1 `0x10`, 지정 타입의 maxHP/4 옵션, 반·1/4 이하 색 경계를 유지한다. **패치**는 현재 프레임 폭 전체·2픽셀 본체·3픽셀 행 간격이다. **CD**는 먼저 maxHP를 최소 1로 만들고 HP를 clamp한 뒤 가운데 **19픽셀 길이·5픽셀 높이**로 그린다. 판본별 위치/배경 fill도 다르다.

패치의 추가 막대는 원본 순서로 건설(0x7b), 자원(0xbe), 세 타이머(0xd9/9/0x6b), `0x10` 상태(0xc3), `0x20` 상태(0xfe), 비용/수량(0xf0)을 출력한다. 현재 건설/자원/내용물/contained 타이머 조회는 명시 게임 규칙 경계다. 조회된 실제 타입 +0x1b8/+0x1bc/+0x1cc와 raw +42/+46, endTime-now 및 cost를 사용한다. 타이머 20/60/60은 **막대 최대값**이며 남은 시각을 그 값으로 나누는 처리가 아니다.

상태 프로세스는 원본 Kernel 순서와 예약 슬롯 **0 제외**를 유지한다. 타입/멤버 SID의 첫 일치, flag 1의 flag 2보다 높은 우선순위, 음수 절삭·최댓값 선택을 처리한다. native는 40,000개 실제 슬롯과 마지막 슬롯까지 읽고, 호스트는 호출자가 제공한 현재 슬롯 view를 읽는다. 목록/타입/시각을 실제 ProcessKernel에서 자동 공급하는 연결은 후속이다.

## 독립 근거와 검증

`tools/decomp_spritedraw_oracle.py`는 세 PE의 전체 일반 Draw·타입/footprint·선택 헤더·체력·막대·지도/Kernel 스캔·CRT 몸체를 실행한다. VFX 출력/색 표 선택, 객체 가상 frame/Draw, 건설 활성/자원/내용물/contained 타이머 조회만 명시 경계다. 경계는 Draw의 기대 프레임·배치·선·막대를 계산하지 않는다. 출력 명령열과 선택 SID/alias 쓰기, 전체 raw/타입/SHP/지도/프로세스 보존, ABI·x87·SEH, 허용 실행/쓰기와 SHA/진입/정상 반환/경계 집계를 감사한다.

각 PE **463개**, 새 독립 원본 **1,389개**·누적 **513,123개**, 두 x87 정상 반환 **2,778회**다. 모든 관찰 명령열이 같았다. finite 정상 반환 입력을 검증했으며 NaN/무한대·64비트 절삭 범위 밖 좌표/시각은 호스트 안전 계약으로 거부한다.

새 C++ **12개**, Release 경고/오류 **0**·CTest 내부 **758개·실패 0**(153.83초, 전체 153.86초), 근거 감사 **114종**·새 변이 **11개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과). 집중 검사 **12개·실패 0**. 새 변이는 그림자 bit/경계·화면 WORD·WORD 소유자·가상 -1·선 길이·HP 색 경계·Kernel 슬롯 0·그림자 소스/대상 치환·표면 보정·공통 Draw 연결의 차이를 모두 검출했다. 초기 변이 검사에서 부족했던 WORD 입력과 실패 후 빈 결과 접근을 보강하고 최종 전체 검사를 다시 통과했다. 최종 미검출/건너뜀 **0개**, 사본 소스 바이트 복구 완료.

실제 세 아카이브에서 **318개 블록 바인딩**·모든 물리 선택 헤더·논리 개수/gumpFrame·현재 alias를 검사하고 정상 SHP **297개**(109/94/94)의 원색 픽셀을 기존 `DrawShape`와 대조했다. 소유자/명시 그림자 색, 투명/불투명 0·255, 클립/캐시/선 끝점/빈 막대는 별도 C++ 검사로 확인한다. 297개 픽셀 검사는 **클론의 두 출력 경로 비교**이며 원본 게임 화면 캡처 일치의 근거는 아니다.

로그: `extracted/spritedraw-{export,export-cd,smoke,oracle,build,focused,ctest,audits,mutations,assets}.log`. 내보내기: `extracted/spritedraw/{originals,originalCD,original1037}/`. 원본/AGENTS.md·dotnetpj는 변경하지 않았고 커밋/푸시는 없다.

## 남은 연결

게임 조회/가상 frame/Draw·실제 플레이어/명시 색 표·추가 상태 타입 필드/ProcessKernel 공급과 전체 Client/GameWorld 프레임은 후속이다. 초기화하지 않은 색/조회 경계를 조용히 생략하지 않고 사용하는 분기에서 예외로 알린다. `MakeSpriteAssetViews`가 +0x1b8/+0x1bc/+0x1cc까지 자산에서 자동 준비했다는 뜻은 아니다.

다음은 공통 kind 2의 **Gump 실제 Draw `00466310`/CD `00492aa0` 및 생성**, 일반/미리보기 자산 상태 공급·입력/클릭 선택, SP lazy 전역, 구름/배경/dirty/장치, 건설 SHP/팔레트·준비 소리/contained·건설 GUI다. **게임 내 건설/미션 완주는 아직 불가능하다.**
