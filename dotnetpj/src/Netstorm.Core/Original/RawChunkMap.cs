using System.Runtime.InteropServices;

namespace Netstorm.Core.Original;

/// <summary>
/// 섬이 없는 셀 번호다(원본 DAT_0050c038 = 0x7f, cpppj <c>o/ChunkMap.h</c>의 <c>kNoIsland</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-7에서 추가했다. <c>FortTerritoryIslandRecord</c>의 signed 비교에 쓴다.</remarks>
public static class ChunkMapShape
{
    /// <summary>섬 없음 표시 바이트다.</summary>
    public const byte NoIsland = 0x7f;

    /// <summary>한 변의 청크 수(고정 16)다.</summary>
    public const int Side = 16;

    /// <summary>청크 한 변의 셀 수(16)다.</summary>
    public const int CellsPerChunk = 16;
}

/// <summary>
/// 청크 하나분의 섬 정보다(원본 12바이트, cpppj <c>o/ChunkMap.h</c>의 <c>ChunkEntry</c>).
/// +0 섬 목록 번호, +1 지역 패턴 칸의 방향 글자, +2 <c>Territory</c> 레코드 6바이트, +8 지역 안의 순서다.
/// </summary>
/// <remarks>2026-11-03 F2c-7에서 추가했다. 필드 순서는 원문 12바이트 배치를 그대로 둔다.</remarks>
[StructLayout(LayoutKind.Sequential, Pack = 1)]
public struct ChunkEntry
{
    /// <summary>섬 목록 번호다(0x7f이면 섬 없음).</summary>
    public byte Island;

    /// <summary>지역 패턴 칸의 방향 글자다.</summary>
    public sbyte Side;

    /// <summary><c>Territory</c> 레코드 6바이트다.</summary>
    [MarshalAs(UnmanagedType.ByValArray, SizeConst = 6)]
    public byte[] Record;

    /// <summary>지역 안에서 몇 번째 칸인가.</summary>
    public int Ordinal;
}

/// <summary>
/// 청크 좌표의 사각 범위다. right·bottom을 포함한다(cpppj <c>o/ChunkMap.h</c>의 <c>ChunkRect</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-7에서 추가했다.</remarks>
public struct ChunkRect
{
    /// <summary>사각의 왼쪽 경계다.</summary>
    public int Left;

    /// <summary>사각의 위쪽 경계다.</summary>
    public int Top;

    /// <summary>사각의 오른쪽 경계다.</summary>
    public int Right;

    /// <summary>사각의 아래쪽 경계다.</summary>
    public int Bottom;

    /// <summary>
    /// 네 경계로 범위를 만든다.
    /// </summary>
    /// <param name="left">왼쪽 경계다.</param>
    /// <param name="top">위쪽 경계다.</param>
    /// <param name="right">오른쪽 경계다.</param>
    /// <param name="bottom">아래쪽 경계다.</param>
    public ChunkRect(int left, int top, int right, int bottom)
    {
        Left = left;
        Top = top;
        Right = right;
        Bottom = bottom;
    }
}

/// <summary>
/// 청크 배치에 쓰는 섬의 소유·영역 목록이다(cpppj <c>o/ChunkMap.h</c>의 <c>IslandList</c>).
/// </summary>
/// <remarks>2026-11-03 F2c-7에서 추가했다.</remarks>
public sealed class IslandList
{
    /// <summary>등록 순서의 섬 정보다.</summary>
    private readonly List<IslandEntry> islands = [];

    /// <summary>
    /// 번호는 더하기 전의 개수다(원본 FUN_0046ee60).
    /// </summary>
    /// <param name="owner">소유자다.</param>
    /// <param name="territory">영역 번호(0~19)다.</param>
    /// <returns>등록된 번호다.</returns>
    public int Add(int owner, int territory)
    {
        islands.Add(new IslandEntry(owner, territory));
        return islands.Count - 1;
    }

    /// <summary>
    /// 범위 안의 번호만 있다(원본 FUN_0046ee30).
    /// </summary>
    /// <param name="island">섬 번호다.</param>
    /// <returns>등록돼 있으면 참이다.</returns>
    public bool Exists(int island)
    {
        return island >= 0 && island < islands.Count;
    }

    /// <summary>
    /// 일반 경로다. 섬이 owner와 territory에 모두 일치하는가(원본 0046f200의 절반 경로).
    /// 원본은 실행 상태에 따라 소유자 비교를 건너뛰기도 하지만 여기서는 일반 경로만 옮겼다.
    /// </summary>
    /// <param name="island">섬 번호다.</param>
    /// <param name="owner">비교할 소유자다.</param>
    /// <param name="territory">비교할 영역이다.</param>
    /// <returns>둘 다 일치하면 참이다.</returns>
    public bool Matches(int island, int owner, int territory)
    {
        if (!Exists(island))
        {
            return false;
        }
        IslandEntry info = islands[island];
        return info.Owner == owner && info.Territory == territory;
    }

    /// <summary>
    /// 등록 순서의 섬 목록이다.
    /// </summary>
    /// <returns>내부 목록의 읽기 전용 보기다.</returns>
    public IReadOnlyList<IslandEntry> Islands()
    {
        return islands;
    }
}

/// <summary>
/// 섬 목록의 한 항목이다(cpppj <c>o/ChunkMap.h</c>의 <c>IslandInfo</c>).
/// </summary>
/// <param name="owner">소유자다.</param>
/// <param name="territory">영역 번호(0~19)다.</param>
/// <remarks>2026-11-03 F2c-7에서 추가했다.</remarks>
public readonly record struct IslandEntry(int Owner, int Territory);

/// <summary>
/// 청크 격자로 나눈 지역의 소속 조회 지도다(cpppj <c>o/ChunkMap</c>).
/// 섬 목록 등록 부분도 함께 둔다.
/// </summary>
/// <remarks>2026-11-03 F2c-7에서 추가했다. <c>FortTerritoryIslandRecord</c>가 같은 y 바깥·x 안쪽 조회에 쓴다.</remarks>
public sealed class ChunkMap
{
    /// <summary>원문 첫 DWORD(mode)다. cleanup/prefix와 현재 consumer가 직접 공유한다.</summary>
    private uint mode = 1;

    /// <summary>X 길이다.</summary>
    private readonly int xLength;

    /// <summary>Y 길이다.</summary>
    private readonly int yLength;

    /// <summary>현재 등록·선거받은 전체 256칸 목록이다.</summary>
    private readonly ChunkEntry[] entries = new ChunkEntry[ChunkMapShape.Side * ChunkMapShape.Side];

    /// <summary>
    /// 가로·세로 청크 수로 만든다(원본 +4, +8). 미션 맵은 16×16이다.
    /// </summary>
    /// <param name="xLength">가로 청크 수다.</param>
    /// <param name="yLength">세로 청크 수다.</param>
    /// <exception cref="ArgumentException">원본 청크 지도 크기 범위를 벗어났다.</exception>
    public ChunkMap(int xLength = ChunkMapShape.Side, int yLength = ChunkMapShape.Side)
    {
        if (xLength < 1 || yLength < 1 || xLength > ChunkMapShape.Side || yLength > ChunkMapShape.Side)
        {
            throw new ArgumentException("Chunk map size", nameof(xLength));
        }
        this.xLength = xLength;
        this.yLength = yLength;
        Reset();
    }

    /// <summary>
    /// 같은 객체의 원문 첫 DWORD를 cleanup/prefix와 현재 consumer가 직접 공유한다.
    /// </summary>
    public uint Mode
    {
        get => mode;
        set => mode = value;
    }

    /// <summary>
    /// 00432360: 현재 mode를 보관하고 1로 둔 뒤 현재 활성 크기만 2바이트씩 지우고 mode를 복원한다.
    /// </summary>
    public void Reset()
    {
        uint saved = mode;
        mode = 1;
        // 원문은 y 바깥·x 안쪽 순서로 현재 활성 크기만 훑는다
        for (int y = 0; y < yLength; y++)
        {
            for (int x = 0; x < xLength; x++)
            {
                // 실제 저장 순서는 x×16+y이며 행 우선 물리로 바꾸지 않는다
                ref ChunkEntry entry = ref At(x, y);
                entry.Island = 0;
                entry.Side = 0;
                entry.Record = new byte[6];
                entry.Ordinal = 0;
                entry.Island = ChunkMapShape.NoIsland;
            }
        }
        mode = saved;
    }

    /// <summary>
    /// 고정 256칸은 현재 폭·높이와 별개이므로 부분 Reset 보존 영역도 그대로 노출한다.
    /// </summary>
    /// <returns>고정 물리 전체다.</returns>
    public Span<ChunkEntry> Entries()
    {
        return entries.AsSpan();
    }

    /// <summary>
    /// 원본 배열은 x×16+y 순서다. 범위 밖 좌표는 원본처럼 가장자리로 당긴다.
    /// </summary>
    /// <param name="x">가로 좌표다.</param>
    /// <param name="y">세로 좌표다.</param>
    /// <returns>해당 칸의 참조다.</returns>
    public ref ChunkEntry At(int x, int y)
    {
        x = Math.Clamp(x, 0, xLength - 1);
        y = Math.Clamp(y, 0, yLength - 1);
        return ref entries[x * ChunkMapShape.Side + y];
    }

    /// <summary>
    /// 섬 번호가 섬 없음이 아니면 섬이 있다(원본 FUN_00432300).
    /// </summary>
    /// <param name="x">가로 좌표다.</param>
    /// <param name="y">세로 좌표다.</param>
    /// <returns>섬이 있으면 참이다.</returns>
    public bool HasIsland(int x, int y)
    {
        return At(x, y).Island != ChunkMapShape.NoIsland;
    }

    /// <summary>
    /// 원본 00432210: 찾지 못하면 (16, 16, 1, 1)이 된다(원본 그대로).
    /// </summary>
    /// <param name="island">찾을 섬 번호다.</param>
    /// <returns>해당 섬 칸들의 사각 범위다.</returns>
    public ChunkRect Bounds(int island)
    {
        int left = ChunkMapShape.Side, top = ChunkMapShape.Side, right = 0, bottom = 0;
        // 지도 전체를 y 바깥, x 안쪽으로 훑는다
        for (int y = 0; y < yLength; y++)
        {
            // 한 줄의 청크다
            for (int x = 0; x < xLength; x++)
            {
                if (At(x, y).Island != island)
                {
                    continue;
                }
                left = Math.Min(left, x);
                top = Math.Min(top, y);
                right = Math.Max(right, x);
                bottom = Math.Max(bottom, y);
            }
        }
        return new ChunkRect(left, top, right + 1, bottom + 1);
    }

    /// <summary>
    /// 가로 청크 수다.
    /// </summary>
    /// <returns>가로 청크 수다.</returns>
    public int XLength()
    {
        return xLength;
    }

    /// <summary>
    /// 세로 청크 수다.
    /// </summary>
    /// <returns>세로 청크 수다.</returns>
    public int YLength()
    {
        return yLength;
    }

    /// <summary>
    /// 원본 00431fc0: 영역 배율을 잡는 범위의 기본값 (1,1)~(15,15)이다.
    /// </summary>
    /// <returns>기본 청크 범위다.</returns>
    public static ChunkRect DefaultChunkRect()
    {
        return new ChunkRect(1, 1, 15, 15);
    }
}
