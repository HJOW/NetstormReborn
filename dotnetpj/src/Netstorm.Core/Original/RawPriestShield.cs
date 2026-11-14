using System.Buffers.Binary;
using Netstorm.Core.Simulation;

// 실제 보호막 조회·생성·소유자·공간과 소리/안내 요청의 순서를 복원한다.
namespace Netstorm.Core.Original;

/// <summary>보호막 요청이 효과 콜백 뒤 다시 읽는 현재 전역이다.</summary>
public sealed class PriestShieldState
{
    /// <summary>BYTE로 줄이지 않는 현재 로컬 플레이어 DWORD다.</summary>
    public uint LocalPlayer { get; set; }
    /// <summary>현재 요새 로딩 중첩 DWORD다.</summary>
    public uint LoadingDepth { get; set; }
    /// <summary>+0/-0일 때만 안내하며 NaN은 제외하는 원본 시각이다.</summary>
    public double NoticeClock { get; set; }
}

/// <summary>원본 전역 안내 소리 함수의 파일명과5인자다.</summary>
public sealed class PriestShieldNotice
{
    /// <summary>원본 이동 불가 안내 소리 파일이다.</summary>
    public string File { get; } = "ourPriestImmobile.wav";
    /// <summary>cdecl 호출의 원본0/0/0/1/0 인자다.</summary>
    public uint[] Arguments { get; } = [0, 0, 0, 1, 0];
}

/// <summary>보호막의 실제 하위 생성·Pop·소리·안내 효과다.</summary>
public sealed class PriestShieldHooks
{
    /// <summary>flags2로 보호막 자산을 실제 client 풀에 생성한다.</summary>
    public Func<uint, uint, int>? Create { get; set; }
    /// <summary>생성된 보호막의 실제 소유자를 지정한다.</summary>
    public Action<int, uint>? SetOwner { get; set; }
    /// <summary>보호막 가상 Pop의 전체 공간 효과다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
    /// <summary>원문 소리 프로세스new40바이트 성공 관문이다.</summary>
    public Func<bool>? ReserveSound { get; set; }
    /// <summary>파일명 하나로 원본 소리 토큰을 조회한다.</summary>
    public Func<string, uint>? LoadSound { get; set; }
    /// <summary>보호막 위치에 원본0/1 인자로 소리 프로세스를 부착한다.</summary>
    public Action<int, uint, uint, uint>? AttachSound { get; set; }
    /// <summary>로컬 안내 소리의 원본 인자를 받는다.</summary>
    public Action<PriestShieldNotice>? NoticeSound { get; set; }
    /// <summary>PATCH의 PriestImmobile 문자열 포맷 경계다.</summary>
    public Func<string, string>? Translate { get; set; }
    /// <summary>포맷 결과를 Tell 섹션 이름으로 전달한다.</summary>
    public Action<string>? Tell { get; set; }

    /// <summary>원문 함수 객체처럼 효과만 값 복사하며 캡처 대상의 같은 수명은 유지한다.</summary>
    internal PriestShieldHooks Copy() => new()
    {
        Create = Create, SetOwner = SetOwner, Pop = Pop, ReserveSound = ReserveSound,
        LoadSound = LoadSound, AttachSound = AttachSound, NoticeSound = NoticeSound, Translate = Translate, Tell = Tell,
    };
}

/// <summary>원본00493d30의 조회→생성→좌표 캡처→소유자/Pop→소리→현재 로컬 안내를 실행한다.</summary>
public sealed class RawPriestShield
{
    /// <summary>보호막 자산이 속한 동일 원시 풀이다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>같은 실제 해시·현재 역할TYPE를 읽는 조회다.</summary>
    private readonly RawPriestForcefield _lookup;
    /// <summary>콜백 이후 현재 로컬 조건을 다시 읽는 상태다.</summary>
    private readonly PriestShieldState _state;
    /// <summary>원문 함수 객체처럼 복사한 필수 실제 효과다.</summary>
    private readonly PriestShieldHooks _hooks;

    /// <summary>같은 풀과 판본별 필수 효과를 첫 조회/생성 전에 검사한다.</summary>
    public RawPriestShield(RawSidPool pool, RawPriestForcefield lookup, PriestShieldState state, PriestShieldHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(lookup);
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, lookup.Pool()) || hooks.Create is null || hooks.SetOwner is null || hooks.Pop is null
            || hooks.ReserveSound is null || hooks.LoadSound is null || hooks.AttachSound is null || hooks.NoticeSound is null
            || (pool.Edition == OriginalEdition.Patch1078 && (hooks.Translate is null || hooks.Tell is null)))
            throw new ArgumentException("사제 보호막 생성 풀·필수 효과 연결 오류");
        _pool = pool; _lookup = lookup; _state = state; _hooks = hooks.Copy();
    }

    /// <summary>기존 보호막이면 전부 생략하고 새 생성 뒤 좌표·나중 로컬 상태의 읽기 시점을 보존한다.</summary>
    public void Ensure(int priest)
    {
        if (_lookup.Find(priest) != 0) return;
        int shield = _hooks.Create!(_lookup.Type(), 2);
        if (shield < 5 || shield >= _pool.Capacity || (_pool.Slot(shield)[11] & 9) != 0)
            throw new ArgumentException("사제 보호막 생성이 유효 자산 SID를 반환하지 않았다");
        ReadOnlySpan<byte> raw = _pool.Slot(priest);
        float x = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[14..]));
        float y = BitConverter.UInt32BitsToSingle(BinaryPrimitives.ReadUInt32LittleEndian(raw[18..]));
        int ownerOffset = _pool.Edition == OriginalEdition.Patch1078 ? 34 : 32;
        _hooks.SetOwner!(shield, raw[ownerOffset]); _hooks.Pop!(shield, x, y, 0);
        if (_hooks.ReserveSound!())
        {
            uint sound = _hooks.LoadSound!("priestForceField.wav"); _hooks.AttachSound!(shield, sound, 0, 1);
        }
        if (_pool.Slot(priest)[ownerOffset] == _state.LocalPlayer && _state.LoadingDepth == 0 && _state.NoticeClock == 0.0)
        {
            _hooks.NoticeSound!(new PriestShieldNotice());
            if (_pool.Edition == OriginalEdition.Patch1078)
            {
                string text = _hooks.Translate!("PriestImmobile"); _hooks.Tell!(text);
            }
        }
    }

    /// <summary>사제 후반/낙하 어댑터가 같은 실제 풀인지 검증하는 참조다.</summary>
    public RawSidPool Pool() => _pool;

    /// <summary>실제 공장·공통 소유자만 바꾸고 Pop·소리·안내의 나머지 효과를 보존한다.</summary>
    public static PriestShieldHooks MakePriestShieldCreationHooks(RawSidPool pool, RawSquidFactory factory,
        SquidOwner owner, PriestShieldHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(factory);
        ArgumentNullException.ThrowIfNull(owner); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, factory.Pool) || !ReferenceEquals(pool, owner.Pool))
            throw new ArgumentException("보호막·공장·소유자 실제 풀 불일치");
        PriestShieldHooks result = hooks.Copy(); result.Create = (type, flags) => factory.Create(type, (SidAllocation)flags);
        result.SetOwner = (sid, player) => owner.Set(sid, player); return result;
    }
}
