using System.Buffers.Binary;
using System.Text;
using System.Runtime.InteropServices;
using Netstorm.Tests;
using Netstorm.Core.Original;

// 원본 연구 관측 자료와 효과 후 현재 필드 재조회를 대조한다.
namespace Netstorm.Core.Tests;

/// <summary>독립 원본216입력과 선택된 미연결·변이·필드 폭 경계를 확인한다.</summary>
public sealed class RawFactoryResearchTests
{
    /// <summary>계산을 대신하지 않는 명시 입력과 실제 호출 관측 장치다.</summary>
    private sealed class Scene
    {
        /// <summary>원본과 같은 물리 SID를 제공하는 PATCH 풀이다.</summary>
        public readonly RawSidPool Pool = new(24000, true, null, OriginalEdition.Patch1078);
        /// <summary>현재 genus를 가진188개 TYPE다.</summary>
        public readonly OriginalTypeRecord[] Types = Enumerable.Range(0, 188).Select(_ => new OriginalTypeRecord()).ToArray();
        /// <summary>현재 로딩 깊이다.</summary>
        public uint Loading;
        /// <summary>현재 공유 생산 통지다.</summary>
        public readonly UIntBox Dirty = new() { Value = 3 };
        /// <summary>현재 서버 권한이다.</summary>
        public bool Boss = true;
        /// <summary>현재 게임 시각이다.</summary>
        public double Now = 12.5;
        /// <summary>현재 제외할 아웃포스트 종류다.</summary>
        public uint Outpost = 126;
        /// <summary>검색 장치에 공급하는 명시 결과다.</summary>
        public int Found;
        /// <summary>슬롯 장치에 공급하는 명시 결과다.</summary>
        public int Slots = 2;
        /// <summary>실제 호출한 순서와 인자다.</summary>
        public readonly List<string> Trace = [];
        /// <summary>계산 경계에서 호출 순서만 관측한다.</summary>
        public readonly FactoryResearchHooks Hooks;

        /// <summary>원본 실행기에 공급했던 동일 슬롯과 하위 입력을 준비한다.</summary>
        public Scene()
        {
            Raw(50).Clear(); Put(50, 0, 0x509148); Raw(50)[10] = 85; Raw(50)[34] = 1;
            Raw(100).Clear(); Put(100, 0, 0x512208); Put(100, 18, 128, 2); Put(100, 20, 16, 2);
            Types[85].Flags2 = 0x4000;
            Hooks = new()
            {
                Find = (parent, mask, _, ordinal) => { Assert.Equal(50, parent); Assert.Equal(16u, mask); Assert.Equal(0u, ordinal); Trace.Add($"find,{Found}"); return Found; },
                Slots = parent => { Assert.Equal(50, parent); Trace.Add($"slots,{Slots}"); return Slots; },
                Destroy = (sid, flags) => { Trace.Add($"destroy,{sid},{flags}"); Found = 0; },
                Create = (parent, kind, quantity, flags, attach, quality) => { Trace.Add($"create,{parent},{kind},{quantity},{flags},{attach},{quality}"); return 20000; },
                Send = (channel, packet) => Trace.Add($"send,{channel},{Convert.ToHexString(packet.Span).ToLowerInvariant()}"),
                Help = (text, mode) => Trace.Add($"help,{Hex(text)},{mode}"),
                Flare = (parent, count, ev) => Trace.Add($"flare,{parent},{count},{ev}"),
                Refresh = owner => Trace.Add($"refresh,{owner}"),
                Assertion = text => Trace.Add($"assert,{Hex(text)}"),
            };
        }
        /// <summary>매 호출 지점에서 같은 현재 전역을 읽는 본체를 만든다.</summary>
        public RawFactoryResearch Body() => new(Pool, Types, new()
        {
            LoadingDepth = () => Loading, ProductionDirty = Dirty, Boss = () => Boss,
            Now = () => Now, OutpostType = () => Outpost,
        }, Hooks);
        /// <summary>원본 실행기와 같은 고정 물리 슬롯·form0 입력을 공급한다. 게임 할당 결과를 대신하지 않는다.</summary>
        public Span<byte> Raw(int sid) => MemoryMarshal.CreateSpan(ref MemoryMarshal.GetReference(Pool.Slot(sid)), Pool.SlotStride);
        /// <summary>원본 폭으로 지정 슬롯의 입력 필드를 설정한다.</summary>
        public void Put(int sid, int offset, uint value, int width = 4)
        {
            if (width == 2) BinaryPrimitives.WriteUInt16LittleEndian(Raw(sid)[offset..], unchecked((ushort)value));
            else BinaryPrimitives.WriteUInt32LittleEndian(Raw(sid)[offset..], value);
        }
        /// <summary>공통 writer의 실제 연구 또는 업그레이드 원문을 만든다.</summary>
        public byte[] Packet(uint kind, byte mode = 0)
        {
            var packet = new byte[16]; FactoryResearchCommand.Write(50, kind, Now, packet); packet[7] = mode; return packet;
        }
    }

    /// <summary>문자열을 원본 TSV의 소문자 ASCII hex로 표시한다.</summary>
    private static string Hex(string value) => Convert.ToHexString(Encoding.ASCII.GetBytes(value)).ToLowerInvariant();

    /// <summary>독립 원본 행만 기대값으로 사용해 반환·dirty·전체 호출 순서·송신 바이트를 비교한다.</summary>
    [Theory]
    [InlineData("Learn", 32)]
    [InlineData("Input", 72)]
    [InlineData("Receive", 16)]
    [InlineData("Notice", 96)]
    public void Original216(string section, int count)
    {
        string[][] rows = X86Fixture.Read("factory-research", section); Assert.Equal(count, rows.Length);
        // 각 독립 관측 행에 별도 풀과 현재 전역을 공급한다.
        foreach (string[] row in rows)
        {
            var scene = new Scene(); uint result = 0;
            switch (section)
            {
                case "Learn":
                    scene.Found = X86Fixture.Int(row[1]); scene.Slots = X86Fixture.Int(row[2]);
                    scene.Body().Learn(50, 128, X86Fixture.UInt(row[3])); break;
                case "Input":
                    scene.Found = X86Fixture.Int(row[2]); scene.Slots = X86Fixture.Int(row[3]);
                    result = scene.Body().Input(50, 0x03000080u | X86Fixture.UInt(row[4]), (X86Fixture.UInt(row[1]) << 16) | 50u); break;
                case "Receive":
                    scene.Raw(50)[11] = (byte)X86Fixture.UInt(row[2]); scene.Raw(50)[10] = (byte)X86Fixture.UInt(row[4]);
                    scene.Types[scene.Raw(50)[10]].Flags2 = X86Fixture.UInt(row[3]); scene.Boss = X86Fixture.UInt(row[5]) != 0;
                    scene.Body().Receive(X86Fixture.UInt(row[1]), scene.Packet(X86Fixture.UInt(row[6]), (byte)X86Fixture.UInt(row[7]))); break;
                case "Notice":
                    int form = X86Fixture.Int(row[1]); scene.Put(form, 20, X86Fixture.UInt(row[3]), 2);
                    scene.Loading = X86Fixture.UInt(row[4]); scene.Found = X86Fixture.Int(row[5]);
                    scene.Body().Notice(50, form, X86Fixture.UInt(row[2])); break;
            }
            Assert.Equal(X86Fixture.UInt(row[^3]), result); Assert.Equal(X86Fixture.UInt(row[^2]), scene.Dirty.Value);
            Assert.Equal(row[^1], scene.Trace.Count == 0 ? "-" : string.Join(';', scene.Trace));
        }
    }

    /// <summary>입력 로그 뒤 시각을 읽되 기존 양수 슬롯 판단을 보존하고 Learn의 시각 변이는 송신에 섞지 않는다.</summary>
    [Fact]
    public void InputCachesSlotsAndTimeAtOriginalPositions()
    {
        var scene = new Scene(); byte[]? sent = null;
        scene.Hooks.InputLog = (_, slots) => { Assert.Equal(2, slots); scene.Now = 33.75; scene.Slots = 0; };
        scene.Hooks.Slots = _ => { scene.Now += 1; return scene.Slots; };
        scene.Hooks.Send = (channel, packet) => { Assert.Equal(9u, channel); sent = packet.ToArray(); };
        Assert.Equal(1u, scene.Body().Input(50, 0x83000080, 0xff800032));
        Assert.NotNull(sent); Assert.Equal(128, sent[6]); Assert.Equal(33.75, BitConverter.ToDouble(sent, 8));
        Assert.Equal(34.75, scene.Now); Assert.DoesNotContain(scene.Trace, text => text.StartsWith("create,"));
    }

    /// <summary>부호 확장된 전체 종류는 하위 검색/생성에 전달하고 송신에서는 하위BYTE만 남긴다.</summary>
    [Fact]
    public void SignedWordKindDiffersFromWireByte()
    {
        var scene = new Scene(); scene.Body().Input(50, 0x03000080, 0xff800032);
        Assert.Contains("create,50,4294967168,0,16,16,0", scene.Trace);
        Assert.StartsWith("send,9,0158100032008000", scene.Trace[^1]);
    }

    /// <summary>현재 boss 진단 복귀와 효과 후 kind·owner 재조회, dirty 감김을 확인한다.</summary>
    [Fact]
    public void EffectsReadCurrentFieldsAfterDiagnosticsAndFlare()
    {
        var scene = new Scene { Boss = false }; scene.Hooks.Assertion = _ => scene.Boss = true;
        scene.Body().Receive(1, scene.Packet(128)); Assert.Contains("create,50,128,0,16,0,0", scene.Trace);
        scene.Trace.Clear(); uint kind = 0; scene.Dirty.Value = uint.MaxValue;
        scene.Hooks.Flare = (_, _, _) => scene.Put(100, 18, 129, 2);
        scene.Hooks.Find = (_, _, value, _) => { kind = value; return 0; };
        scene.Raw(50)[40] = 9; scene.Hooks.Assertion = _ => scene.Raw(50)[34] = 7;
        scene.Body().Notice(50, 100, 1);
        Assert.Equal(129u, kind); Assert.Equal(0u, scene.Dirty.Value); Assert.Equal(new[] { "refresh,7" }, scene.Trace);
    }

    /// <summary>수신은sender DWORD를줄이지않고 mode1만위임하며 잘못된범위·선택된미연결을거부한다.</summary>
    [Fact]
    public void ReceiveIdentityAndSelectedMissingEffects()
    {
        var scene = new Scene(); RawFactoryResearch body = scene.Body();
        body.Receive(257, scene.Packet(128)); Assert.Empty(scene.Trace);
        Assert.Throws<InvalidOperationException>(() => body.Receive(1, scene.Packet(0, 1)));
        byte[] selected = scene.Packet(0, 1); ReadOnlyMemory<byte> received = default;
        scene.Hooks.UpgradeReceive = packet => received = packet;
        body.Receive(uint.MaxValue, selected); Assert.True(received.Equals((ReadOnlyMemory<byte>)selected));
        Assert.Throws<ArgumentOutOfRangeException>(() => body.Receive(1, new byte[7]));
        Assert.Throws<InvalidOperationException>(() => body.Input(50, 0, 0));
        Assert.True(body.HandlesNotice(50)); scene.Put(50, 0, 0x509210); Assert.True(body.HandlesNotice(50));
        scene.Put(50, 0, 0x5096a0); Assert.False(body.HandlesNotice(50));
        scene.Hooks.Flare = null; Assert.Throws<InvalidOperationException>(() => body.Notice(50, 100, 1)); Assert.Equal(3u, scene.Dirty.Value);
    }
}
