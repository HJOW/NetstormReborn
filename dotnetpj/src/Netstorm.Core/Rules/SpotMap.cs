using Netstorm.Assets;

namespace Netstorm.Core.Rules;

/// <summary>원본 두 판본의 중복 칸 처리 방식 (점유 충돌 시 경로가 다르다)</summary>
public enum SpotEdition
{
    /// <summary>패치 10.78: 이미 겹친 비트가 있으면 그 칸에서 즉시 반환한다 (앞 칸의 OR는 남는다)</summary>
    Patch1078,

    /// <summary>CD 10.72: 겹쳐도 OR와 등록을 계속한다</summary>
    CD1072,
}

/// <summary>점유 등록 결과 (원본 Pop의 spot 경로만 본 것이다)</summary>
public enum SpotRegistration
{
    /// <summary>발자국 전체를 OR 해 등록했다</summary>
    Registered,

    /// <summary>패치판의 중복 칸에서 중단했다 (앞 칸의 OR는 남는다). 배치 가능 여부 판정이 아니다</summary>
    Overlap,

    /// <summary>점유를 쓰지 않는 타입이라 지도를 바꾸지 않았다</summary>
    Skipped,
}

/// <summary>
/// 점유(spot) 지도: 256×256 바이트에 타입 플래그 2 의 하위 바이트를 OR 해 칸의 점유를 적는다.
/// 원본 10.78 의 Pop(004b02d0)·Unpop(004afe50) 가운데 spot 읽기·쓰기 경로만 옮겼다
/// (docs/exe/cpp-spatial-reconstruction.md. 기준 구현은 cpppj/src/o/SquidSpatial.cpp).
/// 한 칸에 적힐 값은 <see cref="SpotRules.EffectiveGenus"/> 의 하위 바이트다.
/// 해시 버킷·표면 word·화면/첫 등록/공통 후처리 사건·영역 조회는 아직 옮기지 않았으므로
/// spatial-x86.tsv 전체(객체·머리·사건 포함)로는 검증하지 않고, 발자국 OR·AND와 판본별 중복 경로·범위 보호만 검사한다.
/// 세션의 <c>BattleMap._occupied</c> 와 배치 판정(원본 0049b510)에는 아직 연결하지 않았다.
/// </summary>
public sealed class SpotMap
{
    /// <summary>월드 한 변의 칸 수 (원본 256×256 바이트 지도)</summary>
    public const int WorldCells = 256;

    /// <summary>spot 쓰기 대상에 들어가는 타입 플래그 2 마스크 (원본 005424b8 건물군 0x50444200 과 0x400ff 의 합집합)</summary>
    public const uint SpotWriteMask = 0x504442FF;

    /// <summary>타입 플래그 1 에서 표면을 뜻하는 비트 (발자국이 없어도 spot 경로를 탄다)</summary>
    public const uint SurfaceFlag1 = 0x800;

    /// <summary>판본별 중복 칸 처리 방식</summary>
    private readonly SpotEdition _edition;

    /// <summary>칸별 점유 바이트 (y * 256 + x 순서, 원본과 같은 행 우선)</summary>
    private readonly byte[] _spots = new byte[WorldCells * WorldCells];

    /// <summary>
    /// 빈 점유 지도를 만든다.
    /// </summary>
    /// <param name="edition">중복 칸 처리 방식 (기본은 패치 10.78)</param>
    /// <param name="initial">이미 존재하는 점유 바이트 (없으면 빈 지도. 있으면 65,536 바이트여야 한다)</param>
    public SpotMap(SpotEdition edition = SpotEdition.Patch1078, byte[]? initial = null)
    {
        _edition = edition;
        // 초기 지도가 있으면 크기를 확인하고 그대로 복사한다.
        if (initial != null)
        {
            if (initial.Length != _spots.Length)
            {
                throw new ArgumentException("초기 점유 지도는 256×256 바이트여야 합니다", nameof(initial));
            }
            Array.Copy(initial, _spots, _spots.Length);
        }
    }

    /// <summary>
    /// 이 오브젝트가 발자국에 spot 을 쓰는지 (원본 SquidSpatial::WritesSpots).
    /// 건물군·표면 타입이면서 매몰·포함 상태가 아닐 때만 쓴다.
    /// </summary>
    /// <param name="flags1">타입 플래그 1</param>
    /// <param name="flags2">타입 플래그 2</param>
    /// <param name="buried">매몰 상태 (원본 부가 바이트의 buried 비트)</param>
    /// <param name="contained">포함 상태 (원본 상태 바이트의 contained 비트)</param>
    public static bool WritesSpots(uint flags1, uint flags2, bool buried, bool contained) =>
        ((flags2 & SpotWriteMask) != 0 || (flags1 & SurfaceFlag1) != 0) && !buried && !contained;

    /// <summary>칸의 현재 점유 바이트를 읽는다.</summary>
    /// <param name="x">칸 x (0~255)</param>
    /// <param name="y">칸 y (0~255)</param>
    public byte At(int x, int y)
    {
        // 지도 밖 칸의 읽기는 원본 배열 밖 접근이므로 거부한다.
        if (x < 0 || y < 0 || x >= WorldCells || y >= WorldCells)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "점유 지도를 읽을 칸이 범위를 벗어났습니다");
        }
        return _spots[y * WorldCells + x];
    }

    /// <summary>점유 바이트 전체를 읽기 전용으로 본다 (y * 256 + x 순서).</summary>
    public ReadOnlySpan<byte> Spots => _spots;

    /// <summary>
    /// 발자국에 점유 값을 OR 해 등록한다 (원본 Pop 의 spot 경로).
    /// </summary>
    /// <param name="x">오브젝트 기준점 x 좌표 (잘못된 유한 좌표는 원본처럼 10 으로 복구한다)</param>
    /// <param name="y">오브젝트 기준점 y 좌표</param>
    /// <param name="width">발자국 가로 칸 수 (1~256)</param>
    /// <param name="height">발자국 세로 칸 수 (1~256)</param>
    /// <param name="flags1">타입 플래그 1</param>
    /// <param name="flags2">타입 플래그 2</param>
    /// <param name="buried">매몰 상태</param>
    /// <param name="contained">포함 상태</param>
    public SpotRegistration Register(float x, float y, int width, int height, uint flags1, uint flags2,
        bool buried = false, bool contained = false)
    {
        // 점유를 쓰지 않는 타입은 좌표 검증 없이 지도를 바꾸지 않는다.
        if (!WritesSpots(flags1, flags2, buried, contained))
        {
            return SpotRegistration.Skipped;
        }
        (float anchorX, float anchorY, int left, int top, int right, int bottom) = Footprint(x, y, width, height);
        // 위쪽 행부터 왼쪽 칸 순서로 갱신한다 (원본과 같은 행 우선).
        for (int cellY = top; cellY <= bottom; cellY++)
        {
            // 오른쪽 아래 기준점까지 각 칸을 갱신한다.
            for (int cellX = left; cellX <= right; cellX++)
            {
                // 실제 적는 값은 EffectiveGenus 의 하위 바이트다.
                byte genus = (byte)SpotRules.EffectiveGenus(flags2, anchorX, anchorY, width, height, cellX, cellY);
                if (genus == 0)
                {
                    continue;
                }
                ref byte spot = ref _spots[cellY * WorldCells + cellX];
                // 패치판은 이미 겹친 비트가 있으면 앞 칸의 OR를 남기고 즉시 반환한다.
                if (_edition == SpotEdition.Patch1078 && (spot & genus) != 0)
                {
                    return SpotRegistration.Overlap;
                }
                spot = (byte)(spot | genus);
            }
        }
        return SpotRegistration.Registered;
    }

    /// <summary>
    /// 발자국의 점유 값을 AND 로 해제한다 (원본 Unpop 의 spot 경로).
    /// 겹쳐 있던 다른 오브젝트의 같은 비트도 지울 수 있다 (원본 그대로).
    /// </summary>
    /// <param name="x">오브젝트 기준점 x 좌표 (잘못된 유한 좌표는 원본처럼 10 으로 복구한다)</param>
    /// <param name="y">오브젝트 기준점 y 좌표</param>
    /// <param name="width">발자국 가로 칸 수 (1~256)</param>
    /// <param name="height">발자국 세로 칸 수 (1~256)</param>
    /// <param name="flags1">타입 플래그 1</param>
    /// <param name="flags2">타입 플래그 2</param>
    /// <param name="buried">매몰 상태</param>
    /// <param name="contained">포함 상태</param>
    public bool Unregister(float x, float y, int width, int height, uint flags1, uint flags2,
        bool buried = false, bool contained = false)
    {
        // 점유를 쓰지 않는 타입은 지도를 바꾸지 않는다.
        if (!WritesSpots(flags1, flags2, buried, contained))
        {
            return false;
        }
        (float anchorX, float anchorY, int left, int top, int right, int bottom) = Footprint(x, y, width, height);
        // 원본과 같은 행/열 순서로 해제한다.
        for (int cellY = top; cellY <= bottom; cellY++)
        {
            // 실제 해제하는 값도 등록 때와 같은 하위 바이트다.
            for (int cellX = left; cellX <= right; cellX++)
            {
                byte genus = (byte)SpotRules.EffectiveGenus(flags2, anchorX, anchorY, width, height, cellX, cellY);
                ref byte spot = ref _spots[cellY * WorldCells + cellX];
                spot = (byte)(spot & ~genus);
            }
        }
        return true;
    }

    /// <summary>
    /// 등록 루프의 정수 칸 범위. EffectiveGenus 내부의 소수 기준점 경계와는 별도다.
    /// 원본 SquidSpatial::Footprint: 오른쪽 아래 기준점을 먼저 자르고 왼쪽·위쪽을 뺀다.
    /// </summary>
    /// <param name="x">기준점 x 좌표</param>
    /// <param name="y">기준점 y 좌표</param>
    /// <param name="width">발자국 가로 칸 수</param>
    /// <param name="height">발자국 세로 칸 수</param>
    private static (float AnchorX, float AnchorY, int Left, int Top, int Right, int Bottom) Footprint(
        float x, float y, int width, int height)
    {
        // 유한하지 않은 좌표는 원본 배열 밖 쓰기가 되므로 거부한다.
        if (!float.IsFinite(x) || !float.IsFinite(y))
        {
            throw new ArgumentOutOfRangeException(nameof(x), "점유 값을 구할 좌표가 유한한 값이 아닙니다");
        }
        // 발자국 크기는 월드 안에 들어야 한다.
        if (width < 1 || height < 1 || width > WorldCells || height > WorldCells)
        {
            throw new ArgumentOutOfRangeException(nameof(width), "점유 발자국의 크기가 범위를 벗어났습니다");
        }
        // 원본은 잘못된 유한 좌표를 (10, 10) 으로 복구한다 (0 이하·256 이상).
        if (x <= 0 || y <= 0 || x >= WorldCells || y >= WorldCells)
        {
            x = 10;
            y = 10;
        }
        // 등록 루프는 float 절삭 뒤의 정수 기준점에서 왼쪽·위쪽을 구한다.
        int right = (int)x;
        int bottom = (int)y;
        int left = right - width + 1;
        int top = bottom - height + 1;
        // 지도 밖 발자국은 원본 assert 이후의 배열 밖 쓰기가 되므로 거부한다.
        if (left < 1 || top < 1 || right >= WorldCells || bottom >= WorldCells)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "점유 발자국이 지도 밖에 있습니다");
        }
        return (x, y, left, top, right, bottom);
    }
}
