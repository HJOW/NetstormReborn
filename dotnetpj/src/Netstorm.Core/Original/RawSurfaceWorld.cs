// 같은 풀·공간 지도에서 표면 객체의 생성·등록·수명·표시를 관리하는 월드의 껍데기를 공유한다.
namespace Netstorm.Core.Original;

// 서피스 월드 모듈을 담는 네임스페이스다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.

// 판본·타입·지형 종류·물리 저장소의 같은 소유 정체성만 두고 실제 조립은 잇지 않는 월드 껍데기다. 적재·표시·삭제·프로세스 본체 조립은 후속 단위에서 잇는다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.
public sealed class RawSurfaceWorld
{
    /// <summary>월드가 따르는 원본 판본이다. 슬롯 오프셋이 다른 모듈이 이 값으로 갈라진다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    private readonly OriginalEdition _edition;

    /// <summary>호출자가 소유하는 같은 전역 TYPE 표다. 조립 때 실제 물리로 옮긴다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>초기 지면에 쓰는 지형 종류 번호다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    private readonly uint _terrainType;

    /// <summary>같은 물리 저장소다. 없으면 이 월드가 새로 만든다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    private readonly RawSurfaceStorage _storage;

    /// <summary>후속 갱신이 필요한 상태 변경이 남았는지다. 원본 Impl.changed에 대응하며 초기값은 true다. 표시 억제와 무관하게 TakeChanged로 한 번만 소비한다. 2026-11-07 F2d 월드 본체 TakeChanged 이식에서 추가됐다.</summary>
    private bool _changed = true;

    /// <summary>중첩 로딩의 표면/사제/자산 후처리가 같은 값을 읽는 로딩 상태다. 원본 Impl.loadState에 대응하며 초기 깊이는 1이다. 런타임 전달과 저장 게임 공유 연결은 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 SetLoadingDepth 이식에서 추가됐다.</summary>
    private readonly IslandPostPopState _loadState = new() { LoadingDepth = 1u };

    /// <summary>같은 판본·타입·지형으로 월드를 만들고 물리 저장소를 빌리거나 새로 만든다.</summary>
    /// <param name="edition">월드가 따르는 원본 판본이다.</param>
    /// <param name="types">같은 전역 TYPE 표다. 비어 있으면 안 된다.</param>
    /// <param name="terrainType">초기 지면에 쓰는 지형 종류 번호다.</param>
    /// <param name="storage">빌릴 물리 저장소다. null이면 같은 판본으로 새로 만든다.</param>
    public RawSurfaceWorld(OriginalEdition edition, IReadOnlyList<OriginalTypeRecord> types, uint terrainType, RawSurfaceStorage? storage = null)
    {
        ArgumentNullException.ThrowIfNull(types);
        if (types.Count == 0)
        {
            throw new ArgumentException("서피스 월드의 실제 TYPE 표가 비었습니다.", nameof(types));
        }

        _edition = edition;
        _types = types;
        _terrainType = terrainType;
        _storage = storage ?? new RawSurfaceStorage(edition);
    }

    /// <summary>월드가 따르는 원본 판본이다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    /// <returns>원본 판본이다.</returns>
    public OriginalEdition Edition() => _edition;

    /// <summary>초기 지면에 쓰는 지형 종류 번호다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    /// <returns>지형 종류 번호다.</returns>
    public uint TerrainType() => _terrainType;

    /// <summary>생성 때 받은 같은 전역 TYPE 표다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    /// <returns>TYPE 표다.</returns>
    public IReadOnlyList<OriginalTypeRecord> Types() => _types;

    /// <summary>빌리거나 새로 만든 같은 물리 저장소다. 2026-11-07 F2d 서피스 월드 껍데기 이식에서 추가됐다.</summary>
    /// <returns>물리 저장소다.</returns>
    public RawSurfaceStorage Storage() => _storage;

    /// <summary>변경 요청을 한 번만 소비한다. 원본 표시가 전체 갱신으로 억제되어도 논리 변경은 남는다. 2026-11-07 F2d 월드 본체 TakeChanged 이식에서 추가됐다.</summary>
    /// <returns>소비 전에 변경이 남았으면 true, 이미 소비했으면 false다.</returns>
    public bool TakeChanged()
    {
        // 원본 std::exchange(impl_->changed, false)를 직역한다.
        bool previous = _changed;
        _changed = false;
        return previous;
    }

    /// <summary>같은 전역 TYPE의 수정 가능 범위를 원본 옵션 소비자에게만 빌린다. 새 사본을 만들지 않는다. 2026-11-07 F2d 월드 본체 MutableTypes 이식에서 추가됐다.</summary>
    /// <returns>생성 때 받은 같은 TYPE 목록의 수정 가능 보기다.</returns>
    public IList<OriginalTypeRecord> MutableTypes()
    {
        // 원본 std::span<RiftTypeRecord>를 직역한다. 목록 자체가 바뀔 수 있으므로 같은 참조를 빌린다.
        if (_types is IList<OriginalTypeRecord> mutable)
        {
            return mutable;
        }

        throw new InvalidOperationException("서피스 월드의 실제 TYPE 표가 수정 가능한 목록이 아닙니다.");
    }

    /// <summary>파생 로더의 중첩 깊이를 표면과 비표면 몸체에 동일하게 전달한다. 런타임 연결 뒤의 전달과 저장 게임 공유 연결은 후속 단위에서 잇는다. 2026-11-07 F2d 월드 본체 SetLoadingDepth 이식에서 추가됐다.</summary>
    /// <param name="depth">쓸 로딩 중첩 깊이다.</param>
    public void SetLoadingDepth(uint depth)
    {
        // 원본 impl_->loadState.loadingDepth 대입을 직역한다. runtimeBound 때의 runtime.loadingDepth 전달은 BindRuntime 단위에서 잇는다.
        _loadState.LoadingDepth = depth;
    }

    /// <summary>현재 로딩 중첩 깊이를 돌려준다. 원본에 없는 C# 조회이며 검사·소비자 연결용이다. 2026-11-07 F2d 월드 본체 SetLoadingDepth 이식에서 추가됐다.</summary>
    /// <returns>현재 로딩 중첩 깊이다.</returns>
    public uint LoadingDepth() => _loadState.LoadingDepth;
}
