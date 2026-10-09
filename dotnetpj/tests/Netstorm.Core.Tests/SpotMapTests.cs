using Netstorm.Assets;
using Netstorm.Core.Rules;

namespace Netstorm.Core.Tests;

/// <summary>
/// 점유 지도(SpotMap)의 발자국 OR 등록·AND 해제와 판본별 중복 경로를 검사한다.
/// 원본 Pop/Unpop 가운데 spot 읽기·쓰기 경로만 옮겼으므로 (cpp-spatial-reconstruction.md),
/// 해시 버킷·표면 word·사건이 필요한 spatial-x86.tsv 전체로는 검증하지 않고
/// cpppj/tests/SpatialTests.cpp 의 경계 검사를 C# 수준으로 옮겼다.
/// </summary>
public sealed class SpotMapTests
{
    /// <summary>다리 1칸 등록은 EffectiveGenus 하위 바이트를 그대로 적는다.</summary>
    [Fact]
    public void Register_WritesGenusLowByte()
    {
        // 빈 지도에 다리 1칸을 등록한다.
        var map = new SpotMap();
        Assert.Equal(SpotRegistration.Registered, map.Register(20, 20, 1, 1, 0, TypeFlagBits.Bridge));
        // EffectiveGenus(bridge) 는 모서리에서도 4 를 돌려준다.
        Assert.Equal(4, map.At(20, 20));
        Assert.Equal(0, map.At(19, 19));
    }

    /// <summary>포대류 3×3 의 가운데 칸만 안쪽 비트를 받는다 (하위 바이트 기준).</summary>
    [Fact]
    public void Register_InteriorCellGetsInteriorBit()
    {
        // 포대류는 발자국의 네 변이 아닌 칸에만 안쪽 비트 8 을 더한다.
        var map = new SpotMap();
        Assert.Equal(SpotRegistration.Registered, map.Register(20, 20, 3, 3, 0, TypeFlagBits.Emplacement));
        // 가운데 칸의 하위 바이트는 0x08 이다.
        Assert.Equal(8, map.At(19, 19));
        // 모서리 칸의 하위 바이트는 0x00 이라 지도가 바뀌지 않는다.
        Assert.Equal(0, map.At(18, 18));
        Assert.Equal(0, map.At(20, 20));
    }

    /// <summary>패치판은 중복 칸에서 앞 칸의 OR를 남기고 중단한다 (C++ Overlap 검사와 같은 배치).</summary>
    [Fact]
    public void PatchOverlap_KeepsPartialChanges()
    {
        // (19, 18) 칸에 미리 다리 비트를 둔다.
        byte[] initial = new byte[SpotMap.WorldCells * SpotMap.WorldCells];
        initial[18 * SpotMap.WorldCells + 19] = 4;
        var map = new SpotMap(SpotEdition.Patch1078, initial);
        // 3×3 다리를 (20, 20) 에 등록하면 (19, 18) 에서 겹친다.
        Assert.Equal(SpotRegistration.Overlap, map.Register(20, 20, 3, 3, 0, TypeFlagBits.Bridge));
        // 먼저 처리한 (18, 18) 의 OR는 남는다.
        Assert.Equal(4, map.At(18, 18));
        // 중복 뒤의 (20, 18) 칸은 처리하지 않았다.
        Assert.Equal(0, map.At(20, 18));
    }

    /// <summary>CD판은 겹쳐도 등록을 계속하고, 해제 때는 겹친 비트도 지운다.</summary>
    [Fact]
    public void CdEdition_ContinuesOnOverlapAndUnregisterClears()
    {
        // 패치 검사와 같은 초기 지도에서 시작한다.
        byte[] initial = new byte[SpotMap.WorldCells * SpotMap.WorldCells];
        initial[18 * SpotMap.WorldCells + 19] = 4;
        var map = new SpotMap(SpotEdition.CD1072, initial);
        // CD판은 중복 칸에서도 끝까지 OR 한다.
        Assert.Equal(SpotRegistration.Registered, map.Register(20, 20, 3, 3, 0, TypeFlagBits.Bridge));
        Assert.Equal(4, map.At(20, 20));
        // 해제하면 미리 있던 비트까지 지운다 (원본 그대로).
        Assert.True(map.Unregister(20, 20, 3, 3, 0, TypeFlagBits.Bridge));
        Assert.Equal(0, map.At(19, 18));
        Assert.Equal(0, map.At(20, 20));
    }

    /// <summary>잘못된 입력은 거부하고, 잘못된 유한 좌표는 (10, 10) 으로 복구한다.</summary>
    [Fact]
    public void RejectsUnsafeInputs_AndRecoversBadCoordinates()
    {
        // 빈 지도에서 시작한다.
        var map = new SpotMap();
        // 발자국 크기가 범위를 벗어나면 거부한다.
        Assert.Throws<ArgumentOutOfRangeException>(() => map.Register(20, 20, 0, 1, 0, TypeFlagBits.Bridge));
        // 점유를 쓰지 않는 상태는 크기 검증 전에 건너뛴다.
        Assert.Equal(SpotRegistration.Skipped, map.Register(20, 20, 1, 1, 0, TypeFlagBits.Bridge, buried: false, contained: true));
        // 유한하지 않은 좌표는 거부한다.
        Assert.Throws<ArgumentOutOfRangeException>(() => map.Register(float.NaN, 20, 1, 1, 0, TypeFlagBits.Bridge));
        // 지도 밖 발자국은 거부한다 (너비 3 으로 x=1 이면 왼쪽이 -1 이다).
        Assert.Throws<ArgumentOutOfRangeException>(() => map.Register(1, 20, 3, 3, 0, TypeFlagBits.Bridge));
        // 0 이하의 유한 좌표는 (10, 10) 으로 복구해 등록한다.
        Assert.Equal(SpotRegistration.Registered, map.Register(0, 20, 3, 3, 0, TypeFlagBits.Bridge));
        Assert.Equal(4, map.At(10, 10));
        // 지도 밖 칸의 읽기도 거부한다.
        Assert.Throws<ArgumentOutOfRangeException>(() => map.At(-1, 0));
        Assert.Throws<ArgumentOutOfRangeException>(() => map.At(256, 0));
    }

    /// <summary>점유를 쓰지 않는 타입·상태는 지도를 바꾸지 않는다.</summary>
    [Fact]
    public void WritesSpots_FiltersSurfaceAndBuriedAndContained()
    {
        // 표면도 아니고 마스크에도 없는 타입은 건너뛴다.
        var map = new SpotMap();
        Assert.False(SpotMap.WritesSpots(0, 0, buried: false, contained: false));
        Assert.Equal(SpotRegistration.Skipped, map.Register(20, 20, 1, 1, 0, 0));
        // 표면 플래그가 있으면 경로는 타지만 하위 바이트가 0 이라 칸은 그대로다.
        Assert.True(SpotMap.WritesSpots(TypeFlagBits.Surface, 0, buried: false, contained: false));
        Assert.Equal(SpotRegistration.Registered, map.Register(20, 20, 1, 1, TypeFlagBits.Surface, 0));
        Assert.Equal(0, map.At(20, 20));
        // 매몰·포함 상태는 표면이어도 쓰지 않는다.
        Assert.False(SpotMap.WritesSpots(TypeFlagBits.Surface, TypeFlagBits.Bridge, buried: true, contained: false));
        Assert.False(SpotMap.WritesSpots(TypeFlagBits.Surface, TypeFlagBits.Bridge, buried: false, contained: true));
        Assert.Equal(SpotRegistration.Skipped, map.Register(30, 30, 1, 1, 0, TypeFlagBits.Bridge, buried: true));
        Assert.False(map.Unregister(30, 30, 1, 1, 0, TypeFlagBits.Bridge, buried: false, contained: true));
    }

    /// <summary>등록한 칸의 값은 EffectiveGenus 하위 바이트와 같다 (표본 대조).</summary>
    [Fact]
    public void Register_MatchesEffectiveGenusLowByte()
    {
        // 다리·섬·포대류와 여러 발자국 크기를 표본으로 본다.
        uint[] flags = [TypeFlagBits.Bridge, TypeFlagBits.Island, TypeFlagBits.Emplacement];
        (int Width, int Height)[] sizes = [(1, 1), (3, 3), (2, 4)];
        // 각 표본을 빈 지도에 등록해 칸마다 비교한다.
        foreach (uint flag in flags)
        {
            // 세 가지 플래그를 순서대로 본다.
            foreach ((int width, int height) in sizes)
            {
                // 세 가지 크기를 순서대로 본다.
                var map = new SpotMap(SpotEdition.CD1072);
                map.Register(40, 40, width, height, 0, flag);
                int right = 40;
                int bottom = 40;
                // 발자국의 모든 칸을 왼쪽·위쪽부터 본다.
                for (int y = bottom - height + 1; y <= bottom; y++)
                {
                    // 한 행의 칸을 왼쪽부터 본다.
                    for (int x = right - width + 1; x <= right; x++)
                    {
                        byte expected = (byte)SpotRules.EffectiveGenus(flag, 40, 40, width, height, x, y);
                        Assert.Equal(expected, map.At(x, y));
                    }
                }
            }
        }
    }
}
