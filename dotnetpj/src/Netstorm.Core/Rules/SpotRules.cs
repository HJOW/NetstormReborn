using Netstorm.Assets;

namespace Netstorm.Core.Rules;

/// <summary>
/// 점유(spot) 비트와 공간 해시 단계의 계산. 원본 10.78 의 004afd30(칸별 점유 비트)·004ace40(해시 단계)을 옮겼다
/// (docs/exe/cpp-hash-reconstruction.md. 기준 구현은 cpppj/src/o/Squid.cpp 의 EffectiveGenus, SquidHash.cpp 의 ObjectLevel).
/// 원본은 256×256 바이트 지도에 타입 플래그 2 의 하위 바이트를 OR 해 칸의 점유를 적는다:
/// 0x2 섬, 0x4 다리, 0x8 발자국 안쪽, 0x10 놓기 막음, 0x20 사격 막음.
/// 이 클래스는 한 칸에 적힐 값만 계산한다. 지도에 쓰는 등록·해제와 배치 판정(원본 0049b510)은 아직 옮기지 않았다.
/// 검증은 원본 기계어 기대값 hash-x86.tsv 의 Genus·Level 행으로 한다.
/// </summary>
public static class SpotRules
{
    /// <summary>발자국 안쪽 비트 (spot 비트 8)</summary>
    public const uint InteriorBit = 8;

    /// <summary>
    /// 안쪽 비트를 받는 건물군의 타입 플래그 2 마스크 (원본 005424b8):
    /// 신전·작업장·포대류·제단 받침(dais)·가이저·거주지.
    /// </summary>
    public const uint InteriorGenusMask = 0x50444200;

    /// <summary>지붕이 있는 타입의 플래그 2 비트 (dais). 중심 열의 위쪽 절반은 안쪽 비트를 받지 않는다</summary>
    public const uint RoofGenus = 0x400000;

    /// <summary>발자국 경계 계산에서 좌표에 더하는 값 (원본 float 비트 3f7ff972)</summary>
    public const float CoordinateBias = 0.9999f;

    /// <summary>월드 한 변의 칸 수</summary>
    private const int WorldCells = 256;

    /// <summary>해시 단계 1 의 최대 변 길이 (그림 크기가 이 값 이하)</summary>
    private const float SmallSize = 2;

    /// <summary>해시 단계 2 의 최대 변 길이</summary>
    private const float MediumSize = 4;

    /// <summary>
    /// 오브젝트가 발자국의 한 칸에 적는 점유 값: 타입 플래그 2 에, 건물군이면 "네 변이 아닌 칸"에만 안쪽 비트 8 을 더한다.
    /// 발자국의 오른쪽 아래 칸은 좌표에 0.9999 를 더해 0 쪽으로 자른 칸이다 (x87 처럼 넓은 정밀도로 더한다).
    /// 지붕 비트가 있으면 정확한 중심 열의 위쪽 절반·중앙 칸은 안쪽 비트를 받지 않는다.
    /// </summary>
    /// <param name="flags2">타입 플래그 2</param>
    /// <param name="x">오브젝트 기준점 x 좌표</param>
    /// <param name="y">오브젝트 기준점 y 좌표</param>
    /// <param name="width">발자국 가로 칸 수</param>
    /// <param name="height">발자국 세로 칸 수</param>
    /// <param name="cellX">값을 구할 칸 x</param>
    /// <param name="cellY">값을 구할 칸 y</param>
    public static uint EffectiveGenus(uint flags2, float x, float y, int width, int height, int cellX, int cellY)
    {
        if (!float.IsFinite(x) || !float.IsFinite(y) || x < 0 || y < 0 || x >= WorldCells || y >= WorldCells
            || width < 1 || height < 1 || width > WorldCells || height > WorldCells
            || cellX < 0 || cellY < 0 || cellX >= WorldCells || cellY >= WorldCells)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "점유 값을 구할 좌표·발자국·칸이 범위를 벗어났습니다");
        }
        int right = (int)((double)x + CoordinateBias);
        int bottom = (int)((double)y + CoordinateBias);
        int left = right - width + 1;
        int top = bottom - height + 1;
        bool interior = cellX != left && cellX != right && cellY != top && cellY != bottom;
        bool roofOpening = (flags2 & RoofGenus) != 0
            && (long)(cellX - left) * 2 == right - left && (long)(cellY - top) * 2 <= bottom - top;
        return (flags2 & InteriorGenusMask) != 0 && interior && !roofOpening ? flags2 | InteriorBit : flags2;
    }

    /// <summary>
    /// 오브젝트의 공간 해시 단계 0~3. 섬·다리는 0 이고, 그 밖에는 발자국이 아니라 현재 그림(SHP 추가 헤더)의 크기로 정한다:
    /// 가로·세로 최댓값이 2 이하면 1, 4 이하면 2, 그보다 크면 3.
    /// </summary>
    /// <param name="flags2">타입 플래그 2</param>
    /// <param name="frameWidth">그림의 칸 단위 가로 크기 (<see cref="ShapeDatabase"/> 추가 헤더의 첫 값)</param>
    /// <param name="frameHeight">그림의 칸 단위 세로 크기</param>
    public static int HashLevel(uint flags2, float frameWidth, float frameHeight)
    {
        if ((flags2 & (TypeFlagBits.Island | TypeFlagBits.Bridge)) != 0)
        {
            return 0;
        }
        if (!float.IsFinite(frameWidth) || !float.IsFinite(frameHeight) || frameWidth < 0 || frameHeight < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(frameWidth), "그림 크기가 범위를 벗어났습니다");
        }
        float size = MathF.Max(frameWidth, frameHeight);
        return size <= SmallSize ? 1 : size <= MediumSize ? 2 : 3;
    }
}
