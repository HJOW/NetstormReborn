# 썬 캐논 실제 미사일 포구 연결 (2026-10-16)

현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-16**이다. 보호된 10.78 `originals/Netstorm.exe`만 정적으로 읽었으며 게임이나 GUI를 실행하지 않았다.

## 원본 연결

10.78의 TYPE128 생성자 `0046b810`은 vtable `0050b9b8`을 기록한다. 원본 PE의 가상 함수 슬롯 `0050b9b8 + 0xb4`를 읽으면 `0041c110`이다. 이는 이미 TYPE140 크로스보우의 `0050b6e8 + 0xb4`와 같은 포구 getter이며 현재 자산 raw `+0x0e/+0x12` 좌표 두 DWORD를 그대로 돌려준다. 기존 TYPE71은 다른 vtable의 `0046a780`로 SHP 기준점 둘을 더하는 별도 경로다.

`RawProjectileGeometry::Muzzle`은 PATCH TYPE128에 대해 raw 위치의 float 비트를 그대로 반환한다. 뒤이어 `RawMissile::Construct`가 기존 순서대로 현재 SHP hotspot을 더하고 TYPE hotspot을 뺀다. TYPE140과 TYPE71의 기존 처리는 바꾸지 않았다.

## 검증 범위와 다음 경계

최초 작업에서는 TYPE128용 비트 단위 포구 검사만 추가했고 빌드·테스트·게임/GUI를 실행하지 않았다. 후속 검토에서 원본 getter의 좌표 DWORD 복사 기계어를 다시 확인했으며, Release 전체 빌드 경고0·오류0과 `ProjectileGeometry`4개 전부 통과를 확인했다. 전체 내부 검사는1413개 중1411통과·기존2실패다. 자세한 검증 범위는 [사정선 검토 기록](cpp-suncannon-rayend-20261016.md)을 따른다.

방향 일치 `rayEnd`는 후속 단위에서 연결했다. 현재 전체 Session은 비행 효과11 callback의 사전 검사에서 멈추므로 새 포구 조회까지 도달하지 못한다. 따라서 단위 검사를 실제 썬 캐논 생성·Pop·비행 완료로 해석하지 않는다. 실제 게임/GUI는 이번 검토에서도 실행하지 않았다.
