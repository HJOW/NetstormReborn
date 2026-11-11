using Netstorm.Assets;
using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>호스트 세션이 실제 타입 정의의 클러스터 코드를 프레임 표에 채우는지 검사한다.
/// 근거: RiftTypeTable의 자산 번호 = 70 + 적재 순서와 TypeCluster.Code. (2026-11-12 0.442에서 추가했다)</summary>
public sealed class HostWorldTypeFillTests
{
    /// <summary>실제 파일에서 정의 목록을 읽어 프레임 표를 채운다.</summary>
    private static (HostSession Session, IReadOnlyList<RiftTypeDefinition> Definitions) LoadRealDefinitions()
    {
        string? data = GameDataLocator.FindDataDirectory(); Assert.NotNull(data);
        var startup = new HostStartupScene(data);
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        IReadOnlyList<RiftTypeDefinition> definitions = MissionFortLoad.LoadTypeDefinitions(startup.Files);
        return (session, definitions);
    }

    /// <summary>실제 정의로 프레임 표를 채우면 자산 칸에 코드가 들어가고 나머지는 비는지 확인한다.</summary>
    [Fact]
    public void FillWorldFramesMapsClustersToAssetNumbers()
    {
        var (session, definitions) = LoadRealDefinitions();
        Assert.Equal(116, definitions.Count);
        session.FillWorldFrames(definitions);
        Assert.Equal(HostSession.WorldFrameCount, session.WorldFrames.Length);
        // 다리(적재 순서 12번·타입 82번)는 실제 클러스터 코드를 가진다.
        Assert.True(session.WorldFrames[82].Codes.Count > 0);
        // 자산 밖 번호(0~69·186~187)는 빈 표로 둔다.
        for (int number = 0; number < 70; ++number)
        {
            Assert.Empty(session.WorldFrames[number].Codes);
        }

        for (int number = 186; number < HostSession.WorldFrameCount; ++number)
        {
            Assert.Empty(session.WorldFrames[number].Codes);
        }
    }

    /// <summary>정의가 없거나 표 폭을 넘으면 채우기를 거절하는지 확인한다.</summary>
    [Fact]
    public void FillWorldFramesRejectsMissingAndOverflowingDefinitions()
    {
        var session = new HostSession(new Kernel(), new GameRandom(), new CrtRandom());
        Assert.Throws<ArgumentNullException>(() => session.FillWorldFrames(null!));
        var overflow = new RiftTypeDefinition[HostSession.WorldFrameCount];
        // 자산 기준 70을 넘으면 표 폭을 초과한다.
        for (int index = 0; index < overflow.Length; ++index)
        {
            overflow[index] = new RiftTypeDefinition();
        }

        Assert.Throws<ArgumentOutOfRangeException>(() => session.FillWorldFrames(overflow));
    }
}
