using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;
using Netstorm.Tests;

// 원본 Reset64입력과 같은 실제70표본/현재 시계/등록표의 효과를 검증한다.
namespace Netstorm.Core.Tests;

/// <summary>서버/Commander 초기화 범위와 수신·평균·기한·부분 쓰기 계약을 확인한다.</summary>
public sealed class ClockSynchronizationTests
{
    /// <summary>원문 비정렬 double8바이트를 읽는다.</summary>
    private static double Read(ReadOnlySpan<byte> bytes, int at) => BitConverter.UInt64BitsToDouble(BinaryPrimitives.ReadUInt64LittleEndian(bytes[at..]));
    /// <summary>현재 시각 입력의 전체64비트를 쓴다.</summary>
    private static void Write(Span<byte> bytes, int at, double value) => BinaryPrimitives.WriteUInt64LittleEndian(bytes[at..], BitConverter.DoubleToUInt64Bits(value));
    /// <summary>독립 원문과 전체 바이트를 비교하는 표현이다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexString(bytes).ToLowerInvariant();
    /// <summary>같은 실제 Client 데이터와 명시 외부 시계/직접 송신을 제공한다.</summary>
    private sealed class Owner
    {
        /// <summary>State/연결 정리/시계가 같은 DWORD를 빌린다.</summary>
        public ClockSyncResetGlobals Globals { get; } = new() { Connection = new() { Value = 11 } };
        /// <summary>실제70표본과 송수신 횟수다.</summary>
        public ClockSynchronizationData Data { get; } = new();
        /// <summary>실제 현재 벽시각 입력이다.</summary>
        public double Wall { get; set; } = 10;
        /// <summary>직접 수신을 등록할 실제 원문 분배기다.</summary>
        public RawPacketDispatch Dispatch { get; } = new(OriginalEdition.Patch1078, new() { Now = () => 123, Record = (_, _, _) => { }, Terminate = _ => { }, Close = _ => { }, Release = _ => { } });
        /// <summary>입력으로 선택한 같은 실제 표본32바이트다.</summary>
        public Memory<byte> Sample(int index) => Data.Samples.Slice(index * 32, 32);
        /// <summary>기대값을 역산하지 않고 세 시각을 명시 입력한다.</summary>
        public void Sample(int index, double request, double server, double received) { Write(Sample(index).Span, 8, request); Write(Sample(index).Span, 16, server); Write(Sample(index).Span, 24, received); }
        /// <summary>현재 전역과 같은 시계 입력을 실제 제품 부모에 연결한다.</summary>
        public RawClockSynchronization Body(ClockSynchronizationHooks? hooks = null, bool canonical = false) => new(Globals, Data, hooks ?? new() { WallSeconds = () => Wall }, canonical ? this : null);
    }

    /// <summary>두 제어어/입력별 반복을 포함한 독립 원본64행의 두96바이트 전체를 대조한다.</summary>
    [Fact]
    public void ResetMatchesAllSixtyFourOriginalInputsAndNeighbourBytes()
    {
        string[][] rows = X86Fixture.Read("clock-sync-reset"); Assert.Equal(64, rows.Length); string key = ""; byte[] sync = [], clock = [];
        // 최초/반복 관측에서 같은 물리를 재사용하고 원본 기대 바이트만 비교한다.
        foreach (string[] row in rows)
        {
            Assert.Equal(13, row.Length); string current = row[0] + ':' + row[1]; if (current != key) { key = current; sync = X86Fixture.Bytes(row[6]); clock = X86Fixture.Bytes(row[7]); Assert.Equal("0", row[2]); }
            var globals = new ClockSyncResetGlobals { Connection = new(sync.AsMemory(20, 4)), ReplyMode = new(sync.AsMemory(32, 4)), GameTicks = new(clock.AsMemory(12, 4)) }; RawClockSyncReset.Reset(globals);
            Assert.Equal(row[8], Hex(sync)); Assert.Equal(row[9], Hex(clock)); Assert.Equal("005c8bf4:4:00000000;005c8c00:4:00000000;0055b4ac:4:00000000", row[11]);
        }
    }

    /// <summary>서버와 Commander의 다른 초기화 범위/순서 및 장기 고정F 복원 기준선을 확인한다.</summary>
    [Fact]
    public void InitializationScopesAndCanonicalBaselineKeepActualClientOwner()
    {
        var owner = new Owner(); owner.Data.Samples.Span.Fill(0xa5); byte[] before = owner.Data.Samples.ToArray(); owner.Data.Sent.Value = 55; owner.Data.Received.Value = 66; owner.Data.Next.Value = 14; owner.Data.LatencyMilliseconds.Value = 13; owner.Globals.GameTicks.Value = 77;
        owner.Dispatch.Version(5, 73, 11); owner.Dispatch.RequireLength(5, 32); using PacketRegistrationLease lease = owner.Dispatch.Registrations.BindSession(owner);
        RawClockSynchronization clock = owner.Body(canonical: true); clock.InitializeServer(owner.Dispatch); Assert.Equal(1u, owner.Globals.ReplyMode.Value); Assert.Equal(0u, owner.Data.Sent.Value | owner.Data.Received.Value); Assert.Equal(11u, owner.Globals.Connection.Value); Assert.Equal(before, owner.Data.Samples.ToArray()); Assert.Equal(14, owner.Data.Next.Value); Assert.Equal(77u, owner.Globals.GameTicks.Value);
        CanonicalPacketReceiver canonical = Assert.IsType<CanonicalPacketReceiver>(owner.Dispatch.Registration(5).CanonicalReceive); Assert.Equal(0x004c1f60u, canonical.NativeEntry); lease.Dispose(); Assert.Same(canonical, owner.Dispatch.Registration(5).CanonicalReceive); Assert.Equal(73, owner.Dispatch.Registration(5).Version); Assert.Equal(11, owner.Dispatch.Registration(5).Mismatch); Assert.Equal(32, owner.Dispatch.Registration(5).Length);
        var next = owner.Body(new() { WallSeconds = () => { Assert.DoesNotContain(owner.Data.Samples.ToArray(), value => value != 0); owner.Data.Sent.Value = 90; owner.Data.Received.Value = 91; owner.Globals.Connection.Value = 92; return 15; } }, true);
        next.Attach(owner.Dispatch, 44); Assert.Equal(44u, owner.Globals.Connection.Value); Assert.Equal(0u, owner.Data.Sent.Value | owner.Data.Received.Value | owner.Globals.ReplyMode.Value); Assert.Equal(15, owner.Data.Next.Value); Assert.Equal(13u, owner.Data.LatencyMilliseconds.Value); Assert.Equal(77u, owner.Globals.GameTicks.Value);
    }

    /// <summary>주소는 시계 호출 전에 보관하고 실제 주소 전달 여부는 호출 뒤 현재flags로 결정한다.</summary>
    [Theory]
    [InlineData(0u, 0u)]
    [InlineData(0u, 0x220u)]
    [InlineData(0x220u, 0u)]
    [InlineData(0x220u, 0x220u)]
    public void ServerReplyKeepsAddressSnapshotAndRereadsCurrentFlags(uint initial, uint current)
    {
        var owner = new Owner(); owner.Globals.ReplyMode.Value = 1; var node = new PacketConnection { Flags = initial }; byte[] address = Enumerable.Range(0, 20).Select(value => (byte)(value + 3)).ToArray(); address.CopyTo(node.Address, 0); byte[] packet = Enumerable.Range(0, 40).Select(value => (byte)value).ToArray(); byte[] tail = packet[32..]; bool sent = false;
        owner.Body(new()
        {
            GameSeconds = () => { node.Flags = current; Array.Fill(node.Address, (byte)0xcc); return 4.75; },
            Reply = (selected, bytes, destination) =>
            {
                Assert.Same(node, selected); Assert.True(bytes.Equals(packet.AsMemory(0, 32))); Assert.Equal(4.75, Read(bytes.Span, 16));
                if (current == 0) Assert.Null(destination); else { Assert.True(destination.HasValue); byte[] expected = initial != 0 ? address : new byte[20]; if (initial == 0) expected[0] = 2; Assert.Equal(expected, destination.Value.ToArray()); }
                bytes.Span[4] = 99; sent = true;
            },
        }).Receive(node, packet);
        Assert.True(sent); Assert.Equal((byte)99, packet[4]); Assert.Equal(tail, packet[32..]); Assert.DoesNotContain(owner.Data.Samples.ToArray(), value => value != 0);
    }

    /// <summary>같은 요청시간의 첫 표본만 복사하고 반복/0요청/NaN의 현재 비교를 유지한다.</summary>
    [Fact]
    public void ReceiveCopiesFirstEqualRequestOnlyAndCountsDuplicatesWithWrap()
    {
        var owner = new Owner(); owner.Sample(1, 5, 1, 2); owner.Sample(2, 5, 3, 4); byte[] second = owner.Sample(2).ToArray(); byte[] packet = Enumerable.Repeat((byte)0x55, 40).ToArray(); Write(packet, 8, 5); Write(packet, 16, 7); owner.Data.Received.Value = uint.MaxValue;
        RawClockSynchronization clock = owner.Body(); clock.Receive(new(), packet); Assert.Equal(0u, owner.Data.Received.Value); Assert.Equal(packet[..32], owner.Sample(1).ToArray()); Assert.Equal(second, owner.Sample(2).ToArray()); Assert.Equal(10, Read(packet, 24)); clock.Receive(new(), packet); Assert.Equal(1u, owner.Data.Received.Value);
        Write(packet, 8, double.NaN); clock.Receive(new(), packet); Assert.Equal(1u, owner.Data.Received.Value); Write(packet, 8, 0); clock.Receive(new(), packet); Assert.Equal(2u, owner.Data.Received.Value); Assert.Equal(packet[..32], owner.Sample(0).ToArray());
    }

    /// <summary>빠른 절반 평균과 만료/2초이상/NaN 제거가 헤더를 보존한다.</summary>
    [Fact]
    public void PollUsesFastestHalfAndClearsOnlyInvalidTimes()
    {
        var owner = new Owner(); owner.Sample(0, 8, 7.5, 8.5); owner.Sample(1, 8, 6.25, 8.25); owner.Sample(2, 8, 6, 9);
        // 미작성 헤더/인접DWORD를 포함한 세 invalid표본의 입력을 구별한다.
        foreach (int index in new[] { 3, 4, 5 }) owner.Sample(index).Span[..8].Fill((byte)(index + 31));
        owner.Sample(3, 7, 0, 0); owner.Sample(4, 6, 1, 9); owner.Sample(5, double.NaN, 1, 10); owner.Body().Poll(); Assert.Equal(125u, owner.Data.LatencyMilliseconds.Value); Assert.Equal(1875u, owner.Globals.GameTicks.Value);
        // 세 double만 지워지고 앞8바이트는 원문과 같이 유지된다.
        foreach (int index in new[] { 3, 4, 5 }) { Assert.All(owner.Sample(index).Span[..8].ToArray(), value => Assert.Equal((byte)(index + 31), value)); Assert.DoesNotContain(owner.Sample(index).Span[8..].ToArray(), value => value != 0); }
    }

    /// <summary>동률3개에서도 원본 CRT 작은 선택 정렬의 교환 순서를 유지한다.</summary>
    [Fact]
    public void EqualRoundTripsUseOriginalCrtTieOrder()
    {
        var owner = new Owner { Wall = 5 }; owner.Sample(0, 4, 3.5, 4.5); owner.Sample(1, 4, 2.5, 4.5); owner.Sample(2, 4, 0.5, 4.5); owner.Body().Poll(); Assert.Equal(250u, owner.Data.LatencyMilliseconds.Value); Assert.Equal(1750u, owner.Globals.GameTicks.Value);
    }

    /// <summary>실제 첫 빈 표본을 빌리고 callback 후 현재송신수/기한 쓰기를 수행한다.</summary>
    [Fact]
    public void SendBorrowsActualSampleAndLateCounterWritesRemainCurrent()
    {
        var owner = new Owner { Wall = 5 }; owner.Sample(0).Span[..8].Fill(0x7b); owner.Data.Next.Value = 5; bool sent = false;
        owner.Body(new() { WallSeconds = () => owner.Wall, Send = (connection, packet) =>
        {
            Assert.Equal(11u, connection); Assert.True(packet.Equals(owner.Sample(0))); Assert.Equal(new byte[] { 0x7b, 5, 32, 0 }, packet.Span[..4].ToArray()); Assert.Equal(5, Read(packet.Span, 8)); Assert.Equal(0, Read(packet.Span, 16)); Assert.Equal(0, Read(packet.Span, 24));
            owner.Data.Sent.Value = 58; owner.Data.Next.Value = 999; owner.Globals.Connection.Value = 99; packet.Span[4] = 0xaa; sent = true;
        } }).Poll();
        Assert.True(sent); Assert.Equal(59u, owner.Data.Sent.Value); Assert.Equal(6, owner.Data.Next.Value); Assert.Equal(99u, owner.Globals.Connection.Value); Assert.Equal((byte)0xaa, owner.Sample(0).Span[4]);
    }

    /// <summary>송신 횟수의 signed관문/수신 관문과60회 기한 종료를 검증한다.</summary>
    [Theory]
    [InlineData(31u, 0u, 31u, 10.0, 0)]
    [InlineData(30u, 0u, 31u, 11.0, 1)]
    [InlineData(59u, 1u, 60u, 0.0, 1)]
    [InlineData(60u, 1u, 61u, 0.0, 1)]
    [InlineData(uint.MaxValue, 0u, 0u, 11.0, 1)]
    public void SendingGatesUseSignedCurrentCountAndStopDeadline(uint before, uint received, uint after, double next, int sends)
    {
        var owner = new Owner(); owner.Data.Sent.Value = before; owner.Data.Received.Value = received; owner.Data.Next.Value = 10; int actual = 0; owner.Body(new() { WallSeconds = () => 10, Send = (_, _) => actual++ }).Poll(); Assert.Equal(sends, actual); Assert.Equal(after, owner.Data.Sent.Value); Assert.Equal(next, owner.Data.Next.Value);
    }

    /// <summary>공간이 없어도 원문 기한은 갱신하고, 송신 실패는 앞선 표본 쓰기만 보존한다.</summary>
    [Fact]
    public void FullSlotsAndMissingSendKeepOriginalPartialEffects()
    {
        var owner = new Owner(); owner.Data.Next.Value = 10; owner.Data.Sent.Value = 29;
        // 아직 만료되지 않은 현재 요청70개로 실제 공간을 채운다.
        for (int index = 0; index < 70; index++) owner.Sample(index, 9, 0, 0);
        owner.Body().Poll(); Assert.Equal(29u, owner.Data.Sent.Value); Assert.Equal(11, owner.Data.Next.Value);
        owner.Data.Samples.Span.Clear(); owner.Data.Next.Value = 10; Assert.Throws<InvalidOperationException>(() => owner.Body().Poll()); Assert.Equal(29u, owner.Data.Sent.Value); Assert.Equal(10, owner.Data.Next.Value); Assert.Equal((byte)5, owner.Sample(0).Span[1]); Assert.Equal(10, Read(owner.Sample(0).Span, 8));
    }

    /// <summary>같은 보정DWORD가 실제 게임 시계와 연결 정리 뷰에 즉시 반영된다.</summary>
    [Fact]
    public void ResetSharesCurrentClockAndConnectionRegistryViews()
    {
        var owner = new Owner(); var registry = new RawConnectionRegistry(); var view = new ConnectionSyncState(() => owner.Globals.Connection.Value, value => owner.Globals.Connection.Value = value); registry.BindSynchronization(view);
        var clock = new GameClock(); clock.BindSynchronizationTicks(() => owner.Globals.GameTicks.Value, owner); owner.Globals.GameTicks.Value = 250; Assert.Equal(.75, clock.GameSeconds(1000)); Assert.Equal(11u, registry.Cleanup().Sync.Synchronization);
        RawClockSyncReset.Reset(owner.Globals); Assert.Equal(0u, registry.Cleanup().Sync.Synchronization); Assert.Equal(1, clock.GameSeconds(1000));
    }
    /// <summary>작은 정렬 경계를 넘는9/31/70표본에서도 빠른 절반의 분석적 평균을 검증한다.</summary>
    [Theory]
    [InlineData(9, 19u, 4136u)]
    [InlineData(31, 62u, 4437u)]
    [InlineData(70, 140u, 4984u)]
    public void FullSampleRangesSortFastestHalfAcrossCrtPartitions(int count, uint latency, uint ticks)
    {
        var owner = new Owner();
        // 역순 RTT와 정확한 이진 분수 offset을 입력해 CRT의 여러 분할을 거친다.
        for (int index = 0; index < count; index++) { double rtt = (count - index) / 64.0, offset = 4 + (count - index) / 16.0; owner.Sample(index, 8, 8 + rtt - offset, 8 + rtt); }
        owner.Body().Poll(); Assert.Equal(latency, owner.Data.LatencyMilliseconds.Value); Assert.Equal(ticks, owner.Globals.GameTicks.Value);
    }

    /// <summary>고정F가 실제 연결/현재 전역을 사용하고 readonly 입력의 앞32바이트만 지역 응답으로 복사한다.</summary>
    [Fact]
    public void CanonicalReceiverUsesActualNodeAndCopiesOnlyWritableReplyPrefix()
    {
        var owner = new Owner(); var node = new PacketConnection(); byte[] incoming = Enumerable.Repeat((byte)0x6a, 40).ToArray(); byte[] before = incoming.ToArray(); byte[]? sent = null;
        owner.Body(new()
        {
            GameSeconds = () => 123.25,
            Reply = (connection, packet, address) => { Assert.Same(node, connection); Assert.Null(address); Assert.Equal(32, packet.Length); Assert.False(((ReadOnlyMemory<byte>)packet).Equals(incoming.AsMemory(0, 32))); sent = packet.ToArray(); },
        }, true).InitializeServer(owner.Dispatch);
        owner.Dispatch.InvokeDirect(5, 7, node, incoming); Assert.NotNull(sent); byte[] expected = before[..32]; Write(expected, 16, 123.25); Assert.Equal(expected, sent); Assert.Equal(before, incoming);
        Assert.Throws<InvalidOperationException>(() => owner.Dispatch.InvokeDirect(5, 7, null, incoming)); Assert.Throws<ArgumentOutOfRangeException>(() => owner.Dispatch.InvokeDirect(5, 7, node, incoming.AsMemory(0, 31)));
    }

}
