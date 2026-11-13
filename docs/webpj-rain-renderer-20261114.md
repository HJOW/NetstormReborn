# webpj 레인 신전 Renderer 합성 검사 (WB04-E397, 2026-11-14)

5-A에서 이월한 cpppj `RainDrawTests.cpp`의 `RainDraw_RendererLayersAndDirtyPixels`와 `RainDraw_RendererFollowingFrameExtendsBounds`를 `webpj/tests/rain-draw.test.mjs`에 옮겼다. 근거는 기준선 `1f00a25a`이며, 해당 시험과 `client/Renderer.cpp/.h`는 기준선 뒤 변경이 없다. 제품 코드 변경 없이 실제 자산을 사용하는 합성 검증을 마쳤다.

## 대조 범위

- 기존 `raindraw` 원본 x86 fixture originals 116행의 가상 Draw 제출 순서 검사는 유지한다. CD 행은 웹 범위 밖이다. 정규화 SHA-256은 `d2a7881326a3d6bd9d551c3c3c97e72074f471db9e9f3f07ea8d06ad4e920fe2`(CD 포함 파일 233행)이다. 새 fixture를 생성하거나 원본 자료를 변경하지 않았다.
- 준비된 `netstorm.tarc`와 `_shapes.shp`에서 `TypeTable`·`ShapeDatabase`·`GameAssets.find('rainVortex')`로 실제 원본 그림을 읽는다. 기대 화면은 Renderer와 공통 그리기 함수를 사용하지 않고 해독한 그림의 기준점·불투명도·화면/clip 경계를 픽셀마다 직접 계산한다.
- 320×240에서 본체 0과 물결 1의 합성, `sceneImage`의 배경 0, 물결 14→2→1 교체와 매번 (9,3) 이동, 삭제 뒤 배경 7 복원을 전체 바이트로 비교한다.
- 10×10에서 본체 1은 화면 밖이고 후속 그림 0만 보이는 원본 입력(48,98)을 비교한다. 후속 프레임이 없을 때는 100바이트 전체가 0임을 추가 확인해 본체만으로 성공하는 검사를 막았다.

## 검증

첫 실행에서 신규 2건과 기존 rain-draw·renderer 집중 검사 **31/31 통과**했다. Node v26.13.0·v22.23.3의 전체 검사도 각각 **3842/3842 통과·실패/건너뜀0**(v26 322.6초·v22 363.9초, 종료 코드0)이다. Node v24는 미설치다. 대응표 검사는 오류0이며 상태 개수는 port644·partial60·todo192로 유지된다. `fixture-manifest --write --only raindraw`로 추적 정보를 갱신했다. 원본/클론 게임, 브라우저 실행이나 청취 검사가 아니다.

## 남은 경계와 다음 작업

5-A의 전용 시험 없이 옮긴 E197·E198·E201 시스템은 실제 Session 조립 시험을 기다린다. 이를 이번 두 합성 시험으로 완료 처리하지 않는다. `ClientRendererFrame`은 `RawCampaignCursorDisplay`·`RawCampaignFactoryDisplay`·`RawMenuControls`·`GameWorld`·`ClientGameplayGlobals`가 선행이다. 다음에는 이 허브가 요구하는 선행 부품을 목록에 펼치고 위에서부터 이식한다. 실제 미션의 L2/L4a·메뉴 Kencloud 연결도 미완료다.

AGENTS.md의 최근 `3ef0a4b9` 변경(별도 실행파일 사용법 문서화)은 cpppj·dotnetpj 대상이며 이번 웹 검사에는 추가 실행파일이 없다.
