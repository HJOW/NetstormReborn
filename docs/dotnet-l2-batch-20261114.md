# dotnetpj 0.499 — 34경로 기준선 일괄 수집과 L2 차이 보고

2026-11-14, HJOW-Athlon. R4의 M1 자료 수집을 완료했으며 **M1 구현 완료는 아니다.** 일반 미션31개를 같은 시드로 실행·대조했고, 데모3개는 cpp 기준선만 확보했다. 새 호스트의 `StartDemo` 진입은 아직 없다. 전체 상태 일치는0/34다.

## 실행과 보호

- 오라클: 제품1310 `cpppj/build/bin/Release/Netstorm.exe`, SHA-256 `cbcf0d0f154fea64301d7e1b7ac4bdb71e39defdbb54f421abdaf1a5e88a8e17`.
- cpppj 변경21파일을 `c909c2dfa..HEAD`로 정적 확인했다.1304~1305는 시험 교정,1306~1310은 CD10.72 분기다. PATCH Player는180바이트·기존 초기화, 메시지와 Start는 기존 PATCH몸체를 유지한다. `ClientMain.InterpretOptions`의 누락 해상도 보존 변경은 이번 명시1024×768 입력에 영향을 주지 않는다. dotnetpj의 CD복원은 범위 밖이며 판본 분기 이식은 해당 파일을 건드릴 때 한다.
- 양쪽 `--fixed-unix-seconds 1760000000 --fixed-frame-ms 25`, 영어·1024×768·창 모드·음소거·빈 사용자 루트. 일반 cpp는 `--mission` 뒤 Play Mission을5/30/60프레임에 선택하고90프레임에 덤프한다. dn은 직접 미션 진입한다. 데모 cpp는5프레임에 `StartDemo,<맵>`을 큐에 넣는다. 각 실행120프레임·종료 코드0, 총65회(일반62회+데모3회).
- 원본 대신 `extracted/dotnet-oracle-data/` 사본을 읽었다. 실제 게임 창을 실행했으며 HJOW-Athlon은 AGENTS 실행 확인 예외PC다. 사제 포획/희생·특정 적 공격·승리·MP재현 명령은 보내지 않았다.
- `originals/`1,394파일의 이름과 SHA-256은 전후 동일하다. 감사 전체는 `extracted/dotnet-l2-0499/originals-audit.json`에 있다. AGENTS·originalCD·README개요·cpppj·LEFT_JOBS.md 미수정.

## 도구와 결과

`dotnetpj/tools/l2_compare.py`는 실행·원본 해시 감사·오라클SHA·명령줄·실행별 로그·world1검사·SID대조를 저장한다. 덤프를 덮어쓰지 않으며 실제 창 실행은 예외PC에서만 허용한다. 실행 없이 기존 자료를 다시 비교할 수 있다.

```powershell
python -X utf8 dotnetpj/tools/l2_compare.py --capture --out extracted/dotnet-l2-0499
python -X utf8 dotnetpj/tools/l2_compare.py --out extracted/dotnet-l2-0499
python -X utf8 -m unittest discover -s dotnetpj/tools -p test_l2_compare.py -v
```

종료0은 전체 L2일치이고1은 차이 또는 미연결이다. 이번 종료1은 예상된 미완료 보고다. 수집 오류0, 일반31경로 대조·데모3경로 대기다. 도구 검사4건이 타입+10BYTE/소유자+34BYTE/프레임+36DWORD·world0거부·중복/손상 슬롯·31+3경로를 검증했다. 상태 바이트와 타입, 프레임 하위 바이트를 구별해 필드 위치 실수를 검출한다.

| 새로 보충한 훈련 미션 | cpp슬롯 | dn슬롯 | 공통 | dn에만 존재 |
| --- | ---: | ---: | ---: | ---: |
| bc5walkers76 | 2358 | 2201 | 2201 | 0 |
| bc6mows76 | 7340 | 7025 | 7025 | 0 |
| bc7sun_bars76 | 5899 | 5835 | 5835 | 0 |
| bc8fury76 | 6509 | 6316 | 6316 | 0 |
| bc9ssun_bars76 | 3973 | 3725 | 3725 | 0 |

전체 결과는 `extracted/dotnet-l2-0499/summary.json`이다(Git제외). 이전의 서로 다른 시드 기준선을 보존하고 별도 폴더에 새로 수집했다.31경로 모두 CRT행296677442이고, Game행·프레임·소유자·장부·프로세스는 잔여 파이프라인 때문에 다르다. tutorial3/4의 dn_only각1개는 이전 단위와 같은 적재 뒤 게임플레이 잔여다. 나머지29경로의 dn_only는0이다. 전체 일치와 구조 부분 일치를 합산하지 않는다.

## 검증과 후속

Release솔루션 빌드 경고0·오류0. 전체 회귀의 Assets366·Client1171은 실패0이고 Core1945개도 완주해 총3482개·실패0·건너뜀0·종료0으로 종결했다(`extracted/dotnet-regression-0499/`, 0.500에서 확인). 제품 코드·PORT_MAP·fixture변화·새 NB수정·외부 접속 변화는 없다.

기준선 확보를 반복하는 단위는 끝났다. 다음은 실제 AI생성자·초기 갱신, 세션 `Bind*`, 요새 이미지·메뉴 미션 시작, Pop후속·농지·장부·표시/그래프 연결이다. 시작 흐름이 같아진 뒤 같은 시드의 기준선으로 M1을 다시 판정한다. 일반 미션31개와 데모3개를 서로 다른 시작 경로로 유지한다.
