// 지형 스탬프 프레임 선택의 CRT·J 경로를 실제 미션 상수로 고정한다.
using Netstorm.Core.Original;

namespace Netstorm.Core.Tests;

// RawTerrainStamp의 J-case·CRT 경로를 검사하는 클래스다. 2026-11-18 0.615에서 R4-2 지면 프레임 불일치(타입·소유자 100%, 프레임 0%)의 원인 좁히기용으로 추가했다.
// 근거: cpppj/src/o/RawTerrainStamp.cpp FrameAt/Initialize와 L2 `extracted/dotnet-l2-0614` 3경로 증거다.
public sealed class TerrainStampFrameTests
{
    /// <summary>실제 미션의 CRT 상자 시작값이다. 호스트 reseed(고정 시각 1760000000, 모드 1) 뒤 상태이며 3경로 덤프의 CRT 행과 같은 계열이다.</summary>
    private const uint MissionSeedBox = 511160713u;

    /// <summary>위 시작값에서 FrameAt 1회가 소비하는 99회 뒤 상자 값이다. 3경로 덤프 CRT 행(296677442)과 일치한다.</summary>
    private const uint MissionBoxAfter99 = 296677442u;

    /// <summary>수동 검증한 CRT LCG 상수다. 원문 holdrand와 같다.</summary>
    private const uint CrtMultiplier = 0x343fdu;

    /// <summary>수동 검증한 CRT LCG 증가값이다.</summary>
    private const uint CrtIncrement = 0x269ec3u;

    /// <summary>수동 검증한 15비트 마스크다.</summary>
    private const uint CrtMask = 0x7fffu;

    /// <summary>수동 검증한 시작 상자 1의 첫 변형값이다. 1*0x343fd+0x269ec3=2745024, 상위 16비트 0x29=41이다.</summary>
    private const int FirstVariantFromOne = 41;

    /// <summary>수동 검증한 시작 상자 1의 둘째 변형값이다. bump 없음(41%4=1, 18467%4=3)이다.</summary>
    private const int SecondVariantFromOne = 18467;

    /// <summary>합성 J-case 지도에서 중심이 받는 기대 프레임이다. JFirst=0, (5,5), 상자 1에서 V[1]=18467, 합=(18467*2)%99=7, V[7]=29359(bump), 선택=(29359+1)%4=0, 프레임=0+1+2+6+0=9다.</summary>
    private const int ExpectedFrameFromOne = 9;

    /// <summary>합성 스탬프를 만든다. 지면 타입 5에 J-clock 코드만 두어 J-case를 강제한다.</summary>
    /// <param name="crtStart">CRT 상자 시작값이다.</param>
    /// <param name="box">FrameAt 뒤 상자 값을 읽는 보관함이다.</param>
    /// <param name="gameSeed">게임 난수 시작 상태다. J-case는 소비하지 않아야 한다.</param>
    /// <returns>합성 지도에 묶인 스탬프다.</returns>
    private static RawTerrainStamp BuildStamp(uint crtStart, uint[] box, uint gameSeed = 7u)
    {
        var pool = new RawSidPool(24000, false, null, OriginalEdition.Patch1078);
        var types = new OriginalTypeRecord[188];
        // 모든 타입 자리를 빈 기록으로 채우는 반복문이다.
        for (int i = 0; i < types.Length; i++)
        {
            types[i] = new OriginalTypeRecord();
        }
        var frames = new RiftTypeFrames[188];
        // 모든 타입 자리를 빈 프레임 표로 채우는 반복문이다.
        for (int i = 0; i < frames.Length; i++)
        {
            frames[i] = new RiftTypeFrames(Array.Empty<RiftFrameCode>());
        }
        var jcodes = new List<RiftFrameCode>();
        // J-clock 40개를 순서대로 두어 Run('J').First=0으로 고정한다.
        for (int i = 0; i < 40; i++)
        {
            jcodes.Add(new RiftFrameCode((byte)'J', (byte)'P', 0, 0));
        }
        frames[5] = new RiftTypeFrames(jcodes);
        var map = new TerrainStampMap
        {
            Codes = new byte[65536],
            Regions = new byte[65536],
            TypeMapping = new int[256],
            Aliases = new int[256],
            Bounds = [4, 4, 6, 6],
        };
        Array.Fill(map.Codes, (byte)'.');
        // 4~6칸 덩어리를 지면 문자 'x'로 채우는 반복문이다.
        for (int x = 4; x <= 6; x++)
        {
            // 각 열의 행을 채운다.
            for (int y = 4; y <= 6; y++)
            {
                map.Codes[x * 256 + y] = (byte)'x';
            }
        }
        map.TypeMapping['x'] = 5;
        map.Aliases['x'] = 'x';
        box[0] = crtStart;
        var hooks = new TerrainStampHooks
        {
            Create = (_, _) => 100,
            SetOwner = (_, _) => { },
            Pop = (_, _, _, _) => { },
            SetRegion = (_, _) => { },
            ChunkRegion = (_, _) => 0,
            Assertion = _ => { },
        };
        return new RawTerrainStamp(pool, types, frames, map, new GameRandom(gameSeed),
            () => box[0], value => box[0] = value, new TerrainStampState(), hooks);
    }

    /// <summary>실제 미션 상수에서 FrameAt 1회가 CRT를 정확히 99회 소비하는지 확인한다.</summary>
    [Fact]
    public void InitializeConsumes99CrtDraws()
    {
        uint[] box = new uint[1];
        RawTerrainStamp stamp = BuildStamp(MissionSeedBox, box);
        stamp.FrameAt(5, 5);
        Assert.Equal(MissionBoxAfter99, box[0]);
    }

    /// <summary>J-case 프레임이 문서 공식과 같은지 독립 계산 없이 고정값으로 확인한다.</summary>
    [Fact]
    public void JCaseFrameMatchesDocumentedFormula()
    {
        uint[] box = new uint[1];
        RawTerrainStamp stamp = BuildStamp(1u, box);
        Assert.Equal(ExpectedFrameFromOne, stamp.FrameAt(5, 5));
    }

    /// <summary>같은 입력은 같은 프레임을 주고 J-case는 게임 난수를 소비하지 않아야 한다.</summary>
    [Fact]
    public void SameInputsGiveSameFrameWithoutGameRandom()
    {
        uint[] first = new uint[1];
        uint[] second = new uint[1];
        RawTerrainStamp stamp = BuildStamp(1u, first, gameSeed: 7u);
        RawTerrainStamp other = BuildStamp(1u, second, gameSeed: 12345u);
        Assert.Equal(stamp.FrameAt(5, 5), other.FrameAt(5, 5));
    }
}
