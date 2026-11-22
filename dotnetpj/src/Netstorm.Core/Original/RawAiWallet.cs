using System.Buffers.Binary;

// 현재 Player가 가리키는 실제 AI 본체의 지갑 DWORD만 읽고 쓰는10.78 모듈이다.
namespace Netstorm.Core.Original;

/// <summary>원문이 각각의 읽기 위치에서 조회하는 현재 비활성/데모 관문이다.</summary>
public sealed class AiWalletReads
{
    /// <summary>00594fc8이 비영이면 Player나 메모리를 읽지 않고 반환한다.</summary>
    public Func<bool>? Disabled { get; init; }
    /// <summary>가산 뒤00463b50에서 읽는 현재 데모 여부다.</summary>
    public Func<bool>? Demo { get; init; }
}

/// <summary>선택된 실제 메모리와 호출이 끝날 때 반납하는 전체 Runtime 대여를 함께 보유한다.</summary>
/// <param name="Memory">현재 Player+b0가 가리키는 같은 실제 메모리 저장소다.</param>
/// <param name="Owner">일반 owner는 보유만 하고 IRawRuntimeLease는 마지막 조회 뒤 반납한다.</param>
public readonly record struct AiWalletMemoryAccess(RawAiBrainMemory? Memory, object? Owner);

/// <summary>cpppj RawAiWallet.cpp의00413bb0/00413c20이며 SP나 값형 지갑을 대신 사용하지 않는다.</summary>
public sealed class RawAiWallet
{
    /// <summary>PATCH Player 한 행의 실제 폭이다.</summary>
    private const int PlayerBytes = 180;
    /// <summary>실제 알려진 Player 저장소의 행 수다.</summary>
    private const int PlayerSlots = 9;
    /// <summary>Player 안의 실제 brain 토큰 DWORD 위치다.</summary>
    private const int BrainOffset = 0xb0;
    /// <summary>같은0xb08 AI 본체 안의 지갑 DWORD 위치다.</summary>
    private const int WalletOffset = 0x2d8;
    /// <summary>데모 가산 뒤 적용하는 signed 하한이다.</summary>
    private const int DemoFloor = 1190;
    /// <summary>비활성/비양수/NULL brain의 관문을 지난 뒤에만 메모리를 빌린다.</summary>
    private readonly Func<AiWalletMemoryAccess> _acquire;
    /// <summary>재할당을 넘겨 배열을 보관하지 않고 실제 사용 위치의 현재 Player9를 읽는다.</summary>
    private readonly Func<ReadOnlyMemory<byte>> _players;
    /// <summary>함수 객체의 설치 사본이며 설치 시 현재값을 읽지 않는다.</summary>
    private readonly Func<bool> _disabled;
    /// <summary>가산 이후에만 실행하는 현재 미션 조회기다.</summary>
    private readonly Func<bool> _demo;

    /// <summary>독립 호출은 같은 실제 메모리/고정뷰를 빌리고 원래 Player9 폭만 확인한다.</summary>
    public RawAiWallet(RawAiBrainMemory memory, ReadOnlyMemory<byte> players, AiWalletReads reads)
        : this(() => new(memory, null), FixedPlayers(players), reads)
    {
        ArgumentNullException.ThrowIfNull(memory);
    }

    /// <summary>고정 Player9를 쓰는 호출도 필요한 위치에서만 실제 전체 메모리를 빌린다.</summary>
    public RawAiWallet(Func<AiWalletMemoryAccess> acquire, ReadOnlyMemory<byte> players, AiWalletReads reads)
        : this(acquire, FixedPlayers(players), reads) { }

    /// <summary>현재 base/메모리/모드 조회기만 연결하며 Player 초기화나 Session 선택을 하지 않는다.</summary>
    public RawAiWallet(Func<AiWalletMemoryAccess> acquire, Func<ReadOnlyMemory<byte>> players, AiWalletReads reads)
    {
        ArgumentNullException.ThrowIfNull(acquire); ArgumentNullException.ThrowIfNull(players); ArgumentNullException.ThrowIfNull(reads);
        _acquire = acquire; _players = players;
        _disabled = reads.Disabled ?? throw new ArgumentException("AI 지갑의 현재 비활성 조회 미연결", nameof(reads));
        _demo = reads.Demo ?? throw new ArgumentException("AI 지갑의 현재 데모 조회 미연결", nameof(reads));
    }

    /// <summary>독립 고정뷰의 폭을 생성 시 확인하되 가변 getter는 호출하지 않는다.</summary>
    private static Func<ReadOnlyMemory<byte>> FixedPlayers(ReadOnlyMemory<byte> players)
    {
        if (players.Length != PlayerBytes * PlayerSlots) throw new ArgumentException("AI 지갑의 실제 Player9 폭 필요", nameof(players));
        return () => players;
    }

    /// <summary>원문 자연 관문 뒤에만 현재 Player base를 읽고 물리 계약 밖은 호스트 예외로 거부한다.</summary>
    private uint CurrentBrain(uint owner)
    {
        if (_disabled() || unchecked((int)owner) < 1) return 0;
        if (owner >= PlayerSlots) throw new ArgumentOutOfRangeException(nameof(owner), "AI 지갑의 실제 Player9 범위");
        ReadOnlyMemory<byte> current = _players();
        if (current.Length != PlayerBytes * PlayerSlots) throw new ArgumentOutOfRangeException(nameof(current), "AI 지갑의 현재 Player9 물리 누락");
        return BinaryPrimitives.ReadUInt32LittleEndian(current.Span.Slice((int)owner * PlayerBytes + BrainOffset, 4));
    }

    /// <summary>선택한 같은 토큰의 현재 실제 본체만 빌린다. 해제/다른 폭을 새 본체로 대체하지 않는다.</summary>
    private static Span<byte> Body(uint token, AiWalletMemoryAccess memory)
    {
        RawAiBrainMemory actual = memory.Memory ?? throw new InvalidOperationException("AI 지갑의 선택된 실제 메모리 owner 미연결");
        byte[] bytes = actual.Bytes(token);
        if (bytes.Length != RawAiBrainConstructor.BytesCount) throw new ArgumentException("AI 지갑의 실제0xb08 본체 필요", nameof(memory));
        return bytes;
    }

    /// <summary>00413bb0의 DWORD 감김 가산 뒤 현재 데모 관문과 저장한 같은 brain의 signed 하한을 실행한다.</summary>
    public void Credit(uint owner, int amount)
    {
        uint token = CurrentBrain(owner); if (token == 0) return;
        AiWalletMemoryAccess memory = _acquire();
        try
        {
            Span<byte> body = Body(token, memory);
            BinaryPrimitives.WriteUInt32LittleEndian(body[WalletOffset..], unchecked(BinaryPrimitives.ReadUInt32LittleEndian(body[WalletOffset..]) + (uint)amount));
            if (_demo())
            {
                body = Body(token, memory);
                int current = BinaryPrimitives.ReadInt32LittleEndian(body[WalletOffset..]);
                BinaryPrimitives.WriteInt32LittleEndian(body[WalletOffset..], current > DemoFloor ? current : DemoFloor);
            }
        }
        finally { RawRuntimePin.Release(memory.Owner); }
    }

    /// <summary>00413c20의 현재 signed 지갑을 읽는다. 데모·SP·난수·화면을 읽거나 바꾸지 않는다.</summary>
    public int Balance(uint owner)
    {
        uint token = CurrentBrain(owner); if (token == 0) return 0;
        AiWalletMemoryAccess memory = _acquire();
        try { return BinaryPrimitives.ReadInt32LittleEndian(Body(token, memory)[WalletOffset..]); }
        finally { RawRuntimePin.Release(memory.Owner); }
    }
}
