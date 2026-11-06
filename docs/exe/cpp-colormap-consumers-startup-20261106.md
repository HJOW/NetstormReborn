# 색표 직접 참조와 최초 팔레트 FILE 연결 — 단위968

2026-11-06 / HJOW-Athlon. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이다. 원본·정밀 산출물을 수정하지 않았다. 제품968/11.01이며 우선순위4를 계속한다.

## 원문 대조와 코드 변경

967의 두8행 표와 첫/끝 색표에 대해 모든 PE 섹션에서 비정렬 DWORD 주소 후보를 찾고, 현재 함수 범위의 실제 x86 피연산자와 구분했다. 후보29개 중28개를 명령 피연산자로 확인했다. 나머지1개는 조사 범위의 알려진 명령 경계에서 검증되지 않은 숫자 후보로 보존했다. 두8행의 이번 직접 참조는 초기화 부모43c4e0뿐이고, 마지막 map8도 계산/FILE 부모 외의 직접 참조는 발견되지 않았다. 간접 별칭·계산 주소·모든 런타임 도달 부재의 증명으로 확대하지 않는다. 새로 확인한 별도 소비자가 없어 두8행을 임의 표시 경로에 연결하지 않았다.

반면 첫 map0에는 실제 소비자가 있었다. VT00509b98+a4의00509c3c가 가리키는 **0045bcd0**은 정밀 functions.tsv의 함수 항목에 없지만 원본48BYTE에서 `ret8`까지 확인된다. dead BYTE(+11)의bit2를 검사한 뒤 현재DWORD프레임(+36)과 TYPE/SHP를 사용해555e38을4dab90에 전달한다. 훨리기그 폭탄의 추가 Draw다. 기존 ObjectHitVtables에는 주소가 있었지만 표시 분배의 본문은 없었다.

`RawCampaignFactoryDisplay`에 이 실제 VT/추가 Draw 분배를 연결했다. 공통 `SpriteShapeMode::Bright160`은 기존 Abstract의1.5배표와 구별하며, 같은 Client 캐시9표의0번행을 사용한다. 소스 마스크 아래 **대상 화면**을 치환하는 기존 VFX 커널로 전달한다. 기존 독립 표시 호출만 이미 검증된9표 계산으로0번행을 준비한다. TYPE163 숫자만으로 다른 VT를 대체하지 않는다. **공중3종 제한에 따라 폭탄 생성/행동/추가 Draw의 전용 실행·검사는 하지 않았다. 이 경로는 원문 코드/정적 대조·컴파일 반영이다.**

다른 직접 파일 소비자를 확인하다 최초 팔레트 호출도 찾았다. 원문38dc0은4a4850(name,1)을 호출하지만, 클론은 값형 `files_.Read`와 기본clear0을 사용했다. PATCH 시작을 기존 `LoadNativePaletteFile(files_,palettePath,1)`에 연결해 현재 FILE/키·saved 직접쓰기·적용/후단 뒤 반납을 최초 ColorSort 캡처보다 먼저 수행한다. 첫 loaded0에서는 clear 효과가 자연 생략되므로 ordinary 시작의 화면 변경 자체를 새 결함 재현으로 주장하지 않는다. CD10.72 호스트의 기존 값형 입구는 유지하며 PATCH 주소를 판본 확인 없이 호출하지 않는다.

## 검증

게임/translated 대상의 첫 Release와 최초 일반 픽셀2검사부터 통과했다. 최초 팔레트 수정 뒤 최종 게임/translated/file 대상 Release도 **종료0·경고0·오류0**이다. 마지막 집중은 **고유7검사, 실패0**이다. 새 검사 함수는0개이며 기존1개를 세 모드에서 네 모드로 확장했고 나머지6개는 기존 검사다.

- `TranslatedPixels_`2개: 기존 Native 기대의 일반 커널296표면과 네 일반 Sprite 모드1,184표면, 합계1,480개의 전체 픽셀을 대조했다. Bright160도 같은 커널을 쓰는지 확인하는 일반 합성 RLE/기존 간헐천 자료다. 비행체/폭탄 객체나45bcd0 본문을 호출하지 않는다.
- `PaletteFileRead_`3개: 기존92관측, 현재 TAFF와 snapshot 구분, RGB/reserved BYTE, 부재/잘못된 길이와 적용 전 실패를 확인했다.
- `ScreenPaletteApply_`2개: 기존166관측과 loaded/clear 자연 생략·필수 장치를 확인했다.

정적 도구 준비 때 ranges의 세미콜론 구분을 잘못 가정한 임시 스캔과 ret8의 마지막BYTE를 빠뜨린 끝 경계가 각각 실패했다. 다중 범위 정규식과 exclusive끝0045bd00으로 교정했으며 원문/fixture를 바꾸지 않았다. 메모는 `extracted/cpp-resume-968/audit-preparation-errors.txt`에 있다. 정적 도구는 게임/CPU 에뮬레이터가 아니다.

## 실제 일반 실행과 범위

현재968 제품으로 MCP 한 프로세스에서 **tutorial1/Tutorial 새 진입2회·Replay2회·Continue/Abort·메인 복귀·종료0**을 확인했다. 입력37/관측26·설치자료 불변이며800×600 미션/메인 PNG2장을 확인했다. 생성 캐시2304BYTE의 SHA256은967의 동일 자료와 같은 `ac1205f05c99c526235676c93f06ad6c6a60c49e161e3fef3cd5a695fa1eaad1`이다. 최초 Native 팔레트 FILE의 일반 시작 검증이며 공중 전용/공격/포획/희생/승리·MP·물리 입력 검증이 아니다.

보호2796개/fixture976개 불변·새fixture0이다. 다음 기준은 `extracted/cpp-resume-968/protected-final.json`이다. [정적 주소/명령 보고서](../../cpppj/colormap-reference-static-20261106.json)와 [현재 단위 보고서](../../cpppj/colormap-consumers-startup-20261106.json)를 따른다. 솔루션83/checking8은 불변이다. 전체 CTest/34미션/ZIP은 실행하지 않았다.

전체 FILE/FileSpec/SEH·Factory/파생·비멀티 수명은 미완료다. 다음969는 같은4번의 Factory/Form과 남은 파일/수명 소비자이며, 이번에 별도 구현 대상을 찾지 못한 두8행/마지막 표를 근거 없이 반복 작업으로 만들지 않는다. 새로운 소비 근거가 나오면 다시 조사한다.
