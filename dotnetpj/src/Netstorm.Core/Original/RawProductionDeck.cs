using System.Buffers.Binary;

// 현재 Player+68과 실제352바이트 Deck의 단일 소유·항목 계산·반납 증빙을 연결한다.
namespace Netstorm.Core.Original;

/// <summary>World/Player 뷰와 분리해 불투명 DWORD 주소와 실제 Deck 할당만 장기 보관한다.</summary>
public sealed class RawProductionDeckStorage
{
    /// <summary>현재 살아 있는352바이트 블록이다. 토큰을 호스트 포인터로 해석하지 않는다.</summary>
    private readonly Dictionary<uint, byte[]> _blocks = [];
    /// <summary>다음 할당의 원문 주소다. 반납이나 새 세션에서 되감지 않는다.</summary>
    private uint _next;
    /// <summary>최초 주소만 검증하며 실제 블록은 Get 때 확보한다.</summary>
    public RawProductionDeckStorage(uint firstToken = 0x30000000)
    {
        if (firstToken == 0 || firstToken > uint.MaxValue - ProductionDeckLayout.Bytes)
            throw new ArgumentOutOfRangeException(nameof(firstToken), "실제 Deck의 토큰 연결 누락");
        _next = firstToken;
    }
    /// <summary>현재 Player 포인터가 없을 때만 할당/초기화한 뒤 같은 Player+68에 쓴다.</summary>
    public uint Get(Span<byte> players, uint owner, Action<Span<byte>>? fill)
    {
        if (owner >= 9 || players.Length != 9 * 180) throw new ArgumentOutOfRangeException(nameof(owner), "실제 Deck의 Player 범위 오류");
        int offset = (int)owner * 180 + 0x68;
        uint current = BinaryPrimitives.ReadUInt32LittleEndian(players[offset..]);
        if (current != 0) { _ = Bytes(current); return current; }
        if (fill is null) throw new InvalidOperationException("실제 Deck 할당 바이트 입력 누락");
        if (_next > uint.MaxValue - ProductionDeckLayout.Bytes) throw new OverflowException("실제 Deck 토큰 주소 고갈");
        uint token = _next; var raw = new byte[ProductionDeckLayout.Bytes]; fill(raw);
        _blocks.TryAdd(token, raw); _next = unchecked(_next + ProductionDeckLayout.Bytes);
        RawDeckDraw.Initialize(Bytes(token).Span);
        BinaryPrimitives.WriteUInt32LittleEndian(players[offset..], token); return token;
    }
    /// <summary>현재 생존 블록의 실제 바이트를 빌린다. 미등록 토큰은 거부한다.</summary>
    public Memory<byte> Bytes(uint token) => _blocks.TryGetValue(token, out byte[]? raw) ? raw
        : throw new ArgumentOutOfRangeException(nameof(token), "현재 Player Deck의 실제 할당 블록 누락");
    /// <summary>0은 무효과이며 비영 토큰은 실제 블록만 반납한다. Player 필드와 다음 주소는 보존한다.</summary>
    public void Release(uint token)
    {
        if (token == 0) return;
        if (!_blocks.Remove(token)) throw new ArgumentOutOfRangeException(nameof(token), "현재 Player Deck의 실제 반납 블록 누락");
    }
    /// <summary>현재 세션이 소유한 포인터를 종료 때 구분한다.</summary>
    internal bool Owns(uint token) => _blocks.ContainsKey(token);
    /// <summary>정상 반납 증빙에서 검사하는 현재 실제 블록 수다.</summary>
    internal int Count => _blocks.Count;
    /// <summary>일반 모듈 소멸의 소유 블록만 반납하며 주소는 되감지 않는다.</summary>
    internal void ClearOwned() => _blocks.Clear();
}

/// <summary>현재 Player의 유일한 원문 Deck을 조회·초기화·추가·활성화·압축하고 수명을 관리한다.</summary>
/// <remarks>근거: cpppj/src/o/RawProductionDeck.cpp/.h. 기술 표시와 생산 재계산은 별도 소비자다.</remarks>
public sealed class RawProductionDeck : IDisposable
{
    /// <summary>같은 실제 모듈의 전체 링크 정리 정상 복귀로만 발급하는 단일 소비 증빙이다.</summary>
    public sealed class PlayerLinksPass
    {
        /// <summary>증빙만으로 기존 모듈 수명을 연장하지 않는 원본 source 정체성이다.</summary>
        internal readonly WeakReference<RawProductionDeck> Source;
        /// <summary>C++ 이동 소비를 관리 참조의 재소비 거부로 표현한다.</summary>
        internal bool Consumed;
        /// <summary>실제 정리 본체가 정상 복귀한 모듈만 증빙을 발급한다.</summary>
        internal PlayerLinksPass(RawProductionDeck source) => Source = new(source);
    }
    /// <summary>현재9개 Player와 templateOwner를 제공하는 같은 월드다.</summary>
    private readonly RawPlayerWorld _player;
    /// <summary>실제 할당 직후의 미정 바이트 공급자다.</summary>
    private readonly Action<Span<byte>> _fill;
    /// <summary>원문 진단 위치의 실제 소비자다.</summary>
    private readonly Action<ProductionDeckAssertion>? _assertion;
    /// <summary>장기 Client 또는 독립 입력에서 소유하는 같은 물리 저장소다.</summary>
    private readonly RawProductionDeckStorage _storage;
    /// <summary>전체 정리 부모의 효과/후속 store가 아직 실행 중인 호출 수다.</summary>
    private int _activeLinkCalls;
    /// <summary>정상 반납 증빙을 소비해 기존 Player 재조회 책임을 반납했는지다.</summary>
    private bool _returnedLinks;
    /// <summary>관리 모듈의 일반 소멸 뒤 재사용을 막는다.</summary>
    private bool _disposed;

    /// <summary>같은 PATCH Player와 명시 할당 입력을 연결하며 실제 할당은 앞당기지 않는다.</summary>
    public RawProductionDeck(RawPlayerWorld player, Action<Span<byte>> fill,
        Action<ProductionDeckAssertion>? assertion = null, uint firstToken = 0x30000000, RawProductionDeckStorage? storage = null)
    {
        ArgumentNullException.ThrowIfNull(player); ArgumentNullException.ThrowIfNull(fill);
        if (player.Pool().Edition != OriginalEdition.Patch1078 || firstToken == 0 || firstToken > uint.MaxValue - ProductionDeckLayout.Bytes)
            throw new ArgumentException("실제 Deck의 PATCH/할당 바이트/토큰 연결 누락");
        _player = player; _fill = fill; _assertion = assertion; _storage = storage ?? new(firstToken);
    }
    /// <summary>다른 월드 장치와 같은 풀인지 확인한다.</summary>
    public RawSidPool Pool => _player.Pool();
    /// <summary>현재 원본 Player 기록과 두 소유자를 재사용한다.</summary>
    public RawPlayerWorld Player => _player;
    /// <summary>현재 owner의 Player+68을 조회하고 없는 경우에만 실제 블록을 만든다.</summary>
    public uint Get(uint owner) { RequireLiveLinks(); return _storage.Get(_player.Records(), owner, _fill); }
    /// <summary>같은 생존 블록을 빌리며 반납된 모듈 접근을 거부한다.</summary>
    public Memory<byte> Bytes(uint token) { RequireLiveLinks(); return _storage.Bytes(token); }
    /// <summary>같은 실제 메모리 소유자가 CRT free를 실행한다. 후속 Player0쓰기는 부모 책임이다.</summary>
    public void Release(uint token) { RequireLiveLinks(); _storage.Release(token); }
    /// <summary>생성 초기화의 다섯 DWORD만 쓴다.</summary>
    public void Initialize(uint token) => RawDeckDraw.Initialize(Bytes(token).Span);
    /// <summary>항목 개수/가중합/남은 수만 지우고 seed/open과 미정 꼬리를 유지한다.</summary>
    public void ClearEntries(uint token)
    {
        Span<byte> raw = Bytes(token).Span;
        ProductionDeckLayout.Put(raw, ProductionDeckLayout.Count, 0); ProductionDeckLayout.Put(raw, ProductionDeckLayout.Chance, 0);
        ProductionDeckLayout.Put(raw, ProductionDeckLayout.Remaining, 0);
    }
    /// <summary>현재 templateOwner Deck의 type BYTE와 전체 입력 DWORD를 첫 일치까지 비교한다.</summary>
    public bool Contains(uint type)
    {
        Memory<byte> raw = Bytes(Get(_player.State().TemplateOwner));
        // 원문처럼 현재 개수를 매번 읽고 첫 일치 뒤 나머지 항목을 읽지 않는다.
        for (int index = 0; index < ProductionDeckLayout.CurrentCount(raw.Span); index++)
            if ((uint)raw.Span[index * ProductionDeckLayout.EntryBytes] == type) return true;
        return false;
    }
    /// <summary>같은 공통 원문 항목 추가를 사용한다.</summary>
    public void Append(uint token, uint type, int chance) => RawDeckDraw.Append(Bytes(token).Span, type, chance, _assertion);
    /// <summary>최초 일치 항목의 signed 음수 power를0으로 바꾸고 비영 increment일 때1만 더한다.</summary>
    public void Activate(uint token, uint type, int increment)
    {
        Memory<byte> raw = Bytes(token);
        // 현재 개수/항목을 재조회하며 종류 DWORD를 BYTE로 미리 자르지 않는다.
        for (int index = 0; index < ProductionDeckLayout.CurrentCount(raw.Span); index++)
        {
            int offset = index * ProductionDeckLayout.EntryBytes;
            if ((uint)raw.Span[offset] != type) continue;
            if (unchecked((sbyte)raw.Span[offset + 2]) < 0) raw.Span[offset + 2] = 0;
            if (increment != 0) raw.Span[offset + 2] = unchecked((byte)(raw.Span[offset + 2] + 1));
            return;
        }
        Append(token, type, 1);
    }
    /// <summary>제외할 항목도 먼저 앞으로 복사하고 signed 음수 power만 개수에서 제거한다.</summary>
    public void Compact(uint token)
    {
        Span<byte> raw = Bytes(token).Span; int count = ProductionDeckLayout.CurrentCount(raw), removed = 0;
        // 원문 앞방향 겹침 복사와 사용하지 않는 물리 꼬리를 보존한다.
        for (int index = 0; index < count; index++)
        {
            int from = index * ProductionDeckLayout.EntryBytes, to = (index - removed) * ProductionDeckLayout.EntryBytes;
            uint word = ProductionDeckLayout.Read(raw, from); byte last = raw[from + 4];
            ProductionDeckLayout.Put(raw, to, word); raw[to + 4] = last;
            if (unchecked((sbyte)raw[from + 2]) < 0) removed++;
        }
        ProductionDeckLayout.Put(raw, ProductionDeckLayout.Count, unchecked((uint)(count - removed)));
    }
    /// <summary>같은 실제 정리 부모와 자신의 Release가 정상 복귀한 경우에만 증빙을 발급한다.</summary>
    public PlayerLinksPass CleanupPlayerLinksForState(Func<Memory<byte>> currentPlayers)
    {
        RequireLiveLinks(); _activeLinkCalls++;
        try
        {
            RawPlayerLinksCleanup.Run(new() { Players = currentPlayers, Release = Release });
            return new(this);
        }
        finally { _activeLinkCalls--; }
    }
    /// <summary>같은 모듈/전체 복귀·실제 블록0·현재 세 링크0을 확인하고 증빙을 한 번 소비한다.</summary>
    public void ReturnPlayerLinksForState(PlayerLinksPass pass)
    {
        ArgumentNullException.ThrowIfNull(pass);
        if (pass.Consumed || !pass.Source.TryGetTarget(out RawProductionDeck? source) || !ReferenceEquals(source, this))
            throw new ArgumentException("같은 실제 Deck의 Player 링크 정상 복귀 증빙 필요", nameof(pass));
        RequireLiveLinks();
        if (_activeLinkCalls != 0) throw new InvalidOperationException("진행 중 Player 링크 효과의 증빙 소비 금지");
        if (_storage.Count != 0) throw new InvalidOperationException("현재 실제 Deck 소유 블록 반납 미완료");
        byte[] records = _player.Records();
        if (records.Length != 9 * 180) throw new InvalidOperationException("현재 Player 링크 반납의 실제9기록 누락");
        // 정리한 세 DWORD만 확인하고 다른 상태는 변경하지 않는다.
        for (int owner = 0; owner < 9; owner++)
            if (ProductionDeckLayout.Read(records, owner * 180 + 0x9c) != 0 || ProductionDeckLayout.Read(records, owner * 180 + 8) != 0
                || ProductionDeckLayout.Read(records, owner * 180 + 0x68) != 0)
                throw new InvalidOperationException("현재 Player 링크 세 DWORD 반납 미완료");
        pass.Consumed = true; _returnedLinks = true;
    }
    /// <summary>기존 Player 뷰를 읽지 않고 실제 증빙 소비 여부만 제공한다.</summary>
    public bool PlayerLinksReturnedForState => _returnedLinks;
    /// <summary>반납된 Player 뷰로 새 Deck을 만들거나 메모리를 조회/반납하지 못하게 한다.</summary>
    private void RequireLiveLinks()
    {
        ObjectDisposedException.ThrowIf(_disposed, this);
        if (_returnedLinks) throw new InvalidOperationException("반납된 Player 링크의 Deck 소비 금지");
    }
    /// <summary>현재 Player의 자기 소유 포인터만 지운 뒤 자기 블록을 반납한다. 주소는 되감지 않는다.</summary>
    public void Dispose()
    {
        if (_disposed) return;
        if (_activeLinkCalls != 0) throw new InvalidOperationException("진행 중 Player 링크 정리의 Deck 소멸 금지");
        if (!_returnedLinks)
        {
            // 각 기록의 현재 포인터만 읽어 외부 모듈이 교체한 포인터는 보존한다.
            for (int owner = 0; owner < 9; owner++)
            {
                byte[] records = _player.Records(); int offset = owner * 180 + 0x68;
                if (_storage.Owns(ProductionDeckLayout.Read(records, offset))) ProductionDeckLayout.Put(records, offset, 0);
            }
            _storage.ClearOwned();
        }
        _disposed = true;
    }
}
