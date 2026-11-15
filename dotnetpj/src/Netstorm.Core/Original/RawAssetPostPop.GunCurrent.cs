// Gun 네 현재 부모의 효과·진단 복귀·물리 WORD 순서를 보존한다.
namespace Netstorm.Core.Original;

/// <summary>실제 호출 시점의 서버·로딩·Form 판정과 현재 슬롯을 읽는 Gun 몸체다.</summary>
public sealed partial class RawAssetPostPop
{
    /// <summary>선택한 원문 Gunprocess.cpp 식/행을 전달하며 진단 복귀를 허용한다.</summary>
    private void GunAssertion(string expression, uint line) => AssetNeed(_hooks.Assertion)(expression, "\\Ns\\O\\Gunprocess.cpp", line);

    /// <summary>현재 각도/그래프→선택 생성→원문 진단→현재 TYPE/HP→Damageable 순서다.</summary>
    private void GunCurrent(int sid, uint flags, uint kind)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        int extra = patch ? 40 : 35, word = patch ? 36 : 34;
        if (kind == GunAngleKind && (flags & 7) != 0)
        {
            int angle = patch ? unchecked((int)Read(_pool.Slot(sid), word)) : _pool.Slot(sid)[word];
            int graph = AssetNeed(_hooks.GunFrame)(angle); AssetNeed(_hooks.SetGraph)(sid, graph);
        }
        bool branch = kind == GunAngleKind ? (flags & 7) != 0 : (flags & 1) != 0;
        if (branch && (kind != GunSunKind || (_pool.Slot(sid)[extra] & 1) == 0))
        {
            if (_state.CurrentServer && (_pool.Slot(sid)[extra] & 1) == 0)
            {
                if (kind == GunArcherKind) AssetNeed(_hooks.RemoveAnimation)(sid);
                AssetNeed(_hooks.CreateGunProcess)(sid);
                if (patch)
                {
                    // 생성/제거 효과 뒤 현재 전역을 읽으며 checking과 독립인 원문 진단도 보존한다.
                    if (kind == GunSunKind && _state.LoadingDepth == 0 && ((flags & 0x50) != 0 || sid > 4 && sid < 15000))
                        GunAssertion("duringLoad || !(popFlags &( pfDONT_TRANSMIT|pfCLIENT_POP)) && !isClientSid()", 0x32a);
                    if (kind == GunArcherKind && !_state.FortMode && ((flags & 0x50) != 0 || sid > 4 && sid < 15000))
                        GunAssertion("inFortMode || ( !(popFlags &( pfDONT_TRANSMIT|pfCLIENT_POP)) && !isClientSid() )", 0x3c8);
                    // getter가 checking을 꺼도 이미 선택한 실패 진단은 취소하지 않는다.
                    if (_state.CheckingForms && !AssetNeed(_hooks.CheckForms)(sid))
                        GunAssertion("checkForms()", kind == GunAngleKind ? 0x347u : kind == GunFirstKind ? 0x361u : kind == GunSunKind ? 0x32bu : 0x3c9u);
                }
            }
            if (kind == GunSunKind && _state.LoadingDepth != 0 && (Type(sid).Flags1 & 0x1000) != 0)
                Write(_pool.RawSlot(sid), word, Read(_pool.Slot(sid), 12, 2), patch ? 4 : 1);
        }
        if (kind == GunSunKind && (Type(sid).Flags1 & 0x1000) != 0)
            SetAssetWord(sid, Read(_pool.Slot(sid), word, patch ? 4 : 1));
        _damageable.DamageablePostPop(sid, flags);
    }
}
