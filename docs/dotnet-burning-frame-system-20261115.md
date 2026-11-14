# dotnetpj 0.536 — 연소 실제 프레임·캡처/그리기·삭제/프로세스

2026-11-15, HJOW-Athlon. 직전0.535는502ab30b7다. 기준은 cpppj RawBurningFrame.h/.cpp의e5a198df450ae720da38f5110ebc611119ea2f38, RawBurningLifetimeDraw/BurningVisualSystem의h/.cpp의961f6be53a319c5c9cf78c7cc8eb2fdd007938a7, client/BurningFrameCapture.h/.cpp의ab9361f11f2bc7618251f17d047f6d601c5c08fc, Renderer.h의731c9dee99fdaf98338fc54f33c9d62a886251f6/.cpp의5a2cd047486b18fb1d3978f8bbd4b1dee6924413이다. 디컴파일 산출물은 변경하지 않았다.

## 구현

RawBurningFrame은 capacity0·첫캡처/첫좌표·현재대상·visible·시각차/누적·source/전체capacity입자·두dirty를 이식했다. 캡처는 기존픽셀/행을 반납하고 모든입자를0으로 둔 뒤 현재대상을 진단하며, 거부된 캡처의 이전메타데이터/selected는 원문대로 보존한다. PATCH/CD의 signed life비교·평면위치 덧셈순서·넓은 가속/저장vx 재조회·영역의equal/ordered비교·여백/카메라 감산순서와 원문float저장을 구별한다. 바람은 각입자 가속에서 같은현재owner를 읽는다. 표시밖 nonzero소리는 처음 선택한 프레임에서만 정지하고 미연결은 해당지점에서 실패한다.

RawBurningLifetimeDraw는 실제8색수와 인접DWORD snapshot을 구별해 life9의index8도 실제제공범위에서 읽는다. 좌표 선행정수화→카메라/float저장→색조회→재정수화/clip→5픽셀의 순서를 보존한다. 이동입자만 현재total까지 그리며 source는 제외한다. 실제Type71/128+VT의 발사기캡처·조건부83디스크와캐논+a4빈함수, 첫7slot/이후4프레임중1회의RenderProcess+18 gate도 연결했다. 삭제는 이미지→행→현재소리→입자→등록/base→선택객체 순서이며 공유팔레트를 반납하지 않는다.

BurningVisualSystem은 같은 RenderProcessSystem/Host/Form/Kernel에 실제14를 부착한다. pending/ready 중간상태·단독소유권 이전·중첩Add거부·부분생성실패 반납·현재frame/배율·삭제dirty와토큰반납을 구현했다. C#은 Host.Attach 직전에 pending참조를 비워 원문의 unique_ptr이전/Host실패시Dispose를 대응시킨다. Draw는현재상태를 먼저 값복사한 뒤 clip장치를 호출하고 팔레트는 같은현재owner를 읽는다. 시스템 종료는 다른효과를 건드리지 않고 자기실제client Form만 즉시삭제한다.

Client의 BurningFrameCapture는 기존 실제 RawShapeToBuffer/plain RLE를 같은풀에 연결하며 입력함수사본/소리를 보존한다. 필요한 공통 DirtyRegions도 Renderer에서 분리 이식했다. 자르지않은추가·첫병합·불투명/무시표식·100칸한계·Count0뒤첫flags/카메라목록owner를 보존한다. Renderer.h/.cpp 대응행은 이 변경표 부분을 가리키며 전체Renderer 이식완료를 뜻하지 않는다. 기존 RawShapeToBuffer 전체의base는 올리지 않았다.

## 원본 대조

실제 originals/originalCD의 읽기전용TYPE/SHP와 기존VFX캡처로 양판본 프레임426입력을 대조했다. 기대픽셀을 캡처입력에 공급하지 않는다. 실제판본의로딩목록/TYPE후처리·클러스터/물리헤더로 같은캡처표를 준비하며 이번원본프레임 대상은TYPE70이다. 같은426입력은 독립바람을 반대로 두고 생성후공유owner를 바꿔 한 번 더 실행했다. 현재카메라/영역/깊이/입자전체/시각/메타데이터/캡처픽셀/행/난수/병합dirty전체가 일치했다.

그리기579·소멸64·실제6double gradient/cold12·프레임gate128·발사기88·변경표133도 일치했다. 합계1430고유원본입력/공유바람추가426포함1856실행이다. 기존renderer fixture394행은 이미연결돼 있었고 이번에는Dirty133부분을 새로소비했다. Order261은 기존별도검사다. 새4fixture1297행을 추가했으며 원본/golden/기대값을 변경하지 않았다.

## 실제 수명/현재계약과 검증

실제같은Host/Form/Kernel에서 생성→캡처/단계전환→source/이동입자→5픽셀출력→Host삭제→소리/버퍼/등록반납을 실행했다. 실제부착후입자확보실패/재시도·생성중Kernel/Draw재진입과중첩거부·현재slot/frame gate뒤배율검사·다른효과등록보존·clip장치전카메라사본/현재팔레트·표시밖소리선택을 추가6검사로 확인했다. 소리는핸들요청/순서 관찰이며 실장치재생은 실행하지 않았다.

실제Capture/Frame/Sources 세소비자에 checking상위bit/복귀진단을 연결한3검사는 같은원본전체출력과 원문줄/두차례조회가 유지됨을 확인한다. 실제보호캡처 어댑터의동일풀/함수사본·소리보존과 DirtyRegions의Count0/첫flags/원문100칸한계도 검사했다. 최초전체프레임4·확장원본대조포함28·실제시스템포함최종집중46검사 모두 통과했다. renderer중복추가를막는검사준비guard를 확인하고 기존연결을 재사용했다. 제품/오라클 보정은 없었다.

독립바람사본 사용변이는19검사중2실패, clip뒤카메라사본변이는19검사중1실패로 검출했고 각각소스바이트/SHA를 복원했다. 원복 뒤 전체비증분Release빌드 경고0·오류0, 관련Core205/205·Client41/41은 실패0·건너뜀0·각종료0으로 통과했다. 이번집중결과는 최근전체회귀0.529와 구별한다. 자료는 extracted/dotnet-burning-frame-0536-mutations/와 extracted/dotnet-burning-frame-0536-final/이다(Git제외).

## 남은 제품 조립

제품 HostSession의 공통표시/연소owner와 실제SHPCapture·현재프레임/뷰/팔레트/소리장치·일반burning/사제회복/피해·전체삭제/배치를 연결한다(다음0.537). 현재실제시스템의합성수명/원본대조를 제품미션의표시/플레이 완료로 합산하지 않는다. 전체Renderer/HUD/카메라·메뉴미션시작·brain/다른파생은 계속잔여다.

PORT_MAP846행(L1 414+port432), fixture291개(4개추가). 전체floor/reflected 유지·새외부접속/NB수정없음. 게임창·사제포획/희생·특정적공격·승리·MP 재현없음. 보호원본/CD·AGENTS.md·README개요·cpppj·LEFT_JOBS.md·다른프로젝트 변경없음. 목표active·M1전체0/34·후속단계잔여다.
