# SoundSceneMusic partial → port: 난수·곡 기한 수정과 남은 차이 판정 (WB04-E1258)

대응표 `cpppj/src/client/SoundSceneMusic.cpp` 행(295줄)에 남아 있던 E1228 감사의 차이 ②~⑥ 와 "남음" 항목을 코드 수정 두 가지와 판정으로 닫아 행을 `partial` 에서 `port` 로 바꿨다(partial 24→23단위, 17,866→17,571줄, port 984).

## 코드 수정 두 가지

### ② State7 첫 곡 선택이 공유 게임 난수를 한 번 소비하지 않음 → 고침

원본 `SceneMusic::Start()` 는 전투 장면이면 `random_.Next(4000)/1000` 으로 시작 원소를 고르는데 이 `random_` 은 **공유 게임 난수**(`o::GameRandom&`, 메뉴 구름·세션이 함께 쓰는 단일 객체)이다. 웹(E1119)은 월드 난수열을 건드리지 않으려고 `Math.random` 을 써 왔지만, 게임 난수 모드 0 은 고정 시드(`FIXED_RANDOM_SEED`)라 미션 이후의 난수열이 원본과 한 칸 어긋난다(AI·생성 무작위의 순서가 달라짐).

- `netstorm.js` `_beginSceneMusic`: `random.next` 가 Client 난수 전역의 `game.next(limit)` 를 쓰게 바꿨다. 난수 전역이 없으면 오류.
- `e2e/scene-music.spec.js` 새 시험(실제 Chromium, Tutorial1): 한 번의 동기 평가 안에서 `_endSceneMusic()` → `_beginSceneMusic()` 을 하며 게임 난수 상태가 **선형 합동 한 단계**(`상태 × 0x10003 + 3`)만 전진함을 확인한다(두 단계는 아님). `Math.random` 으로 되돌리면 실패함을 확인했다. 기존 같은 파일의 천둥 곡 시험은 난수 강제를 `Math.random` 대신 `game.next` 로 바꿨다(`0.3`×4000=1200 → 색인 1 → 다음 곡 thu22).

### ③ 음악을 끈 동안의 곡 기한이 곡 길이 표를 써서 원문 180초와 다름 → 고침

원본 `MusicChannelDuration` 은 주 채널이 파일을 연 적이 없거나 닫았으면 0 을 돌려주고, 감독은 "30초보다 긴 곡만 그 길이를 믿는다. 길이가 0 이면 180초 뒤에 다시 확인한다"(`SoundSceneMusic.cpp:76~84`). 소리·음악 옵션을 끈 상태에서는 파일이 열리지 않으므로 곡 기한(날씨·천둥 주기 포함)이 180초이다. 웹은 켜짐 여부와 관계없이 곡 길이 표(원소 곡 207~233초)를 써 왔다.

- `audio/scene-music-driver.js`: `duration` 훅이 `musicEnabled`·`soundEnabled` 가 모두 꺼져 있지 않을 때만 표의 길이를, 아니면 0 을 돌려준다(속성이 없는 시험 대역은 켜짐으로 본다).
- `tests/scene-music-driver.test.mjs` 새 시험: 켜짐은 180초가 지나도 곡이 이어지고 곡 길이 뒤에 넘어가며, 음악 끔·소리 끔은 179초에는 그대로이고 181초에 다음 곡을 고른다. 변형(항상 표 사용)으로 실패함을 확인했다.

## 나머지 판정

| 남은 차이 | 판정 | 근거 |
| --- | --- | --- |
| ⑤ 희생곡이 끝난 뒤 의식이 진행 중이면 원문은 희생곡 유지(`Next` 의 `sacrificing` 검사) | **도달하지 않음** | `Next()` 의 이 검사는 곡이 끝나는 시각에만 돈다. 희생 의식은 시작 때 `requestMusic('sacrifice.mus')` 로 곡을 바꾸고 기한을 다시 잡으므로 이 검사가 참이 되려면 의식이 희생곡 길이(139.9초) 이상 이어져야 한다. 의식은 몇 초이다. cpppj 도 이 훅을 항상 `false` 로 둔다(`ClientAudio.cpp:28`, 어떤 호출자도 연결하지 않음) |
| ⑥ 희생곡 선택 시 `#trim` 이 곡을 놓아 받기·해독 동안 무음 구간 | **웹 플랫폼 적응** | 웹은 곡을 비동기로 받아 해독한다(원본은 파일을 바로 연다). 미션 안에서 곡 선택은 초 단위로 드물고 무음은 받기·해독 시간뿐이다. 미리 받아 두면 곡당 12MB 가까운 해독 버퍼를 항상 잡아야 해 하지 않았다 |
| 대기실 곡(멀티플레이 전용) | **웹 범위 밖** | `waitingRoom()` 은 멀티플레이 대기실 판정이다 |
| 원소 팔레트(`ascendancyPalette`) | **도달하지 않음** | cpppj `ClientMain.cpp:659` 는 설정 키 `ascendancyPalette == 1` 일 때만 켠다. 메뉴 항목이 없고 전체 영어 대본·번역·설정 자산에 이 키가 없는 숨은 설정(`monitor` 와 같은 부류, E1255)이라 웹에서 켤 방법이 없다 |
| 번개 섬광(`thunderFlash`) | **차이 아님** | 창 모드에서 원본도 화면 변화가 없는 no-op(E1228 판정) |
| ② 외 "게임 난수 공유" | 위 ② 에서 닫음 | |

## 시험

- 새 Node 시험 1건(③), 새 e2e 1건(②)과 갱신한 e2e 1건(천둥). 장면 음악 Node 시험 전부(`scene-music-driver`·`scene-music-native-fixtures`)와 e2e `scene-music.spec.js` 2건 통과.

- 전체 회귀: Node 26.13.0 전체 6,792건 모두 통과(실패 0, 451초, 로그 `extracted/webpj-e1258-node26.log`).

## 남은 일

- 실기 청취는 하지 않았다(재생기의 현재 곡·해독 표·구동기 상태와 난수 상태로 확인).
