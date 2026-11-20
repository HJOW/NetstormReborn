# 미션 장면 음악 연결 (WB04-E1119)

미션이 시작돼도 메인 메뉴 곡(`ser22.mus`)이 계속 재생되던 것을, 원본처럼 원소 곡(`wind22`·`rain22`·`thu22`·`sun22`)을 무작위 곡부터 순환 재생하도록 앱에 연결했다. 곡 선택·기한 규칙은 [E1059 장면 음악 감독](webpj-scene-music-20261120.md)(`audio/scene-music.js`, 독립 관측 3,113행 대조)을 그대로 쓰고, 이 단위는 그 감독을 앱의 Web Audio 재생기·미션 음악 적재기·벽시계·미션 수명에 잇는 구동기 `audio/scene-music-driver.js` 를 추가했다.

## 원본의 호출 지점과 웹의 대응

| 원본(cpppj) | 웹 |
| --- | --- |
| 시작할 때 `StartScene(false)`(메뉴 곡) | 기존 `_playMenuMusic()` 경로 그대로(미션 중에는 구동기가 켜져 있으면 다시 틀지 않음) |
| State7 `ownerMode.battle=true; StartScene(true)` — World 설치보다 앞 | `_runCampaignState7` 몸체에서 `_beginSceneMusic()` → `SceneMusicDriver.begin()` |
| 메인 루프 `SceneFrame()`(미니맵·Gump·Kernel 앞) | `_frame()` 의 `recount()` 다음 `this._sceneMusic?.frame()` |
| 미션을 떠날 때 `StartScene(false)` | `_releaseCampaignWorld()` 맨 앞 `_endSceneMusic()` → `end()`(World 가 없어도 호출) |

- 곡 길이는 파일을 열어 재는 대신 `MUSIC_DATA_BYTES`(각 곡 WAV data 청크 크기)로 원본 `SoundMusic` 과 같은 식 `길이/(비트×0.125)/빈도/채널` 에서 구한다. 시험이 공개 자산 9곡의 파일 머리를 직접 읽어 크기와 식의 결과를 정확히 대조한다(원소 곡 200~233초, fanfare 21.56초).
- 곡이 시작되면 감독이 정한 다음 원소 곡(`색인+1`)을 미리 받아 곡이 바뀔 때 끊김이 없게 한다. 해독 버퍼는 한 곡에 약 35MB(22,050Hz·2채널·float32)라 지금 곡·다음 곡·메뉴 곡 말고는 `MissionAudioAssets.releaseMusic()` 으로 놓는다(미션 뒤에는 메뉴 곡만 남음).
- 곡 선택 난수는 월드 난수열을 건드리지 않도록 따로 쓴다(`Math.random`). 원본은 게임 난수(`00532710`)를 한 번 소비한다 — 게임 난수 공유는 L2/L3 상태 대조 때 맞출 항목으로 남긴다.
- 천둥 곡(`thu22.mus`)이 시작될 때 날씨 효과음 `thunderCrack.wav` 를 낸다(미션 효과음 준비에서 이미 받음).

## 아직 연결하지 않은 것

- 희생 의식 곡(`sacrifice.mus`)·대기실 곡(`anticipation.mus`)과 원소 팔레트·번개 섬광(원본은 옵션/월드 부품 필요), 승리·패배 곡의 요청 연결(`fanfare.mus`·`defeat.mus` 재생은 E1061, 요청은 미션 완료 흐름과 함께).
- 곡이 바뀔 때 원본은 같은 채널에서 교체하지만 웹은 새 소스를 시작하고 이전 소스를 멈춘다(`AudioSystem.playMusic`, 짧은 겹침 없음).
- 실제 청취 판정(곡 이어짐·음량)은 사용자 확인 범위다.

## 검증

- `tests/scene-music-driver.test.mjs`(Node 11건): 곡 길이 표가 실제 파일 머리와 같음, 난수→시작 곡 색인 순환(2500→sun22, 1500→thu22+천둥 효과음, 3999→wind22 감김), 받은 뒤 재생·다음 곡 미리 받기, 기한 직전/직후 전환과 곡마다 다른 길이, 지난 곡 놓기, `end()` 의 메뉴 곡 복귀·버퍼 정리·중복 호출, 받는 중 떠남·재생기 닫힘·받기 실패(오류 알림)·이미 해독됨·장치 누락 거부.
- `tests/mission-audio-assets.test.mjs`: `releaseMusic` 이 재생기 표와 해독 사본을 모두 비워 다시 받게 하고 없는 곡·반납 뒤에도 안전함.
- 변형 확인: 구동기에 변형 10개를 넣어 9개를 시험이 잡았고 `frame()` 의 `active` 보호만 같은 동작이라 남았다(비활성 때 곡 이름이 항상 메뉴 곡이라 `request` 가 곧바로 반환).
- `e2e/scene-music.spec.js`(Chromium 실제 Tutorial1): 메뉴 곡 재생 → 미션 시작 뒤 원소 곡이 현재 곡(AudioContext running) → 해독 표가 메뉴·현재·다음 곡뿐 → 벽시계를 1000초 앞당기면 다음 곡으로 넘어가고 그다음 곡을 미리 받음 → 음악 설정 껐다 켜도 메뉴 곡이 끼어들지 않음 → 미션을 떠나면 `ser22.mus` 와 메뉴 곡 한 개만 남음. 오류 0.
- PORT_MAP `SoundSceneMusic.cpp` 행의 web 칸에 구동기를 등록하고 `port-map --check` 오류 0.

## E1120 추가: 희생 의식 곡 요청

사제 희생(제단 의식)은 `sacrifice.mus` 를 장면 음악에 요청한다(원본 `CampaignClientServices` 의 `requestMusic` → `ClientAudio::RequestMusic`). 웹은 이 서비스를 연결하지 않아 미션에서 로컬 사제가 희생하면 `희생 의식 실제 음악 요청 미연결` 오류가 났으므로 `services.requestMusic` 을 현재 장면 음악 구동기의 `request(name)` 로 연결했다. 구동기가 없으면(소리 끔·재생기 없음) 조용히 건너뛰고, 미션 장면이 아니면 요청을 무시한다. 같은 곡 요청·결과 곡 잠금·승리 곡 관문은 감독 규칙을 그대로 따른다(결과 상태가 1이 아니면 승리 곡 요청은 거른다).

- 희생 곡은 원소 곡이 아니므로 다음 곡을 미리 받지 않고 지난 원소 곡 버퍼는 놓는다. 기한은 희생 곡 길이(139.92초)를 따르고, 지나면 색인 순환의 다음 원소 곡으로 돌아간다.
- `sacrificing()`(희생 중이면 다음 곡에서 희생 곡을 고르는 판정)은 cpppj 도 월드 연결이 없어 항상 거짓이므로 같게 둔다.
- 시험: `tests/scene-music-driver.test.mjs` 에 `request` 2건(요청 후 받기·재생·버퍼 정리·기한 뒤 복귀, 같은 곡/승리 곡/비미션), `tests/netstorm-gameplay-services.test.mjs` 에 서비스 연결 1건, Chromium `scene-music` 에 실제 파일 `sacrifice.mus` 요청·해독·현재 곡 확인을 더했다. 변형 3개(요청 보호 제거·요청 무시·서비스 제거)를 모두 시험이 잡았다.
