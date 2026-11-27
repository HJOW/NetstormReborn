using Netstorm.Client.Gump;
using Netstorm.Core.Original;

// 객체 정보가 현재 세션의 실제 Player·동맹·포획 상태를 사용한다.
namespace Netstorm.Client.Host;

/// <summary>현재 세션과 외부 메뉴 장치로 객체 정보 본체를 조립한다.</summary>
public sealed partial class HostSession
{
    /// <summary>완전 적재 관문 뒤 현재 TYPE·클래스·Player와 메뉴 조회를 묶는다.</summary>
    /// <param name="definitions">같은 실제 자산 순서의 TYPE 정의다.</param>
    /// <param name="ui">현재 메뉴·번역·서식·소유자 이름 및 가상 수량 장치다.</param>
    /// <returns>같은 세션의 현재 값과 외부 메뉴를 쓰는 정보 작성기다.</returns>
    /// <remarks>cpppj RawCampaignSession::MakeObjectMenus의 정보 부분이며 다른 객체 메뉴 본체는 포함하지 않는다.</remarks>
    public RawObjectMenuInfo MakeObjectMenuInfo(IReadOnlyList<RiftTypeDefinition> definitions, ObjectMenuInfoHooks ui)
    {
        if (!_nativeLoaded) throw new InvalidOperationException("설치 전 실제 Session 객체 메뉴 요청");
        ArgumentNullException.ThrowIfNull(definitions); ArgumentNullException.ThrowIfNull(ui);
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes ?? throw new InvalidOperationException("객체 정보 현재 TYPE 미연결");
        if (types.Count != 188 || (long)RiftTypeTable.FirstAssetTypeNumber + definitions.Count > types.Count) throw new ArgumentException("객체 정보 실제 TYPE 정의 개수 오류", nameof(definitions));
        var classes = Enumerable.Repeat("", types.Count).ToArray();
        // 실제 TYPE 초기화 순서대로 누락된 class 문자열만 보충한다.
        for (int index = 0; index < definitions.Count; index++)
        {
            string text = definitions[index].String("class") ?? ""; int zero = text.IndexOf('\0'); if (zero >= 0) text = text[..zero];
            if (text.Length > 23) throw new ArgumentOutOfRangeException(nameof(definitions), "객체 정보 실제 class 길이 오류");
            classes[index + RiftTypeTable.FirstAssetTypeNumber] = text;
        }
        var state = new ObjectMenuInfoState
        {
            LocalOwner = () => MissionPlayer.BattleOwner,
            Editor = () => Gameplay.World.Editor,
            MissionPost = () => WorldGlobals.MissionPost ? 1u : 0u,
        };
        var hooks = new ObjectMenuInfoHooks
        {
            Count = ui.Count, Description = ui.Description, OwnerName = ui.OwnerName, Translate = ui.Translate,
            Substitute = ui.Substitute, Print = ui.Print, Title = ui.Title, Header = ui.Header,
            Alliance = (owner, local) => owner < 9 && local < 9 ? Alliances[owner * 9 + local] : throw new ArgumentOutOfRangeException(nameof(owner), "객체 정보 동맹 범위 오류"),
            GroundImmobile = sid => PriestWorld.PriestState.GroundImmobile(sid),
            OwnerStatus = owner => owner < 9 ? PlayerRecords[checked((int)owner * 180 + 0x50)] : throw new ArgumentOutOfRangeException(nameof(owner), "객체 정보 Player 범위 오류"),
        };
        return new RawObjectMenuInfo(Pool, types, classes, state, hooks);
    }
}
