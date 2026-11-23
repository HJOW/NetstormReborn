// 같은 실제 Surface/Runtime을 빌리는 Root 첫 장치 묶음을 조립한다.
// cpppj/src/client/ClientMain.cpp 447~536행의 PostPop 선택 재검사·Resource·Reward·Deletion·PlayerRecords와
// 현재 Menu 데모 getter·CreateAiWallet 실제 Native Reward 공급을 이식한다.
// 0.901에서 RU-1 실제 owner 조립용으로 추가됐다.
using Netstorm.Core.Original;

namespace Netstorm.Client;

/// <summary>첫 World/Host 전에 같은 실제 Client 장치만 설치하는 Root 첫 장치 묶음이다.</summary>
public sealed partial class ClientSurfaceGlobals
{
    /// <summary>첫 Process 저장소 생성 뒤·첫 메뉴/World 전에 같은 실제 장치를 설치한다. 0.901에서 추가됐다.</summary>
    /// <param name="sp">같은 실제 SP 저장소 owner다.</param>
    /// <param name="random">같은 실제 게임 난수 owner다.</param>
    /// <param name="demo">현재 메뉴 데모 getter다. 미션 없으면 거짓이다.</param>
    /// <param name="refreshSp">실제 SP 표시 갱신 장치다. 메뉴 없으면 거부한다.</param>
    /// <param name="rememberPlacement">실제 UI 기억점 장치다.</param>
    /// <param name="unitLost">실제 전역 소리 장치다.</param>
    public void BindNativeRootFirstDevices(
        ClientSpGlobals sp,
        GameRandom random,
        Func<bool> demo,
        Action refreshSp,
        Action<uint, uint, uint> rememberPlacement,
        Action<int> unitLost)
    {
        ArgumentNullException.ThrowIfNull(sp);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(demo);
        ArgumentNullException.ThrowIfNull(refreshSp);
        ArgumentNullException.ThrowIfNull(rememberPlacement);
        ArgumentNullException.ThrowIfNull(unitLost);

        // PostPop 선택 재검사·실제 AI 통지는 같은 현재 Player/기억 선택을 읽는다.
        // 근거: cpppj ClientMain.cpp 448~455행.
        Storage.BindPostPopDevices(new SquidPostPopDevices
        {
            AiNotice = (owner, sid, added) => NotifyAiObject(owner, sid, added),
            LocalOwner = () => Gameplay.Player.BattleOwner,
            RecheckSelection = RecheckCurrentSelection,
        });

        // 자원 일곱 항목은 실제 현재 게임 난수와 선택된 큰 하위만 같은 전체 Runtime에서 호출한다.
        // 근거: cpppj ClientMain.cpp 468~478행.
        Storage.BindResourceDevices(new RawSurfaceResourceDevices
        {
            Random = () => random,
            CarrierCheck = sid =>
            {
                Host.HostSession session = Gameplay.RequireSession();
                using IDisposable pin = Gameplay.PinCurrentRuntime(session);
                session.CheckCarrierPostPop(sid);
            },
            DamageableRegular = (sid, @event, count, payload) =>
            {
                Host.HostSession session = Gameplay.RequireSession();
                using IDisposable pin = Gameplay.PinCurrentRuntime(session);
                return session.HandleDamageableResourceRegular(sid, @event, count, payload);
            },
            StartConstruction = sid =>
            {
                Host.HostSession session = Gameplay.RequireSession();
                using IDisposable pin = Gameplay.PinCurrentRuntime(session);
                session.StartResourceConstruction(sid);
            },
            Destroy = (sid, flags) =>
            {
                Host.HostSession session = Gameplay.RequireSession();
                using IDisposable pin = Gameplay.PinCurrentRuntime(session);
                return session.DestroyResourceObject(sid, flags);
            },
            GeyserRegistration = (sid, operation) =>
            {
                var registration = new RawGeyserAiRegistration(OriginalEdition.Patch1078, Gameplay.PlayerRecords.Records);
                registration.BindPatch((token, geyser, state) =>
                {
                    Host.HostSession session = Gameplay.RequireSession();
                    using IDisposable pin = Gameplay.PinCurrentRuntime(session);
                    session.NotifyGeyserAi(token, geyser, state);
                });
                registration.Register(unchecked((uint)sid), operation);
            },
            CarrierRegular = (sid, @event, count, payload) =>
            {
                Host.HostSession session = Gameplay.RequireSession();
                using IDisposable pin = Gameplay.PinCurrentRuntime(session);
                return session.HandleCarrierResourceRegular(sid, @event, count, payload);
            },
        });

        // 보상 SP/editor/표시와 실제 AI 지갑 가산을 같은 owner로 묶는다.
        // 근거: cpppj ClientMain.cpp 484~487행.
        RawAiWallet wallet = CreateAiWallet(demo);
        Storage.BindRewardDevices(new RawSurfaceRewardDevices
        {
            Sp = sp.Store,
            Editor = Gameplay.World.EditorBox,
            RefreshSp = refreshSp,
            CreditWallet = wallet.Credit,
        });

        // 공통 삭제 기억점·소리는 같은 실제 선택/전역 장치를 빌린다.
        // 근거: cpppj ClientMain.cpp 488행.
        Storage.BindDeletionDevices(new RawSurfaceDeletionDevices
        {
            RememberPlacement = rememberPlacement,
            UnitLost = unitLost,
        });

        // 삭제 주소 소비 시점마다 같은 실제 Player owner/base를 읽는다.
        // 근거: cpppj ClientMain.cpp 525행.
        BindPlayerRecords(Gameplay.PlayerRecords);
    }

    /// <summary>장부 뒤 현재 mouseType/기억점/selected 재검사다. 0.901에서 추가됐다.</summary>
    private void RecheckCurrentSelection()
    {
        // 원문 da650의 실제 선택 필요 위치만 현재 UI/전체 World를 보유하며 미선택은 원문 그대로 자연 반환한다.
        // 근거: cpppj ClientMain.cpp 452~454행.
        if (Gameplay.Selection.MouseType.Value == 0)
        {
            return;
        }

        float x = BitConverter.UInt32BitsToSingle(Gameplay.Selection.Remembered[0]);
        float y = BitConverter.UInt32BitsToSingle(Gameplay.Selection.Remembered[1]);
        int found = Storage.ConnectionsExecutor().FindTypeAt(x, y, Gameplay.Selection.MouseType.Value);
        if (found == 0)
        {
            return;
        }

        uint selected = Gameplay.Selection.Selected.Value;
        if (selected != 0 && selected != unchecked((uint)found))
        {
            return;
        }

        // 실제 선택 효과는 현재 Session 본체에서 잇는다. 미연결을 성공으로 꾸미지 않는다.
        Host.HostSession session = Gameplay.RequireSession();
        using IDisposable pin = Gameplay.PinCurrentRuntime(session);
        throw new InvalidOperationException("호스트 선택 재검사의 실제 세션 본체 미연결(R6)");
    }
}
