using System.Buffers.Binary;
using System.Runtime.InteropServices;
using System.Text;
using Netstorm.Core.Original;

// 실제 장기 Player/Subscriber/표면/Chat으로 원문 식별자 조회를 조립한다.
namespace Netstorm.Client;

/// <summary>ClientChatRuntime.cpp ResolveClientChatIdentity의 매진입 현재 저장소 부모다.</summary>
public sealed class ClientChatIdentityRuntime
{
    /// <summary>현재 Player/core/주소를 소유하는 실제 게임 전역이다.</summary>
    private readonly ClientGameplayGlobals _game;
    /// <summary>현재 전투/메타 모드의 실제 장기 표면이다.</summary>
    private readonly ClientSurfaceGlobals _surface;
    /// <summary>같은 실제 My/Login/이름 소유자다.</summary>
    private readonly ClientSubscriberGlobals _subscribers;
    /// <summary>같은 현재100행의 실제 저장소다.</summary>
    private readonly ClientChatGlobals _chat;
    /// <summary>각 Identity 진입 전에 읽는 실제 현재 표시 BYTE 이름이다.</summary>
    private readonly Func<string> _currentName;
    /// <summary>실제 메타 분기에서만 요구하는 현재 CS40행 제공자다.</summary>
    private readonly Func<byte[]>? _metaRecords;
    /// <summary>주소 정체성과 필수 현재 이름만 연결하며 Player/meta/이름을 선행 조회하지 않는다.</summary>
    public ClientChatIdentityRuntime(ClientGameplayGlobals game, ClientSurfaceGlobals surface, ClientSubscriberGlobals subscribers,
        ClientChatGlobals chat, Func<string> currentName, Func<byte[]>? metaRecords = null)
    {
        _game = game ?? throw new ArgumentNullException(nameof(game)); _surface = surface ?? throw new ArgumentNullException(nameof(surface));
        _subscribers = subscribers ?? throw new ArgumentNullException(nameof(subscribers)); _chat = chat ?? throw new ArgumentNullException(nameof(chat));
        _currentName = currentName ?? throw new ArgumentNullException(nameof(currentName)); _metaRecords = metaRecords;
        if (!ReferenceEquals(surface.Gameplay, game) || !ReferenceEquals(game.Subscribers, subscribers)) throw new ArgumentException("Chat Identity의 실제 같은 Game/Surface/Subscriber 필요");
    }
    /// <summary>현재 Core Player 뷰의 정확한 실제 배열만 빌리며 복사로 현재 물리를 대신하지 않는다.</summary>
    private byte[] Players()
    {
        Memory<byte> current = _game.CoreData.Players;
        if (!MemoryMarshal.TryGetArray((ReadOnlyMemory<byte>)current, out ArraySegment<byte> segment) || segment.Array is null || segment.Offset != 0 || segment.Count != segment.Array.Length || segment.Count != 9 * 180)
            throw new InvalidOperationException("Chat Identity의 현재 실제 Player9행 배열 미설치/부분뷰 미지원");
        return segment.Array;
    }
    /// <summary>이번 동기 현재 이름을 캡처한 뒤 현재 물리와 각 사용 위치의 getter로 본체를 만든다.</summary>
    private RawPlayerIdentity Create(string name) => new(OriginalEdition.Patch1078, Players(), [], _chat.Records, new()
    {
        LocalOwner = () => _game.Player.BattleOwner, Subscriber = () => _subscribers.MySubscriber.Value,
        Address = () => _game.LocalAddress.Value, Name = () => name,
        Battle = () => _surface.Storage.OwnerMode.Battle, Editor = () => _game.World.Editor != 0, Metamap = () => _surface.Storage.OwnerMode.Challenge,
        MetaRecords = () => (_metaRecords ?? throw new InvalidOperationException("Chat Identity의 실제 현재 CS40행 미연결"))(),
    });
    /// <summary>현재 이름과 Player 배열을 매진입 다시 빌리고 기존 원문60BYTE 본체를 그대로 사용한다.</summary>
    public bool Resolve(byte[] record)
    {
        string name = _currentName(); return Create(name).Resolve(record);
    }
    /// <summary>flags2와 현재 구독자로 실제 소유자를 찾고 실패해도 원문 초기0을 유지한다.</summary>
    public uint FindSenderOwner(uint subscriber)
    {
        byte[] record = GC.AllocateUninitializedArray<byte>(60); RawPlayerIdentity.Initialize(record, 2); BinaryPrimitives.WriteUInt32LittleEndian(record.AsSpan(4, 4), subscriber);
        Resolve(record); return BinaryPrimitives.ReadUInt32LittleEndian(record.AsSpan(0, 4));
    }
    /// <summary>일반 본문 이름 getter의 실제 SOMEONE fallback과 NUL 포함 대상 복사를 수행한다.</summary>
    public uint CopyName(uint subscriber, Span<byte> output)
    {
        byte[] record = GC.AllocateUninitializedArray<byte>(60); RawPlayerIdentity.Initialize(record, 0); BinaryPrimitives.WriteUInt32LittleEndian(record.AsSpan(4, 4), subscriber); string name = "SOMEONE";
        if (Resolve(record))
        {
            ReadOnlySpan<byte> current = record.AsSpan(8); int end = current.IndexOf((byte)0); if (end < 0) throw new ArgumentOutOfRangeException(nameof(record), "Chat 대상 이름 getter의 실제 NUL 없음"); name = Encoding.Latin1.GetString(current[..end]);
        }
        byte[] bytes = Encoding.Latin1.GetBytes(name); if (output.Length <= bytes.Length) throw new ArgumentOutOfRangeException(nameof(output), "Chat 대상 이름 getter의 실제 목적 물리 부족"); bytes.CopyTo(output); output[bytes.Length] = 0; return 1;
    }
}
