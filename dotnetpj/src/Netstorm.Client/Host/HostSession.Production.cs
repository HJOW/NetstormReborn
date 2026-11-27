using System.Buffers.Binary;
using Netstorm.Client.Mission;
using Netstorm.Core.Original;
using TotalMadeView = Netstorm.Core.Simulation.TotalMade;

// 같은 실제 내용물·Player·장부·Deck·연구와 외부 UI/설정/운반 장치를 연결한다.
namespace Netstorm.Client.Host;

/// <summary>생산 계산을 대신하지 않는 실제 외부 UI·설정·입력·로그·송신 장치다.</summary>
public sealed class CampaignProductionDevices
{
    /// <summary>현재 실제 getTeleportTechFree 설정을 조회한다.</summary>
    public Func<string, int>? ConfigInt { get; set; }
    /// <summary>현재 DWORD 등록ID의 실제 Gump를 조회한다.</summary>
    public Func<uint, uint>? Gump { get; set; }
    /// <summary>선택된 실제 Gump 가상+28을 호출한다.</summary>
    public Action<uint, uint, uint, uint>? Handle { get; set; }
    /// <summary>종속 해제의 실제 UI 명령을 원문 입력 큐에 넣는다.</summary>
    public Action<uint, uint, uint>? Command { get; set; }
    /// <summary>실제 종류/문장을 보존하는 로그다.</summary>
    public Action<uint, string>? Log { get; set; }
    /// <summary>선택된 연구 요청의 실제 채널9 운반이다. 미설치는 입력 호출 위치에서 거부한다.</summary>
    public Action<uint, ReadOnlyMemory<byte>>? Send { get; set; }
    /// <summary>선택된 researchHelp 절의 실제 모드별 도움말 UI다.</summary>
    public Action<string, uint>? ResearchHelp { get; set; }
    /// <summary>같은 Carrier의 실제 번개/음향/예약/공간 장치다.미설치 효과는 선택 시 실패한다.</summary>
    public CarrierContentNoticeHooks? CarrierContent { get; set; }
    /// <summary>같은 실제 Session 커서 부모의 Cancel이다. 설치 전 호출은 별도 관문이 거부한다.</summary>
    public Action? CancelCursor { get; set; }
}

/// <summary>RawCampaignSession.cpp의 생산/연구와 같은 현재 부모 통지를 조립한다.</summary>
public sealed partial class HostSession
{
    /// <summary>현재 같은 실제 Player의 Deck 뷰다. 장기 블록은 Gameplay가 소유한다.</summary>
    private RawProductionDeck? _worldProductionDeck;
    /// <summary>현재 장부 세 표와 장기 금지 표의 동일 뷰다.</summary>
    private TotalMadeView? _worldProductionTotals;
    /// <summary>같은 현재 Player/공장 목록/Deck을 재계산하는 본체다.</summary>
    private RawCampaignProductionBindings? _worldProduction;
    /// <summary>현재 연구 입력/수신/부모 통지 본체다.</summary>
    private RawFactoryResearch? _worldResearch;
    /// <summary>현재 같은 SP와 실제 Gump를 사용하는 Player 통지다.</summary>
    private RawPlayerContentNotice? _worldPlayerContent;
    /// <summary>현재 로딩/저장·사제 역할과 같은 Carrier 내용물 가상 통지다.</summary>
    private RawCarrierContentNotice? _worldCarrierContent;
    /// <summary>현재 실제 정렬 부착과 같은 부모 제거 통지다.</summary>
    private RawContentFormHost? _worldContentForms;
    /// <summary>같은 생산 Form과 자산 수신이 현재 UI 통지에 공유하는 실제 종속 종단이다.</summary>
    private RawSquidContainedUnpop? _worldContentDetach;
    /// <summary>같은 장기 저장소가 약하게 빌리는 현재 세션의 내용물 통지다.</summary>
    private RawSurfaceContentBindings? _nativeContentRuntime;
    /// <summary>AI와 생산이 함께 사용하는 현재 내용물 역할/권한 뷰다.</summary>
    private ContainedFinderState? _worldProductionFinder;
    /// <summary>현재 생산 Deck을 빌린다. 반납 뒤에는 기존 Player에 접근하지 않는다.</summary>
    public RawProductionDeck WorldProductionDeck => _worldProductionDeck ?? throw new InvalidOperationException("호스트 생산 Deck 미연결");
    /// <summary>같은 현재 금지 표/집계를 메뉴와 미션 소비자가 공유한다.</summary>
    public TotalMadeView WorldProductionTotals => _worldProductionTotals ?? throw new InvalidOperationException("호스트 생산 금지 표 미연결");
    /// <summary>같은 실제 생산 재계산과 슬롯 집계다.</summary>
    public RawCampaignProductionBindings WorldProduction => _worldProduction ?? throw new InvalidOperationException("호스트 생산 재계산 미연결");
    /// <summary>현재 생산/Player 부모로 기술을 표시한다. loaded/known/TYPE/phase의 새 관문을 추가하지 않는다.</summary>
    public void IndicateTechnology(uint type) => WorldProduction.IndicateTechnology(type);
    /// <summary>같은 실제 연구 입력/수신/부모 통지다.</summary>
    public RawFactoryResearch WorldResearch => _worldResearch ?? throw new InvalidOperationException("호스트 시설 연구 미연결");
    /// <summary>현재 같은 Carrier 내용물 본체를 실제 통지 소비자에게 빌린다.</summary>
    public RawCarrierContentNotice WorldCarrierContent => _worldCarrierContent ?? throw new InvalidOperationException("호스트 Carrier 내용물 통지 미연결");
    /// <summary>요새 적재·연구와 같은 실제 내용물 생성/정렬 부착을 월드 소비자에게 빌린다.</summary>
    public RawContainedCreate WorldContentCreate => _fortContained ?? throw new InvalidOperationException("호스트 실제 내용물 생성 미연결");
    /// <summary>설치된 같은 연구/업그레이드 수신을 실제 공유 패킷 표에 등록한다.대여/반납은 전송 owner가 맡는다.</summary>
    public void RegisterFactoryResearchCommands(Netstorm.Net.RawPacketDispatch dispatch) => Netstorm.Net.FactoryResearchRegistration.Register(dispatch, WorldResearch);
    /// <summary>생산/AI의 현재 내용물 상태를 하나만 만들고 같은 역할/권한/진단 전역을 연결한다.</summary>
    private ContainedFinderState SessionContainedFinder
    {
        get
        {
            if (_worldProductionFinder is not null) return _worldProductionFinder;
            var finder = Gameplay.ProductionFinder;
            finder.Assertion ??= static (text, file, line) => throw new InvalidOperationException($"내용물 조회 원본 진단: {text} {file}:{line}");
            _worldProductionFinder = finder; return finder;
        }
    }
    /// <summary>초기 UI와 AI/생산이 같은 실제 장기 검색 전역을 빌린다.</summary>
    public ContainedFinderState ProductionFinder => SessionContainedFinder;
    /// <summary>계산/생성/삭제를 복제하지 않고 기존 실제 Player·공장·SP·현재 장부/역할에 연결한다.</summary>
    public RawCampaignProductionBindings BuildProductionWorldDevices(CampaignProductionDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        if (_worldProduction is not null) throw new InvalidOperationException("호스트 생산/연구 중복 연결");
        if (devices.ConfigInt is null || devices.Gump is null || devices.Handle is null || devices.Command is null || devices.Log is null)
            throw new ArgumentException("호스트 생산 실제 설정/UI/입력/로그 장치 누락", nameof(devices));
        _ = BindLivePlayerWorld(); RawPlayerWorld player = PlayerWorld; _ = WorldFlare;
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes ?? throw new InvalidOperationException("호스트 생산 현재 TYPE 미연결");
        RawContainedCreate create = _fortContained ?? throw new InvalidOperationException("호스트 생산 실제 내용물 생성 미연결");
        Func<string, int> config = devices.ConfigInt; Func<uint, uint> gump = devices.Gump;
        Action<uint, uint, uint, uint> handle = devices.Handle; Action<uint, uint, uint> command = devices.Command; Action<uint, string> log = devices.Log;
        Action<uint, ReadOnlyMemory<byte>>? send = devices.Send; Action<string, uint>? help = devices.ResearchHelp;
        var deck = new RawProductionDeck(player, raw => raw.Clear(), where => throw new InvalidOperationException($"생산 Deck 원본 진단: {where}"), storage: Gameplay.ProductionDeckStorage);
        try
        {
            var totals = new TotalMadeView(types, TotalMade, Gameplay.ProductionRestrictions,
                where => throw new InvalidOperationException($"생산 집계 원본 진단: {where}"));
            ContainedFinderState finder = SessionContainedFinder;
            var carrier = new RawCarrierPreDestroy(Pool, finder, new()
            {
                ValidateDamageable = (_, _) => throw new InvalidOperationException("생산 검색의 캐리어 파괴 검사는 별도 실제 경로가 필요합니다"),
                DamageablePre = (_, _) => throw new InvalidOperationException("생산 검색의 캐리어 파괴는 별도 실제 경로가 필요합니다"),
                DependentFound = () => throw new InvalidOperationException("생산 검색의 종속 파괴 후처리는 별도 실제 경로가 필요합니다"),
            });
            var production = new RawCampaignProductionBindings(Pool, types, TerrainBookkeeping, OwnerMode, player, carrier, create, deck, totals, finder,
                new() { Mission = () => BridgeState.Mission, FullDeck = () => Gameplay.FullDeckBox.Value,
                    FreeTechnologyType = () => Gameplay.FreeTechnologyTypeBox.Value, TempleProduct = () => GameAux.TempleProductTypeBox.Value },
                new() { ConfigInt = config, ModeAssertion = () => throw new InvalidOperationException("생산 원본 진단: inFortMode || inBattleMode") });
            RawProductionSync sync = CreateProductionSync(player, deck, carrier, create, types, finder);
            var playerNotice = new RawPlayerContentNotice(Pool, new()
            {
                CursorA = () => Sp.OffsetCurrent, CursorB = () => unchecked((uint)Sp.TrackingOffsetCurrent.Value),
                ProblemCode = Gameplay.PatchDiagnostic, FallbackCode = () => Gameplay.PatchSample.Value, MoneyChecks = Gameplay.ContentMoneyChecksBox,
                BattleShown = () => MissionPlayer.PlayerKnown, LocalPlayer = () => MissionPlayer.BattleOwner,
                HasFirstSpPool = () => Sp.FirstPoolCurrent is not null, FirstPermutation = () => Sp.Permutation[0],
                GumpFormType = () => Gameplay.GumpFormTypeBox.Value, ContentEvent = () => Gameplay.ContentEventBox.Value,
            }, new() { Money = owner => BitConverter.UInt32BitsToSingle(Sp.Get(owner)), Gump = gump, Handle = handle });
            var parentNotice = new RawContentParentNotice(Pool, NotifyProductionParentContent);
            CarrierContentNoticeHooks carrierEffects = devices.CarrierContent?.Copy() ?? new();
            carrierEffects.CheckCarrier ??= parent => { _ = PriestWorld.CheckCarrier(parent); };
            carrierEffects.TypeAt ??= FindWorldTypeAt;
            var carrierNotice = new RawCarrierContentNotice(Pool, types, new()
            {
                SavedGame = () => Gameplay.SavedGameBox.Value, LoadingDepth = () => _worldLoadingDepth,
                BombBlocked = () => Gameplay.ContentBombBlockedBox.Value, LocalOwner = () => MissionPlayer.BattleOwner,
                PriestType = () => GameAux.PriestTypeBox.Value, FreeSoundBlockerType = () => GameAux.RainWalkerTypeBox.Value,
                CameraX = () => Gameplay.ContentScreenOrigin[0], CameraY = () => Gameplay.ContentScreenOrigin[1],
            }, carrierEffects);
            var contentState = new ContentFormState { ContentType = () => Gameplay.ContentFormTypeBox.Value,
                Assertion = (text, file, line) => throw new InvalidOperationException($"내용물 부착 원본 진단: {text} {file}:{line}") };
            contentState.InBattleFlag.Bind(() => OwnerMode.Battle); contentState.CheckingLinksFlag.Bind(() => AiChecking.Read() != 0);
            var forms = new RawContentFormHost(Pool, types, parentNotice.Notice, state: contentState); forms.BindCurrentTypes(types);
            var detach = new RawSquidContainedUnpop(Pool, new()
            {
                ContentFormType = Gameplay.ContentFormTypeBox, GumpFormType = Gameplay.GumpFormTypeBox, UiEvent = Gameplay.ContentRemovalEventBox,
                CheckingDead = new(() => AiChecking.Read()), InMetamapMode = () => OwnerMode.Challenge, Boss = () => Gameplay.Authority.Boss,
            }, new()
            {
                Parent = sid => RawContainedLink.CommonContainedParent(Pool, sid), Previous = sid => RawContainedLink.CommonContainedPrevious(Pool, sid),
                SetPrevious = (sid, previous) => RawContainedLink.CommonContainedSetPrevious(Pool, sid, previous), ParentNotice = parentNotice.Notice,
                Command = command, Assertion = contentState.Assertion,
            });
            forms.Forms.BindUnpop(new() { Detach = detach.Detach, Assertion = contentState.Assertion });
            forms.BindPop((sid, flags) => _fortFactory!.PopForm(sid, flags,
                (form, requested) => (_worldTransmission ?? throw new InvalidOperationException("내용물 Form 실제 패킷 writer 미연결")).Pop(form, requested)));
            var research = new RawFactoryResearch(Pool, types, new()
            {
                LoadingDepth = () => _worldLoadingDepth, ProductionDirty = TerrainBookkeeping.ProductionDirtyBox,
                Boss = () => Gameplay.Authority.Boss, Now = () => ProcessTime.Value, OutpostType = () => Gameplay.OutpostTypeBox.Value,
            }, new()
            {
                Find = (parent, mask, kind, ordinal) => carrier.FindContained(parent, mask, kind, ordinal), Slots = parent => production.AppendFactoryProducts(parent, 0),
                Create = create.Create, Destroy = BuildSessionDestroy("연구 내용물", PlayerProcesses), Refresh = owner => { _ = production.Refresh(owner); },
                Flare = (parent, count, ev) => WorldFlare.Spawn(parent, count, ev),
                Send = (channel, packet) => (send ?? throw new InvalidOperationException("연구 실제 채널9 운반 미연결"))(channel, packet),
                Help = (section, mode) => (help ?? throw new InvalidOperationException("연구 실제 도움말 UI 미연결(R5)"))(section, mode),
                InputLog = (parent, slots) => log(5, $"{Pool.Slot(parent)[34]}: slotsRemaining/A={slots}\n"),
                UpgradeReceive = packet => WorldFactoryUpgradeCommand.Receive(packet),
                Assertion = text => throw new InvalidOperationException($"연구 원본 진단: {text}"),
            });
            // Native 저장소는 현재 세션 본체와 외곽 Runtime만 빌린다. 포획 안내는 미복원 상태를 유지한다.
            if (_surfaceStorage is { } storage && storage.UsesNativeTransmissionDevices())
            {
                var contentRuntime = new RawSurfaceContentBindings { Notice = NotifyProductionParentContent, Pin = () => PinRuntime() };
                storage.BindContentRuntime(contentRuntime); _nativeContentRuntime = contentRuntime;
            }
            _worldProductionDeck = deck; _worldProductionTotals = totals; _worldProduction = production;
            _worldProductionSync = sync; _productionCancelCursor = devices.CancelCursor;
            _worldPlayerContent = playerNotice; _worldCarrierContent = carrierNotice; _worldContentForms = forms; _worldContentDetach = detach; _worldResearch = research;
            return production;
        }
        catch { deck.Dispose(); throw; }
    }
    /// <summary>현재 실제 VT+24에 해당하는 연구/Mana/Player 본체만 실행하며 빈 통지는 상위 분배기가 처리한다.</summary>
    private void NotifyProductionParentContent(int parent, int form, uint reason)
    {
        uint entry = new RawContentParentNotice(Pool).NativeEntry(parent);
        if (entry == 0x453b50) { WorldResearch.Notice(parent, form, reason); return; }
        if (entry == 0x475220) { WorldResearch.ManaNotice(parent, form, reason); return; }
        if (entry == 0x490750) { (_worldPlayerContent ?? throw new InvalidOperationException("호스트 실제 Player 내용물 통지 미연결")).Notify(parent, (uint)form, reason); return; }
        if (entry == 0x429560) { WorldCarrierContent.Notice(parent, form, reason); return; }
        throw new InvalidOperationException($"현재 내용물 부모의 실제 가상24 미연결: 0x{entry:X8}");
    }
    /// <summary>기존 생성기를 유지하며 실제 내용물 가상 부착을 현재 설치된 정렬/통지 장치로 보낸다.</summary>
    private void AttachWorldContainedForm(int form, int parent, uint flags)
    {
        if (_worldContentForms is { } forms) forms.AttachCurrent(form, parent, flags);
        else (_fortForms ?? throw new InvalidOperationException("호스트 공통 Form 부착 미연결")).Attach(form, parent, flags);
    }
    /// <summary>현재 실제 ContentForm 가상 표만 동일 원문 Unpop/부모 제거 통지로 분배한다.</summary>
    private bool TryUnpopProductionContent(int sid)
    {
        if (_worldContentForms is not { } forms || BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid)) != 0x512208) return false;
        forms.Unpop(sid); return true;
    }
    /// <summary>프로세스 정리가 끝난 뒤 같은 실제 링크 부모를 정상 반납하고 현재 장치를 놓는다.</summary>
    private void ReleaseProductionWorld()
    {
        if (_worldProductionDeck is not { } deck) return;
        if (!deck.PlayerLinksReturnedForState) deck.ReturnPlayerLinksForState(deck.CleanupPlayerLinksForState(() => PlayerRecords));
        // 다른 세션의 대여는 지우지 않고 현재 세션의 내용물 후단만 반납한다.
        if (_nativeContentRuntime is { } contentRuntime) { _surfaceStorage!.ReleaseContentRuntime(contentRuntime); _nativeContentRuntime = null; }
        deck.Dispose(); _worldProductionDeck = null; _worldProductionTotals = null; _worldProduction = null;
        _worldProductionSync = null; _productionCancelCursor = null;
        _worldResearch = null; _worldPlayerContent = null; _worldCarrierContent = null; _worldContentForms = null; _worldContentDetach = null; _worldProductionFinder = null;
    }
}
