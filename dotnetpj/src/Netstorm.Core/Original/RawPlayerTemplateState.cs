namespace Netstorm.Core.Original;

/// <summary>
/// DataManager 현재 State의 물리 바이트와 MemStream 현재 cursor를 원본 저장소로 소유한다(원본 <c>PlayerTemplateStateStream</c>).
/// C++의 값 구조체 참조 반환과 같은 참조 의미를 위해 클래스로 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class PlayerTemplateStateStream
{
    /// <summary>실제 현재 State 데이터다. 외부 수명이 서비스보다 길어야 한다.</summary>
    public Memory<byte> Bytes { get; set; }

    /// <summary>lookup의 pos 0과 existing 최소 28 검사를 같은 DWORD로 보존하는 현재 위치다.</summary>
    public uint Position { get; set; }

    /// <summary>현재 논리 끝이다.</summary>
    public uint End { get; set; }

    /// <summary>원본 data pointer 0을 길이 0과 구별한다.</summary>
    public bool Present { get; set; } = true;
}

/// <summary>빈 State를 만들 때 원본이 보존하는 미정 스택 입력 28바이트를 현재 외부 입력으로 공급한다.</summary>
/// <param name="slot">Template 슬롯.</param>
/// <param name="bytes">채울 28바이트.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate void PlayerStateBytesProvider(uint slot, Span<byte> bytes);

/// <summary>기존 State를 반납하고 새 현재 섹션에 같은 28바이트를 써서 새 stream을 돌려준다.</summary>
/// <param name="slot">Template 슬롯.</param>
/// <param name="bytes">쓸 28바이트.</param>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public delegate PlayerTemplateStateStream PlayerStateReplacer(uint slot, ReadOnlySpan<byte> bytes);

/// <summary>원본 빈 State의 미정 스택 입력과 실제 섹션 재생성/28바이트 쓰기를 분리한다(원본 <c>PlayerTemplateStateInitialization</c>).</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public sealed class PlayerTemplateStateInitialization
{
    /// <summary>원본이 보존하는 미정 바이트를 현재 외부 입력으로 공급한다.</summary>
    public PlayerStateBytesProvider? Bytes { get; init; }

    /// <summary>기존 State를 반납하고 새 현재 섹션에 같은 28바이트를 쓴다.</summary>
    public PlayerStateReplacer? Replace { get; init; }
}

/// <summary>원본 getter/Notify가 보고 후 계속하는 진단 종류다.</summary>
/// <remarks>2026-11-01 0.128에서 추가했다.</remarks>
public enum PlayerTemplateStateAssertion
{
    /// <summary>Get의 Template 첫 DWORD가 0이다.</summary>
    MissingStateTemplate,

    /// <summary>Notify의 현재 manager.fileName 첫 문자가 '.'이다.</summary>
    DotFileName,

    /// <summary>Notify에서 isMine이 0이 아닌데 Template +12가 0이 아니다.</summary>
    MineTransientFlag,

    /// <summary>isMine 자연 반환이 없는데 현재 inMission이 0이 아니다.</summary>
    InMissionSave,

    /// <summary>Notify의 isMine/미션 진단 뒤 Template 첫 DWORD가 0이다.</summary>
    MissingNotifyTemplate,
}

/// <summary>
/// 기존 State 28바이트와 parent가 쓴 isMine을 실제 Player 소어루저 후처리에서 직접 조회하는 좁은 서비스다(PATCH 4bd550/CD 427920 getter, PATCH 4bd540/4bd360·CD 427910/4275d0 Notify).
/// 실제 Template9 header/이름/State cursor를 같은 원본 전역으로 조립하며 existing State getter의 pos 0/최소 28과 Notify의 isMine 재조회/선택 저장 거부를 보존한다.
/// cpppj <c>o/RawPlayerTemplateState</c>를 옮겼다.
/// </summary>
/// <remarks>2026-11-01 0.128에서 추가했다. 헤더는 외부 소유 바이트를 읽고 쓰며 사본을 만들지 않는다.</remarks>
public sealed class RawPlayerTemplateState
{
    /// <summary>Template 기록 한 개의 실제 폭이다.</summary>
    private const int HeaderBytes = 20;

    /// <summary>Template 칸 수다.</summary>
    private const int TemplateCount = 9;

    /// <summary>State 한 개의 실제 최소 폭이다.</summary>
    private const int StateBytes = 28;

    /// <summary>CD/PATCH 판본이다. CD의 즉시 depth 증감과 각 진단 줄을 분리한다.</summary>
    private readonly OriginalEdition _edition;

    /// <summary>실제 9×20바이트 헤더(current isMine/flags)를 원본 자료로 빌린다.</summary>
    private readonly Memory<byte> _headers;

    /// <summary>현재 헤더가 빌린 manager의 같은 mutable State/cursor를 조회한다.</summary>
    private readonly Func<uint, PlayerTemplateStateStream> _states;

    /// <summary>같은 현재 DataManager의 실제 파일 이름을 조회한다.</summary>
    private readonly Func<uint, string> _names;

    /// <summary>같은 Template/Object/Priest/Meta 로딩 깊이를 읽고 쓴다.</summary>
    private readonly Func<uint> _depth;

    /// <summary>같은 로딩 깊이에 쓴다.</summary>
    private readonly Action<uint> _setDepth;

    /// <summary>실제 미션 ctor가 설정한 594fa0/CD 52e16c를 공유한다.</summary>
    private readonly Func<bool> _mission;

    /// <summary>원본 보고 뒤 현재 조건을 재조회하는 진단이다.</summary>
    private readonly Action<PlayerTemplateStateAssertion> _assertion;

    /// <summary>CD 즉시 증감값을 실제 소비자에게 전달한다.</summary>
    private readonly Action<uint> _depthChanged;

    /// <summary>분리 stream 입력의 확장용 할당이다. 실제 Session은 최소 확장 부모를 쓴다.</summary>
    private readonly Func<uint, uint, Memory<byte>>? _allocate;

    /// <summary>분리 할당 어댑터의 원문 복사가 끝난 뒤 이전 State 할당을 반납한다.</summary>
    private readonly Action<uint>? _allocationComplete;

    /// <summary>실제로 빈 State가 선택됐을 때만 쓰는 필수 생성 경계다.</summary>
    private PlayerTemplateStateInitialization _initialization = new();

    /// <summary>실제 임시 섹션/이름 교체까지 수행하는 현재 최소 확장 소비자다.</summary>
    private Func<uint, int, PlayerTemplateStateStream>? _ensureMinimum;

    /// <summary>isMine/미션/누락/preventSave 관문 뒤에만 선택하는 실제 파일 저장 소비자다.</summary>
    private Action<uint>? _save;

    /// <summary>
    /// 현재 manager 조회를 빌려 같은 State/이름을 쓰며 자체 배열 사본을 만들지 않는다.
    /// Template 180바이트·State 조회·이름 조회·깊이·미션 상태는 서비스보다 오래 살아야 한다.
    /// </summary>
    /// <param name="edition">판본.</param>
    /// <param name="headers">실제 9×20바이트 헤더.</param>
    /// <param name="state">현재 State/cursor 조회.</param>
    /// <param name="name">현재 파일 이름 조회.</param>
    /// <param name="loadingDepth">로딩 깊이 읽기.</param>
    /// <param name="setLoadingDepth">로딩 깊이 쓰기.</param>
    /// <param name="inMission">미션 중 여부 읽기.</param>
    /// <param name="assertion">진단.</param>
    /// <param name="depthChanged">CD 깊이 증감 알림.</param>
    /// <param name="allocate">분리 stream 확장용 할당.</param>
    /// <param name="allocationComplete">분리 할당 복사 뒤 이전 State 반납.</param>
    public RawPlayerTemplateState(
        OriginalEdition edition,
        Memory<byte> headers,
        Func<uint, PlayerTemplateStateStream> state,
        Func<uint, string> name,
        Func<uint> loadingDepth,
        Action<uint> setLoadingDepth,
        Func<bool> inMission,
        Action<PlayerTemplateStateAssertion> assertion,
        Action<uint> depthChanged,
        Func<uint, uint, Memory<byte>>? allocate = null,
        Action<uint>? allocationComplete = null)
    {
        _edition = edition;
        _headers = headers;
        _states = state ?? throw new ArgumentException("Player Template실제9기록/State/진단/깊이효과누락");
        _names = name ?? throw new ArgumentException("Player Template실제9기록/State/진단/깊이효과누락");
        _depth = loadingDepth ?? throw new ArgumentException("Player Template실제9기록/State/진단/깊이효과누락");
        _setDepth = setLoadingDepth ?? throw new ArgumentException("Player Template실제9기록/State/진단/깊이효과누락");
        _mission = inMission ?? throw new ArgumentException("Player Template실제9기록/State/진단/깊이효과누락");
        _assertion = assertion ?? throw new ArgumentException("Player Template실제9기록/State/진단/깊이효과누락");
        _depthChanged = depthChanged ?? throw new ArgumentException("Player Template실제9기록/State/진단/깊이효과누락");
        _allocate = allocate;
        _allocationComplete = allocationComplete;
        if (headers.Length != TemplateCount * HeaderBytes)
        {
            throw new ArgumentException("Player Template실제9기록/State/진단/깊이효과누락");
        }
    }

    /// <summary>
    /// 004bd550이 실제로 쓰는 필드만 초기화한다. 입력의 다른 바이트와 flags 상위 5비트는 보존한다.
    /// 원본 AND/OR와 개별 DWORD/WORD/BYTE 쓰기 밖의 미정 바이트는 변경하지 않는다.
    /// </summary>
    /// <param name="bytes">State 28바이트.</param>
    public static void InitializeStateBytes(Span<byte> bytes)
    {
        if (bytes.Length != StateBytes)
        {
            throw new ArgumentException("Player State는 28BYTE여야 한다");
        }
        bytes[0] = (byte)((bytes[0] & 0xf9) | 1);
        bytes.Slice(4, 6).Clear();
        bytes.Slice(12, 6).Clear();
        bytes.Slice(22, 4).Clear();
    }

    /// <summary>PATCH 빈 State를 실제 미정 입력/섹션 생성 소비자에 연결한다. CD 빈 State는 별도 근거가 필요하므로 PATCH에만 이 소비자를 설치한다.</summary>
    /// <param name="initialization">미정 입력과 섹션 생성 소비자.</param>
    public void BindInitialization(PlayerTemplateStateInitialization initialization)
    {
        ArgumentNullException.ThrowIfNull(initialization);
        if (_edition != OriginalEdition.Patch1078 || initialization.Bytes is null || initialization.Replace is null)
        {
            throw new ArgumentException("빈 State의 PATCH 미정 입력/실제 생성 연결 누락");
        }
        _initialization = initialization;
    }

    /// <summary>PATCH 실제 4d290이 만든 새 현재 State를 직접 받는다. 단순 새 버퍼 할당과 달리 실제 부모가 돌려준 새 현재 stream을 연결한다.</summary>
    /// <param name="ensure">최소 확장 소비자.</param>
    public void BindMinimumSize(Func<uint, int, PlayerTemplateStateStream> ensure)
    {
        if (_edition != OriginalEdition.Patch1078 || ensure is null)
        {
            throw new ArgumentException("최소 State 확장의 PATCH 실제 부모 연결 누락");
        }
        _ensureMinimum = ensure;
    }

    /// <summary>PATCH Notify의 실제 0044d8d0 저장 부모를 같은 현재 Template 칸에 연결한다. CD 저장 부모를 PATCH 추정으로 대신하지 않는다.</summary>
    /// <param name="save">실제 파일 저장 소비자.</param>
    public void BindFileSave(Action<uint> save)
    {
        if (_edition != OriginalEdition.Patch1078 || save is null)
        {
            throw new ArgumentException("Template Notify의 PATCH 실제 파일 저장 연결 누락");
        }
        _save = save;
    }

    /// <summary>현재 Template 한 기록의 DWORD를 원본 little endian 폭으로 조회한다. 호스트 포인터로 reinterpret하지 않는다.</summary>
    /// <param name="slot">Template 슬롯.</param>
    /// <param name="offset">기록 안 DWORD 위치.</param>
    private uint Header(uint slot, int offset)
    {
        if (slot >= TemplateCount || offset > 16)
        {
            throw new ArgumentOutOfRangeException(null, "Player Template실제9개/20BYTE기록범위");
        }
        return RawStorageBytes.Read(_headers.Span, (int)slot * HeaderBytes + offset);
    }

    /// <summary>
    /// PATCH 4bd550/CD 427920: 처음 pos를 0으로 쓰고, 짧은 원문은 확장 뒤 pos/end를 28로 만든다. PATCH 빈 State는 연결된 생성 소비자를 쓴다.
    /// 실제 State 이름 선택/조회는 이 9개 전용 뷰의 동일 State cursor를 쓰며 데이터 사본을 만들지 않는다.
    /// </summary>
    /// <param name="slot">Template 슬롯.</param>
    /// <returns>현재 State 28바이트 이상.</returns>
    public Span<byte> Get(uint slot)
    {
        if (_edition == OriginalEdition.Cd1072)
        {
            _setDepth(_depth() + 1);
            _depthChanged(_depth());
            _setDepth(_depth() - 1);
            _depthChanged(_depth());
        }
        if (Header(slot, 0) == 0)
        {
            _assertion(PlayerTemplateStateAssertion.MissingStateTemplate);
        }
        if (Header(slot, 0) == 0)
        {
            throw new InvalidOperationException("Player Stategetter실제manager0메모리경계");
        }
        PlayerTemplateStateStream current = _states(slot);
        // 실제 lookupState(writable 1)의 현재 MemStream pos 0 쓰기다
        current.Position = 0;
        if (!current.Present || current.End == 0)
        {
            if (_initialization.Bytes is null || _initialization.Replace is null)
            {
                throw new InvalidOperationException("Player 빈 State의 미정 바이트/실제 섹션 생성 미연결");
            }
            byte[] bytes = new byte[StateBytes];
            _initialization.Bytes(slot, bytes);
            InitializeStateBytes(bytes);
            // 이전 stream 참조를 새 현재 State에 계속 쓰지 않는다
            current = _initialization.Replace(slot, bytes);
            if (!current.Present || current.End != StateBytes || current.Position != StateBytes || current.Bytes.Length < StateBytes)
            {
                throw new InvalidOperationException("Player 실제 빈 State 생성/쓰기 계약 위반");
            }
        }
        if (unchecked((int)current.End) < StateBytes && _ensureMinimum is not null)
        {
            current = _ensureMinimum(slot, StateBytes);
            if (!current.Present || unchecked((int)current.End) < StateBytes || current.End > (uint)current.Bytes.Length)
            {
                throw new InvalidOperationException("Player 최소 State 확장의 실제 현재 stream 계약 위반");
            }
        }
        // 생성/확장된 경우에는 소비자가 돌려준 같은 새 현재 stream을 쓴다
        PlayerTemplateStateStream stream = current;
        if (unchecked((int)stream.End) < StateBytes)
        {
            if (_allocate is null)
            {
                throw new InvalidOperationException("Player shortState 실제MemStream할당 미연결");
            }
            // 원본 44d290은 새 record를 할당한 뒤 현재 원문의 길이/포인터를 다시 읽는다
            Memory<byte> replacement = _allocate(slot, StateBytes);
            if (replacement.Length < StateBytes || stream.End > StateBytes || stream.End > (uint)stream.Bytes.Length)
            {
                throw new ArgumentOutOfRangeException(null, "Player shortState 현재원문/새할당 범위");
            }
            Span<byte> target = replacement.Span;
            stream.Bytes.Span[..(int)stream.End].CopyTo(target);
            target[(int)stream.End..StateBytes].Clear();
            // 임시 MemStream을 현재 State로 바꾸며 pos/end는 둘 다 28이다
            stream.Bytes = replacement[..StateBytes];
            stream.Position = StateBytes;
            stream.End = StateBytes;
            // 원본을 모두 복사한 뒤 이전 실제 버퍼를 반납한다
            _allocationComplete?.Invoke(slot);
        }
        if (stream.Bytes.Length < StateBytes || stream.End > (uint)stream.Bytes.Length)
        {
            throw new ArgumentOutOfRangeException(null, "Player 현재State물리BYTE범위");
        }
        return stream.Bytes.Span;
    }

    /// <summary>
    /// PATCH 4bd540/4bd360·CD 427910/4275d0의 Notify다. 실제 isMine 1 또는 preventSave에서 자연 반환한다.
    /// 연결된 실제 저장 부모만 호출하며 isMine 후 재검사에 의한 early return은 빈 callback 대체가 아니다.
    /// </summary>
    /// <param name="slot">Template 슬롯.</param>
    public void Changed(uint slot)
    {
        if (slot >= TemplateCount)
        {
            throw new ArgumentOutOfRangeException(null, "Player Notify 실제9칸 범위");
        }
        // 원본은 첫 manager +c 바이트를 진단보다 먼저 읽으므로 manager 0은 여기서 물리 경계로 거부한다
        if (Header(slot, 0) == 0)
        {
            throw new ArgumentOutOfRangeException(null, "Player Notify manager0 파일명 첫BYTE 물리접근경계");
        }
        string name = _names(slot);
        // 빈 정상 문자열의 첫 NUL도 원본의 유효한 첫 바이트이며 점 이름만 진단한다
        if (name.Length > 0 && name[0] == '.')
        {
            _assertion(PlayerTemplateStateAssertion.DotFileName);
        }
        if (Header(slot, 8) != 0)
        {
            if (Header(slot, 12) != 0)
            {
                _assertion(PlayerTemplateStateAssertion.MineTransientFlag);
            }
            if (Header(slot, 8) != 0)
            {
                return;
            }
        }
        if (_mission())
        {
            _assertion(PlayerTemplateStateAssertion.InMissionSave);
        }
        if (Header(slot, 0) == 0)
        {
            _assertion(PlayerTemplateStateAssertion.MissingNotifyTemplate);
        }
        if (Header(slot, 16) != 0)
        {
            return;
        }
        if (_save is null)
        {
            throw new InvalidOperationException("Player Template실제DataManager파일저장44d8d0/CD44cc40미연결");
        }
        _save(slot);
    }
}
