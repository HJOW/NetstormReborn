using System.Buffers;
using System.Buffers.Binary;
using System.Runtime.InteropServices;
using Netstorm.Core.Original;

// 실제 Player와 공장 할당을 세션보다 오래 보관하는 Client 저장소다.
namespace Netstorm.Client.Player;

/// <summary>현재 opaque32 주소와 실제9행·공장 물리 및 단일 고정뷰 대여를 소유한다.</summary>
/// <remarks>cpppj client/ClientPlayerRecords 대응이다. 고정뷰 대여가 같은 현재 공장 헤더와 후처리 목록을 연결한다.</remarks>
public sealed class ClientPlayerRecords
{
    /// <summary>실제 DWORD 배열의 바이트를 복사 없이 빌리는 관리 메모리다.</summary>
    private sealed class Block : MemoryManager<byte>
    {
        /// <summary>정렬된 실제 할당이다.</summary>
        public readonly uint[] Words;
        /// <summary>기존 Core의 실제 배열 주소/부분뷰 계약을 공유하는 Player 물리다.</summary>
        public readonly byte[]? PlayerBytes;
        /// <summary>Player와 공장 반납 종류를 구별한다.</summary>
        public readonly bool Player;
        /// <summary>정확한 DWORD 수만 확보한다.</summary>
        public Block(uint bytes, bool player)
        {
            Words = player ? [] : new uint[checked((int)(bytes / 4))]; Player = player;
            PlayerBytes = player ? new byte[checked((int)bytes)] : null;
        }
        /// <summary>같은 실제 배열을 바이트로 해석한다.</summary>
        public override Span<byte> GetSpan() => PlayerBytes is { } bytes ? bytes : MemoryMarshal.AsBytes(Words.AsSpan());
        /// <summary>Player는 같은 실제 배열을 공개하고 공장은 정렬된DWORD를복사없이빌린다.</summary>
        public Memory<byte> Borrow => PlayerBytes is { } bytes ? bytes : Memory;
        /// <summary>opaque32를 호스트 포인터로 공개하지 않는다.</summary>
        public override MemoryHandle Pin(int elementIndex = 0) => throw new NotSupportedException("Player 물리의 호스트 포인터 노출 금지");
        /// <summary>이 저장소는 관리 메모리 뷰만 제공한다.</summary>
        public override void Unpin() => throw new NotSupportedException("Player 물리 고정은 지원하지 않음");
        /// <summary>개별 반납은 주소 장부에서 수행하고 빌린 관리 뷰는 강제 파괴하지 않는다.</summary>
        protected override void Dispose(bool disposing) { }
    }

    /// <summary>한 소유자가 유지하는 판본이다.</summary>
    private readonly OriginalEdition _edition;
    /// <summary>실제 생존 할당만 보관한다.</summary>
    private readonly SortedDictionary<uint, Block> _blocks = [];
    /// <summary>공장이 초기화하지 않는120바이트의 실제 외부 입력이다.</summary>
    private readonly Action<Span<byte>> _allocationBytes;
    /// <summary>발급 주소는 세션 반납이나 재초기화로 되감지 않는다.</summary>
    private uint _next = 0x30000000;
    /// <summary>현재 원문 Player base다.</summary>
    private uint _base;
    /// <summary>살아 있는 고정뷰 소비자가 재할당을 막는다.</summary>
    private bool _inUse;
    /// <summary>실제 초기화 부모가 정상 복귀했는지다.</summary>
    private bool _initialized;
    /// <summary>현재 Player 슬롯을 갱신하는 실제 장기 Core 소유자의 약한 연결이다.</summary>
    private WeakReference<ClientCoreDataGlobals>? _core;

    /// <summary>처음에는 base0·빈 물리이며 실제 초기화에서만 할당한다.</summary>
    public ClientPlayerRecords(Action<Span<byte>>? allocationBytes = null, OriginalEdition edition = OriginalEdition.Patch1078,
        RawProductionDeckStorage? productionDeckStorage = null)
    {
        _allocationBytes = allocationBytes ?? CurrentAllocationBytes;
        if (edition is not OriginalEdition.Patch1078 and not OriginalEdition.Cd1072) throw new ArgumentOutOfRangeException(nameof(edition));
        _edition = edition; ProductionDeckStorage = productionDeckStorage ?? new();
    }

    /// <summary>같은 실제005951f8 DWORD다. 초기화와 로그인 부모가 함께 빌린다.</summary>
    public UIntBox FortCountBox { get; } = new();
    /// <summary>같은 실제005951fc DWORD다.</summary>
    public UIntBox LastLoginPlayerBox { get; } = new();
    /// <summary>최초BSS0이고 실제 동맹 자식이 갱신하는 방향표다.</summary>
    public uint[] Alliances { get; } = new uint[81];
    /// <summary>Player+68의 실제 Deck을 다른 장기 소비자와 공유한다.</summary>
    public RawProductionDeckStorage ProductionDeckStorage { get; }
    /// <summary>현재 판본의 실제 한 행 폭이다.</summary>
    public uint Stride => _edition == OriginalEdition.Cd1072 ? 172u : 180u;
    /// <summary>현재 발급된 실제 주소다.</summary>
    public uint Base => _base;
    /// <summary>실제 부모의 정상 복귀 상태다.</summary>
    public bool Initialized => _initialized;
    /// <summary>현재 base의 실제9행 전체를 빌린다. 재할당을 넘겨 보관할 때는 Acquire가 필요하다.</summary>
    public Memory<byte> Records => _base != 0 ? Memory(_base, Stride * 9) : Memory<byte>.Empty;
    /// <summary>현재 같은 Core 소유자를 순수 조회한다.</summary>
    public ClientCoreDataGlobals? BoundCoreData => _core is not null && _core.TryGetTarget(out var core) ? core : null;

    /// <summary>대여하지 않은 상태에서만 같은 장기 Core의 실제 Player 슬롯과 연결한다.</summary>
    public void BindCoreData(ClientCoreDataGlobals core)
    {
        ArgumentNullException.ThrowIfNull(core);
        if (_inUse || (BoundCoreData is { } bound && !ReferenceEquals(bound, core))) throw new InvalidOperationException("Player와 실제 Core 소유자 연결 계약");
        _core = new(core); core.Players = Records;
    }

    /// <summary>실제 소비자가 살아 있는 동안 재할당을 막는 단일 대여를 발급한다.</summary>
    public PlayerRecordsUse Acquire()
    {
        if (_inUse) throw new InvalidOperationException("Player 고정뷰를 사용하는 이전 세션이 아직 살아 있음");
        _inUse = true; return new(this);
    }

    /// <summary>고정뷰 소비자가 없을 때 원문 초기화 부모를 실행한다.</summary>
    public void Initialize(Action<uint, uint> alliance)
    {
        if (_inUse) throw new InvalidOperationException("현재 Player 고정뷰 사용 중 재할당");
        InitializeBody(alliance);
    }

    /// <summary>이전 Core0 관문 뒤 같은 실제 메모리 장치와 두 카운터를 부모에게 빌린다.</summary>
    private void InitializeBody(Action<uint, uint> alliance)
    {
        if (BoundCoreData is { } core && _base != 0 && MemoryMarshal.TryGetArray(core.Current, out ArraySegment<byte> selected)
            && MemoryMarshal.TryGetArray(Records, out ArraySegment<byte> previous) && ReferenceEquals(previous.Array, selected.Array)
            && selected.Offset >= previous.Offset && selected.Offset - previous.Offset < previous.Count)
            throw new InvalidOperationException("이전 Player 해제 전 실제 Core0 선택 필요");
        _initialized = false;
        var globals = new PlayerInitializationGlobals(FortCountBox, LastLoginPlayerBox) { PlayerBase = () => _base, WritePlayerBase = WriteBase };
        var hooks = new PlayerInitializationHooks
        {
            Memory = Memory, FreeFactory = token => Free(token, false), FreePlayers = token => Free(token, true),
            MallocPlayers = bytes => Allocate(bytes, true), NewFactory = bytes => Allocate(bytes, false), Alliance = alliance,
        };
        if (_edition == OriginalEdition.Cd1072) RawPlayerInitialization.InitializeCdPlayerRecords(globals, hooks);
        else RawPlayerInitialization.InitializePlayerRecords(globals, hooks);
        _initialized = true;
    }

    /// <summary>원문 주소 저장 즉시 같은 Core의 빌린 Player 슬롯도 갱신한다.</summary>
    private void WriteBase(uint value) { _base = value; if (BoundCoreData is { } core) core.Players = Records; }

    /// <summary>현재 네이티브 힙의 실제 할당 입력만 복사하고 즉시 반납한다. 고정 검사는 명시 공급자로 대체한다.</summary>
    private static void CurrentAllocationBytes(Span<byte> destination)
    {
        if (destination.IsEmpty) return;
        nint allocation = Marshal.AllocHGlobal(destination.Length);
        try { byte[] bytes = new byte[destination.Length]; Marshal.Copy(allocation, bytes, 0, bytes.Length); bytes.CopyTo(destination); }
        finally { Marshal.FreeHGlobal(allocation); }
    }

    /// <summary>실제 허용된 폭만 확보하며 공장 입력 실패 시 주소를 소비하지 않는다.</summary>
    private uint Allocate(uint bytes, bool player)
    {
        if (bytes != (player ? Stride * 9 : 120)) throw new InvalidOperationException("Player 실제 할당 크기 오류");
        if (_next > uint.MaxValue - 0x1000) throw new OverflowException("Player 물리 주소 소진");
        var block = new Block(bytes, player); if (!player) _allocationBytes(block.GetSpan());
        uint token = _next; _blocks.Add(token, block); _next += 0x1000; return token;
    }

    /// <summary>NULL은 무효과이며 다른 주소는 정확한 생존 할당과 종류를 요구한다.</summary>
    private void Free(uint token, bool player)
    {
        if (token == 0) return;
        if (!_blocks.TryGetValue(token, out var block) || block.Player != player) throw new InvalidOperationException("Player 실제 반납 주소/종류 오류");
        _blocks.Remove(token);
    }

    /// <summary>현재 주소 이하의 마지막 실제 블록에서 정확한 바이트 폭만 빌린다.</summary>
    private Memory<byte> Memory(uint address, uint bytes)
    {
        Block? selected = null; uint start = 0;
        // 발급 주소가 작은 순서이므로 다음 블록에 도달하면 현재 후보를 확정한다.
        foreach (var pair in _blocks) { if (pair.Key > address) break; start = pair.Key; selected = pair.Value; }
        if (selected is null || (ulong)address - start + bytes > (ulong)selected.GetSpan().Length) throw new ArgumentOutOfRangeException(nameof(address), "Player 실제 물리 주소/요청폭 범위 오류");
        return selected.Borrow.Slice(checked((int)(address - start)), checked((int)bytes));
    }

    /// <summary>현재 Player 행에서 실제 DWORD를 읽는다.</summary>
    private uint Read(uint owner, uint offset)
    {
        if (owner >= 9 || offset > Stride - 4) throw new ArgumentOutOfRangeException(nameof(owner), "현재 Player DWORD 범위 오류");
        return BinaryPrimitives.ReadUInt32LittleEndian(Memory(unchecked(_base + owner * Stride + offset), 4).Span);
    }

    /// <summary>동일 현재 행의 정확한DWORD만 쓰며 주소/범위 검사는 실제 읽기 위치와 공유한다.</summary>
    private void Write(uint owner, uint offset, uint value)
    {
        _ = Read(owner, offset);
        BinaryPrimitives.WriteUInt32LittleEndian(Memory(unchecked(_base + owner * Stride + offset), 4).Span, value);
    }

    /// <summary>현재 헤더의 용량→포인터 순서로 실제 공장 배열의 정확한 부분을 빌린다.</summary>
    public Span<uint> Factory(uint owner)
    {
        uint capacity = Read(owner, 4), token = Read(owner, 0);
        if (capacity > uint.MaxValue / 4) throw new ArgumentOutOfRangeException(nameof(owner), "Player factory 용량 넘침");
        if (capacity == 0) return [];
        Memory<byte> bytes = Memory(token, capacity * 4);
        if ((token & 3) != 0) throw new InvalidOperationException("Player factory DWORD 정렬 오류");
        return MemoryMarshal.Cast<byte, uint>(bytes.Span);
    }

    /// <summary>반납은 게임 값에 손대지 않고 다음 실제 초기화만 허용한다.</summary>
    public sealed class PlayerRecordsUse : IDisposable
    {
        /// <summary>이 대여를 발급한 실제 할당 소유자다.</summary>
        private ClientPlayerRecords? _owner;
        /// <summary>부분 실패 후 같은 대여에서 초기화를 반복하지 않는다.</summary>
        private bool _attempted;
        /// <summary>현재 실제 기록을 정상 초기화하거나 명시 인계받았는지다.</summary>
        private bool _initialized;
        /// <summary>유일한 대여 관문을 통과한 뒤 만들어진다.</summary>
        internal PlayerRecordsUse(ClientPlayerRecords owner) => _owner = owner;
        /// <summary>반납한 대여를 실제 소비자 연결에 다시 쓰지 않는다.</summary>
        private ClientPlayerRecords Owner => _owner ?? throw new ObjectDisposedException(nameof(PlayerRecordsUse));
        /// <summary>같은 대여의 첫 호출에서만 실제 부모를 실행한다.</summary>
        public void Initialize(Action<uint, uint> alliance)
        {
            var owner = Owner; if (_attempted) throw new InvalidOperationException("살아 있는 Player 고정뷰 대여의 초기화 반복");
            _attempted = true; owner.InitializeBody(alliance); _initialized = true;
        }
        /// <summary>앞선 실제 부모가 완료한 현재 Player를 초기화 없이 인계받는다.</summary>
        public void AdoptCurrent()
        {
            var owner = Owner;
            if (_attempted || !owner._initialized || owner._base == 0) throw new InvalidOperationException("초기화 완료된 현재 Player 대여 계약");
            _attempted = true; _initialized = true;
        }
        /// <summary>현재 대여가 실제 기록을 소비할 수 있는지 조회한다.</summary>
        public bool Initialized { get { _ = Owner; return _initialized; } }
        /// <summary>완료된 실제 Player의 현재+0/+4/+8과 같은 소유자/행 정체성을 후처리 목록에 연결한다.</summary>
        public void BindFactory(RawPostPopList list, uint slot)
        {
            var owner = Owner;
            if (!_initialized || slot >= 9) throw new InvalidOperationException("초기화되지 않은 Player factory 연결");
            ArgumentNullException.ThrowIfNull(list);
            list.BindCurrent(new PostPopListCurrentBinding
            {
                SpanStorage = () => owner.Factory(slot), Capacity = () => owner.Read(slot, 4),
                ReadCount = () => owner.Read(slot, 8), WriteCount = value => owner.Write(slot, 8, value),
                SourceOwner = owner, SourceSlot = slot,
            });
        }
        /// <summary>같은 장기소유자/행이면 기존getter만 재사용하고 다른 목록의 중복 연결 거부를 유지한다.</summary>
        public void ReuseOrBindFactory(RawPostPopList list, uint slot)
        {
            var owner = Owner;
            if (!_initialized || slot >= 9) throw new InvalidOperationException("초기화되지 않은 Player factory 재사용");
            ArgumentNullException.ThrowIfNull(list);
            if (list.MatchesCurrentSource(owner, slot)) return;
            BindFactory(list, slot);
        }
        /// <summary>게임 저장소를 초기화하지 않고 단일 대여만 반납한다.</summary>
        public void Dispose() { if (_owner is not { } owner) return; _owner = null; owner._inUse = false; }
    }
}
