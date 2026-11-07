// 같은 풀·공간 지도에서 표면 객체의 생성·등록·수명·표시를 관리하는 월드의 껍데기를 공유한다.
using Netstorm.Core.Bridges;

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

    /// <summary>붕괴 처리가 읽는 원본 실행 상태 전역이다. 원본 Impl.mode에 대응하며 초기값은 싱글 플레이(로컬이 서버) 전투 화면이다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    private BridgeDecayMode _mode = BridgeDecayMode.SinglePlayer;

    /// <summary>런타임 경계를 설치했는지다. 원본 Impl.runtimeBound에 대응한다. 첫 객체 생성 전 한 번만 연결하며 BeginLoad·FinishLoad·BindRuntime 단위에서 갱신한다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    private bool _runtimeBound = false;

    /// <summary>적재 작업 중인지다. 원본 Impl.loading에 대응한다. BeginLoad·FinishLoad 단위에서 갱신한다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    private bool _loading = false;

    /// <summary>현재 자료의 적재가 끝났는지다. 원본 Impl.loaded에 대응한다. FinishLoad 단위에서 갱신한다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    private bool _loaded = false;

    /// <summary>프레임의 게임 시각과 벽 시각 및 시간차다. 원본 Impl.time에 대응하며 초기값은 0이다. 2026-11-07 F2d 월드 본체 PrepareFrame 이식에서 추가됐다.</summary>
    private FrameTime _time;

    /// <summary>프로세스 실행·부착·이벤트 분배에 쓰는 같은 저장소 상태다. 원본 Impl.processState에 대응하며 호출자 저장소보다 오래 살 수 없다. 2026-11-07 F2d 월드 본체 PrepareFrame 이식에서 추가됐다.</summary>
    private readonly SquidProcessState _processState;

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
        _processState = _storage.Processes().State();
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

    /// <summary>최초 객체·런타임 등록 전에 원본 모드의 실제 소유권·연결색·다리 붕괴 소비자를 함께 설정한다. 세션 전역만 바꾸고 비공개 편집 값은 남기지 않는다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    /// <param name="fort">현재(편집) 모드다. 켜져 있으면 작업장 목록을 고친다.</param>
    /// <param name="battle">전투 모드다. 연결색과 작업장 목록 조건에 쓴다.</param>
    /// <param name="challenge">도전 모드다. 켜지면 유효 소유자 번호가 1~39로 바뀐다.</param>
    /// <param name="editor">편집기다. 켜져 있으면 다리 붕괴 스캔을 돌리지 않는다.</param>
    public void ConfigureInstallModes(bool fort, bool battle, bool challenge, bool editor)
    {
        // 원본 첫 객체/Runtime 등록 전 관문을 직역한다. 지정 Take가 freeCount를 보존할 수 있어 실제 슬롯도 함께 검사한다.
        RawSidPool pool = _storage.Pool;
        if (_runtimeBound || _loading || _loaded || pool.FreeCount != (uint)(pool.Capacity - RawSidPool.FirstOrdinary))
        {
            throw new InvalidOperationException("실제 설치 모드는 첫 객체·런타임 등록 전에 설정해야 합니다.");
        }

        // 이미 만든 실제 슬롯이 하나라도 있으면 늦은 설정을 거부한다.
        for (int sid = RawSidPool.FirstOrdinary; sid < pool.Capacity; sid++)
        {
            if ((pool.Slot(sid)[RawSidPool.StateOffset] & RawSidPool.FreeBit) == 0)
            {
                throw new InvalidOperationException("실제 설치 모드 설정 전에 객체가 존재합니다.");
            }
        }

        SquidOwnerMode ownerMode = _storage.OwnerMode;
        ownerMode.Fort = fort;
        ownerMode.Battle = battle;
        ownerMode.Challenge = challenge;
        _storage.Links.Battle = battle;
        _mode = _mode with { Editor = editor };
    }

    /// <summary>현재 붕괴 처리 실행 상태 전역을 돌려준다. 원본에 없는 C# 조회이며 검사·소비자 연결용이다. 2026-11-07 F2d 월드 본체 ConfigureInstallModes 이식에서 추가됐다.</summary>
    /// <returns>현재 실행 상태 전역이다.</returns>
    public BridgeDecayMode DecayMode() => _mode;

    /// <summary>빈·죽음·무효·포함 상태를 동시에 거르는 마스크다. 2026-11-07 F2d 월드 본체 Objects 이식에서 추가됐다.</summary>
    private const byte InactiveMask = 15;

    /// <summary>정렬되지 않은 원본 필드를 리틀 엔디언으로 읽는다. 2026-11-07 F2d 월드 본체 Objects 이식에서 추가됐다.</summary>
    /// <param name="bytes">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 폭이다. CD의 인접 extra 바이트를 섞지 않게 지정 폭만 읽는다.</param>
    /// <returns>읽은 값이다.</returns>
    private static uint ReadSlot(ReadOnlySpan<byte> bytes, int offset, int width)
    {
        // 지정 폭만 읽어 필드 밖의 바이트는 섞지 않는다.
        uint value = 0u;
        for (int i = 0; i < width; i++)
        {
            value |= (uint)bytes[offset + i] << (8 * i);
        }

        return value;
    }

    /// <summary>표시·검사에 현재 살아 있는 자산만 제공한다. ProcessForm은 제외한다. 2026-11-07 F2d 월드 본체 Objects 이식에서 추가됐다.</summary>
    /// <returns>살아 있는 자산의 스냅샷 목록이다.</returns>
    public List<SurfaceSnapshot> Objects()
    {
        // 현재 살아 있는 자산 번호만 읽는다. ProcessForm·반납 슬롯은 표시 대상이 아니다.
        var result = new List<SurfaceSnapshot>();
        bool patch = _edition == OriginalEdition.Patch1078;
        RawSidPool pool = _storage.Pool;
        for (int sid = RawSidPool.FirstOrdinary; sid < pool.Capacity; sid++)
        {
            ReadOnlySpan<byte> raw = pool.Slot(sid);
            if ((raw[RawSidPool.StateOffset] & InactiveMask) != 0 || raw[RawSidPool.TypeOffset] < OriginalConstructors.FirstAssetType)
            {
                continue;
            }

            uint type = raw[RawSidPool.TypeOffset];
            uint owner = raw[patch ? 34 : 32];
            int frame = unchecked((int)ReadSlot(raw, patch ? 36 : 34, patch ? 4 : 1));
            float x = BitConverter.UInt32BitsToSingle(ReadSlot(raw, 14, 4));
            float y = BitConverter.UInt32BitsToSingle(ReadSlot(raw, 18, 4));
            result.Add(new SurfaceSnapshot(sid, new SurfaceSeed(type, owner, frame, x, y)));
        }

        return result;
    }

    /// <summary>같은 시각만 공급한다. 실제 Client는 이후 전역 Kernel을 정확히 한 번 실행한다. 2026-11-07 F2d 월드 본체 PrepareFrame 이식에서 추가됐다.</summary>
    /// <param name="time">공급할 프레임 시각이다.</param>
    public void PrepareFrame(FrameTime time)
    {
        // 실제 현재 시각으로 독립 스냅샷의 게임 값만 교정한다.
        if (_processState.SharedNow is not null)
        {
            time.Game = _processState.Now;
        }

        if (!double.IsFinite(time.Game) || time.Game < _time.Game)
        {
            throw new ArgumentException("원본 월드의 역행·비유한 게임 시각입니다.", nameof(time));
        }

        // bound Client 시각은 스냅샷으로 역쓰기하지 않는다.
        if (_processState.SharedNow is null)
        {
            _processState.Now = time.Game;
        }

        _time = time;
    }

    /// <summary>현재 공급된 프레임 시각을 돌려준다. 원본에 없는 C# 조회이며 검사·소비자 연결용이다. 2026-11-07 F2d 월드 본체 PrepareFrame 이식에서 추가됐다.</summary>
    /// <returns>현재 프레임 시각이다.</returns>
    public FrameTime CurrentTime() => _time;

    /// <summary>같은 월드·전송·공간 재등록을 참가자별로 실행한다. 멀티플레이 3차 사전 구현이며 전송기가 없어 명시 오류로 멈춘다. 2026-11-07 F2d 월드 본체 전송 사전 구현에서 추가됐다.</summary>
    /// <param name="owner">동기화할 참가자 번호다.</param>
    public void SynchronizePeer(uint owner)
    {
        // 원본 squidTransmission 미설치 logic_error를 직역한다. 실제 PeerWorldSync 연결은 3차 로컬 구현 때 잇는다.
        throw new InvalidOperationException("현재 판본의 실제 Squid 전송기가 설치되지 않았습니다.");
    }

    /// <summary>공통 삭제의 패킷을 같은 현재 가상 기록기·버퍼·방송 부모로 전달한다. 멀티플레이 3차 사전 구현이며 전송기가 없어 명시 오류로 멈춘다. 2026-11-07 F2d 월드 본체 전송 사전 구현에서 추가됐다.</summary>
    /// <param name="sid">삭제 패킷을 보낼 객체 번호다.</param>
    /// <param name="flags">삭제 전달 플래그다.</param>
    public void TransmitDeletion(int sid, uint flags)
    {
        // 원본 squidTransmission 미설치 logic_error를 직역한다. 실제 Transmit 연결은 3차 로컬 구현 때 잇는다.
        throw new InvalidOperationException("현재 판본의 실제 삭제 전송기가 설치되지 않았습니다.");
    }
}
