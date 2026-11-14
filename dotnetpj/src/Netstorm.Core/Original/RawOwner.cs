namespace Netstorm.Core.Original;

/// <summary>
/// 소유자 지정이 보는 기본 모드 영역(원본 SquidOwnerMode).
/// </summary>
public sealed class SquidOwnerMode
{
    /// <summary>현재(편집) 모드. 켜져 있으면 작업장 목록을 고친다.</summary>
    public bool Fort { get; set; }

    /// <summary>전투 모드. 편집 모드와 같이 켜져 있어야 작업장 목록을 고친다.</summary>
    public bool Battle { get; set; }

    /// <summary>도전 모드. 켜지면 유효 소유자 번호가 1~39로 바뀐다(원본 CL_NUM_CHAL_PLAYERS).</summary>
    public bool Challenge { get; set; }
}

/// <summary>공통 Owner의 현재 TYPE/선택 진단을 실제 장기 장치에 연결한다.</summary>
public sealed class SquidOwnerDiagnostics
{
    /// <summary>대여할 현재 TYPE 표다. null/빈 표는 생성 시 사본을 사용한다.</summary>
    public IReadOnlyList<OriginalTypeRecord>? Types { get; set; }
    /// <summary>원문 범위 진단의 식/파일/행과 정상 복귀를 연결한다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

/// <summary>같은 실제 모드/장부와 현재 TYPE로 공통 Owner를 실행한다. cpp SquidOwner.h/.cpp의 수명/조회 계약이다.</summary>
public sealed class SquidOwner
{
    /// <summary>원문 물리 슬롯을 빌린다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>생성 시 값 복사한 독립 TYPE 기본 표다.</summary>
    private readonly OriginalTypeRecord[] _types;
    /// <summary>같은 실제 Player별 작업장 목록이다.</summary>
    private readonly RawPostPopState _bookkeeping;
    /// <summary>효과 사이에도 다시 읽는 현재 편집/전투/도전 모드다.</summary>
    private readonly SquidOwnerMode _mode;
    /// <summary>현재 TYPE를 대여하고 함수 객체를 복사한 진단이다.</summary>
    private SquidOwnerDiagnostics _diagnostics = new();
    /// <summary>판본의 TYPE 길이를 확인하고 생성 시 독립 깊은 사본을 소유한다.</summary>
    public SquidOwner(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, RawPostPopState bookkeeping, SquidOwnerMode mode)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(types); ArgumentNullException.ThrowIfNull(bookkeeping); ArgumentNullException.ThrowIfNull(mode);
        int expected = pool.Edition == OriginalEdition.Patch1078 ? 188 : 171;
        if (types.Count != expected) throw new ArgumentException("소유자 지정 타입 판본/크기 오류", nameof(types));
        _pool = pool; _types = types.Select(type => type.Clone()).ToArray(); _bookkeeping = bookkeeping; _mode = mode;
    }
    /// <summary>사전 검증 실패 때 기존 연결을 보존하고 현재 표/진단 함수를 대여/복사한다.</summary>
    public void BindDiagnostics(SquidOwnerDiagnostics diagnostics)
    {
        ArgumentNullException.ThrowIfNull(diagnostics);
        if (diagnostics.Types is { Count: > 0 } types && types.Count != _types.Length) throw new ArgumentException("공통 Owner 현재 TYPE 판본/길이 오류", nameof(diagnostics));
        _diagnostics = new SquidOwnerDiagnostics { Types = diagnostics.Types, Assertion = diagnostics.Assertion };
    }
    /// <summary>실제로 선택된 genus 조회 위치에서만 현재 TYPE 범위를 검사한다.</summary>
    private uint TypeGenus(byte type)
    {
        var current = _diagnostics.Types is { Count: > 0 } types ? types : _types;
        if (type >= current.Count) throw new ArgumentOutOfRangeException(nameof(type), "공통 Owner 현재 TYPE 물리 범위");
        return current[type].Flags2;
    }
    /// <summary>세션 장치가 동일 풀인지 확인할 실제 참조다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>현재 모드/TYPE·선택된 목록과 진단을 원문 공통 본문에 연결한다.</summary>
    public void Set(int sid, uint player)
    {
        var hooks = new CommonOwnerHooks
        {
            Fort = () => _mode.Fort, Battle = () => _mode.Battle, Challenge = () => _mode.Challenge,
            Genus = TypeGenus, Assertion = _diagnostics.Assertion,
            Remove = (owner, value) => { if (owner == 0) return; if (owner >= _bookkeeping.OwnerFactories.Length) throw new ArgumentOutOfRangeException(nameof(owner), "공통 Owner 제거의 현재 Player 물리 범위"); Remove(_bookkeeping.OwnerFactories[owner], value); },
            Append = (owner, value) => { if (owner == 0) return; if (owner >= _bookkeeping.OwnerFactories.Length) throw new ArgumentOutOfRangeException(nameof(owner), "공통 Owner 추가의 현재 Player 물리 범위"); Append(_bookkeeping.OwnerFactories[owner], value); },
        };
        RawCommonOwner.SetCommonSquidOwner(_pool, sid, player, hooks);
    }
    /// <summary>같은 목록의 원문 저장 뒤 개수 증가를 실행한다.</summary>
    private static void Append(RawPostPopList list, int sid) => list.Append(sid);

    /// <summary>같은 목록의 현재 getter/중복 전체 제거/꼬리 보존을 실행한다.</summary>
    private static void Remove(RawPostPopList list, int sid) => list.RemoveAll(unchecked((uint)sid));
}
