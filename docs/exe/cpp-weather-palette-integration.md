# 장면 음악의 날씨 팔레트·색 표·화면 갱신 연결

> 후속 갱신(2026-10-10, HJOW-Athlon): [번개 생성·팔레트 진행·정상/중단 복구](cpp-palette-flash-reconstruction.md)를 복원해 천둥 곡과 현재 월드 Kernel에 연결했다. 창 모드는 원본처럼 팔레트를 바꾸지 않는다. 실제 전체화면 표시와 raw 부모 수명은 후속이며 아래 번개 미복원 설명은 앞 단계 이력이다.

2026-10-10, `HJOW-Athlon`. 마지막 디컴파일 수행 PC도 **HJOW-Athlon, 2026-10-10**이다. AGENTS.md·두 인계 문서를 읽었고 현재/마지막 PC가 일치한다. 새 `palettecolor` 목록을 세 판본에서 각각 **2개 함수**씩 읽기 전용으로 내보냈다. 원본 게임과 분석 프로그램은 실행하지 않았다.

장면 음악이 요청하는 화면 갱신·설정 조회·팔레트 적용을 현재 클라이언트에 연결했다. **현재 보존된 10.78/CD 자료에는 `windy.col`, `rainy.col`, `thundery.col`, `sunny.col`이 없다.** 실제 원본 날씨 RGB 재현은 아직 검증할 수 없다. 네 파일이 있는 자료에서는 원본 이름을 그대로 읽으며, 파일이 없는데 `ascendancyPalette=1`을 강제하면 기존 파일 계층의 읽기 예외로 종료한다. 기본 자료에서 이 설정은 꺼져 있다. 검사 팔레트는 Git 제외의 별도 사본에만 생성하고 게임 자산으로 배포하지 않는다.

## 원본과 구현

| 근거 | 구현/검증 범위 |
| --- | --- |
| 색 검색 `004a2820` / CD·10.37 `004246e0` | [`FindPaletteColor`](../../cpppj/src/client/Screen.h), 전체 실제 몸체를 독립 x86으로 실행 |
| 팔레트 로더 `004a4850` / CD `00424760` | 기존 COL/BGRX 읽기·적용에 원소별 네 색 검색을 연결. 전체 로더는 정적 판독 |
| 날씨 적용 `00469c80` / CD `00436960` | 기존 `SceneMusic::ApplyWeather`의 독립 관찰을 유지하며 실제 화면 경계를 연결 |
| 화면 갱신 `0043dad0` / CD `004ee9d0` | 현재 `UberGump`의 재합성 예약·`Renderer::InvalidateAll`에 연결. 원본 가상 clear/resize 전체 복원은 후속 |
| 설정 비교 `00441470("ascendancyPalette",1)` | 설정값이 **정확히 1**일 때만 적용. 효과음·음악 옵션과 별개 |
| 경로 조합 `00459c60` | 클론의 게임 폴더 아래 `DataDir`와 완전한 파일 이름을 사용. `GamePalSpec`은 `.COL`을 덧붙이므로 날씨 이름에는 사용하지 않음 |

색 검색은 논리 팔레트의 R·G·B 세 바이트만 읽는다. 차·곱·합을 32비트로 감아 signed 정수 거리로 해석하고, 이전 최솟값은 float32에 저장한다. **새 정수 거리를 이전 float32 최솟값과 비교한 뒤 저장**하는 x87 순서를 지킨다. 처음 거리 상한은 10,000,000이며, RGB byte 범위에서는 같은 거리의 첫 번호를 유지한다. 범위 밖 RGB/감긴 음수 거리에서는 저장 반올림으로 정수 동률이어도 뒤 번호를 선택할 수 있다. `/fp` 환경 전체의 동치를 주장하지 않으며 기본 nearest 반올림의 두 x87 정밀도에서 대조했다.

원본 로더는 바람·비·천둥·해 순서로 `(255,255,22)`, `(22,22,255)`, `(255,22,22)`, `(181,140,111)`에 가까운 색 번호를 표에 쓴다. `Screen::WeatherTints()`가 이 표를 계산하고 `ClientAudio::SetSceneTints()`가 감독 상태에 복사한다. 시작 메뉴 음악보다 먼저 초기 팔레트의 색 표를 공급한다.

순서는 그대로 유지한다: 화면 갱신 요청 → 설정 조회 → **기존 표의 현재 원소 색을 tint에 대입** → 팔레트 로드/새 색 표 → 갱신 표시 1 → 천둥 경계/효과음. 팔레트 로더는 tint를 덮어쓰지 않는다. `Start` 끝에서는 새 표의 현재 색을 한 번 더 읽는 원본 동작도 유지한다.

`ClientAudio` 생성자가 외부 `SceneMusicHooks`를 받아 실제 음악/효과음 연결과 함께 보관한다. 미연결 기능만 기본값으로 채우며, 팔레트 설정 조회와 로더 중 하나만 주면 생성 단계에서 거부한다. 현재 클라이언트는 화면 갱신·설정·팔레트를 연결하고 희생·대기실·번개 효과는 기본값으로 남긴다.

`UberGump::Frame()`은 커널의 월드 변경과 날씨 갱신 예약을 같은 프레임에 합성한다. `GameWorld::SetPalette()`는 선택 색·지면 명도 계산의 RGB를 바꾸고 팔레트 의존 변환 캐시를 지운다. 새 미션도 직전 미션에서 유지된 현재 팔레트를 받는다. 소프트웨어 커서를 쓰면 새 팔레트로 커서 색을 다시 찾는다(이번 창 검사는 기본 하드웨어 커서).

함께 발견한 **해상도 변경의 색 손실**도 고쳤다. 이전 코드는 새 Screen의 표시 팔레트만 채우고 저장 팔레트는 비웠다. 이어 `SetMode`가 빈 저장 팔레트를 재적용해 대부분 검정으로 덮었다. 현재 RGBQUAD 256개를 `LoadPalette`로 새 장치의 저장/표시 팔레트에 함께 넣어 800×600·640×480 전환에도 색을 보존한다.

팔레트의 **GDI 객체 누수**도 수정했다. 기존 코드가 창 DC에서만 이전 선택을 복원해, 메모리 DC에 남은 팔레트의 `DeleteObject`가 실패했다(실제 Win32 호출에서 반환 0 확인). 메모리 DC의 이전 팔레트를 보관하고 교체/종료 때 두 DC의 선택을 모두 되돌린 뒤 삭제한다. 새 단위 검사는 창 없이 실제 GDI에서 100회 교체 중 객체 수 고정·종료 뒤 시작 값 복귀를 확인한다.

## 검증

- Release 경고/오류 **0**, CTest 내부 **538개·실패 0**(137.63초). 새 검사 5개: 세 PE 색 검색, 논리 팔레트/날씨 표, 실제 ClientAudio 경계·tint 대입 순서, 반쪽 팔레트 연결 거부, 실제 GDI 100회 교체/종료 수명.
- 새 독립 원본 **786개**(판본별 262개), 전체 검색 진입/정상 반환 **1,572회**(두 x87 제어값). cdecl 스택·보존 레지스터·x87 균형·스택 밖 쓰기 거부·입력 팔레트 불변·실제 몸체 범위·스텁/OS 0·입력 전체/순서·SHA를 감사한다. 누적 독립 입력 **454,594개**.
- 원본 근거 감사 **82종 모두 통과**. 기존 장면 음악·위치 효과음 관찰도 변경 없이 통과했다.
- [`cpp_weather_smoke.py`](../../tools/cpp_weather_smoke.py)는 10.78 자료를 Git 제외 검사 폴더에 복사하고, `gifcloud.col`의 채널을 XOR한 **합성 네 팔레트**를 사본에만 생성한다. 무음 실제 클론 창에서 **38개 상태**와 **23개 화면**을 검사했다. 브리핑 정지 중 네 원소 순환, tint의 로드 전/후 순서, 같은 프레임 RGB가 해당 검사 COL의 256색 안에 있음, 설정 1/0/2, 소리·음악 끔, 메뉴 복귀·두 해상도·미션 재진입을 확인했다. 네 실제 날씨 RGB에 대한 검증은 아니다.
- GDI 수정 뒤 날씨/화면 검사를 다시 실행해 통과했다. 기존 `cpp_audio_smoke.py`·`cpp_world_smoke.py`·`cpp_menu_smoke.py`·`cpp_window_smoke.py` 회귀 통과. 10.78/CD 원본 불변과 설정 파일 원상 복원을 확인했다. 소리/날씨 검사 보호 대상 원본 **1,392개**의 내용/존재 상태가 유지됐다. 모든 새 장치/창 검사는 무음이며 사람의 청취는 하지 않았다.

처음 날씨 검사는 네 COL이 없어서 자료 조회에서 실패했다(클론 실행 전). 검사를 별도 사본의 합성 COL로 전환해 연결만 확인하도록 범위를 명시했다. Pillow의 `getdata` 경고 때문에 첫 성공 로그에 PowerShell 오류 레코드가 섞였으므로, RGB 바이트 읽기로 바꾸어 최종 검사를 다시 실행했다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name palettecolor
python -X utf8 tools/decomp_palettecolor_oracle.py --verify
cpppj/build/bin/Release/netstorm_tests.exe --filter ScreenPalette
cpppj/build/bin/Release/netstorm_tests.exe --filter ClientAudio
python -X utf8 tools/cpp_weather_smoke.py
```

로그는 Git 제외 `extracted/palettecolor-{export,oracle,build-gdi,ctest-final,weather-smoke-gdi,audio-smoke,world-smoke,menu-smoke,window-smoke-gdi}.log`·`palettecolor-audits.json`에 있다. 검사 사본은 `extracted/cpp-weather-data-*`, 화면/상태는 `extracted/cpp-audio-smoke/weather-*`에 남긴다.

## 남은 일

네 실제 날씨 COL 확보, 번개 프로세스 `00470d40`·`00470fa0`·프레임/팔레트 복구, 전체 이름 붙은 색 표와 원본 GUI 갱신, `paletteDirty`의 원본 후속 소비, tint를 쓰는 후속 화면 효과가 남는다. 희생/결과/대기실 요청·월드 난수 공유·효과음 호출자/월드 SoundProcess·raw 비표면 객체/GUI·건설·경제·전투·AI·승패도 남으며 미션 완주는 아직 불가능하다. outpost/LAN은 3차, 한국어/D2Coding·화면 요구사항·MCP는 4차를 유지한다.
