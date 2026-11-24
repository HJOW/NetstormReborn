// 원본71h 전투 설명 수신의 부분 복사·정제·실제 표시 갱신 순서를 대조한다.
// cpppj/src/client/NativeMetaBattleDescription.cpp 의 ReceiveNativeMetaBattleDescription(0042cad0)/ConfirmNativeMetaBattleDescription 을 대조한다.
// 0.941에서 R4-6 전송·수신 묶음용으로 추가됐다.

namespace Netstorm.Client.Gump;

// 0042cad0 전투 설명 수신의 실제 CS/진단/표시 호출을 기록한다.
public sealed class NativeMetaBattleDescriptionTests
{
    /// <summary>같은 실제 CS와 호출을 기록하는 실제 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>같은 실제 CS다.</summary>
        public readonly ClientChallengeGlobals Challenge = new();
        /// <summary>실제 진단 기록이다.</summary>
        public readonly List<string> Logs = [];
        /// <summary>실제 새로고침한 객체 번호다.</summary>
        public readonly List<uint> Refreshed = [];
        /// <summary>실제 갱신한 전투 번호다.</summary>
        public readonly List<uint> Updated = [];

        /// <summary>같은 실제 CS를 준비한다.</summary>
        public Scene()
        {
            Challenge.Prepare();
        }

        /// <summary>같은 실제 외부 장치 묶음이다.</summary>
        public NativeMetaBattleDescriptionHooks Hooks() => new()
        {
            Log = text => Logs.Add(text),
            RefreshObject = number => Refreshed.Add(number),
            UpdateBattle = battle => Updated.Add(battle),
        };

        /// <summary>실제 설명 패킷을 원문 폭으로 조립한다. 길이에 종단 NUL을 포함한다.</summary>
        public static byte[] Packet(uint battle, string text)
        {
            byte[] nameBytes = System.Text.Encoding.Latin1.GetBytes(text + "\0");
            byte[] packet = new byte[10 + nameBytes.Length];
            packet[4] = (byte)battle;
            packet[9] = (byte)nameBytes.Length;
            nameBytes.CopyTo(packet, 10);
            return packet;
        }
    }

    /// <summary>무효 전투는 로그만 쓰고 표를 바꾸지 않는다. 0.941에서 추가됐다.</summary>
    [Fact]
    public void InvalidBattleLogsOnly()
    {
        var scene = new Scene();
        NativeMetaBattleDescription.Receive(scene.Challenge, Scene.Packet(3, "ab"), scene.Hooks());
        Assert.Equal(["ERROR in handleChatBattleUpdate: battle 3 is not valid!\n"], scene.Logs);
        Assert.Empty(scene.Refreshed);
        Assert.Empty(scene.Updated);
    }

    /// <summary>초과 길이는 로그 뒤 현재 번호 갱신만 수행한다. 0.941에서 추가됐다.</summary>
    [Fact]
    public void OverlongDescriptionUpdatesBattleOnly()
    {
        var scene = new Scene();
        var battle = scene.Challenge.Battle(2).Span;
        battle[0] = 1;
        string text = new('x', 42);
        NativeMetaBattleDescription.Receive(scene.Challenge, Scene.Packet(2, text), scene.Hooks());
        Assert.Equal(["ERROR in handleChatBattleUpdate: desc too long\n[bufferLen == 41, optionLen == 43]\n"], scene.Logs);
        Assert.Equal([2u], scene.Updated);
        Assert.Empty(scene.Refreshed);
    }

    /// <summary>유효 설명은 복사·NUL·제어 문자 정제 뒤 표시를 새로고침한다. 0.941에서 추가됐다.</summary>
    [Fact]
    public void ValidDescriptionCopiesSanitizesAndRefreshes()
    {
        var scene = new Scene();
        var battle = scene.Challenge.Battle(2).Span;
        battle[0] = 1;
        battle[0x20] = 7;
        NativeMetaBattleDescription.Receive(scene.Challenge, Scene.Packet(2, "A\tB"), scene.Hooks());
        Assert.Empty(scene.Logs);
        Span<byte> span = scene.Challenge.Battle(2).Span;
        Assert.Equal((byte)'A', span[0xf9]);
        Assert.Equal((byte)' ', span[0xfa]);
        Assert.Equal((byte)'B', span[0xfb]);
        Assert.Equal(0, span[0xf8 + 4]);
        Assert.Equal([7u], scene.Refreshed);
        Assert.Equal([2u], scene.Updated);
    }

    /// <summary>원문 확정은 첫 LF를 자르고 NUL 뒤는 버린다. 0.941에서 추가됐다.</summary>
    [Fact]
    public void ConfirmCutsAtLineFeedAndNul()
    {
        char[] first = new char[256]; "ab\ncd".CopyTo(0, first, 0, 5);
        Assert.Equal("ab", NativeMetaBattleDescription.Confirm(first));
        char[] second = new char[256]; "ab\0cd".CopyTo(0, second, 0, 5);
        Assert.Equal("ab", NativeMetaBattleDescription.Confirm(second));
        Assert.Throws<ArgumentOutOfRangeException>(() => NativeMetaBattleDescription.Confirm(new char[255]));
    }

    /// <summary>미연결 장치와 짧은 패킷은 일반 성공으로 꾸미지 않는다. 유효 경로의 진단 누락은 허용한다. 0.938에서 추가하고 0.942에서 복원했다.</summary>
    [Fact]
    public void MissingDevicesFailExplicitly()
    {
        var scene = new Scene();
        Assert.Throws<ArgumentNullException>(() => NativeMetaBattleDescription.Receive(scene.Challenge, Array.Empty<byte>(), null!));
        Assert.Throws<ArgumentNullException>(() => NativeMetaBattleDescription.Receive(null!, new byte[12], scene.Hooks()));
        var battle = scene.Challenge.Battle(4).Span;
        battle[0] = 0;
        var bare = new NativeMetaBattleDescriptionHooks();
        Assert.Throws<InvalidOperationException>(() => NativeMetaBattleDescription.Receive(scene.Challenge, Scene.Packet(4, "ab"), bare));
        Assert.Throws<ArgumentOutOfRangeException>(() => NativeMetaBattleDescription.Receive(scene.Challenge, new byte[3], scene.Hooks()));
        // 유효 경로는 진단을 쓰지 않으므로 진단 누락이어도 성공한다. 0.942에서 바로잡았다.
        var noLog = scene.Hooks();
        noLog.Log = null;
        var active = scene.Challenge.Battle(5).Span;
        active[0] = 1;
        active[0x20] = 3;
        NativeMetaBattleDescription.Receive(scene.Challenge, Scene.Packet(5, "ab"), noLog);
        Assert.Empty(scene.Logs);
        Assert.Equal([3u], scene.Refreshed);
        Assert.Equal([5u], scene.Updated);
        // 유효 경로는 표시·갱신 장치가 없으면 명시 실패한다. 0.942에서 추가됐다.
        var noRefresh = scene.Hooks();
        noRefresh.RefreshObject = null;
        Assert.Throws<InvalidOperationException>(() => NativeMetaBattleDescription.Receive(scene.Challenge, Scene.Packet(5, "ab"), noRefresh));
        var noUpdate = scene.Hooks();
        noUpdate.UpdateBattle = null;
        Assert.Throws<InvalidOperationException>(() => NativeMetaBattleDescription.Receive(scene.Challenge, Scene.Packet(5, "ab"), noUpdate));
    }

    /// <summary>상한 길이 41은 잘라내지 않고 그대로 복사한다. 0.942에서 추가됐다.</summary>
    [Fact]
    public void BoundaryLength41CopiesFully()
    {
        var scene = new Scene();
        var battle = scene.Challenge.Battle(2).Span;
        battle[0] = 1;
        battle[0x20] = 9;
        string text = new('y', 40);
        NativeMetaBattleDescription.Receive(scene.Challenge, Scene.Packet(2, text), scene.Hooks());
        Assert.Empty(scene.Logs);
        Span<byte> span = scene.Challenge.Battle(2).Span;
        Assert.Equal((byte)'y', span[0xf9]);
        Assert.Equal((byte)'y', span[0xf9 + 39]);
        Assert.Equal(0, span[0xf8 + 41]);
        Assert.Equal([9u], scene.Refreshed);
        Assert.Equal([2u], scene.Updated);
    }
}
