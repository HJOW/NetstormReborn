// Native 장기 Host의 기본 삭제/전송 선택 장치와 실제 공통 분배기를 같은 저장소 수명에 연결한다.
// cpppj/src/client/RawSurfaceWorld.h 의 RawSurfaceProcessDevices 구조와
// cpppj/src/client/RawSurfaceStorage.cpp 의 BindProcessDevices/CurrentProcessSelection/BaseDestroy 와
// DefaultProcessAssetHooks/ProcessFormDispatch 장기 본문을 이식한다. 0.569에서 추가됐다. (R4-1 잔여 중 Host 기본 삭제/전송)
// 0.571에서 ContentLifetime 결합을 추가했다.
using Netstorm.Core.Bridges;

namespace Netstorm.Core.Original;

/// <summary>실제 Client의 현재 선택/선택 해제 부모만 장기 수명으로 빌리는 묶음이다.</summary>
public sealed class RawSurfaceProcessDevices
{
    /// <summary>현재 전체 선택 DWORD다. Form이라는 이유로 0/WORD로 합성하지 않는다. 0.569에서 추가됐다.</summary>
    public Func<uint>? Selected { get; set; }

    /// <summary>실제 현재 Client 선택 정리 부모를 선택된 효과 위치에서 호출한다. 0.569에서 추가됐다.</summary>
    public Action? ClearSelection { get; set; }

    /// <summary>원문 함수 묶음처럼 값만 복사하며 장치를 실행하지 않는다. 0.569에서 추가됐다.</summary>
    internal RawSurfaceProcessDevices Copy() => new() { Selected = Selected, ClearSelection = ClearSelection };
}

/// <summary>World 없이 같은 장기 Host와 공통/의존 삭제 본체를 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>실제 장기 선택 장치의 함수 사본이다. 첫 Host/삭제기 전에만 설치한다. 0.569에서 추가됐다.</summary>
    private RawSurfaceProcessDevices _processDevices = new();

    /// <summary>PATCH 공통 삭제 진단의 명시 설치 여부다. World 대여/반납으로 바뀌지 않는다. 0.569에서 추가됐다.</summary>
    private bool _patchDestroyDiagnostics;

    /// <summary>Native의 같은 전역 pre/post 깊이와 현재 Pool/Unpop/TYPE를 공유하는 유일한 기본 삭제기다. 0.569에서 추가됐다.</summary>
    private RawSquidDestroy? _baseDestroy;

    /// <summary>같은 Pool의 현재 공장/TYPE와 가상 삭제를 World 밖에서도 사용하는 장기 감시 본체다. 0.569에서 추가됐다.</summary>
    private RawDependFormHost? _dependForms;

    /// <summary>같은 공장/현재 TYPE와 가상 삭제를 사용하는 장기 Gump 본체다. 0.569에서 추가됐다.</summary>
    private RawGumpFormHost? _gumpForms;

    /// <summary>현재 Pool/Graph/권한을 읽는 같은 장기 가상10 몸체다. 0.569에서 추가됐다.</summary>
    private RawSquidDestroyDispatch? _virtualDestroy;

    /// <summary>같은 공통 삭제 깊이에 실제 자산 가상10과 Process/Depend Form을 등록하는 유일한 분배기다. 0.569에서 추가됐다.</summary>
    private RawSquidFormDispatch? _processForms;

    /// <summary>PATCH 공통 삭제의 실제 Client 진단 결합을 명시 설치한다. World/Host를 만들지 않는다. 0.569에서 추가됐다.</summary>
    public void EnablePatchDestroyDiagnostics()
    {
        // PATCH 판본에서만 실제 Client 진단을 선택한다.
        if (Pool.Edition != OriginalEdition.Patch1078)
        {
            throw new ArgumentException("PATCH 공통 삭제 Client 판본 불일치");
        }

        _patchDestroyDiagnostics = true;
    }

    /// <summary>현재 장치의 삭제 효과를 선택한 지점만 요구하며 복귀 뒤 다음 TYPE 조회를 막지 않는다. 0.569에서 추가됐다.</summary>
    private SquidDestroyDiagnostics? DestroyDiagnostics()
    {
        // 명시 설치 전에는 독립 저장소는 기존 무출력 계약을 유지한다.
        if (!_patchDestroyDiagnostics)
        {
            return null;
        }

        // 실제 Client의 현재 진단/로그와 최초 비용 표를 같은 장기 owner에서 읽는다.
        return new SquidDestroyDiagnostics
        {
            Assertion = (expression, file, line) => AssertDisplay(expression, file, line),
            Trace = (level, text) => TraceGraph(level, text),
            LoadedCostTypes = Types,
        };
    }

    /// <summary>실제 장기 선택 장치만 최초 연결하며 게임 전역 값을 쓰지 않는다. 0.569에서 추가됐다.</summary>
    /// <param name="devices">설치할 실제 장기 기본 선택 장치 묶음이다.</param>
    public void BindProcessDevices(RawSurfaceProcessDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        // Native 전송 뒤 Host/삭제기 전에만 실제 Client 장치를 보관한다.
        if (!UsesNativeTransmissionDevices() || _borrowed || _processes is not null || _baseDestroy is not null || _processDevices.Selected is not null
            || devices.Selected is null || devices.ClearSelection is null)
        {
            throw new ArgumentException("장기 Process 기본 선택 장치의 최초 Native 계약", nameof(devices));
        }

        _processDevices = devices.Copy();
    }

    /// <summary>앞선 진단/전송 효과 뒤 현재 Client 선택을 잘림 없이 다시 읽는다. 0.569에서 추가됐다.</summary>
    public uint CurrentProcessSelection()
    {
        // 선택 조회 장치가 없으면 독립 저장소는 명시 실패한다.
        Func<uint>? selected = _processDevices.Selected;
        if (selected is null)
        {
            throw new InvalidOperationException("장기 Process 삭제의 현재 선택 조회 미연결");
        }

        return selected();
    }

    /// <summary>Native의 같은 전역 pre/post 깊이와 현재 Pool/Unpop/TYPE를 공유한다. 0.569에서 추가됐다.</summary>
    public RawSquidDestroy BaseDestroy()
    {
        if (_baseDestroy is null)
        {
            var destroy = new RawSquidDestroy(Pool, UnpopExecutor(), Types);
            // PATCH 명시 설치 뒤에만 실제 Client 진단 경로를 연결한다.
            SquidDestroyDiagnostics? diagnostics = DestroyDiagnostics();
            if (diagnostics is not null)
            {
                destroy.BindPatchDiagnostics(diagnostics);
            }

            _baseDestroy = destroy;
        }

        return _baseDestroy;
    }

    /// <summary>같은 장기 부모에서 현재 VT의 실제 삭제 효과를 선택한다. 0.569에서 추가됐다.</summary>
    public SquidDestroyHooks DefaultProcessAssetHooks()
    {
        var hooks = new SquidDestroyHooks
        {
            SelectedRaw = () => CurrentProcessSelection(),
            UnpopForm = sid => FactoryExecutor().UnpopForm(sid),
            Emit = @event =>
            {
                // 서버 권한 전송은 같은 장기 writer의 실제 삭제로 알린다.
                if (@event.Effect == SquidDestroyEffect.Transmit)
                {
                    TransmissionExecutor().Transmit(@event.Sid, -1, -2, 0, @event.Flags);
                    return;
                }

                // 선택 해제는 실제 현재 Client 선택 정리 부모를 선택된 효과 위치에서 호출한다.
                if (@event.Effect == SquidDestroyEffect.ClearSelection)
                {
                    Action? clear = _processDevices.ClearSelection;
                    if (clear is null)
                    {
                        throw new InvalidOperationException("장기 Process 삭제의 실제 선택 정리 미연결");
                    }

                    clear();
                    return;
                }

                EmitDefaultAssetEffect(@event);
            },
        };
        return hooks;
    }

    /// <summary>현재 VT의 실제 Pre/Post/부모 통지를 같은 장기 삭제 부모에 분배한다. 0.569에서 추가됐다. 0.573에서 VT 라우팅 골격으로 바꿨다.</summary>
    /// <param name="event">분배할 삭제 효과다.</param>
    private void EmitDefaultAssetEffect(SquidDestroyEvent @event)
    {
        if (!UsesNativeTransmissionDevices())
        {
            throw new InvalidOperationException("장기 자산 가상 효과의 Native 판본 계약");
        }

        NativeAssetDeletionEntry entry = CurrentAssetDeletionEntry(@event.Sid);
        if (@event.Effect == SquidDestroyEffect.ReleaseDependent)
        {
            // 실제 77VT의 빈 ret8이며 상태·TYPE·통지 인자를 추가로 읽지 않는다.
            if (entry.Notice == 0x0041c100u)
            {
                return;
            }

            // 실제 세 VT는 두 인자를 무시하며 서버 관문 없이 현재 가상10(flags0)을 부른다.
            if (entry.Notice == 0x0048f0a0u)
            {
                DestroyAsset(@event.Sid, 0);
                return;
            }

            throw new InvalidOperationException($"장기 자산의 전용 부모 삭제 통지 미연결: 0x{entry.Notice:X8}");
        }

        bool pre = @event.Effect == SquidDestroyEffect.PreDestroy;
        if (!pre && @event.Effect != SquidDestroyEffect.PostDestroy)
        {
            throw new InvalidOperationException("장기 자산의 가상 삭제 효과 종류 오류");
        }

        uint method = pre ? entry.Pre : entry.Post;
        // 공통 삭제 몸체는 장기 본체를 공유한다. 나머지는 다음 단위에서 잇는다.
        if (method == 0x004b0950u)
        {
            CommonDeletion().PreDestroy(@event.Sid, @event.Flags);
            return;
        }

        if (method == 0x004b0840u)
        {
            CommonDeletion().PostDestroy(@event.Sid, @event.Flags);
            return;
        }

        // 실제 제단 Post의 단일 tail jump와 피해 후처리는 같은 장기 본체를 공유한다.
        if (method == 0x00448b60u || method == 0x0044b9e0u)
        {
            DamageablePost().PostDestroy(@event.Sid, @event.Flags);
            return;
        }

        // 실제 Damageable 가상 전처리와 너겟 단일 점프 thunk는 같은 장기 본체를 공유한다.
        if (method == 0x0044b4b0u || method == 0x00484f80u)
        {
            DamageablePre().PreDestroy(@event.Sid, @event.Flags);
            return;
        }

        // 실제 Point Blast의 자연 관문은 같은 장기 본체를 공유한다.
        if (method == 0x0041e960u)
        {
            BombPre().PreDestroy(@event.Sid, @event.Flags);
            return;
        }

        // Outpost는 실제 두 목록 제거 뒤에만 현재 투표/부모를 선택한다.
        if (method == 0x00455840u)
        {
            OutpostPreDestroy(@event.Sid, @event.Flags);
            return;
        }

        // 실제 간헐천 고유 수명 접두는 같은 장기 본체를 공유한다.
        if (method == 0x00485580u)
        {
            GeyserLifetime().PreDestroy(@event.Sid, @event.Flags);
            return;
        }

        // 실제 간헐천 고유 수명 후미는 같은 장기 본체를 공유한다.
        if (method == 0x00485540u)
        {
            GeyserLifetime().PostDestroy(@event.Sid, @event.Flags);
            return;
        }

        // 실제 오벨리스크의 단일 jmp는 현재 공통Pre와 동일한 인자/순서를 사용한다.
        if (method == 0x0041f910u)
        {
            CommonDeletion().PreDestroy(@event.Sid, @event.Flags);
            return;
        }

        // 실제 다섯 Carrier VT는 같은 장기 finder/보충 부모를 사용한다.
        if (method == 0x00426890u)
        {
            CarrierPre().PreDestroy(@event.Sid, @event.Flags);
            return;
        }

        // 네 실제 신전은 같은 전체 장기 부모를 선택한다.
        if (method == 0x004dca50u)
        {
            Action<int, uint>? preBody = _vortexPre;
            if (preBody is null)
            {
                throw new InvalidOperationException("장기 신전의 실제 삭제 전처리 미결합");
            }

            preBody(@event.Sid, @event.Flags);
            return;
        }

        // 실제 세 VT만 같은 WORD 대상 정리·Damageable 부모를 선택한다.
        if (method == 0x0045e7e0u)
        {
            FlyerPre().PreDestroy(@event.Sid, @event.Flags);
            return;
        }

        // 현재 사제 목록/보호막 조회와 실제 Carrier 부모를 공유한다.
        if (method == 0x004919b0u)
        {
            PriestPre().PreDestroy(@event.Sid, @event.Flags);
            return;
        }

        // 같은 실제 50칸 거주 구역 목록을 압축한 뒤 공통 Post다.
        if (method == 0x0049a4a0u)
        {
            ResidencePostDestroy(@event.Sid, @event.Flags);
            return;
        }

        // 기존 30칸 population 제거 본문을 같은 장기 목록에 연결한다.
        if (method == 0x00450920u)
        {
            DudePostDestroy(@event.Sid, @event.Flags);
            return;
        }

        // 다리·섬·무인도 고유 삭제는 같은 장기 표면 수명으로 분배한다.
        if (method == 0x004221b0u || method == 0x00422300u || method == 0x00442240u || method == 0x00442640u)
        {
            (SurfaceLifetime().Hooks().Emit ?? throw new InvalidOperationException("장기 표면 수명 분배의 자산 효과 경계 미연결"))(@event);
            return;
        }

        throw new InvalidOperationException($"장기 자산의 전용 삭제 몸체 미연결: 0x{method:X8}");
    }

    /// <summary>같은 Pool의 현재 공장/TYPE와 가상 삭제를 World 밖에서도 사용하는 장기 감시 본체다. 0.569에서 추가됐다.</summary>
    public RawDependFormHost DependForms()
    {
        if (_dependForms is null)
        {
            _dependForms = new RawDependFormHost(Pool, BaseDestroy());
        }

        return _dependForms;
    }

    /// <summary>같은 공장/현재 TYPE와 가상 삭제와 실제 UI 등록표를 사용하는 장기 Gump 본체다. 0.569에서 추가됐다.</summary>
    public RawGumpFormHost GumpForms()
    {
        if (_gumpForms is null)
        {
            _gumpForms = new RawGumpFormHost(Pool, FactoryExecutor(), Types, BaseDestroy());
        }

        return _gumpForms;
    }

    /// <summary>현재 Pool/Graph/권한을 읽는 같은 장기 가상10 몸체다. 0.569에서 추가됐다.</summary>
    public RawSquidDestroyDispatch VirtualDestroy()
    {
        if (_virtualDestroy is null)
        {
            // 단일 미션 전투의 기본 실행 상태(서버·권한 있음, 편집기 아님)를 빌린다.
            _virtualDestroy = new RawSquidDestroyDispatch(BaseDestroy(), NeighborsExecutor(), BridgeDecayMode.SinglePlayer, GraphExecutor());
        }

        return _virtualDestroy;
    }

    /// <summary>현재 World가 없어도 같은 실제 Host와 공통/의존의 가상10을 결합한다. 0.569에서 추가됐다.</summary>
    public RawSquidFormDispatch ProcessFormDispatch()
    {
        if (_processForms is null)
        {
            var forms = new RawSquidFormDispatch(
                Pool,
                BaseDestroy(),
                DefaultProcessAssetHooks(),
                (sid, flags, hooks) => VirtualDestroy().Destroy(sid, flags, hooks));
            // 현재 World가 없어도 같은 실제 Host를 결합한다.
            forms.Bind(Processes().Host());
            // Native 뒤에는 같은 장기 의존/Gump/Content 수명 Form도 결합한다.
            if (UsesNativeTransmissionDevices())
            {
                forms.Bind(DependForms());
                forms.Bind(GumpForms());
                forms.Bind(ContentLifetime());
            }

            _processForms = forms;
        }

        return _processForms;
    }
}
