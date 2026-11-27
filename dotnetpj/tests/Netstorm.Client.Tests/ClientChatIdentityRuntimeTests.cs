using System.Buffers.Binary;
using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Client.Player;
using Netstorm.Core.Original;

// 실제 현재 Player 배열/이름/모드/Chat을 매진입 다시 빌리는지 검사한다.
namespace Netstorm.Client.Tests;

/// <summary>현재 장기 소유자와 실제 RawIdentity 접합의 집중 검사다.</summary>
public sealed class ClientChatIdentityRuntimeTests
{
    /// <summary>같은 실제 구독자/게임/표면/Chat을 초기화한다.</summary>
    private sealed class Scene
    {
        /// <summary>현재 두DWORD/이름 소유자다.</summary>
        public readonly ClientSubscriberGlobals Subscribers = new();
        /// <summary>같은 Player/Core/주소 소유자다.</summary>
        public readonly ClientGameplayGlobals Game;
        /// <summary>같은 모드/지형 저장소다.</summary>
        public readonly ClientSurfaceGlobals Surface;
        /// <summary>같은 현재100행이다.</summary>
        public readonly ClientChatGlobals Chat;
        /// <summary>현재 표시 이름이며 resolver 생성 때 복제하지 않는다.</summary>
        public string Name = "LocalUser";
        /// <summary>현재 이름 조회 횟수다.</summary>
        public int NameReads;
        /// <summary>현재 Player9행을 실제 부모로 준비한다.</summary>
        public Scene(bool initialize = true)
        {
            Game = new(Subscribers); Surface = new(Game, new(new ClientLocalStateGlobals(Game, new())), new(), () => 0); Chat = new(OriginalEdition.Patch1078, () => Subscribers.MySubscriber.Value);
            if (initialize) ClientPlayerRuntime.Initialize(Game); Game.Player.BattleOwner = 1; Subscribers.MySubscriber.Value = 100; Game.LocalAddress.Value = 0x11223344;
        }
        /// <summary>생성은 이름/Player/메타를 읽지 않는 실제 facade다.</summary>
        public ClientChatIdentityRuntime Identity(Func<byte[]>? meta = null) => new(Game, Surface, Subscribers, Chat, () => { NameReads++; return Name; }, meta);
    }
    /// <summary>현재 조회 입력을 만드는 같은60BYTE sparse 초기화다.</summary>
    private static byte[] Record(uint subscriber = 0, string? name = null, uint mode = 0)
    {
        byte[] record = new byte[60]; record.AsSpan().Fill(0xa5); RawPlayerIdentity.Initialize(record, mode); BinaryPrimitives.WriteUInt32LittleEndian(record.AsSpan(4, 4), subscriber); if (name is not null) Encoding.Latin1.GetBytes(name + '\0').CopyTo(record, 8); return record;
    }
    /// <summary>현재 실제 Player 배열을 복사 없이 확인하고 명시 owner 입력만 쓴다.</summary>
    private static void Player(ClientGameplayGlobals game, int owner, uint subscriber, string name)
    {
        Assert.True(MemoryMarshal.TryGetArray((ReadOnlyMemory<byte>)game.CoreData.Players, out ArraySegment<byte> segment)); Assert.NotNull(segment.Array); Assert.Equal(0, segment.Offset); Assert.Equal(1620, segment.Count);
        Encoding.Latin1.GetBytes(name + '\0').CopyTo(segment.Array!, owner * 180 + 12); BinaryPrimitives.WriteUInt32LittleEndian(segment.Array!.AsSpan(owner * 180 + 44, 4), subscriber);
    }
    /// <summary>생성은 준비 안 된 Player/이름을 조회하지 않고 실제 소비에서만 물리 오류를 낸다.</summary>
    [Fact]
    public void BindingIsLazyAndLocalLookupUsesCurrentPhysicalFields()
    {
        var scene = new Scene(false); var identity = scene.Identity(); Assert.Equal(0, scene.NameReads); Assert.Throws<InvalidOperationException>(() => identity.Resolve(Record(100))); Assert.Equal(1, scene.NameReads);
        ClientPlayerRuntime.Initialize(scene.Game); byte[] record = Record(100); Assert.True(identity.Resolve(record)); Assert.Equal(1u, BinaryPrimitives.ReadUInt32LittleEndian(record)); Assert.Equal(100u, BinaryPrimitives.ReadUInt32LittleEndian(record.AsSpan(4, 4))); Assert.Equal(0x11223344u, BinaryPrimitives.ReadUInt32LittleEndian(record.AsSpan(48, 4))); Assert.Equal("LocalUser", Encoding.Latin1.GetString(record, 8, 9)); Assert.Equal(0xa5a5a5a5u, BinaryPrimitives.ReadUInt32LittleEndian(record.AsSpan(56, 4)));
        scene.Name = "Renamed"; scene.Subscribers.MySubscriber.Value = 101; record = Record(101); Assert.True(identity.Resolve(record)); Assert.Equal("Renamed", Encoding.Latin1.GetString(record, 8, 7)); Assert.Equal(3, scene.NameReads);
    }
    /// <summary>같은 facade가 Player 재할당 뒤 새 현재 배열을 읽으며 이전 배열을 고정하지 않는다.</summary>
    [Fact]
    public void ReallocationUsesNewCurrentPlayerArray()
    {
        var scene = new Scene(); scene.Surface.Storage.OwnerMode.Battle = true; Player(scene.Game, 2, 200, "Alice"); var identity = scene.Identity(); byte[] first = Record(name: "Alice"); Assert.True(identity.Resolve(first)); Assert.Equal(2u, BinaryPrimitives.ReadUInt32LittleEndian(first));
        scene.Game.Player.PlayerKnown = 0; ClientPlayerRuntime.Initialize(scene.Game); Player(scene.Game, 3, 300, "Alice"); byte[] next = Record(name: "Alice"); Assert.True(identity.Resolve(next)); Assert.Equal(3u, BinaryPrimitives.ReadUInt32LittleEndian(next)); Assert.Equal(300u, BinaryPrimitives.ReadUInt32LittleEndian(next.AsSpan(4, 4)));
    }
    /// <summary>현재 Chat 행을 일반 분기에서 읽으며 미선택 Meta 장치를 선행 요구하지 않는다.</summary>
    [Fact]
    public void ChatAndMetaKeepActualSelectionBoundaries()
    {
        var scene = new Scene(); scene.Chat.Channels.Add(200, "Peer", 0); var identity = scene.Identity(() => throw new IOException("일반조회중Meta선택")); byte[] record = Record(name: "Peer"); Assert.True(identity.Resolve(record)); Assert.Equal(200u, BinaryPrimitives.ReadUInt32LittleEndian(record.AsSpan(4, 4))); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(record));
        scene.Surface.Storage.OwnerMode.Challenge = true; Assert.Throws<InvalidOperationException>(() => scene.Identity().Resolve(Record(name: "missing")));
    }
    /// <summary>실제 이름 복사는 일부 주소 밖의 꼬리를 보존하고 미조회는 원문 SOMEONE으로 쓴다.</summary>
    [Fact]
    public void NameGetterUsesRealLookupAndSomeOneFallbackWithNul()
    {
        var scene = new Scene(); var identity = scene.Identity(); byte[] output = new byte[64]; output.AsSpan().Fill(0xa5); Assert.Equal(1u, identity.CopyName(100, output)); Assert.Equal("LocalUser", Encoding.Latin1.GetString(output, 0, 9)); Assert.Equal(0, output[9]); Assert.Equal(0xa5, output[10]);
        Assert.Equal(1u, identity.CopyName(200, output)); Assert.Equal("SOMEONE", Encoding.Latin1.GetString(output, 0, 7)); Assert.Equal(0, output[7]); Assert.Throws<ArgumentOutOfRangeException>(() => identity.CopyName(200, new byte[7]));
    }

    /// <summary>소유자 이름은 현재 로컬과 Player 기록을 다시 읽고 부분 조회는 원문 대체 이름을 쓴다.</summary>
    [Fact]
    public void OwnerNameReadsCurrentPlayerRecordsAndFallback()
    {
        var scene = new Scene(); var identity = scene.Identity();
        Assert.Equal("LocalUser", identity.NameForOwner(1));
        scene.Name = "Renamed"; Assert.Equal("Renamed", identity.NameForOwner(1));
        scene.Surface.Storage.OwnerMode.Battle = true; Player(scene.Game, 2, 200, "Alice");
        Assert.Equal("Alice", identity.NameForOwner(2));
        Player(scene.Game, 2, 201, "Bob"); Assert.Equal("Bob", identity.NameForOwner(2));
        Player(scene.Game, 2, 0, "Unresolved"); Assert.Equal("SOMEONE", identity.NameForOwner(2));
    }
}
