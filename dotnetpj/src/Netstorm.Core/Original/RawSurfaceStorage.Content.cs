// Native 장기 Content 장치와 정렬(Form) 본체를 같은 저장소 수명에 연결한다.
// cpppj/src/client/RawSurfaceContent.cpp 의 BindContentDevices/BindContentRuntime 와
// RawSurfaceWorld.h 의 ContentType/ContentState/ContentForms/NoticeContent 장기 본문을 이식한다.
// 0.570에서 추가됐다. (R4-1 잔여 중 Content 장치·정렬 결합, ContentLifetime/Reward 연결은 다음 단위)
namespace Netstorm.Core.Original;

/// <summary>같은 실제 현재 전역과 Client 장치를 장기 본체보다 오래 보관하는 Content 장치 묶음이다.</summary>
public sealed class RawSurfaceContentDevices
{
    /// <summary>미리보기/현재 커서 전체 DWORD의 실제 주소를 별칭으로 빌린다. 0.570에서 추가됐다.</summary>
    public UIntBox? Cursor { get; set; }

    /// <summary>로컬 Player 번호의 실제 DWORD 주소를 별칭으로 빌린다. 0.570에서 추가됐다.</summary>
    public UIntBox? LocalOwner { get; set; }

    /// <summary>내용물 TYPE의 실제 DWORD 주소를 별칭으로 빌린다. 0.570에서 추가됐다.</summary>
    public UIntBox? ContentType { get; set; }

    /// <summary>커서 취소·로컬/원격 수입·위치 소리의 실제 Client 경계다. 지갑/SP는 같은 Reward가 처리하므로 여기 두지 않는다. 0.570에서 추가됐다.</summary>
    public ContentLifetimeHooks Effects { get; set; } = new();

    /// <summary>원문 함수 묶음처럼 값만 복사하며 장치를 실행하지 않는다. 0.570에서 추가됐다.</summary>
    internal RawSurfaceContentDevices Copy() => new()
    {
        Cursor = Cursor,
        LocalOwner = LocalOwner,
        ContentType = ContentType,
        Effects = new()
        {
            CancelCursor = Effects.CancelCursor,
            LocalNotice = Effects.LocalNotice,
            RemoteNotice = Effects.RemoteNotice,
            Sound = Effects.Sound,
        },
    };
}

/// <summary>현재 World가 소유하고 Storage가 약하게 빌리는 큰 Content 통지 경계다.</summary>
public sealed class RawSurfaceContentBindings
{
    /// <summary>파생 부모의 현재 VT+24를 같은 전체 Session 본문에 전달한다. 0.570에서 추가됐다.</summary>
    public ContentFormNotice? Notice { get; set; }

    /// <summary>정렬/부모 통지 뒤 실제 사제 포획 안내를 같은 현재 Runtime에서 실행한다. 0.570에서 추가됐다.</summary>
    public ContentPriestNotice? Captured { get; set; }

    /// <summary>선택 호출의 정상/예외 복귀까지 실제 전체 owner를 보유한다. 0.570에서 추가됐다.</summary>
    public Func<object?>? Pin { get; set; }
}

/// <summary>World 없이 같은 장기 물리/현재 전역으로 Content 정렬 본체를 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 실제 현재 전역과 Client 장치를 장기 본체보다 오래 유지하는 Content 장치다. 0.570에서 추가됐다.</summary>
    private RawSurfaceContentDevices _contentDevices = new();

    /// <summary>파생 Session 효과만 빌리고 반납 뒤 죽은 콜백을 보유하지 않는 약한 Content 대여다. 0.570에서 추가됐다.</summary>
    private WeakReference<RawSurfaceContentBindings>? _contentRuntime;

    /// <summary>장기 상태/Factory보다 먼저 반납하는 동일 정렬 본체다. 0.570에서 추가됐다.</summary>
    private RawContentFormHost? _contentForms;

    /// <summary>같은 현재 전투/checking/TYPE와 Client 진단 장치를 정렬 본체가 빌리는 상태다. 0.570에서 추가됐다.</summary>
    private readonly ContentFormState _contentFormState = new();

    /// <summary>같은 커서/owner/권한/너겟 주소와 장기 수입 명령을 빌리는 Content 수명 상태다. 0.570에서 추가됐다.</summary>
    private readonly ContentLifetimeState _contentLifetimeState = new() { Notice = 0x020000b4u };

    /// <summary>실제 장기 Content 장치가 설치됐는지만 읽는다. 공장/Host/원문 값은 만들지 않는다. 0.570에서 추가됐다.</summary>
    public bool UsesNativeContentDevices() => _contentDevices.Cursor is not null;

    /// <summary>첫 Host/World 앞에서 실제 현재 주소/장치만 연결하고 값을 쓰지 않는다. 0.570에서 추가됐다.</summary>
    /// <param name="devices">설치할 실제 장기 Content 장치 묶음이다.</param>
    public void BindContentDevices(RawSurfaceContentDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        // Native 전송 뒤, 첫 정렬 본체/대여/World 전에만 실제 Client 장치를 보관한다.
        if (!UsesNativeTransmissionDevices() || _contentDevices.Cursor is not null || _containedUnpopExecutor is not null || _borrowed || _contentForms is not null
            || devices.Cursor is null || devices.LocalOwner is null || devices.ContentType is null
            || devices.Effects.CancelCursor is null || devices.Effects.LocalNotice is null || devices.Effects.RemoteNotice is null || devices.Effects.Sound is null)
        {
            throw new ArgumentException("장기 Content의 실제 최초 현재 전역/장치 계약", nameof(devices));
        }

        _contentDevices = devices.Copy();
        _contentLifetimeState.SharedLocalOwner = _contentDevices.LocalOwner;
    }

    /// <summary>큰 통지의 실제 현재 대여만 약하게 보관한다. 0.570에서 추가됐다.</summary>
    /// <param name="runtime">현재 World가 소유하는 실제 Content 통지 대여다.</param>
    public void BindContentRuntime(RawSurfaceContentBindings runtime)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        // Native 뒤에만 연결하며 중복 대여를 거부한다.
        if (!UsesNativeTransmissionDevices())
        {
            throw new ArgumentException("장기 Content의 Native 판본 계약", nameof(runtime));
        }

        // 현재 대여가 살아 있으면 새 World가 덮지 못한다.
        if (_contentRuntime is not null && _contentRuntime.TryGetTarget(out _))
        {
            throw new ArgumentException("현재 Content 파생 통지 대여 중복/누락", nameof(runtime));
        }

        _contentRuntime = new(runtime);
    }

    /// <summary>현재 세션이 소유한 같은 Content 대여만 결정적으로 반납한다.</summary>
    /// <param name="runtime">반납할 현재 세션의 대여다.</param>
    public void ReleaseContentRuntime(RawSurfaceContentBindings runtime)
    {
        if (!ReferenceEquals(ContentRuntime(), runtime)) throw new ArgumentException("Content의 같은 현재 대여 반납 필요", nameof(runtime));
        _contentRuntime = null;
    }

    /// <summary>현재 살아 있는 같은 Content 대여만 선택 호출 동안 강하게 빌린다. 0.570에서 추가됐다.</summary>
    private RawSurfaceContentBindings? ContentRuntime()
    {
        // 약한 대여가 살아 있을 때만 현재 통지를 읽는다.
        if (_contentRuntime is not null && _contentRuntime.TryGetTarget(out RawSurfaceContentBindings? current))
        {
            return current;
        }

        return null;
    }

    /// <summary>Native는 실제 finder의 현재 DWORD를 빌리고 독립 저장소는 기존 내용물 번호를 읽는다. 0.570에서 추가됐다.</summary>
    public uint ContentType() => _contentDevices.ContentType?.Value ?? ContainedType.Value;

    /// <summary>같은 커서/owner/권한/너겟 주소와 장기 수입 명령을 빌린다. 0.570에서 추가됐다.</summary>
    public ContentLifetimeState ContentState()
    {
        // 같은 장기 boss/너겟 주소를 실제 owner에서 빌리며 값 사본을 만들지 않는다.
        _contentLifetimeState.SharedBoss = () => Authority().Boss;
        _contentLifetimeState.SharedNuggetType = GameAux().NuggetTypeBox;
        if (_contentLifetimeState.CurrentCursor is null)
        {
            // 실제 Client는 최초 연결한 finder DWORD를 읽는다.
            _contentLifetimeState.CurrentCursor = () => (_contentDevices.Cursor ?? throw new InvalidOperationException("장기 Content의 실제 현재 cursor owner 미연결")).Value;
        }

        return _contentLifetimeState;
    }

    /// <summary>Native Session/NoWorld가 같은 정렬·부착·현재 TYPE 본체를 사용한다. 0.570에서 추가됐다.</summary>
    public RawContentFormHost ContentForms()
    {
        if (!UsesNativeTransmissionDevices() || _edition != OriginalEdition.Patch1078)
        {
            throw new InvalidOperationException("장기 Content의 실제 PATCH Client 장치 미연결");
        }

        if (_contentForms is null)
        {
            _contentFormState.InBattleFlag.Bind(() => OwnerMode.Battle);
            _contentFormState.CheckingLinksFlag.Bind(() => Checking().Read());
            _contentFormState.ContentType = () => ContentType();
            _contentFormState.Assertion = AssertDisplay;
            // 정렬 본체는 같은 장기 상태/Factory보다 먼저 반납한다.
            var body = new RawContentFormHost(
                Pool,
                Types,
                (parent, form, reason) => NoticeContent(parent, form, reason),
                null,
                _contentFormState,
                (parent, previousOwner) =>
                {
                    // 선택된 큰 후단의 실제 전체 Runtime pin을 보유한다.
                    RawSurfaceContentBindings? current = ContentRuntime();
                    if (current is null)
                    {
                        throw new InvalidOperationException("Content의 실제 현재 포획 안내 Runtime 미연결");
                    }

                    Func<object?>? pin = current.Pin;
                    if (pin is null)
                    {
                        throw new InvalidOperationException("선택된 큰 후단의 실제 전체 Runtime pin 미연결");
                    }

                    object? owner = pin();
                    try
                    {
                        ContentPriestNotice? captured = current.Captured;
                        if (captured is null)
                        {
                            throw new InvalidOperationException("Content의 실제 현재 포획 안내 장치 미연결");
                        }

                        captured(parent, previousOwner);
                    }
                    finally
                    {
                        RawRuntimePin.Release(owner);
                    }
                });
            body.BindCurrentTypes(Types);
            body.BindPop((sid, flags) => FactoryExecutor().PopForm(sid, flags));
            body.BindUnpop(sid => FactoryExecutor().UnpopForm(sid));
            body.BindReattach((sid, parent, flags) =>
            {
                // 네 내장 Form의 실제 가상50만 선택한다.
                ReadOnlySpan<byte> raw = Pool.Slot(sid);
                uint table = (uint)(raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24));
                // 각 가상 테이블을 같은 풀의 현재 판본으로 조회한다.
                foreach (uint type in new uint[] { 1u, 2u, 3u, 6u })
                {
                    // 판본별 실제 Form VT를 같은 원문 표에서 읽는다.
                    uint expected = RawSquidForms.VtableFor(type);
                    if (table == expected)
                    {
                        if (type == 6u)
                        {
                            body.Attach(sid, parent, flags);
                            return;
                        }

                        FactoryExecutor().AttachForm(sid, parent, flags);
                        return;
                    }
                }

                throw new InvalidOperationException("Content Regular의 현재 가상50 미연결");
            });
            _contentForms = body;
        }

        return _contentForms;
    }

    /// <summary>실제 빈 부모와 파생 부모를 현재 VT로 구분한다. 0.570에서 추가됐다.</summary>
    /// <param name="parent">통지를 받는 부모 번호다.</param>
    /// <param name="form">부착·제거된 Form 번호다.</param>
    /// <param name="reason">1 = 부착, 2 = 제거다.</param>
    public void NoticeContent(int parent, int form, uint reason)
    {
        var notice = new RawContentParentNotice(
            Pool,
            (selectedParent, selectedForm, selectedReason) =>
            {
                // 파생 호출만 실제 전체 Runtime pin을 보유한다.
                RawSurfaceContentBindings? current = ContentRuntime();
                if (current is null)
                {
                    throw new InvalidOperationException("Content의 실제 현재 파생 부모 Runtime 미연결");
                }

                Func<object?>? pin = current.Pin;
                if (pin is null)
                {
                    throw new InvalidOperationException("선택된 큰 후단의 실제 전체 Runtime pin 미연결");
                }

                object? owner = pin();
                try
                {
                    ContentFormNotice? call = current.Notice;
                    if (call is null)
                    {
                        throw new InvalidOperationException("Content의 실제 현재 파생 부모 가상24 미연결");
                    }

                    call(selectedParent, selectedForm, selectedReason);
                }
                finally
                {
                    RawRuntimePin.Release(owner);
                }
            });
        notice.Notice(parent, form, reason);
    }
}
