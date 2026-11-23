// 실제 게임 Root의 메시지 공장 팔레트 프로브 경로를 검사한다.
// cpppj/src/client/UberHud.inc 51~61행의 savedMessage/equalText/constructMessage/formatMessage/testMessage/visitMessage/messageEvent 를 이식한다.
// 0.905에서 RU-3 메시지 공장 묶음용으로 추가됐다.
using System.Text;
using Netstorm.Assets;

namespace Netstorm.Client.Gump;

/// <summary>같은 등록표·파일·문자열 owner를 빌리는 메시지 공장 프로브의 계약이다.</summary>
public sealed class RawUberMessageProbeTests
{
    /// <summary>같은 실제 파일 이름 작성기와 기록용 외부 장치를 준비한다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 원문 파일 이름 작성기다.</summary>
        public RawFileName Names { get; } = new();

        /// <summary>안정된 문자열 토큰 표다.</summary>
        public Dictionary<uint, string> Stored { get; } = [];

        /// <summary>다음에 게시할 토큰이다.</summary>
        public uint Next { get; set; } = 0x71000000;

        /// <summary>실제 파일 조회에 쓸 바이트다. null이면 없음으로 응답한다.</summary>
        public byte[]? FileBytes { get; set; }

        /// <summary>남긴 원문 진단 줄이다.</summary>
        public List<string> Logs { get; } = [];

        /// <summary>실제 전체 부모에 전달된 바이트다.</summary>
        public List<byte[]> Palettes { get; } = [];

        /// <summary>실제 후단에 전달된 생성 호출이다.</summary>
        public List<(uint Token, string Name)> Constructs { get; } = [];

        /// <summary>실제 후단에 전달된 사건이다.</summary>
        public List<(uint Token, uint Event, uint First, uint Second)> Events { get; } = [];

        /// <summary>같은 실제 배경 이름이다.</summary>
        public string Background { get; set; } = "kencloud";

        /// <summary>같은 실제 owner들로 장치 묶음을 만든다.</summary>
        public UberMessageProbeHooks Hooks() => new()
        {
            BackgroundName = () => Background,
            StoredText = token => Stored[token],
            StoreText = text => { uint token = Next++; Stored.Add(token, text); return token; },
            TryRead = _ => FileBytes,
            LogProbe = line => Logs.Add(line),
            ChangePalette = bytes => Palettes.Add(bytes),
            Construct = (token, name) => Constructs.Add((token, name)),
            MessageEvent = (token, @event, first, second) => Events.Add((token, @event, first, second)),
        };
    }

    /// <summary>보호PE의 빈 root 메시지 주소다. 0.905에서 추가됐다.</summary>
    [Fact]
    public void SavedMessageIs565e70()
    {
        Assert.Equal(0x565e70u, RawUberMessageProbe.SavedMessage);
    }

    /// <summary>주소 일치와 현재 배경 이름 일치를 함께 판정한다. 0.905에서 추가됐다.</summary>
    [Fact]
    public void EqualTextMatchesCaseInsensitively()
    {
        var scene = new Scene();
        var probe = new RawUberMessageProbe(scene.Names, scene.Hooks());
        Assert.True(probe.EqualText(0x565e70u, "kencloud"));
        Assert.True(probe.EqualText(0x565e70u, "KENCloud"));
        Assert.False(probe.EqualText(0x565e70u, "other"));
        Assert.False(probe.EqualText(0x1234u, "kencloud"));
    }

    /// <summary>같은 9인자를 전체 원문 작성기의 네 부품으로 전달한다. 0.905에서 추가됐다.</summary>
    [Fact]
    public void FormatMessageBuildsProbePath()
    {
        var scene = new Scene();
        var probe = new RawUberMessageProbe(scene.Names, scene.Hooks());
        uint token = probe.FormatMessage([0xfffffffeu, 0x503c8bu, 0xfffffffeu, 0x508ab4u, 0xfffffffeu, 0x565e70u, 0xfffffffeu, 0x51501cu, 0u]);
        Assert.Equal(@".\d\kencloud.col", scene.Stored[token]);
    }

    /// <summary>원문 9인자 변경은 명시 거부한다. 0.905에서 추가됐다.</summary>
    [Fact]
    public void FormatMessageRejectsChangedExpression()
    {
        var scene = new Scene();
        var probe = new RawUberMessageProbe(scene.Names, scene.Hooks());
        Assert.Throws<ArgumentException>(() => probe.FormatMessage([0xfffffffeu, 0x503c8bu, 0xfffffffeu, 0x508ab4u, 0xfffffffeu, 0x565e70u, 0xfffffffeu, 0x51501cu]));
        Assert.Throws<ArgumentException>(() => probe.FormatMessage([0xfffffffeu, 0x503c8bu, 0xfffffffeu, 0x508ab4u, 0xfffffffeu, 0x1234u, 0xfffffffeu, 0x51501cu, 0u]));
    }

    /// <summary>실제 VFS 조회 결과만 조건으로 사용하며 없는 파일을 만들지 않는다. 0.905에서 추가됐다.</summary>
    [Fact]
    public void TestMessageProbesWithoutCreatingFiles()
    {
        var scene = new Scene();
        var probe = new RawUberMessageProbe(scene.Names, scene.Hooks());
        uint token = probe.FormatMessage([0xfffffffeu, 0x503c8bu, 0xfffffffeu, 0x508ab4u, 0xfffffffeu, 0x565e70u, 0xfffffffeu, 0x51501cu, 0u]);
        scene.FileBytes = [1, 2, 3];
        Assert.Equal(1u, probe.TestMessage(token));
        Assert.Contains(scene.Logs, line => line.Contains("exists True", StringComparison.Ordinal));
        scene.FileBytes = null;
        Assert.Equal(0u, probe.TestMessage(token));
        Assert.Contains(scene.Logs, line => line.Contains("exists False", StringComparison.Ordinal));
    }

    /// <summary>원문이 선택한 같은 메시지 포인터로 실제 전체 부모를 호출한다. 0.905에서 추가됐다.</summary>
    [Fact]
    public void VisitMessagePassesNulTerminatedPointer()
    {
        var scene = new Scene();
        var probe = new RawUberMessageProbe(scene.Names, scene.Hooks());
        uint token = probe.FormatMessage([0xfffffffeu, 0x503c8bu, 0xfffffffeu, 0x508ab4u, 0xfffffffeu, 0x565e70u, 0xfffffffeu, 0x51501cu, 0u]);
        probe.VisitMessage(token);
        byte[] expected = Encoding.Latin1.GetBytes(@".\d\kencloud.col" + '\0');
        Assert.Equal(expected, Assert.Single(scene.Palettes));
    }

    /// <summary>Uber 실제 배경 이름 주소가 아니면 명시 거부한다. 0.905에서 추가됐다.</summary>
    [Fact]
    public void ConstructMessageChecksRealBackgroundAddress()
    {
        var scene = new Scene();
        var probe = new RawUberMessageProbe(scene.Names, scene.Hooks());
        probe.ConstructMessage(7, 0x565e70u);
        Assert.Equal([(7u, "kencloud")], scene.Constructs);
        Assert.Throws<ArgumentException>(() => probe.ConstructMessage(7, 0x1234u));
    }

    /// <summary>실제 배경 GIF 후단에 사건을 전달한다. 0.905에서 추가됐다.</summary>
    [Fact]
    public void DispatchMessageEventForwards()
    {
        var scene = new Scene();
        var probe = new RawUberMessageProbe(scene.Names, scene.Hooks());
        probe.DispatchMessageEvent(7, 0x88u, 1, 2);
        Assert.Equal([(7u, 0x88u, 1u, 2u)], scene.Events);
    }

    /// <summary>미연결 장치는 일반 성공으로 꾸미지 않고 명시 실패한다. 0.905에서 추가됐다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitlyInsteadOfNoOp()
    {
        var scene = new Scene();
        Assert.Throws<InvalidOperationException>(() => new RawUberMessageProbe(scene.Names, new()).EqualText(0x565e70u, "kencloud"));
        Assert.Throws<InvalidOperationException>(() => new RawUberMessageProbe(scene.Names, new()).FormatMessage([0xfffffffeu, 0x503c8bu, 0xfffffffeu, 0x508ab4u, 0xfffffffeu, 0x565e70u, 0xfffffffeu, 0x51501cu, 0u]));
        var full = scene.Hooks();
        var probe = new RawUberMessageProbe(scene.Names, full);
        uint token = probe.FormatMessage([0xfffffffeu, 0x503c8bu, 0xfffffffeu, 0x508ab4u, 0xfffffffeu, 0x565e70u, 0xfffffffeu, 0x51501cu, 0u]);
        Assert.Throws<InvalidOperationException>(() => new RawUberMessageProbe(scene.Names, new UberMessageProbeHooks { BackgroundName = full.BackgroundName, StoredText = full.StoredText, StoreText = full.StoreText, TryRead = full.TryRead, ChangePalette = full.ChangePalette, Construct = full.Construct, MessageEvent = full.MessageEvent }).TestMessage(token));
        Assert.Throws<InvalidOperationException>(() => new RawUberMessageProbe(scene.Names, new UberMessageProbeHooks { BackgroundName = full.BackgroundName, StoredText = full.StoredText, StoreText = full.StoreText, TryRead = full.TryRead, LogProbe = full.LogProbe, Construct = full.Construct, MessageEvent = full.MessageEvent }).VisitMessage(token));
    }
}
