# 거주 구역의 현재 복구 상태 공유

WB04-E1391, 2026-11-27, HJOW-Athlon. 완료.

CP1375(`84182644a`)는 Residence의 지면 통지 뒤 기존 지역 입력 대신 같은 Client의 savedGame 전역을 읽도록 고쳤다. 웹도 실제 Owner의 `localStateGlobals.savedGame()` 상자를 World·Contents·AssetPostPopState가 함께 빌리게 했다. 잔여 프레임 계산은 `notifySurface`가 돌아온 뒤 `readSavedGame()`을 호출한다.

공유false도 현재 입력으로 사용한다. 독립 원본 fixture가 쓰던 `residenceAnimation` 필드는 공유 상자가 없을 때의 입력으로 유지한다. 설정의 애니메이션 옵션으로 해석하지 않는다. 기존 프레임/nibble 산식과 통지·Regular·목록 쓰기 순서는 바꾸지 않았다.

새 검사는 통지 전에 false였다가 true가 된 경우와 그 반대, 공유true/legacyfalse·공유false/legacytrue를 모두 구별한다. 현재 프레임17·nibble5는 복구true일 때 잔여3, false일 때17이다. 실제 typed Session에서도 Contents/World와 같은 상자임을 확인하고 현재 값이 양쪽으로 전파되는지 대조했다. 기존 x86 독립 관측은 그대로 유지한다.

- 원본 AssetPostPop·실제 Owner·Session 상태 집중 **60/60 통과**(5.193초), 건너뜀0.
- webpack **3,498,659바이트**. 번들 재현1/1(47.517초)·대응표7/7(126.836초)이 통과했다. 실제 Chromium의4:3 시작 그림360프레임/9초·동일 입력 두 실행·90프레임 화면·90/130프레임 덤프의 원본 대조1경로도 통과했다(2.5분).

RawAssetPostPop.cpp/.h의 이전base`417f22d2` 뒤 변경4삽입/2삭제 전체를 반영해 base를`84182644a`로 갱신했다. RawCampaignSession.cpp도 E1390의 오류 문맥과 이번 동일 상자 연결로 이전base`ffd35d367` 뒤4줄 전체를 처리해`aed29e3d1`로 갱신했다. ClientMain·GameClock의 epoch/최초Capture와 RawCampaignSessionLogin의 재접속SP는 다음 판정 대상이다.

첫 번들 재현 검사는 빌드 완료 전에 이전3,498,464바이트를 읽어 실패했다. 빌드 완료를 확인한 뒤 현재3,498,659바이트로 재검사해 통과했다. 이 순서 교정에서 제품 소스나 기대 바이트를 수정하지 않았다.
