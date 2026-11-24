// Meta 버전 차이 확인의 같은 진단/알림 호출을 검사한다.
// cpppj/src/client/ClientCampaignTransport.cpp 의 CheckClientMetaVersion(004b4160) 을 대조한다.
// 0.937에서 R4-6 전송·수신 묶음용으로 추가됐다.

namespace Netstorm.Client.Tests;

// 004b4160 버전 판정의 실제 장치 호출을 기록하는 계약이다.
public sealed class ClientMetaVersionTests
{
    /// <summary>같은 실제 설정/진단/알림 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>실제 설정 조회 기록이다.</summary>
        public readonly List<string> ConfigKeys = [];
        /// <summary>실제 진단 기록이다.</summary>
        public readonly List<(uint Line, string Text)> Logs = [];
        /// <summary>실제 알림 기록이다.</summary>
        public readonly List<string> Tells = [];
        /// <summary>현재 숨김 설정값이다.</summary>
        public int Suppress;

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public ClientMetaVersionHooks Hooks() => new()
        {
            Config = key => { ConfigKeys.Add(key); return Suppress; },
            Log = (line, text) => Logs.Add((line, text)),
            Tell = text => Tells.Add(text),
        };
    }

    /// <summary>같은 버전은 진단·알림·설정 조회를 모두 생략한다. 0.937에서 추가됐다.</summary>
    [Fact]
    public void MatchingVersionSkipsLogTellAndConfig()
    {
        var scene = new Scene();
        Assert.True(ClientMetaVersion.Check(10, 78, scene.Hooks()));
        Assert.Empty(scene.Logs);
        Assert.Empty(scene.Tells);
        Assert.Empty(scene.ConfigKeys);
    }

    /// <summary>부 버전 불일치는 진단만 쓰고 수락한다. 0.937에서 추가됐다.</summary>
    [Fact]
    public void MinorMismatchLogsButAccepts()
    {
        var scene = new Scene();
        Assert.True(ClientMetaVersion.Check(10, 79, scene.Hooks()));
        Assert.Equal([(0x1du, "VERSION DIFFERENCE: You are 10.78. Meta Server is 10.79.\n")], scene.Logs);
        Assert.Empty(scene.Tells);
        Assert.Empty(scene.ConfigKeys);
    }

    /// <summary>주 버전 불일치는 숨김이 꺼졌을 때 알리고 거짓을 반환한다. 0.937에서 추가됐다.</summary>
    [Fact]
    public void MajorMismatchTellsAndRejects()
    {
        var scene = new Scene();
        Assert.False(ClientMetaVersion.Check(11, 80, scene.Hooks()));
        Assert.Equal([(0x1du, "VERSION DIFFERENCE: You are 10.78. Meta Server is 11.80.\n")], scene.Logs);
        Assert.Equal(["MetaVerDiff(11.80)"], scene.Tells);
        Assert.Equal(["suppressVersionMessage"], scene.ConfigKeys);
    }

    /// <summary>숨김이 켜진 주 버전 불일치는 진단만 쓰고 수락한다. 0.937에서 추가됐다.</summary>
    [Fact]
    public void SuppressedMajorMismatchAcceptsSilently()
    {
        var scene = new Scene { Suppress = 1 };
        Assert.True(ClientMetaVersion.Check(11, 80, scene.Hooks()));
        Assert.Equal([(0x1du, "VERSION DIFFERENCE: You are 10.78. Meta Server is 11.80.\n")], scene.Logs);
        Assert.Empty(scene.Tells);
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않는다. 0.937에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        Assert.Throws<ArgumentNullException>(() => ClientMetaVersion.Check(10, 78, null!));
        var scene = new Scene();
        var bare = new ClientMetaVersionHooks();
        Assert.Throws<InvalidOperationException>(() => ClientMetaVersion.Check(11, 80, bare));
        var noTell = scene.Hooks(); noTell.Tell = null;
        Assert.Throws<InvalidOperationException>(() => ClientMetaVersion.Check(11, 80, noTell));
        var noConfig = scene.Hooks(); noConfig.Config = null;
        Assert.Throws<InvalidOperationException>(() => ClientMetaVersion.Check(11, 80, noConfig));
    }
}
