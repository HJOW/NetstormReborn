using Netstorm.Core.Rules;

namespace Netstorm.Core.Original;

/// <summary>
/// 월드 좌표와 크기 단계로 오브젝트 번호의 연결 목록 머리를 찾는 4단계 공간 해시(원본 SquidHash, 10.78).
/// 단계 0~3 은 한 버킷이 월드 1·2·4·16칸 한 변을 맡고, 배열 한 변은 256÷(맡은 칸 수)다(256·128·64·16).
/// 버킷 값은 그 칸에 놓인 첫 오브젝트의 슬롯 번호(16비트)이며 0 이면 비어 있다. 체인의 다음 번호는 각 슬롯의 +4 에 있다.
/// 이 클래스는 머리 배열과 주소 계산만 맡는다. 체인에 넣고 빼는 Pop·Unpop 은 <see cref="RawUnpop"/> 같은 호출자의 몫이다.
/// 월드 좌표는 0 방향으로 절삭한 뒤 단계 크기로 나눈다(원본 004b2a90). 유한하지 않거나 0 미만 또는 256 이상인 좌표는 예외다.
/// 선택 단계(Select)와 칸 좌표 조회(At)는 표면 탐색기를 옮길 때 더한다.
/// 근거: docs/exe/cpp-hash-reconstruction.md, cpppj/src/o/SquidHash.cpp (W02 Unpop 조각, 2026-10-16).
/// </summary>
public sealed class RawSquidHash
{
    /// <summary>단계의 수.</summary>
    public const int Levels = 4;

    /// <summary>월드 한 변의 칸 수.</summary>
    public const int WorldCells = SpotMap.WorldCells;

    /// <summary>단계별로 버킷 하나가 맡는 월드 칸 한 변(원본 005424f8).</summary>
    private static readonly int[] Scales = [1, 2, 4, 16];

    /// <summary>단계별 머리 배열. 인덱스는 y 가 바깥, x 가 안쪽인 행 우선이다.</summary>
    private readonly ushort[][] _heads = new ushort[Levels][];

    /// <summary>모든 머리가 0 인 해시를 만든다.</summary>
    public RawSquidHash()
    {
        // 단계마다 배열 한 변의 제곱만큼 머리를 확보한다
        for (int level = 0; level < Levels; level++)
        {
            int side = Side(level);
            _heads[level] = new ushort[side * side];
        }
    }

    /// <summary>모든 단계의 머리를 0 으로 지운다(원본 초기화).</summary>
    public void Reset()
    {
        // 큰 표면 배열부터 작은 일반 공간 배열까지 모두 지운다
        foreach (ushort[] heads in _heads)
        {
            Array.Clear(heads);
        }
    }

    /// <summary>단계의 배열 한 변(칸이 아니라 버킷 수). 단계가 0~3 밖이면 예외다.</summary>
    /// <param name="level">단계 0~3.</param>
    public static int Side(int level) => level >= 0 && level < Levels
        ? WorldCells / Scales[level]
        : throw new ArgumentOutOfRangeException(nameof(level), level, "공간 해시 단계는 0~3 입니다");

    /// <summary>
    /// 표시·일반 조회가 쓰는 최소 단계. 섬·다리 부류(플래그2 0x2·0x4)는 0이며,
    /// 그 밖은 현재 프레임의 가로·세로 크기 가운데 큰 값으로 1·2·3을 정한다(원본 SquidHash::ObjectLevel).
    /// 크기가 유한하지 않거나 음수이면 예외다. W02 Frame 조각(2026-10-20)에서 더했다.
    /// </summary>
    /// <param name="flags2">타입 플래그2.</param>
    /// <param name="frameWidth">현재 프레임의 가로 크기.</param>
    /// <param name="frameHeight">현재 프레임의 세로 크기.</param>
    public static int ObjectLevel(uint flags2, float frameWidth, float frameHeight)
    {
        // 섬·다리 부류는 표시 단계를 세지 않는다
        if ((flags2 & 0x6) != 0)
        {
            return 0;
        }
        if (!float.IsFinite(frameWidth) || !float.IsFinite(frameHeight) || frameWidth < 0 || frameHeight < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(frameWidth), "해시 프레임 크기가 범위를 벗어났습니다");
        }
        float size = Math.Max(frameWidth, frameHeight);
        return size <= 2 ? 1 : size <= 4 ? 2 : 3;
    }

    /// <summary>월드 좌표가 가리키는 버킷의 배열 인덱스. 좌표는 0 방향으로 절삭한 뒤 단계 크기로 나눈다.</summary>
    /// <param name="level">단계 0~3.</param>
    /// <param name="x">월드 x 좌표.</param>
    /// <param name="y">월드 y 좌표.</param>
    public static int BucketIndex(int level, float x, float y)
    {
        int side = Side(level);
        if (!float.IsFinite(x) || !float.IsFinite(y) || x < 0 || y < 0 || x >= WorldCells || y >= WorldCells)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "공간 해시 월드 좌표가 범위를 벗어났습니다");
        }
        int scale = Scales[level];
        return ((int)y / scale) * side + (int)x / scale;
    }

    /// <summary>
    /// 월드 좌표가 가리키는 버킷의 머리. 값은 점유가 아니라 체인의 첫 오브젝트 번호다.
    /// 참조로 돌려주므로 대입하면 머리가 바뀐다.
    /// </summary>
    /// <param name="level">단계 0~3.</param>
    /// <param name="x">월드 x 좌표.</param>
    /// <param name="y">월드 y 좌표.</param>
    public ref ushort Bucket(int level, float x, float y) => ref _heads[level][BucketIndex(level, x, y)];

    /// <summary>
    /// 단계의 모든 머리를 읽기 전용으로 본다(배열 순서). 표면 조회는 0 단계를 읽는다.
    /// </summary>
    /// <param name="level">단계 0~3.</param>
    public ReadOnlySpan<ushort> Entries(int level)
    {
        Side(level);
        return _heads[level];
    }

    /// <summary>
    /// 버킷 번호가 가리키는 머리. 탐색기가 쓰는 4단계 해시·적 raw 체인의 연결 고리다.
    /// 참조로 돌려주므로 대입하면 머리가 바뀐다. 원본 SquidHash::Cell 과 같다 (W02 탐색 조각, 2026-10-20).
    /// </summary>
    /// <param name="level">단계 0~3.</param>
    /// <param name="x">버킷 x 번호.</param>
    /// <param name="y">버킷 y 번호.</param>
    public ref ushort Cell(int level, int x, int y)
    {
        int side = Side(level);
        if (x < 0 || y < 0 || x >= side || y >= side)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "공간 해시 버킷 번호가 범위를 벗어났습니다");
        }
        return ref _heads[level][y * side + x];
    }
}
